//
// Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
// This file is part of Dynawo, an hybrid C++/Modelica open source suite
// of simulation tools for power systems.
//

/**
 * @file  MarginCalculationLauncher.cpp
 *
 * @brief Margin Calculation launcher: implementation of the algorithm and interaction with dynawo core
 *
 */

#include <iostream>
#include <cmath>
#include <ctime>
#include <iomanip>

#include "boost/date_time/posix_time/posix_time.hpp"

#include <libzip/ZipFile.h>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipEntry.h>
#include <libzip/ZipInputStream.h>
#include <libzip/ZipOutputStream.h>

#include <DYNFileSystemUtils.h>
#include <DYNExecUtils.h>
#include <DYNSimulation.h>
#include <DYNSubModel.h>
#include <DYNTrace.h>
#include <DYNModel.h>
#include <DYNParameter.h>
#include <DYNModelMulti.h>
#include <JOBXmlImporter.h>
#include <JOBIterators.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <JOBSimulationEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBTimelineEntry.h>

#include "DYNMarginCalculationLauncher.h"
#include "DYNMultipleJobs.h"
#include "DYNMarginCalculation.h"
#include "DYNScenario.h"
#include "MacrosMessage.h"
#include "DYNScenarios.h"
#include "DYNAggrResXmlExporter.h"
#include "DYNMPIContext.h"

using multipleJobs::MultipleJobs;
using DYN::Trace;

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return mpi::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
}

void
MarginCalculationLauncher::createScenarioWorkingDir(const std::string& scenarioId, double variation) const {
  std::stringstream subDir;
  subDir << "step-" << variation << "/" << scenarioId;
  std::string workingDir = createAbsolutePath(subDir.str(), workingDirectory_);
  if (!exists(workingDir))
    create_directory(workingDir);
  else if (!is_directory(workingDir))
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
}


void
MarginCalculationLauncher::cleanResultDirectories(const std::vector<boost::shared_ptr<Scenario> >& events) const {
  mpi::Context::sync();
  for (auto loadIncrease : loadIncreaseStatus_) {
    cleanResult(computeLoadIncreaseScenarioId(loadIncrease.first));
  }
  for (auto loadLevel : scenarioStatus_) {
    for (auto scenario : events) {
      cleanResult(SimulationResult::getUniqueScenarioId(scenario->getId(), loadLevel.first));
    }
  }
}

void
MarginCalculationLauncher::readTimes(const std::string& jobFileLoadIncrease, const std::string& jobFileScenario) {
  // job
  job::XmlImporter importer;
  boost::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(workingDirectory_ + "/" + jobFileLoadIncrease);
  //  implicit : only one job per file
  job::job_iterator jobIt = jobsCollection->begin();
  tLoadIncrease_ = (*jobIt)->getSimulationEntry()->getStopTime();

  jobsCollection = importer.importFromFile(workingDirectory_ + "/" + jobFileScenario);
  jobIt = jobsCollection->begin();
  tScenario_ = (*jobIt)->getSimulationEntry()->getStopTime() - (*jobIt)->getSimulationEntry()->getStartTime();
}

void printTask(std::queue< MarginCalculationLauncher::task_t >& toRun) {
  if (mpi::context().isRootProc()) {
    std::queue< MarginCalculationLauncher::task_t > tmp_q = toRun;
    while (!tmp_q.empty()) {
      MarginCalculationLauncher::task_t task = tmp_q.front();
      std::cout << "task " << task.minVariation_ << " " << task.maxVariation_ << " ";
      for (auto id : task.ids_) {
        std::cout << id << " ";
      }
      std::cout << std::endl;
      tmp_q.pop();
    }
  }
  mpi::context().sync();
}

void
MarginCalculationLauncher::launch() {
  assert(multipleJobs_);
  boost::posix_time::ptime t0 = boost::posix_time::second_clock::local_time();
  results_.clear();
  boost::shared_ptr<MarginCalculation> marginCalculation = multipleJobs_->getMarginCalculation();
  if (!marginCalculation) {
    throw DYNAlgorithmsError(MarginCalculationTaskNotFound);
  }
  const boost::shared_ptr<LoadIncrease>& loadIncrease = marginCalculation->getLoadIncrease();
  const boost::shared_ptr<Scenarios>& scenarios = marginCalculation->getScenarios();
  if (!scenarios) {
    throw DYNAlgorithmsError(SystematicAnalysisTaskNotFound);
  }
  const std::string& baseJobsFile = scenarios->getJobsFile();
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();

  // Retrieve from jobs file tLoadIncrease and tScenario
  readTimes(loadIncrease->getJobsFile(), baseJobsFile);

  std::queue< task_t > toRun;
  std::vector<size_t> allEvents;
  for (size_t i=0, iEnd = events.size(); i < iEnd ; i++)
    allEvents.push_back(i);
  toRun.push(task_t(100., 100., allEvents));

  results_.push_back(LoadIncreaseResult());
  size_t idx = results_.size() - 1;
  results_[idx].resize(events.size());
  results_[idx].setLoadLevel(100.);

  // step one : launch the loadIncrease and then all events with 100% of the load increase
  // if there is no crash => no need to go further
  // We start with 100% as it is the most common result of margin calculations on real large cases
  SimulationResult result100;
  findOrLaunchLoadIncrease(loadIncrease, 100., marginCalculation->getAccuracy(), result100);
  results_[idx].setStatus(result100.getStatus());
  std::vector<double > maximumVariationPassing(events.size(), 0.);
  if (result100.getSuccess()) {
    findAllLevelsBetween(0., 100., marginCalculation->getAccuracy(), allEvents, toRun);
//    mpi::context().sync();
//    printTask(toRun);
    findOrLaunchScenarios(baseJobsFile, events, toRun, results_[idx]);

    // analyze results
    unsigned int nbSuccess = 0;
    size_t id = 0;
    for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
        itEnd = results_[idx].end(); it != itEnd; ++it, ++id) {
      TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
      if (it->getStatus() == CONVERGENCE_STATUS) {  // event OK
        nbSuccess++;
        maximumVariationPassing[id] = 100.;
      }
    }
    if (nbSuccess == events.size()) {  // all events succeed
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, 100.) << Trace::endline;
      boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
      boost::posix_time::time_duration diff = t1 - t0;
      TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      cleanResultDirectories(events);
      return;
    }
  }  // if the loadIncrease failed, nothing to do, the next algorithm will try to find the right load level
  TraceInfo(logTag_) << Trace::endline;

  if (marginCalculation->getCalculationType() == MarginCalculation::GLOBAL_MARGIN || events.size() == 1) {
    double value = computeGlobalMargin(loadIncrease, baseJobsFile, events, maximumVariationPassing, marginCalculation->getAccuracy());
    if (value < marginCalculation->getAccuracy()) {
      results_.push_back(LoadIncreaseResult());
      idx = results_.size() - 1;
      results_[idx].resize(events.size());
      results_[idx].setLoadLevel(0.);
      // step two : launch the loadIncrease and then all events with 0% of the load increase
      // if one event crash => no need to go further
      SimulationResult result0;
      findOrLaunchLoadIncrease(loadIncrease, 0., marginCalculation->getAccuracy(), result0);
      results_[idx].setStatus(result0.getStatus());
      double variation0 = 0.;
      if (result0.getSuccess()) {
        toRun = std::queue< task_t >();
        std::vector<size_t> eventsIds;
        for (size_t i = 0; i < events.size() ; ++i) {
          if (variation0 > maximumVariationPassing[i] || DYN::doubleEquals(variation0, maximumVariationPassing[i])) {
            eventsIds.push_back(i);
          } else {
            TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioNotSimulated, events[i]->getId()) << Trace::endline;
            results_[idx].getResult(i).setScenarioId(events[i]->getId());
            results_[idx].getResult(i).setVariation(0.);
            results_[idx].getResult(i).setSuccess(true);
            results_[idx].getResult(i).setStatus(CONVERGENCE_STATUS);
          }
        }
        toRun.push(task_t(0., 0., eventsIds));
        findOrLaunchScenarios(baseJobsFile, events, toRun, results_[idx]);
      } else {
        TraceInfo(logTag_) << "============================================================ " << Trace::endline;
        TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueLoadIncrease, 0.) << Trace::endline;
        boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
        boost::posix_time::time_duration diff = t1 - t0;
        TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
        TraceInfo(logTag_) << "============================================================ " << Trace::endline;
        cleanResultDirectories(events);
        return;  // unable to launch the initial simulation with 0% of load increase
      }

      // analyze results
      for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
             itEnd = results_[idx].end(); it != itEnd; ++it) {
        TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
        if (it->getStatus() != CONVERGENCE_STATUS) {  // one event crashes
          cleanResultDirectories(events);
          TraceInfo(logTag_) << "============================================================ " << Trace::endline;
          TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, 0.) << Trace::endline;
          boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
          boost::posix_time::time_duration diff = t1 - t0;
          TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
          TraceInfo(logTag_) << "============================================================ " << Trace::endline;
          return;
        }
      }
      TraceInfo(logTag_) << Trace::endline;
    }
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
    TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, value) << Trace::endline;
    boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration diff = t1 - t0;
    TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  } else {
    assert(marginCalculation->getCalculationType() == MarginCalculation::LOCAL_MARGIN);
    std::vector<double> results(events.size(), 0.);
    double value = computeLocalMargin(loadIncrease, baseJobsFile, events, marginCalculation->getAccuracy(), results);
    if (result100.getSuccess()) {
      value = 100.;
    }
    std::vector <size_t> eventsIds;
    for (size_t i = 0; i < results.size(); ++i) {
      if (results[i] < marginCalculation->getAccuracy()) {
        eventsIds.push_back(i);
      }
    }
    if (!eventsIds.empty()) {
      results_.push_back(LoadIncreaseResult());
      idx = results_.size() - 1;
      results_[idx].resize(eventsIds.size());
      results_[idx].setLoadLevel(0.);
      SimulationResult result0;
      findOrLaunchLoadIncrease(loadIncrease, 0., marginCalculation->getAccuracy(), result0);
      results_[idx].setStatus(result0.getStatus());
      if (result0.getSuccess()) {
        toRun = std::queue<task_t>();
        toRun.push(task_t(0., 0., eventsIds));
        LoadIncreaseResult liResultTmp;
        liResultTmp.resize(events.size());
        findOrLaunchScenarios(baseJobsFile, events, toRun, liResultTmp);
        for (size_t i = 0; i < eventsIds.size(); ++i) {
          results_[idx].getResult(i) = liResultTmp.getResult(eventsIds[i]);
        }
        // analyze results
        for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
             itEnd = results_[idx].end(); it != itEnd; ++it) {
          TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
        }
      }
    }
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
    TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueLoadIncrease, value) << Trace::endline;
    for (size_t i = 0; i < results.size(); ++i) {
      TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, events[i]->getId(), results[i]) << Trace::endline;
    }
    boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration diff = t1 - t0;
    TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  }
  cleanResultDirectories(events);
}

double
MarginCalculationLauncher::computeGlobalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events,
    std::vector<double >& maximumVariationPassing, double tolerance) {
  double minVariation = 0.;
  double maxVariation = 100.;

  while ( maxVariation - minVariation > tolerance ) {
    double newVariation = round((minVariation + maxVariation)/2.);
    results_.push_back(LoadIncreaseResult());
    size_t idx = results_.size() - 1;
    results_[idx].resize(events.size());
    results_[idx].setLoadLevel(newVariation);
    SimulationResult result;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, tolerance, result);
    results_[idx].setStatus(result.getStatus());
    // If at some point loadIncrease for 0. is launched and is not working no need to continue
    std::map<double, LoadIncreaseStatus, dynawoDoubleLess>::const_iterator itZero = loadIncreaseStatus_.find(0.);
    if (itZero != loadIncreaseStatus_.end() && !itZero->second.success)
      return 0.;

    if (result.getSuccess()) {
      std::queue< task_t > toRun;
      std::vector<size_t> eventsIds;
      for (size_t i=0; i < events.size() ; ++i) {
        if (newVariation > maximumVariationPassing[i]) {
          eventsIds.push_back(i);
        } else {
          results_[idx].getResult(i).setScenarioId(events[i]->getId());
          results_[idx].getResult(i).setVariation(newVariation);
          results_[idx].getResult(i).setSuccess(true);
          results_[idx].getResult(i).setStatus(CONVERGENCE_STATUS);
        }
      }
      findAllLevelsBetween(minVariation, maxVariation, tolerance, eventsIds, toRun);
      findOrLaunchScenarios(baseJobsFile, events, toRun, results_[idx]);

      // analyze results
      unsigned int nbSuccess = 0;
      size_t id = 0;
      for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
          itEnd = results_[idx].end(); it != itEnd; ++it, ++id) {
        if (newVariation < maximumVariationPassing[id] || DYN::doubleEquals(newVariation, maximumVariationPassing[id]))
          TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioNotSimulated, it->getUniqueScenarioId()) << Trace::endline;
        else
          TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
        if (it->getStatus() == CONVERGENCE_STATUS || newVariation < maximumVariationPassing[id] ||
          DYN::doubleEquals(newVariation, maximumVariationPassing[id])) {  // event OK
          nbSuccess++;
          if (newVariation > maximumVariationPassing[id])
            maximumVariationPassing[id] = newVariation;
        }
      }
      if (nbSuccess == events.size() )  // all events succeed
        minVariation = newVariation;
      else
        maxVariation = newVariation;  // at least, one crash
    } else {
      maxVariation = newVariation;  // load increase crashed
    }
    TraceInfo(logTag_) << Trace::endline;
  }
  return minVariation;
}

void
MarginCalculationLauncher::findAllLevelsBetween(const double minVariation, const double maxVariation, const double tolerance,
    const std::vector<size_t>& eventIdxs, std::queue< task_t >& toRun) {
  if (eventIdxs.empty()) {
    toRun = std::queue< task_t >();
    return;
  }
  auto& context = mpi::context();
  unsigned nbMaxToAdd = context.nbProcs()/eventIdxs.size();

  if (nbMaxToAdd == 0) nbMaxToAdd = 1;
  double newVariation = round((minVariation + maxVariation)/2.);
  std::queue< std::pair<double, double> > minMaxStack;

//  if (mpi::context().isRootProc())
//    std::cout << " findAllLevelsBetween " << minVariation << " " << maxVariation << std::endl;

  toRun.push(task_t(minVariation, maxVariation, eventIdxs));
  if (maxVariation - newVariation > tolerance)
    minMaxStack.push(std::make_pair(newVariation, maxVariation));
//  if (mpi::context().isRootProc()) {
//    std::cout << " toRun.size() " << toRun.size() << " nbMaxToAdd " << nbMaxToAdd << " nextVar " <<
//    round((minMaxStack.front().first + minMaxStack.front().second)/2) << std ::endl;
//  }
  while (toRun.size() < nbMaxToAdd && !minMaxStack.empty()) {
    double min = minMaxStack.front().first;
    double max = minMaxStack.front().second;
    minMaxStack.pop();
    double nextVar = round((min + max)/2.);
    auto it = loadIncreaseStatus_.find(nextVar);
    if (it == loadIncreaseStatus_.end() || !it->second.success) continue;
    toRun.push(task_t(min, max, eventIdxs));
    if (max - nextVar > tolerance)
      minMaxStack.push(std::make_pair(nextVar, max));
    if (nextVar - min > tolerance)
      minMaxStack.push(std::make_pair(min, nextVar));
  }
}

double
MarginCalculationLauncher::computeLocalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events, double tolerance,
    std::vector<double>& results) {
  double maxLoadVarForLoadIncrease = 0.;
  std::queue< task_t > toRun;
  std::vector<size_t> all;
  for (size_t i=0; i < events.size() ; ++i)
    all.push_back(i);
  toRun.push(task_t(0., 100., all));

  while (!toRun.empty()) {
    std::queue< task_t > toRunCopy(toRun);  // Needed as findOrLaunchScenarios modifies the queue
    task_t task = toRun.front();
    toRun.pop();
    const std::vector<size_t>& eventsId = task.ids_;
    double newVariation = round((task.minVariation_ + task.maxVariation_)/2.);
    results_.push_back(LoadIncreaseResult());
    size_t idx = results_.size() - 1;
    results_[idx].resize(eventsId.size());
    results_[idx].setLoadLevel(newVariation);
    SimulationResult result;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, tolerance, result);
    results_[idx].setStatus(result.getStatus());
    // If at some point loadIncrease for 0. is launched and is not working no need to continue
    std::map<double, LoadIncreaseStatus, dynawoDoubleLess>::const_iterator itZero = loadIncreaseStatus_.find(0.);
    if (itZero != loadIncreaseStatus_.end() && !itZero->second.success) {
      for (size_t i = 0; i < results.size(); ++i)
        results[i] = 0.;
      return 0.;
    }

    if (result.getSuccess()) {
      if (maxLoadVarForLoadIncrease < newVariation)
        maxLoadVarForLoadIncrease = newVariation;
      LoadIncreaseResult liResultTmp;
      liResultTmp.resize(events.size());
      findOrLaunchScenarios(baseJobsFile, events, toRunCopy, liResultTmp);

      // analyze results
      task_t below(task.minVariation_, newVariation);
      task_t above(newVariation, task.maxVariation_);
      for (size_t i = 0; i < eventsId.size(); ++i) {
        results_[idx].getResult(i) = liResultTmp.getResult(eventsId[i]);
        TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd,
            results_[idx].getResult(i).getUniqueScenarioId(), getStatusAsString(results_[idx].getResult(i).getStatus())) << Trace::endline;
        if (results_[idx].getResult(i).getStatus() == CONVERGENCE_STATUS) {  // event OK
          if (results[eventsId[i]] < newVariation)
            results[eventsId[i]] = newVariation;
          if ( task.maxVariation_ - newVariation > tolerance ) {
            above.ids_.push_back(eventsId[i]);
          }
        } else {
          if ( newVariation - task.minVariation_ > tolerance )
            below.ids_.push_back(eventsId[i]);
        }
      }
      if (!below.ids_.empty())
        toRun.push(below);
      if (!above.ids_.empty())
        toRun.push(above);
    } else if ( newVariation - task.minVariation_ > tolerance ) {
      task_t below(task.minVariation_, newVariation);
      for (size_t i = 0; i < eventsId.size(); ++i) {
        below.ids_.push_back(eventsId[i]);
      }
      if (!below.ids_.empty())
        toRun.push(below);
    }
    TraceInfo(logTag_) << Trace::endline;
  }
  return maxLoadVarForLoadIncrease;
}

void MarginCalculationLauncher::findOrLaunchScenarios(const std::string& baseJobsFile,
    const std::vector<boost::shared_ptr<Scenario> >& events,
    std::queue< task_t >& toRun,
    LoadIncreaseResult& result) {
  if (toRun.empty()) return;
  task_t task = toRun.front();
  toRun.pop();
  const std::vector<size_t>& eventsId = task.ids_;
  double newVariation = round((task.minVariation_ + task.maxVariation_)/2.);
  if (mpi::context().nbProcs() == 1) {
    std::string iidmFile = generateIDMFileNameForVariation(newVariation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      // read inputs only if not already existing with enough variants defined
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, iidmFile);
    }
    for (unsigned int i=0; i < eventsId.size(); ++i) {
      launchScenario(inputsByIIDM_[iidmFile], events[eventsId[i]], newVariation, result.getResult(eventsId[i]));
    }
    return;
  }

//  if (mpi::context().isRootProc())
//    std::cout << "before prepareEvents2Run" << std::endl;
//  printTask(toRun);

  auto found = scenarioStatus_.find(newVariation);
  std::vector<size_t> eventsIdNoTreated;
  if (found != scenarioStatus_.end()) {
    TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioResultsFound, newVariation) << Trace::endline;
    for (const auto& eventId : eventsId) {
      auto resultId = SimulationResult::getUniqueScenarioId(events.at(eventId)->getId(), newVariation);
      result.getResult(eventId) = importResult(resultId);
      if (result.getResult(eventId).getStatus() == NOT_TREATED)
        eventsIdNoTreated.push_back(eventId);
    }
//    for (auto eventIdNoTreated : eventsIdNoTreated) {
//      if (mpi::context().isRootProc())
//        std::cout << "eventIdNoTreated " << eventIdNoTreated << std::endl;
//    }
    if (eventsIdNoTreated.empty())
      return;
    else
      task.ids_ = eventsIdNoTreated;
  }

  std::vector<Pair > events2Run;
  prepareEvents2Run(task, toRun, events2Run);

  int i = 0;
  if (mpi::context().isRootProc()) {
    std::cout << "events2Run" << std::endl;
    for (auto& event : events2Run) {
      std::cout << " " << i << " event " << event.index_ << " " << event.variation_ << std::endl;
      ++i;
    }
  }
  mpi::context().sync();

  for (std::vector<Pair >::const_iterator itEvents = events2Run.begin(); itEvents != events2Run.end(); ++itEvents) {
    double variation = itEvents->variation_;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, iidmFile);
    }
  }

  std::vector<bool> successes;
  mpi::forEach(0, events2Run.size(), [this, &events2Run, &events, &successes](unsigned int i){
    double variation = events2Run[i].variation_;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    size_t eventIdx = events2Run[i].index_;
    SimulationResult resultScenario;
    createScenarioWorkingDir(events.at(eventIdx)->getId(), variation);
    // std::cout << "proc" << mpi::context().rank() << " Task :" << events.at(eventIdx)->getId() << std::endl;
    launchScenario(inputsByIIDM_.at(iidmFile), events.at(eventIdx), variation, resultScenario);
    successes.push_back(resultScenario.getSuccess());
    exportResult(resultScenario);
  });
  // Sync successes
  std::vector<bool> allSuccesses = synchronizeSuccesses(successes);
  for (unsigned int i = 0; i < events2Run.size(); i++) {
    auto& event = events2Run.at(i);
    // variation = event.second
    scenarioStatus_[event.variation_].resize(events.size());
    scenarioStatus_.at(event.variation_).at(event.index_).success = allSuccesses.at(i);
  }
  assert(scenarioStatus_.count(newVariation) > 0);

  for (const auto& eventId : eventsId) {
    auto resultId = SimulationResult::getUniqueScenarioId(events.at(eventId)->getId(), newVariation);
    result.getResult(eventId) = importResult(resultId);
  }

  for (unsigned int i=0; i < events2Run.size(); i++) {
    double variation = events2Run[i].variation_;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    inputsByIIDM_.erase(iidmFile);  // remove iidm file used for scenario to save RAM
  }
}

MarginCalculationLauncher::Pair::Pair(size_t index, double variation) : index_(index), variation_(variation) {
}

void
MarginCalculationLauncher::prepareEvents2Run(const task_t& requestedTask,
    std::queue< task_t >& toRun,
    std::vector<Pair >& events2Run) {
  const std::vector<size_t>& eventsId = requestedTask.ids_;
  double newVariation = round((requestedTask.minVariation_ + requestedTask.maxVariation_)/2.);

//  printTask(toRun);
//  if (mpi::context().isRootProc()) {
//    std::cout << " requestedTask " << requestedTask.minVariation_ << " " << requestedTask.maxVariation_ << " newVariation " << newVariation << " events ";
//    for (size_t i = 0; i < eventsId.size(); ++i) {
//      std::cout << eventsId[i] << " ";
//    }
//    std::cout << std::endl;
//  }
//  mpi::context().sync();

  auto it = loadIncreaseStatus_.find(newVariation);
  if (it != loadIncreaseStatus_.end() && it->second.success) {
    for (size_t i = 0; i < eventsId.size(); ++i) {
      events2Run.push_back(Pair(eventsId[i], newVariation));
    }
  }
  while (events2Run.size() < mpi::context().nbProcs() && !toRun.empty()) {
    task_t newTask = toRun.front();
    toRun.pop();
    const std::vector<size_t>& newEventsId = newTask.ids_;
    double variation = round((newTask.minVariation_ + newTask.maxVariation_)/2.);
    it = loadIncreaseStatus_.find(variation);
    if (it == loadIncreaseStatus_.end() || !it->second.success) continue;
    if (scenarioStatus_.find(variation) != scenarioStatus_.end()) continue;
    for (size_t i = 0, iEnd = newEventsId.size(); i < iEnd; ++i) {
      events2Run.push_back(Pair(newEventsId[i], variation));
    }
  }

  for (const auto& loadIncrease : loadIncreaseStatus_) {
    // if (mpi::context().isRootProc()) std::cout << "loadIncrease.first " << loadIncrease.first << std::endl;
    if (loadIncrease.second.success) {
      auto found = scenarioStatus_.find(loadIncrease.first);
      unsigned int eventIndex = 0;
      while (events2Run.size() % mpi::context().nbProcs() != 0  && eventIndex < eventsId.size()) {
        Pair event(eventsId[eventIndex], loadIncrease.first);
        if (std::find(events2Run.begin(), events2Run.end(), event) == events2Run.end() && found == scenarioStatus_.end()) {
          // if (mpi::context().isRootProc()) std::cout << " add job loadIncrease.first " << loadIncrease.first << " " << eventIndex << std::endl;
          events2Run.push_back(event);
        }
        ++eventIndex;
      }
    }
  }
}

void
MarginCalculationLauncher::launchScenario(const MultiVariantInputs& inputs, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result) {
  if (mpi::context().nbProcs() == 1)
    std::cout << " Launch task :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation << "/" << scenario->getId();
  std::string workingDir = createAbsolutePath(subDir.str(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();

  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);
  std::stringstream dumpFile;
  dumpFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".dmp";
  //  force simulation to load previous dump and to use final values
  params.InitialStateFile_ = dumpFile.str();
  params.iidmFile_ = generateIDMFileNameForVariation(variation);

  // startTime and stopTime are adapted depending on the variation length
  double startTime = tLoadIncrease_ - (100. - variation)/100. * inputs_.getTLoadIncreaseVariationMax();
  params.startTime_ = startTime;
  params.stopTime_ = startTime + tScenario_;

  result.setScenarioId(scenario->getId());
  result.setVariation(variation);
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs);

  if (simulation) {
    simulation->setTimelineOutputFile("");
    simulation->setConstraintsOutputFile("");
    // The event time should be adapted (the list of events models supported currently corresponds to events really used)
    boost::shared_ptr<DYN::ModelMulti> modelMulti = boost::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getEnvVar("DYNAWO_DDB_DIR");
    std::vector<boost::shared_ptr<DYN::SubModel> > subModels = modelMulti->findSubModelByLib(DDBDir + "/EventQuadripoleDisconnection"
      + DYN::sharedLibraryExtension());
    std::vector<boost::shared_ptr<DYN::SubModel> > subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventConnectedStatus"
      + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventSetPointBoolean" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/SetPoint" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventSetPointReal" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventSetPointDoubleReal" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventSetPointGenerator" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventSetPointLoad" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/LineTrippingEvent" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/TfoTrippingEvent" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    subModelsToAdd = modelMulti->findSubModelByLib(DDBDir + "/EventQuadripoleConnection" + DYN::sharedLibraryExtension());
    subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    for (std::vector<boost::shared_ptr<DYN::SubModel> >::const_iterator it = subModels.begin(); it != subModels.end(); ++it) {
      double tEvent = (*it)->findParameterDynamic("event_tEvent").getValue<double>();
      (*it)->setParameterValue("event_tEvent", DYN::PAR, tEvent - (100. - variation) * inputs_.getTLoadIncreaseVariationMax() / 100., false);
      (*it)->setSubModelParameters();
    }
    simulate(simulation, result);
  }

//  if (mpi::context().nbProcs() == 1)
//    std::cout << " Task :" << scenario->getId() << " status =" << getStatusAsString(result.getStatus()) << std::endl;
  std::cout << "proc" << mpi::context().rank() << " End Task: variation " << variation
  << " scenario " << scenario->getId() << " status = " << getStatusAsString(result.getStatus()) << std::endl;
}

std::vector<double>
MarginCalculationLauncher::generateVariationsToLaunch(unsigned int maxNumber, double variation, double tolerance) const {
  std::set<double, dynawoDoubleLess> variationsToLaunch;
  variationsToLaunch.insert(variation);

  // Hack to add 0. if the load increase is below 50. as we know we never did 0. in the first place
  if (loadIncreaseStatus_.find(0.) == loadIncreaseStatus_.end() && variation < 50.)
    variationsToLaunch.insert(0.);
  if (variationsToLaunch.size() < maxNumber) {
    std::queue< std::pair<double, double> > levels;
    double closestVariationBelow = 0.;
    double closestVariationAbove = 100.;
    for (const auto& status : loadIncreaseStatus_) {
      if (closestVariationBelow < status.first && status.first < variation)
        closestVariationBelow = status.first;
      if (status.first < closestVariationAbove &&  variation < status.first)
        closestVariationAbove = status.first;
    }
    levels.push(std::make_pair(closestVariationBelow, closestVariationAbove));
    while (!levels.empty() && variationsToLaunch.size() < maxNumber) {
      std::pair<double, double> currentLevel = levels.front();
      levels.pop();
      double nextVariation = round((currentLevel.first + currentLevel.second)/2.);
      variationsToLaunch.insert(nextVariation);
      if (currentLevel.second - nextVariation > tolerance)
        levels.push(std::make_pair(nextVariation, currentLevel.second));
      if (DYN::doubleNotEquals(nextVariation, 50.) && nextVariation - currentLevel.first > tolerance)
        levels.push(std::make_pair(currentLevel.first, nextVariation));
    }
  }
  std::vector<double> variationsToLaunchVector(variationsToLaunch.begin(), variationsToLaunch.end());
  return variationsToLaunchVector;
}

std::string
MarginCalculationLauncher::computeLoadIncreaseScenarioId(double variation) {
  std::stringstream ss;
  ss << "loadIncrease-" << variation;
  return ss.str();
}

std::vector<bool>
MarginCalculationLauncher::synchronizeSuccesses(const std::vector<bool>& successes) {
  auto& context = mpi::context();
  std::vector<std::vector<bool>> gatheredSuccesses;
  context.gather(successes, gatheredSuccesses);
  std::vector<bool> allSuccesses;
  if (context.isRootProc()) {
    auto size =
      std::accumulate(gatheredSuccesses.begin(), gatheredSuccesses.end(), 0, [](size_t sum, const std::vector<bool>& data) { return sum + data.size(); });
    allSuccesses.resize(size);
    for (unsigned int i = 0; i < context.nbProcs(); i++) {
      const auto& vect = gatheredSuccesses.at(i);
      for (unsigned int j = 0; j < vect.size(); j++) {
        // variations were attributed to procs following the formula: "index % nbprocs == rank" throught forEach function
        allSuccesses.at(j * context.nbProcs() + i) = vect.at(j);
      }
    }
  }
  context.broadcast(allSuccesses);
  return allSuccesses;
}

void
MarginCalculationLauncher::findOrLaunchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, const double tolerance, SimulationResult& result) {
  TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, variation) << Trace::endline;

  auto found = loadIncreaseStatus_.find(variation);
  if (found != loadIncreaseStatus_.end()) {
    result = importResult(computeLoadIncreaseScenarioId(variation));
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseResultsFound, variation) << Trace::endline;
    return;
  }

  if (mpi::context().nbProcs() == 1) {
    inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
    launchLoadIncrease(loadIncrease, variation, result);

    // Hack to add 0. if the load increase is below 50. as we know we never did 0. in the first place
    if (variation < 50. && !result.getSuccess() && loadIncreaseStatus_.find(0.) == loadIncreaseStatus_.end()) {
      TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, 0.) << Trace::endline;
      SimulationResult result0;
      inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
      launchLoadIncrease(loadIncrease, 0., result0);
      loadIncreaseStatus_.insert(std::make_pair(0., LoadIncreaseStatus(result0.getSuccess())));
    }
    return;
  }

  // Algo to generate variations to launch
  auto& context = mpi::context();
  std::vector<double> variationsToLaunch = generateVariationsToLaunch(context.nbProcs(), variation, tolerance);

  // Launch Simulations
  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  std::vector<bool> successes;
  mpi::forEach(0, variationsToLaunch.size(), [this, &loadIncrease, &variationsToLaunch, &successes](unsigned int i){
    SimulationResult resultScenario;
    createScenarioWorkingDir(loadIncrease->getId(), variationsToLaunch.at(i));
    launchLoadIncrease(loadIncrease, variationsToLaunch.at(i), resultScenario);
    successes.push_back(resultScenario.getSuccess());
    exportResult(resultScenario);
  });
  // Sync successes
  std::vector<bool> allSuccesses = synchronizeSuccesses(successes);
  // Fill load increase status
  for (unsigned int i = 0; i < variationsToLaunch.size(); i++) {
    auto currVariation = variationsToLaunch.at(i);
    loadIncreaseStatus_.insert(std::make_pair(currVariation, LoadIncreaseStatus(allSuccesses.at(i))));
    auto loadIncreaseResult = importResult(computeLoadIncreaseScenarioId(currVariation));
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseEnd, currVariation, getStatusAsString(loadIncreaseResult.getStatus())) << Trace::endline;
  }
  assert(loadIncreaseStatus_.count(variation) > 0);
  result = importResult(computeLoadIncreaseScenarioId(variation));
}

void
MarginCalculationLauncher::launchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, SimulationResult& result) {
  if (mpi::context().nbProcs() == 1)
    std::cout << "Launch loadIncrease of " << variation << "%" <<std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation << "/" << loadIncrease->getId();
  std::string workingDir = createAbsolutePath(subDir.str(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  job->getOutputsEntry()->setTimelineEntry(boost::shared_ptr<job::TimelineEntry>());
  job->getOutputsEntry()->setConstraintsEntry(boost::shared_ptr<job::ConstraintsEntry>());

  SimulationParameters params;
  //  force simulation to dump final values (would be used as input to launch each event)
  params.activateDumpFinalState_ = true;
  params.activateExportIIDM_ = true;
  std::stringstream iidmFile;
  iidmFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".iidm";
  params.exportIIDMFile_ = iidmFile.str();
  std::stringstream dumpFile;
  dumpFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".dmp";
  params.dumpFinalStateFile_ = dumpFile.str();

  result.setScenarioId(computeLoadIncreaseScenarioId(variation));
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    boost::shared_ptr<DYN::ModelMulti> modelMulti = boost::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getMandatoryEnvVar("DYNAWO_DDB_DIR");
    std::vector<boost::shared_ptr<DYN::SubModel> > subModels = modelMulti->findSubModelByLib(DDBDir + "/DYNModelVariationArea" + DYN::sharedLibraryExtension());
    for (std::vector<boost::shared_ptr<DYN::SubModel> >::const_iterator it = subModels.begin(); it != subModels.end(); ++it) {
      double startTime = (*it)->findParameterDynamic("startTime").getValue<double>();
      double stopTime = (*it)->findParameterDynamic("stopTime").getValue<double>();
      inputs_.setTLoadIncreaseVariationMax(stopTime - startTime);
      int nbLoads = (*it)->findParameterDynamic("nbLoads").getValue<int>();
      for (int k = 0; k < nbLoads; ++k) {
        std::stringstream deltaPName;
        deltaPName << "deltaP_load_" << k;
        double deltaP = (*it)->findParameterDynamic(deltaPName.str()).getValue<double>();
        (*it)->setParameterValue(deltaPName.str(), DYN::PAR, deltaP*variation/100., false);

        std::stringstream deltaQName;
        deltaQName << "deltaQ_load_" << k;
        double deltaQ = (*it)->findParameterDynamic(deltaQName.str()).getValue<double>();
        (*it)->setParameterValue(deltaQName.str(), DYN::PAR, deltaQ*variation/100., false);
      }
      // change of the stop time to keep the same ramp of variation.
      double originalDuration = stopTime - startTime;
      double newStopTime = startTime + originalDuration * variation / 100.;
      (*it)->setParameterValue("stopTime", DYN::PAR, newStopTime, false);
      (*it)->setSubModelParameters();  // update values stored in subModel
      // Limitation for this log : will only be printed for root process
      TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseModelParameter, (*it)->name(), newStopTime, variation/100.) << Trace::endline;
    }
    simulation->setStopTime(tLoadIncrease_ - (100. - variation)/100. * inputs_.getTLoadIncreaseVariationMax());
    simulate(simulation, result);
  }
}

void
MarginCalculationLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  Trace::resetCustomAppenders();  // to force flush
  Trace::resetPersistantCustomAppender(logTag_, DYN::INFO);  // to force flush
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportLoadIncreaseResultsToStream(results_, aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
    std::ifstream inFile(createAbsolutePath("dynawo.log", workingDirectory_).c_str());
    if (inFile.is_open()) {
      std::string line;
      std::stringstream strStream;
      while (getline(inFile, line)) {
        strStream << line << "\n";
      }
      mapData["dynawo.log"] = strStream.str();
      inFile.close();
    }
  } else {
    exporter.exportLoadIncreaseResultsToFile(results_, outputFileFullPath_);
  }

  std::map<std::string, SimulationResult> bestResults;
  std::map<std::string, SimulationResult> worstResults;
  for (std::vector<LoadIncreaseResult>::const_iterator itLoadIncreaseResult = results_.begin();
       itLoadIncreaseResult != results_.end(); ++itLoadIncreaseResult) {
    double loadLevel = itLoadIncreaseResult->getLoadLevel();
    for (std::vector<SimulationResult>::const_iterator itSimulationResult = itLoadIncreaseResult->begin();
         itSimulationResult != itLoadIncreaseResult->end(); ++itSimulationResult) {
      std::string scenarioId = itSimulationResult->getScenarioId();
      if (itSimulationResult->getSuccess()) {
        std::map<std::string, SimulationResult>::iterator itBest = bestResults.find(scenarioId);
        if (itBest == bestResults.end() || (loadLevel > itBest->second.getVariation()))
          bestResults[scenarioId] = *itSimulationResult;
      } else {
        std::map<std::string, SimulationResult>::iterator itWorst = worstResults.find(scenarioId);
        if (itWorst == worstResults.end() || loadLevel < itWorst->second.getVariation())
          worstResults[scenarioId] = *itSimulationResult;
      }
    }
  }

  for (std::map<std::string, SimulationResult>::iterator itBest = bestResults.begin();
       itBest != bestResults.end(); ++itBest) {
    if (zipIt) {
      storeOutputs(itBest->second, mapData);
    } else {
      writeOutputs(itBest->second);
    }
  }
  for (std::map<std::string, SimulationResult>::iterator itWorst = worstResults.begin();
       itWorst != worstResults.end(); ++itWorst) {
    if (zipIt) {
      storeOutputs(itWorst->second, mapData);
    } else {
      writeOutputs(itWorst->second);
    }
  }
}

std::string
MarginCalculationLauncher::generateIDMFileNameForVariation(double variation) const {
  std::stringstream iidmFile;
  iidmFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".iidm";
  return iidmFile.str();
}

}  // namespace DYNAlgorithms
