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

#ifdef WITH_OPENMP
#include <omp.h>
#endif

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

#include "DYNMarginCalculationLauncher.h"
#include "DYNMultipleJobs.h"
#include "DYNMarginCalculation.h"
#include "DYNScenario.h"
#include "MacrosMessage.h"
#include "DYNScenarios.h"
#include "DYNAggrResXmlExporter.h"

using multipleJobs::MultipleJobs;
using DYN::Trace;

namespace DYNAlgorithms {

MarginCalculationLauncher::~MarginCalculationLauncher() {
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
MarginCalculationLauncher::launch() {
  assert(multipleJobs_);
  results_.clear();
  boost::shared_ptr<MarginCalculation> marginCalculation = multipleJobs_->getMarginCalculation();
  if (!marginCalculation) {
    throw DYNAlgorithmsError(MarginCalculationTaskNotFound);
  }
  const boost::shared_ptr<LoadIncrease>& loadIncrease = marginCalculation->getLoadIncrease();
  const boost::shared_ptr<Scenarios>& scenarios = marginCalculation->getScenarios();
  const std::string& baseJobsFile = scenarios->getJobsFile();
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();
#ifdef WITH_OPENMP
  omp_set_num_threads(nbThreads_);
#endif

  std::queue< task_t > toRun;
  std::vector<size_t> allEvents;
  for (size_t i=0, iEnd = events.size(); i < iEnd ; i++)
    allEvents.push_back(i);
  toRun.push(task_t(100, 100, allEvents));
  toRun.push(task_t(0, 0, allEvents));

  results_.push_back(LoadIncreaseResult());
  size_t idx = results_.size() - 1;
  results_[idx].resize(events.size());
  results_[idx].setLoadLevel(100.);

  // step one : launch the loadIncrease and then all events with 100% of the load increase
  // if there is no crash => no need to go further
  // We start with 100% as it is the most common result of margin calculations on real large cases
  SimulationResult result100;
  findOrLaunchLoadIncrease(loadIncrease, 100, marginCalculation->getAccuracy(), result100);
  results_[idx].setStatus(result100.getStatus());
  std::vector<double > maximumVariationPassing(events.size(), 0.);
  if (result100.getSuccess()) {
    findOrLaunchScenarios(baseJobsFile, events, toRun, results_[idx]);

    // analyze results
    unsigned int nbSuccess = 0;
    size_t id = 0;
    for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
        itEnd = results_[idx].end(); it != itEnd; ++it, ++id) {
      Trace::info(logTag_) << DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
      if (it->getStatus() == CONVERGENCE_STATUS) {  // event OK
        nbSuccess++;
        maximumVariationPassing[id] = 100;
      }
    }
    if (nbSuccess == events.size()) {  // all events succeed
      Trace::info(logTag_) << "============================================================ " << Trace::endline;
      Trace::info(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, 100.) << Trace::endline;
      Trace::info(logTag_) << "============================================================ " << Trace::endline;
      return;
    }
  }  // if the loadIncrease failed, nothing to do, the next algorithm will try to find the right load level
  Trace::info(logTag_) << Trace::endline;

  toRun = std::queue< task_t >();
  toRun.push(task_t(0, 0, allEvents));

  results_.push_back(LoadIncreaseResult());
  idx = results_.size() - 1;
  results_[idx].resize(events.size());
  results_[idx].setLoadLevel(0.);
  // step two : launch the loadIncrease and then all events with 0% of the load increase
  // if one event crash => no need to go further
  SimulationResult result0;
  findOrLaunchLoadIncrease(loadIncrease, 0, marginCalculation->getAccuracy(), result0);
  results_[idx].setStatus(result0.getStatus());
  if (result0.getSuccess()) {
    findOrLaunchScenarios(baseJobsFile, events, toRun, results_[idx]);
  } else {
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
    Trace::info(logTag_) << DYNAlgorithmsLog(LocalMarginValueLoadIncrease, 0.) << Trace::endline;
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
    return;  // unable to launch the initial simulation with 0% of load increase
  }

  // analyze results
  for (std::vector<SimulationResult>::const_iterator it = results_[idx].begin(),
      itEnd = results_[idx].end(); it != itEnd; ++it) {
    Trace::info(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
    if (it->getStatus() != CONVERGENCE_STATUS) {  // one event crashes
      Trace::info(logTag_) << "============================================================ " << Trace::endline;
      Trace::info(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, it->getUniqueScenarioId(), 0.) << Trace::endline;
      Trace::info(logTag_) << "============================================================ " << Trace::endline;
      return;
    }
  }
  Trace::info(logTag_) << Trace::endline;

  if (marginCalculation->getCalculationType() == MarginCalculation::GLOBAL_MARGIN || events.size() == 1) {
    double value = computeGlobalMargin(loadIncrease, baseJobsFile, events, maximumVariationPassing, marginCalculation->getAccuracy());
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
    Trace::info(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, value) << Trace::endline;
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
  } else {
    assert(marginCalculation->getCalculationType() == MarginCalculation::LOCAL_MARGIN);
    std::vector<double> results(events.size(), 0);
    double value = computeLocalMargin(loadIncrease, baseJobsFile, events, marginCalculation->getAccuracy(), results);
    if (result100.getSuccess()) {
      value = 100;
    }
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
    Trace::info(logTag_) << DYNAlgorithmsLog(LocalMarginValueLoadIncrease, value) << Trace::endline;
    for (size_t i = 0, iEnd = results.size(); i < iEnd; ++i) {
      Trace::info(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, events[i]->getId(), results[i]) << Trace::endline;
    }
    Trace::info(logTag_) << "============================================================ " << Trace::endline;
  }
}

double
MarginCalculationLauncher::computeGlobalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events,
    std::vector<double >& maximumVariationPassing, double tolerance) {
  double minVariation = 0;
  double maxVariation = 100;

  while ( maxVariation - minVariation > tolerance ) {
    double newVariation = round((minVariation + maxVariation)/2);
    results_.push_back(LoadIncreaseResult());
    size_t idx = results_.size() - 1;
    results_[idx].resize(events.size());
    results_[idx].setLoadLevel(newVariation);
    SimulationResult result;
    std::stringstream variationString;
    variationString << newVariation;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, tolerance, result);
    results_[idx].setStatus(result.getStatus());
    if (result.getSuccess()) {
      std::queue< task_t > toRun;
      std::vector<size_t> eventsIds;
      for (size_t i=0, iEnd = events.size(); i < iEnd ; i++) {
        if (newVariation > maximumVariationPassing[i]) {
          eventsIds.push_back(i);
        } else {
          results_[idx].getResult(i).setScenarioId(events[i]->getId());
          results_[idx].getResult(i).setVariation(variationString.str());
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
        if (newVariation <= maximumVariationPassing[id])
          Trace::info(logTag_) << DYNAlgorithmsLog(ScenarioNotSimulated, it->getUniqueScenarioId()) << Trace::endline;
        else
          Trace::info(logTag_) << DYNAlgorithmsLog(ScenariosEnd, it->getUniqueScenarioId(), getStatusAsString(it->getStatus())) << Trace::endline;
        if (it->getStatus() == CONVERGENCE_STATUS || newVariation <= maximumVariationPassing[id]) {  // event OK
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
    Trace::info(logTag_) << Trace::endline;
  }
  return minVariation;
}

void
MarginCalculationLauncher::findAllLevelsBetween(const double minVariation, const double maxVariation, const double tolerance,
    const std::vector<size_t>& eventIdxs, std::queue< task_t >& toRun) {
  unsigned nbMaxToAdd = nbThreads_/eventIdxs.size();
  if (nbMaxToAdd == 0) nbMaxToAdd = 1;
  double newVariation = round((minVariation + maxVariation)/2);
  std::queue< std::pair<double, double> > minMaxStack;

  toRun.push(task_t(minVariation, maxVariation, eventIdxs));
  if (maxVariation - newVariation > tolerance)
    minMaxStack.push(std::make_pair(newVariation, maxVariation));
  while (toRun.size() < nbMaxToAdd && !minMaxStack.empty()) {
    double min = minMaxStack.front().first;
    double max = minMaxStack.front().second;
    minMaxStack.pop();
    double nextVar = round((min + max)/2);
    if (loadIncreaseCache_.find(nextVar) == loadIncreaseCache_.end()) continue;
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
  for (size_t i=0, iEnd = events.size(); i < iEnd ; i++)
    all.push_back(i);
  toRun.push(task_t(0, 100, all));

  while (!toRun.empty()) {
    std::queue< task_t > toRunCopy(toRun);  // Needed as findOrLaunchScenarios modifies the queue
    task_t task = toRun.front();
    toRun.pop();
    const std::vector<size_t>& eventsId = task.ids_;
    double newVariation = round((task.minVariation_ + task.maxVariation_)/2);
    results_.push_back(LoadIncreaseResult());
    size_t idx = results_.size() - 1;
    results_[idx].resize(eventsId.size());
    results_[idx].setLoadLevel(newVariation);
    SimulationResult result;
    findOrLaunchLoadIncrease(loadIncrease, newVariation, tolerance, result);
    results_[idx].setStatus(result.getStatus());

    if (result.getSuccess()) {
      if (maxLoadVarForLoadIncrease < newVariation)
        maxLoadVarForLoadIncrease = newVariation;
      LoadIncreaseResult liResultTmp;
      liResultTmp.resize(events.size());
      findOrLaunchScenarios(baseJobsFile, events, toRunCopy, liResultTmp);

      // analyze results
      task_t below(task.minVariation_, newVariation);
      task_t above(newVariation, task.maxVariation_);
      for (size_t i = 0, iEnd = eventsId.size(); i < iEnd; ++i) {
        results_[idx].getResult(i) = liResultTmp.getResult(eventsId[i]);
        Trace::info(logTag_) << DYNAlgorithmsLog(ScenariosEnd,
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
      for (size_t i = 0, iEnd = eventsId.size(); i < iEnd; ++i) {
        below.ids_.push_back(eventsId[i]);
      }
      if (!below.ids_.empty())
        toRun.push(below);
    }
    Trace::info(logTag_) << Trace::endline;
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
  double newVariation = round((task.minVariation_ + task.maxVariation_)/2);
  if (nbThreads_ == 1) {
    std::string iidmFile = generateIDMFileNameForVariation(newVariation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      // read inputs only if not already existing with enough variants defined
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, eventsId.size(), iidmFile);
    }
    for (unsigned int i=0; i < eventsId.size(); i++) {
      inputsByIIDM_.at(iidmFile).setCurrentVariant(i);
      launchScenario(inputsByIIDM_[iidmFile], events[eventsId[i]], newVariation, result.getResult(eventsId[i]));
    }
    return;
  }
  std::map<double, LoadIncreaseResult, dynawoDoubleLess>::iterator it = scenariosCache_.find(newVariation);
  if (it != scenariosCache_.end()) {
    Trace::info(logTag_) << DYNAlgorithmsLog(ScenarioResultsFound, newVariation) << Trace::endline;
    for (unsigned int i=0; i < eventsId.size(); i++)
      result.getResult(eventsId[i]) = it->second.getResult(eventsId[i]);
    return;
  }
  std::vector<std::pair<size_t, double> > events2Run;
  prepareEvents2Run(task, toRun, events2Run);
  for (unsigned int i=0; i < events2Run.size(); i++) {
    double variation = events2Run[i].second;
    size_t eventIdx = events2Run[i].first;
    if (scenariosCache_.find(variation) == scenariosCache_.end()) {
      scenariosCache_[variation] = LoadIncreaseResult();  // Reserve memory
      scenariosCache_[variation].resize(events.size());
    }
    createScenarioWorkingDir(events[eventIdx]->getId(), variation);
  }

  for (std::vector<std::pair<size_t, double> >::const_iterator it = events2Run.begin(); it != events2Run.end(); ++it) {
    double variation = it->second;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    if (inputsByIIDM_.count(iidmFile) == 0) {
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, events.size(), iidmFile);
    }
  }

#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int i=0; i < events2Run.size(); i++) {
    double variation = events2Run[i].second;
    std::string iidmFile = generateIDMFileNameForVariation(variation);
    size_t eventIdx = events2Run[i].first;
    inputsByIIDM_.at(iidmFile).setCurrentVariant(eventIdx);
    launchScenario(inputsByIIDM_.at(iidmFile), events[eventIdx], variation, scenariosCache_[variation].getResult(eventIdx));
  }
  assert(scenariosCache_.find(newVariation) != scenariosCache_.end());
  for (unsigned int i=0; i < eventsId.size(); i++)
    result.getResult(eventsId[i]) = scenariosCache_[newVariation].getResult(eventsId[i]);
}

void
MarginCalculationLauncher::prepareEvents2Run(const task_t& requestedTask,
    std::queue< task_t >& toRun,
    std::vector<std::pair<size_t, double> >& events2Run) {
  const std::vector<size_t>& eventsId = requestedTask.ids_;
  double newVariation = round((requestedTask.minVariation_ + requestedTask.maxVariation_)/2);
  for (size_t i = 0, iEnd = eventsId.size(); i < iEnd; ++i) {
    events2Run.push_back(std::make_pair(eventsId[i], newVariation));
  }
  while (events2Run.size() < static_cast<size_t>(nbThreads_) && !toRun.empty()) {
    task_t newTask = toRun.front();
    toRun.pop();
    const std::vector<size_t>& newEventsId = newTask.ids_;
    double variation = round((newTask.minVariation_ + newTask.maxVariation_)/2);
    if (scenariosCache_.find(variation) != scenariosCache_.end()) continue;
    for (size_t i = 0, iEnd = newEventsId.size(); i < iEnd; ++i) {
      events2Run.push_back(std::make_pair(newEventsId[i], variation));
    }
  }
}

void
MarginCalculationLauncher::launchScenario(const MultiVariantInputs& inputs, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result) {
  if (nbThreads_ == 1)
    std::cout << " Launch task :" << scenario->getId() << " dydFile =" << scenario->getDydFile() << std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation << "/" << scenario->getId();
  std::string workingDir = createAbsolutePath(subDir.str(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();
  addDydFileToJob(job, scenario->getDydFile());

  SimulationParameters params;
  std::stringstream dumpFile;
  dumpFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".dmp";
  //  force simulation to load previous dump and to use final values
  params.InitialStateFile_ = dumpFile.str();
  params.iidmFile_ = generateIDMFileNameForVariation(variation);
  std::stringstream scenarioId;
  scenarioId << variation;
  result.setScenarioId(scenario->getId());
  result.setVariation(scenarioId.str());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs);

  if (simulation)
    simulate(simulation, result);

  if (nbThreads_ == 1)
    std::cout << " Task :" << scenario->getId() << " status =" << getStatusAsString(result.getStatus()) << std::endl;
}

void
MarginCalculationLauncher::findOrLaunchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, const double tolerance, SimulationResult& result) {
  Trace::info(logTag_) << DYNAlgorithmsLog(VariationValue, variation) << Trace::endline;
  if (nbThreads_ == 1) {
    inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile(), 1);
    inputs_.setCurrentVariant(0);
    launchLoadIncrease(loadIncrease, variation, result);
    return;
  }

  std::map<double, SimulationResult, dynawoDoubleLess>::const_iterator it = loadIncreaseCache_.find(variation);
  if (it != loadIncreaseCache_.end()) {
    Trace::info(logTag_) << DYNAlgorithmsLog(LoadIncreaseResultsFound, variation) << Trace::endline;
    result = it->second;
    return;
  }
  std::vector<double> variationsToLaunch;
  if (loadIncreaseCache_.empty()) {
    // First time we call this, we know we have at least 2 threads.
    variationsToLaunch.push_back(0.);
    variationsToLaunch.push_back(100.);
  }
  if (static_cast<int>(variationsToLaunch.size()) < nbThreads_) {
    std::queue< std::pair<double, double> > levels;
    double closestVariationBelow = 0.;
    double closestVariationAbove = 100.;
    for (std::map<double, SimulationResult, dynawoDoubleLess>::const_iterator it = loadIncreaseCache_.begin(),
        itEnd = loadIncreaseCache_.end(); it != itEnd; ++it) {
      if (closestVariationBelow < it->first && it->first < variation)
        closestVariationBelow = it->first;
      if (it->first < closestVariationAbove &&  variation < it->first)
        closestVariationAbove = it->first;
    }
    levels.push(std::make_pair(closestVariationBelow, closestVariationAbove));
    while (!levels.empty() && static_cast<int>(variationsToLaunch.size()) < nbThreads_) {
      std::pair<double, double> currentLevel = levels.front();
      levels.pop();
      double nextVariation = round((currentLevel.first + currentLevel.second)/2);
      variationsToLaunch.push_back(nextVariation);
      if (currentLevel.second - nextVariation > tolerance)
        levels.push(std::make_pair(nextVariation, currentLevel.second));
      if (nextVariation != 50. && nextVariation - currentLevel.first > tolerance)
        levels.push(std::make_pair(currentLevel.first, nextVariation));
    }
  }
  for (unsigned int i=0; i < variationsToLaunch.size(); i++) {
    loadIncreaseCache_[variationsToLaunch[i]] = SimulationResult();  // Reserve memory
    createScenarioWorkingDir(loadIncrease->getId(), variationsToLaunch[i]);
  }

  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile(), variationsToLaunch.size());

#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int i=0; i < variationsToLaunch.size(); i++) {
    inputs_.setCurrentVariant(i);
    launchLoadIncrease(loadIncrease, variationsToLaunch[i], loadIncreaseCache_[variationsToLaunch[i]]);
  }
  assert(loadIncreaseCache_.find(variation) != loadIncreaseCache_.end());
  result = loadIncreaseCache_[variation];
}

void
MarginCalculationLauncher::launchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, SimulationResult& result) {
  if (nbThreads_ == 1)
    std::cout << "Launch loadIncrease of " << variation << "%" <<std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation << "/" << loadIncrease->getId();
  std::string workingDir = createAbsolutePath(subDir.str(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  SimulationParameters params;
  //  force simulation to dump final values (would be used as input to launch each events)
  params.activateDumpFinalState_ = true;
  params.activateExportIIDM_ = true;
  std::stringstream iidmFile;
  iidmFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".iidm";
  params.exportIIDMFile_ = iidmFile.str();
  std::stringstream dumpFile;
  dumpFile << workingDirectory_ << "/loadIncreaseFinalState-" << variation << ".dmp";
  params.dumpFinalStateFile_ = dumpFile.str();

  std::string DDBDir = getMandatoryEnvVar("DYNAWO_DDB_DIR");
  std::stringstream scenarioId;
  scenarioId << "loadIncrease-" << variation;
  result.setScenarioId(scenarioId.str());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    boost::shared_ptr<DYN::ModelMulti> modelMulti = boost::dynamic_pointer_cast<DYN::ModelMulti>(simulation->model_);
    std::vector<boost::shared_ptr<DYN::SubModel> > subModels = modelMulti->findSubModelByLib(DDBDir + "/DYNModelVariationArea" + DYN::sharedLibraryExtension());
    for (unsigned int i=0; i < subModels.size(); i++) {
      double startTime = subModels[i]->findParameterDynamic("startTime").getValue<double>();
      double stopTime = subModels[i]->findParameterDynamic("stopTime").getValue<double>();
      int nbLoads = subModels[i]->findParameterDynamic("nbLoads").getValue<int>();
      for (int k = 0; k < nbLoads; ++k) {
        std::stringstream deltaPName;
        deltaPName << "deltaP_load_" << k;
        double deltaP = subModels[i]->findParameterDynamic(deltaPName.str()).getValue<double>();
        subModels[i]->setParameterValue(deltaPName.str(), DYN::PAR, deltaP*variation/100., false);

        std::stringstream deltaQName;
        deltaQName << "deltaQ_load_" << k;
        double deltaQ = subModels[i]->findParameterDynamic(deltaQName.str()).getValue<double>();
        subModels[i]->setParameterValue(deltaQName.str(), DYN::PAR, deltaQ*variation/100., false);
      }
      // change of the stop time to keep the same ramp of variation.
      double originalDuration = stopTime - startTime;
      double newStopTime = startTime + originalDuration * variation / 100.;
      subModels[i]->setParameterValue("stopTime", DYN::PAR, newStopTime, false);
      subModels[i]->setSubModelParameters();  // update values stored in subModel
      Trace::info(logTag_) << DYNAlgorithmsLog(LoadIncreaseModelParameter, subModels[i]->name(), newStopTime, variation/100) << Trace::endline;
    }
    simulate(simulation, result);

    simulation->printTimeline(result.getTimelineStream());
    simulation->printConstraints(result.getConstraintsStream());
  }
  Trace::info(logTag_) << DYNAlgorithmsLog(LoadIncreaseEnd, getStatusAsString(result.getStatus())) << Trace::endline;
}

void
MarginCalculationLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  Trace::resetCustomAppenders();  // to force flush
  Trace::resetPersistantCustomAppenders();  // to force flush
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

  for (size_t i=0, iEnd = results_.size(); i < iEnd; i++) {
    for (std::vector<SimulationResult>::const_iterator it = results_[i].begin(),
        itEnd = results_[i].end(); it != itEnd; ++it) {
      if (zipIt) {
        storeOutputs(*it, mapData);
      } else {
        writeOutputs(*it);
      }
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
