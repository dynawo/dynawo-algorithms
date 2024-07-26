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
#include "DYNMultiProcessingContext.h"

using DYN::Trace;

static const char LOAD_INCREASE[] = "loadIncrease";

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
}

void
MarginCalculationLauncher::createScenarioWorkingDir(const std::string& scenarioId, double variation) const {
  std::stringstream subDir;
  subDir << "step-" << variation;
  std::string workingDir = createAbsolutePath(scenarioId, createAbsolutePath(subDir.str(), workingDirectory_));
  if (!exists(workingDir))
    createDirectory(workingDir);
  else if (!isDirectory(workingDir))
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
}


void
MarginCalculationLauncher::cleanResultDirectories(const std::vector<boost::shared_ptr<Scenario> >& events) const {
  multiprocessing::Context::sync();
  for (const auto& loadIncrease : loadIncreaseStatus_) {
    cleanResult(computeLoadIncreaseScenarioId(loadIncrease.first));
  }
  for (const auto& loadLevel : scenarioStatus_) {
    for (const auto& scenario : events) {
      cleanResult(SimulationResult::getUniqueScenarioId(scenario->getId(), loadLevel.first));
    }
  }
}

void
MarginCalculationLauncher::readTimes(const std::string& jobFileLoadIncrease, const std::string& jobFileScenario) {
  // job
  job::XmlImporter importer;
  std::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(createAbsolutePath(jobFileLoadIncrease, workingDirectory_));
  //  implicit : only one job per file
  tLoadIncrease_ = jobsCollection->getJobs()[0]->getSimulationEntry()->getStopTime();

  jobsCollection = importer.importFromFile(createAbsolutePath(jobFileScenario, workingDirectory_));
  //  implicit : only one job per file
  const auto& job = jobsCollection->getJobs()[0];
  tScenario_ = job->getSimulationEntry()->getStopTime() - job->getSimulationEntry()->getStartTime();
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

  double maxVariation = 100.;
  double minVariation = 0.;
  double variation = maxVariation;
  results_.emplace_back(events.size());
  const size_t loadIncreaseVariation100Index = results_.size() - 1;
  findOrLaunchLoadIncrease(loadIncrease, variation, minVariation, maxVariation,
                           marginCalculation->getAccuracy(), results_.at(loadIncreaseVariation100Index));

  if (!loadIncreaseStatus_.at(variation).success) {
    variation = minVariation;
    for (auto& status : loadIncreaseStatus_) {
      if (status.second.success && status.first > variation)
        variation = status.first;
    }
    // No computation here. We only update the second element in results_ with the maximum successful load increase.
    // However if no variation was ok at all, we launch a 0% load increase and put it in the results_ array instead.
    results_.emplace_back(events.size());
    const size_t maxLoadIncreaseVariationIndex = results_.size() - 1;
    findOrLaunchLoadIncrease(loadIncrease, variation, minVariation, maxVariation,
                             marginCalculation->getAccuracy(), results_.at(maxLoadIncreaseVariationIndex));
    maxVariation = variation;
  }

  std::queue< task_t > toRun;
  std::vector<size_t> allEvents;
  for (size_t i=0, iEnd = events.size(); i < iEnd ; i++)
    allEvents.push_back(i);
  toRun.emplace(task_t(maxVariation, maxVariation, allEvents));

  // step one : launch the loadIncrease and then all events with 100% of the load increase
  // if there is no crash => no need to go further
  // We start with 100% as it is the most common result of margin calculations on real large cases
  std::vector<double > maximumVariationPassing(events.size(), 0.);
  const size_t maxLoadIncreaseVariationIndex = results_.size() - 1;
  if (results_.at(maxLoadIncreaseVariationIndex).getResult().getSuccess()) {
    if (!DYN::doubleEquals(minVariation, maxVariation))
      findAllLevelsBetween(minVariation, maxVariation, marginCalculation->getAccuracy(), allEvents, toRun);
    findOrLaunchScenarios(baseJobsFile, events, toRun, results_.at(maxLoadIncreaseVariationIndex));

    // analyze results
    unsigned int nbSuccess = 0;
    size_t id = 0;
    for (auto simulationResultIt = results_.at(maxLoadIncreaseVariationIndex).getScenariosResults().cbegin();
          simulationResultIt != results_.at(maxLoadIncreaseVariationIndex).getScenariosResults().cend(); ++simulationResultIt, ++id) {
      TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd, simulationResultIt->getUniqueScenarioId(),
                                                  getStatusAsString(simulationResultIt->getStatus())) << Trace::endline;
      if (simulationResultIt->getStatus() == CONVERGENCE_STATUS) {  // event OK
        nbSuccess++;
        maximumVariationPassing[id] = maxVariation;
      }
    }
    if (nbSuccess == events.size() &&
      ((100. - maxVariation) < marginCalculation->getAccuracy() ||
      DYN::doubleEquals(100. - maxVariation, marginCalculation->getAccuracy()))) {  // all events succeed and maxVariation is within tolerance
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, maxVariation) << Trace::endline;
      boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
      boost::posix_time::time_duration diff = t1 - t0;
      TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      cleanResultDirectories(events);
      return;
    } else if (DYN::doubleEquals(maxVariation, minVariation) && nbSuccess != events.size()) {  // a scenario is not working at 0.
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, maxVariation) << Trace::endline;
      boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
      boost::posix_time::time_duration diff = t1 - t0;
      TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
      TraceInfo(logTag_) << "============================================================ " << Trace::endline;
      cleanResultDirectories(events);
      return;
    }
  } else if (DYN::doubleEquals(maxVariation, minVariation)) {  // launch increase of 0. is not working
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
    TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, maxVariation) << Trace::endline;
    boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration diff = t1 - t0;
    TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
    TraceInfo(logTag_) << "============================================================ " << Trace::endline;
    cleanResultDirectories(events);
    return;
  }
  TraceInfo(logTag_) << Trace::endline;

  // we force the dichotomie between 0 and 100 even if 100 is not working
  maxVariation = 100.;
  minVariation = 0.;

  if (marginCalculation->getCalculationType() == MarginCalculation::GLOBAL_MARGIN || events.size() == 1) {
    double value = computeGlobalMargin(loadIncrease, baseJobsFile, events, maximumVariationPassing,
                                       marginCalculation->getAccuracy(), minVariation, maxVariation);
    if (value < marginCalculation->getAccuracy()) {
      // step two : launch the loadIncrease and then all events with 0% of the load increase
      // if one event crash => no need to go further
      results_.emplace_back(events.size());
      const size_t globalMarginMinLoadLevelIndex = results_.size() - 1;
      findOrLaunchLoadIncrease(loadIncrease,
                                minVariation,
                                minVariation,
                                minVariation,
                                marginCalculation->getAccuracy(),
                                results_.at(globalMarginMinLoadLevelIndex));
      double variation0 = minVariation;
      if (results_.at(globalMarginMinLoadLevelIndex).getResult().getSuccess()) {
        toRun = std::queue< task_t >();
        std::vector<size_t> eventsIds;
        for (size_t i = 0; i < events.size() ; ++i) {
          if (variation0 > maximumVariationPassing[i] || DYN::doubleEquals(variation0, maximumVariationPassing[i])) {
            eventsIds.push_back(i);
          } else {
            TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioNotSimulated, events[i]->getId()) << Trace::endline;
            results_.at(globalMarginMinLoadLevelIndex).getScenarioResult(i).setScenarioId(events[i]->getId());
            results_.at(globalMarginMinLoadLevelIndex).getScenarioResult(i).setVariation(minVariation);
            results_.at(globalMarginMinLoadLevelIndex).getScenarioResult(i).setSuccess(true);
            results_.at(globalMarginMinLoadLevelIndex).getScenarioResult(i).setStatus(CONVERGENCE_STATUS);
          }
        }
        toRun.emplace(task_t(0., 0., eventsIds));
        findOrLaunchScenarios(baseJobsFile, events, toRun, results_.at(globalMarginMinLoadLevelIndex));
      } else {
        TraceInfo(logTag_) << "============================================================ " << Trace::endline;
        TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueLoadIncrease, minVariation) << Trace::endline;
        boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
        boost::posix_time::time_duration diff = t1 - t0;
        TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
        TraceInfo(logTag_) << "============================================================ " << Trace::endline;
        cleanResultDirectories(events);
        return;  // unable to launch the initial simulation with 0% of load increase
      }

      // analyze results
      for (const auto& result : results_.at(globalMarginMinLoadLevelIndex).getScenariosResults()) {
        TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, result.getUniqueScenarioId(), getStatusAsString(result.getStatus())) << Trace::endline;
        if (result.getStatus() != CONVERGENCE_STATUS) {  // one event crashes
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
    double value = computeLocalMargin(loadIncrease, baseJobsFile, events, marginCalculation->getAccuracy(), minVariation, maxVariation, results);
    if (results_.at(maxLoadIncreaseVariationIndex).getResult().getSuccess()) {
      value = maxVariation;
    }
    std::vector <size_t> eventsIds;
    for (size_t i = 0; i < results.size(); ++i) {
      if (results[i] < marginCalculation->getAccuracy()) {
        eventsIds.push_back(i);
      }
    }
    if (!eventsIds.empty()) {
      results_.emplace_back(eventsIds.size());
      const size_t localMarginMinLoadLevelIndex = results_.size() - 1;
      findOrLaunchLoadIncrease(loadIncrease,
                                minVariation,
                                minVariation,
                                minVariation,
                                marginCalculation->getAccuracy(),
                                results_.at(localMarginMinLoadLevelIndex));
      if (results_.at(localMarginMinLoadLevelIndex).getResult().getSuccess()) {
        toRun = std::queue<task_t>();
        toRun.emplace(task_t(minVariation, minVariation, eventsIds));
        LoadIncreaseResult liResultTmp(events.size());
        findOrLaunchScenarios(baseJobsFile, events, toRun, liResultTmp);
        for (size_t i = 0; i < eventsIds.size(); ++i) {
          results_.at(localMarginMinLoadLevelIndex).getScenarioResult(i) = liResultTmp.getScenarioResult(eventsIds[i]);
        }
        // analyze results
        for (const auto& result : results_.at(localMarginMinLoadLevelIndex).getScenariosResults()) {
          TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, result.getUniqueScenarioId(), getStatusAsString(result.getStatus())) << Trace::endline;
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
    std::vector<double >& maximumVariationPassing, double tolerance, double minVariation, double maxVariation) {

  while ( maxVariation - minVariation > tolerance ) {
    double newVariation = round((minVariation + maxVariation)/2.);
    results_.emplace_back(events.size());
    const size_t loadIncreaseIndex = results_.size() - 1;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, minVariation, maxVariation, tolerance, results_.at(loadIncreaseIndex));
    // If at some point loadIncrease for 0. is launched and is not working no need to continue
    std::map<double, LoadIncreaseStatus, dynawoDoubleLess>::const_iterator itZero = loadIncreaseStatus_.find(0.);
    if (itZero != loadIncreaseStatus_.end() && !itZero->second.success)
      return 0.;

    if (results_.at(loadIncreaseIndex).getResult().getSuccess()) {
      std::queue< task_t > toRun;
      std::vector<size_t> eventsIds;
      for (size_t i=0; i < events.size() ; ++i) {
        if (newVariation > maximumVariationPassing[i]) {
          eventsIds.push_back(i);
        } else {
          results_.at(loadIncreaseIndex).getScenarioResult(i).setScenarioId(events[i]->getId());
          results_.at(loadIncreaseIndex).getScenarioResult(i).setVariation(newVariation);
          results_.at(loadIncreaseIndex).getScenarioResult(i).setSuccess(true);
          results_.at(loadIncreaseIndex).getScenarioResult(i).setStatus(CONVERGENCE_STATUS);
        }
      }
      findAllLevelsBetween(minVariation, maxVariation, tolerance, eventsIds, toRun);
      findOrLaunchScenarios(baseJobsFile, events, toRun, results_.at(loadIncreaseIndex));

      // analyze results
      unsigned int nbSuccess = 0;
      size_t id = 0;
      for (const auto& result : results_.at(loadIncreaseIndex).getScenariosResults()) {
        if (newVariation < maximumVariationPassing[id] || DYN::doubleEquals(newVariation, maximumVariationPassing[id]))
          TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioNotSimulated, result.getUniqueScenarioId()) << Trace::endline;
        else
          TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd,
              result.getUniqueScenarioId(), getStatusAsString(result.getStatus())) << Trace::endline;
        if (result.getStatus() == CONVERGENCE_STATUS || newVariation < maximumVariationPassing[id] ||
          DYN::doubleEquals(newVariation, maximumVariationPassing[id])) {  // event OK
          nbSuccess++;
          if (newVariation > maximumVariationPassing[id])
            maximumVariationPassing[id] = newVariation;
        }
        ++id;
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
  auto& context = multiprocessing::context();
  unsigned nbMaxToAdd = context.nbProcs()/eventIdxs.size();

  if (nbMaxToAdd == 0) nbMaxToAdd = 1;
  double newVariation = round((minVariation + maxVariation)/2.);
  std::queue< std::pair<double, double> > minMaxStack;

  toRun.emplace(task_t(minVariation, maxVariation, eventIdxs));
  if (maxVariation - newVariation > tolerance)
    minMaxStack.emplace(std::make_pair(newVariation, maxVariation));
  while (toRun.size() < nbMaxToAdd && !minMaxStack.empty()) {
    double min = minMaxStack.front().first;
    double max = minMaxStack.front().second;
    minMaxStack.pop();
    double nextVar = round((min + max)/2.);
    auto it = loadIncreaseStatus_.find(nextVar);
    if (it == loadIncreaseStatus_.end() || !it->second.success) continue;
    toRun.emplace(task_t(min, max, eventIdxs));
    if (max - nextVar > tolerance)
      minMaxStack.emplace(std::make_pair(nextVar, max));
    if (nextVar - min > tolerance)
      minMaxStack.emplace(std::make_pair(min, nextVar));
  }
}

double
MarginCalculationLauncher::computeLocalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events, const double tolerance, const double minVariation,
    const double maxVariation, std::vector<double>& results) {
  double maxLoadVarForLoadIncrease = 0.;
  std::queue< task_t > toRun;
  std::vector<size_t> all;
  for (size_t i=0; i < events.size() ; ++i)
    all.push_back(i);
  toRun.emplace(task_t(0., 100., all));

  while (!toRun.empty()) {
    std::queue< task_t > toRunCopy(toRun);  // Needed as findOrLaunchScenarios modifies the queue
    task_t task = toRun.front();
    toRun.pop();
    const std::vector<size_t>& eventsId = task.ids_;
    double newVariation = round((task.minVariation_ + task.maxVariation_)/2.);
    results_.emplace_back(eventsId.size());
    const size_t loadIncreaseIndex = results_.size() - 1;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, minVariation, maxVariation, tolerance, results_.at(loadIncreaseIndex));
    // If at some point loadIncrease for 0. is launched and is not working no need to continue
    std::map<double, LoadIncreaseStatus, dynawoDoubleLess>::const_iterator itZero = loadIncreaseStatus_.find(0.);
    if (itZero != loadIncreaseStatus_.end() && !itZero->second.success) {
      for (auto& result : results)
        result = 0.;
      return 0.;
    }

    if (results_.at(loadIncreaseIndex).getResult().getSuccess()) {
      if (maxLoadVarForLoadIncrease < newVariation)
        maxLoadVarForLoadIncrease = newVariation;
      LoadIncreaseResult liResultTmp(events.size());
      findOrLaunchScenarios(baseJobsFile, events, toRunCopy, liResultTmp);

      // analyze results
      task_t below(task.minVariation_, newVariation);
      task_t above(newVariation, task.maxVariation_);
      for (size_t i = 0; i < eventsId.size(); ++i) {
        results_.at(loadIncreaseIndex).getScenarioResult(i) = liResultTmp.getScenarioResult(eventsId[i]);
        TraceInfo(logTag_) << DYNAlgorithmsLog(ScenariosEnd, results_.at(loadIncreaseIndex).getScenarioResult(i).getUniqueScenarioId(),
                                      getStatusAsString(results_.at(loadIncreaseIndex).getScenarioResult(i).getStatus())) << Trace::endline;
        if (results_.at(loadIncreaseIndex).getScenarioResult(i).getStatus() == CONVERGENCE_STATUS) {  // event OK
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
      for (const auto eventId : eventsId) {
        below.ids_.push_back(eventId);
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
  if (multiprocessing::context().nbProcs() == 1) {
    std::string iidmFile = generateIDMFileNameForVariation(newVariation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      // read inputs only if not already existing with enough variants defined
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, iidmFile);
    }
    for (const auto eventId : eventsId)
      launchScenario(inputsByIIDM_[iidmFile], events[eventId], newVariation, result.getScenarioResult(eventId));

    return;
  }

#ifdef _MPI_
  auto found = scenarioStatus_.find(newVariation);
  if (found != scenarioStatus_.end()) {
    TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioResultsFound, newVariation) << Trace::endline;
    for (const auto& eventId : eventsId) {
      auto resultId = SimulationResult::getUniqueScenarioId(events.at(eventId)->getId(), newVariation);
      result.getScenarioResult(eventId) = importResult(resultId);
    }
    return;
  }

  std::vector<std::pair<size_t, double> > events2Run;
  prepareEvents2Run(task, toRun, events2Run);

  for (const auto& event2Run : events2Run) {
    double variation = event2Run.second;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, iidmFile);
    }
  }

  std::vector<bool> successes;
  multiprocessing::forEach(0, events2Run.size(), [this, &events2Run, &events, &successes](unsigned int i){
    double variation = events2Run[i].second;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    size_t eventIdx = events2Run[i].first;
    SimulationResult resultScenario;
    createScenarioWorkingDir(events.at(eventIdx)->getId(), variation);
    launchScenario(inputsByIIDM_.at(iidmFile), events.at(eventIdx), variation, resultScenario);
    successes.push_back(resultScenario.getSuccess());
    exportResult(resultScenario);
  });
  // Sync successes
  std::vector<bool> allSuccesses = synchronizeSuccesses(successes);
  for (unsigned int i = 0; i < events2Run.size(); i++) {
    auto& event = events2Run.at(i);
    scenarioStatus_[event.second].resize(events.size());
    scenarioStatus_.at(event.second).at(event.first).success = allSuccesses.at(i);
  }
  assert(scenarioStatus_.count(newVariation) > 0);

  for (const auto& eventId : eventsId) {
    auto resultId = SimulationResult::getUniqueScenarioId(events.at(eventId)->getId(), newVariation);
    result.getScenarioResult(eventId) = importResult(resultId);
  }

  for (const auto& event2Run : events2Run) {
    double variation = event2Run.second;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    inputsByIIDM_.erase(iidmFile);  // remove iidm file used for scenario to save RAM
  }
#endif
}

void
MarginCalculationLauncher::prepareEvents2Run(const task_t& requestedTask,
    std::queue< task_t >& toRun,
    std::vector<std::pair<size_t, double> >& events2Run) {
  const std::vector<size_t>& eventsId = requestedTask.ids_;
  double newVariation = round((requestedTask.minVariation_ + requestedTask.maxVariation_)/2.);
  auto it = loadIncreaseStatus_.find(newVariation);
  if (it != loadIncreaseStatus_.end() && it->second.success) {
    for (const auto eventId : eventsId) {
      events2Run.emplace_back(std::make_pair(eventId, newVariation));
    }
  }
  while (events2Run.size() < multiprocessing::context().nbProcs() && !toRun.empty()) {
    task_t newTask = toRun.front();
    toRun.pop();
    const std::vector<size_t>& newEventsId = newTask.ids_;
    double variation = round((newTask.minVariation_ + newTask.maxVariation_)/2.);
    it = loadIncreaseStatus_.find(variation);
    if (it == loadIncreaseStatus_.end() || !it->second.success) continue;
    if (scenarioStatus_.find(variation) != scenarioStatus_.end()) continue;
    for (const auto newEventId : newEventsId) {
      events2Run.emplace_back(std::make_pair(newEventId, variation));
    }
  }
}

void
MarginCalculationLauncher::launchScenario(const MultiVariantInputs& inputs, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Launch task :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation;
  std::string workingDir = createAbsolutePath(scenario->getId(), createAbsolutePath(subDir.str(), workingDirectory_));
  std::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();

  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);
  std::stringstream dumpFile;
  dumpFile << "loadIncreaseFinalState-" << variation << ".dmp";
  //  force simulation to load previous dump and to use final values
  params.InitialStateFile_ = createAbsolutePath(dumpFile.str(), workingDirectory_);
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
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getEnvVar("DYNAWO_DDB_DIR");
    decltype(modelMulti->findSubModelByLib("")) subModels;
    auto addSubModelsByLib = [&](const std::string& libName) {
      auto subModelsToAdd = modelMulti->findSubModelByLib(createAbsolutePath(libName + DYN::sharedLibraryExtension(), DDBDir));
      subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    };
    addSubModelsByLib("EventQuadripoleDisconnection");
    addSubModelsByLib("EventConnectedStatus");
    addSubModelsByLib("EventSetPointBoolean");
    addSubModelsByLib("SetPoint");
    addSubModelsByLib("EventSetPointReal");
    addSubModelsByLib("EventSetPointDoubleReal");
    addSubModelsByLib("EventSetPointGenerator");
    addSubModelsByLib("EventSetPointLoad");
    addSubModelsByLib("LineTrippingEvent");
    addSubModelsByLib("TfoTrippingEvent");
    addSubModelsByLib("EventQuadripoleConnection");
    for (const auto& subModel : subModels) {
      double tEvent = subModel->findParameterDynamic("event_tEvent").getValue<double>();
      subModel->setParameterValue("event_tEvent", DYN::PAR, tEvent - (100. - variation) * inputs_.getTLoadIncreaseVariationMax() / 100., false);
      subModel->setSubModelParameters();
    }
    simulate(simulation, result);
  }

  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Task :" << scenario->getId() << " status =" << getStatusAsString(result.getStatus()) << std::endl;
}

std::vector<double>
MarginCalculationLauncher::generateVariationsToLaunch(unsigned int maxNumber, double variation,
                                                      double minVariation, double maxVariation, double tolerance) const {
  std::set<double, dynawoDoubleLess> variationsToLaunch;
  variationsToLaunch.insert(variation);

  // Hack to add 0. if the load increase is below 50. as we know we never did 0. in the first place
  if (loadIncreaseStatus_.find(0.) == loadIncreaseStatus_.end() && variation < 50.)
    variationsToLaunch.insert(0.);
  if (variationsToLaunch.size() < maxNumber) {
    std::queue< std::pair<double, double> > levels;
    double closestVariationBelow = minVariation;
    double closestVariationAbove = maxVariation;
    for (const auto& status : loadIncreaseStatus_) {
      if (closestVariationBelow < status.first && status.first < variation)
        closestVariationBelow = status.first;
      if (status.first < closestVariationAbove &&  variation < status.first)
        closestVariationAbove = status.first;
    }
    levels.emplace(std::make_pair(closestVariationBelow, closestVariationAbove));
    while (!levels.empty() && variationsToLaunch.size() < maxNumber) {
      std::pair<double, double> currentLevel = levels.front();
      levels.pop();
      double nextVariation = round((currentLevel.first + currentLevel.second)/2.);
      variationsToLaunch.insert(nextVariation);
      if (currentLevel.second - nextVariation > tolerance)
        levels.emplace(std::make_pair(nextVariation, currentLevel.second));
      if (DYN::doubleNotEquals(nextVariation, 50.) && nextVariation - currentLevel.first > tolerance)
        levels.emplace(std::make_pair(currentLevel.first, nextVariation));
    }
  }
  std::vector<double> variationsToLaunchVector(variationsToLaunch.begin(), variationsToLaunch.end());
  return variationsToLaunchVector;
}

#ifdef _MPI_
std::vector<bool>
MarginCalculationLauncher::synchronizeSuccesses(const std::vector<bool>& successes) {
  auto& context = multiprocessing::context();
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
#endif

std::string
MarginCalculationLauncher::computeLoadIncreaseScenarioId(double variation) {
  std::stringstream ss;
  ss << LOAD_INCREASE << "-" << variation;
  return ss.str();
}

void
MarginCalculationLauncher::findOrLaunchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
                                                    const double variation,
                                                    const double minVariation,
                                                    const double maxVariation,
                                                    const double tolerance,
                                                    LoadIncreaseResult& loadIncreaseResult) {
#ifndef _MPI_
  static_cast<void>(minVariation);
  static_cast<void>(maxVariation);
  static_cast<void>(tolerance);
#endif

  TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, variation) << Trace::endline;

  auto found = loadIncreaseStatus_.find(variation);
  if (found != loadIncreaseStatus_.end()) {
    const SimulationResult importedLoadIncreaseResult1 = importResult(computeLoadIncreaseScenarioId(variation));
    loadIncreaseResult.setResult(importedLoadIncreaseResult1);
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseResultsFound, variation) << Trace::endline;
    return;
  }

  if (multiprocessing::context().nbProcs() == 1) {
    inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
    SimulationResult& loadIncreaseSimulationResult = loadIncreaseResult.getResult();
    launchLoadIncrease(loadIncrease, variation, loadIncreaseSimulationResult);
    loadIncreaseStatus_.insert(std::make_pair(variation, LoadIncreaseStatus(loadIncreaseSimulationResult.getSuccess())));

    // Hack to add 0. if the load increase is below 50. as we know we never did 0. in the first place
    if (variation < 50. && !loadIncreaseResult.getResult().getSuccess() && loadIncreaseStatus_.find(0.) == loadIncreaseStatus_.end()) {
      TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, 0.) << Trace::endline;
      SimulationResult result0;
      inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
      launchLoadIncrease(loadIncrease, 0., result0);
      loadIncreaseStatus_.insert(std::make_pair(0., LoadIncreaseStatus(result0.getSuccess())));
    }
    return;
  }

#ifdef _MPI_
  // Algo to generate variations to launch
  auto& context = multiprocessing::context();
  std::vector<double> variationsToLaunch = generateVariationsToLaunch(context.nbProcs(), variation, minVariation, maxVariation, tolerance);

  // Launch Simulations
  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  std::vector<bool> successes;
  multiprocessing::forEach(0, variationsToLaunch.size(), [this, &loadIncrease, &variationsToLaunch, &successes](unsigned int i){
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
    const SimulationResult importedLoadIncreaseResult2 = importResult(computeLoadIncreaseScenarioId(currVariation));
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseEnd, currVariation, getStatusAsString(importedLoadIncreaseResult2.getStatus())) << Trace::endline;
  }
  assert(loadIncreaseStatus_.count(variation) > 0);
  const SimulationResult importedLoadIncreaseResult3 = importResult(computeLoadIncreaseScenarioId(variation));
  loadIncreaseResult.setResult(importedLoadIncreaseResult3);
#endif
}

void
MarginCalculationLauncher::launchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, SimulationResult& result) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << "Launch loadIncrease of " << variation << "%" <<std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation;
  std::string workingDir = createAbsolutePath(loadIncrease->getId(), createAbsolutePath(subDir.str(), workingDirectory_));
  std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  SimulationParameters params;
  //  force simulation to dump final values (would be used as input to launch each event)
  params.activateDumpFinalState_ = true;
  params.activateExportIIDM_ = true;
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation << ".iidm";
  params.exportIIDMFile_ = createAbsolutePath(iidmFile.str(), workingDirectory_);
  std::stringstream dumpFile;
  dumpFile << "loadIncreaseFinalState-" << variation << ".dmp";
  params.dumpFinalStateFile_ = createAbsolutePath(dumpFile.str(), workingDirectory_);

  result.setScenarioId(LOAD_INCREASE);
  result.setVariation(variation);
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getMandatoryEnvVar("DYNAWO_DDB_DIR");
    auto subModels = modelMulti->findSubModelByLib(createAbsolutePath(std::string("DYNModelVariationArea") + DYN::sharedLibraryExtension(), DDBDir));
    for (const auto& subModel : subModels) {
      double startTime = subModel->findParameterDynamic("startTime").getValue<double>();
      double stopTime = subModel->findParameterDynamic("stopTime").getValue<double>();
      inputs_.setTLoadIncreaseVariationMax(stopTime - startTime);
      int nbLoads = subModel->findParameterDynamic("nbLoads").getValue<int>();
      for (int k = 0; k < nbLoads; ++k) {
        std::stringstream deltaPName;
        deltaPName << "deltaP_load_" << k;
        double deltaP = subModel->findParameterDynamic(deltaPName.str()).getValue<double>();
        subModel->setParameterValue(deltaPName.str(), DYN::PAR, deltaP*variation/100., false);

        std::stringstream deltaQName;
        deltaQName << "deltaQ_load_" << k;
        double deltaQ = subModel->findParameterDynamic(deltaQName.str()).getValue<double>();
        subModel->setParameterValue(deltaQName.str(), DYN::PAR, deltaQ*variation/100., false);
      }
      // change of the stop time to keep the same ramp of variation.
      double originalDuration = stopTime - startTime;
      double newStopTime = startTime + originalDuration * variation / 100.;
      subModel->setParameterValue("stopTime", DYN::PAR, newStopTime, false);
      subModel->setSubModelParameters();  // update values stored in subModel
      // Limitation for this log : will only be printed for root process
      TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseModelParameter, subModel->name(), newStopTime, variation/100.) << Trace::endline;
    }
    simulation->setStopTime(tLoadIncrease_ - (100. - variation)/100. * inputs_.getTLoadIncreaseVariationMax());
    simulate(simulation, result);
  }
}

void
MarginCalculationLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  Trace::resetCustomAppenders();  // to force flush
#ifndef NDEBUG
  Trace::resetPersistentCustomAppender(logTag_, DYN::DEBUG);  // to force flush
#else
  Trace::resetPersistentCustomAppender(logTag_, DYN::INFO);  // to force flush
#endif

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

  for (const auto& loadIncreaseResult : results_) {
    const double loadLevel = loadIncreaseResult.getResult().getVariation();
    const std::vector<SimulationResult>& allScenariosResults = loadIncreaseResult.getScenariosResults();
    size_t numberOfSuccessfulScenarios = 0;
    for (const SimulationResult& scenarioResult : allScenariosResults) {
      const std::string& scenarioId = scenarioResult.getScenarioId();
      if (scenarioResult.getSuccess()) {
        ++numberOfSuccessfulScenarios;
        const std::map<std::string, SimulationResult>::const_iterator itBest = bestResults.find(scenarioId);
        if (itBest == bestResults.end() || (loadLevel > itBest->second.getVariation()))
          bestResults[scenarioId] = scenarioResult;
      } else {
        const std::map<std::string, SimulationResult>::const_iterator itWorst = worstResults.find(scenarioId);
        if (itWorst == worstResults.end() || loadLevel < itWorst->second.getVariation())
          worstResults[scenarioId] = scenarioResult;
      }
    }
    if (loadIncreaseResult.getResult().getSuccess() && numberOfSuccessfulScenarios == allScenariosResults.size()) {
      const std::map<std::string, SimulationResult>::const_iterator loadIncreaseBestIt = bestResults.find(LOAD_INCREASE);
      if (loadIncreaseBestIt == bestResults.end() || loadLevel > loadIncreaseBestIt->second.getVariation()) {
        bestResults[LOAD_INCREASE] = loadIncreaseResult.getResult();
      }
    } else {
      const std::map<std::string, SimulationResult>::const_iterator loadIncreaseWorstIt = worstResults.find(LOAD_INCREASE);
      if (loadIncreaseWorstIt == worstResults.end() || loadLevel < loadIncreaseWorstIt->second.getVariation()) {
        worstResults[LOAD_INCREASE] = loadIncreaseResult.getResult();
      }
    }
  }

  for (const auto& bestResult : bestResults) {
    if (zipIt) {
      storeOutputs(bestResult.second, mapData);
    } else {
      writeOutputs(bestResult.second);
    }
  }
  for (const auto& worstResult : worstResults) {
    if (zipIt) {
      storeOutputs(worstResult.second, mapData);
    } else {
      writeOutputs(worstResult.second);
    }
  }
}

std::string
MarginCalculationLauncher::generateIDMFileNameForVariation(double variation) const {
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation << ".iidm";
  return createAbsolutePath(iidmFile.str(), workingDirectory_);
}

}  // namespace DYNAlgorithms
