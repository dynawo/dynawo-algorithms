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
  initArrays(marginCalculation->getAccuracy(), events.size());

  bool globalMargin = marginCalculation->getCalculationType() == MarginCalculation::GLOBAL_MARGIN;

  int marginIdGlobal = discreteVars_.size()-1;
  std::vector<int> marginScens(events.size(), -1);

  for (int scenId = 0; scenId < events.size(); ++scenId) {
    const boost::shared_ptr<Scenario> & scenario = events[scenId];

    int varIdInf = 0;
    int varIdSup = globalMargin ? marginIdGlobal : getVarIdStart();

    if (launchScenarioWrapper(loadIncrease, baseJobsFile, scenario, scenId, varIdSup)) {
      if (globalMargin || (varIdSup == discreteVars_.size()-1)) {
        // max possible varID success : scenario finished
        marginScens[scenId] = discreteVars_[varIdSup];
        continue;
      } else {  // local margin computation and possible valid load increases above
        varIdInf = varIdSup;
        varIdSup = getLowestLIFailureVarId();
      }
    }

    while (varIdSup > varIdInf+1) {
      int varIdNext = getVarIdNext(varIdInf, varIdSup);
      if (launchScenarioWrapper(loadIncrease, baseJobsFile, scenario, scenId, varIdNext)) {
        varIdInf = varIdNext;
      } else {
        varIdSup = varIdNext;
        // heuristic optim : test zero only if 50% fails
        if ((varIdInf == 0) && (results_[0].getScenarioResult(scenId).getScenarioId() == "")) {
          if (!launchScenarioWrapper(loadIncrease, baseJobsFile, scenario, scenId, 0)) {
            if (globalMargin) {  // scenario failure at 0% and global margin : exit
              marginScens.clear();
              finish(events, t0, 0, marginScens);
              return;
            } else {  // scenario failure at 0% and local margin : varIdMin rightly contains the correct margin at 0%
              break;
            }
          }
        }  // if 0% succeeds, we are right back on track
      }
    }
    marginIdGlobal = std::min(marginIdGlobal, varIdInf);
    marginScens[scenId] = discreteVars_[varIdInf];
  }

  if (events.size() == 0)  // fringe case that might actually be used in prod
    marginIdGlobal = findMaxLoadIncrease(loadIncrease);

  if (globalMargin)
    marginScens.clear();

  finish(events, t0, discreteVars_[marginIdGlobal], marginScens);
}

void
MarginCalculationLauncher::initArrays(int accuracy, int nbScenarios) {
  discreteVars_.clear();
  results_.clear();

  int step = 0;
  while (step < 100) {
    discreteVars_.push_back(step);
    results_.emplace_back(nbScenarios);
    step += accuracy;
  }
  discreteVars_.push_back(100);
  results_.emplace_back(nbScenarios);
}

int
MarginCalculationLauncher::findMaxLoadIncrease(const boost::shared_ptr<LoadIncrease> & loadIncrease) {
  int varId100 = discreteVars_.size()-1;
  if (launchLoadIncreaseWrapper(loadIncrease, varId100))
    return varId100;

  int varIdInf = 0;  // lowest known that succeeds
  int varIdSup = varId100;  // highest known that fails;

  while (varIdSup > varIdInf+1) {
    int varIdNext = (varIdInf+varIdSup+1)/2;  // bias towards upper in integer roundings
    if (launchLoadIncreaseWrapper(loadIncrease, varIdNext)) {
      varIdInf = varIdNext;
    } else {
      varIdSup = varIdNext;
      if ((varIdInf == 0) && !liDone(0)) {  // heuristic optim : try 50% before 0%
        if (!launchLoadIncreaseWrapper(loadIncrease, 0))
          return -1;
      }
    }
  }

  return varIdInf;
}

int
MarginCalculationLauncher::getVarIdStart() const {
  for (int varId = discreteVars_.size()-1; varId >= 0; --varId)
      if (liDone(varId) && results_[varId].getResult().getSuccess())
        return varId;

  return discreteVars_.size()-1;
}

int
MarginCalculationLauncher::getVarIdNext(int varIdMin, int varIdMax) const {
  int varIdMid = (varIdMin+varIdMax+1)/2;
  int varIdNext = varIdMid;
  int distMin = 1000;

  for (int varId = varIdMin+1; varId <= varIdMax-1; ++varId) {
    if (liDone(varId)) {
      int dist = std::abs(varId-varIdMid);
      if (dist < distMin) {
        varIdNext = varId;
        distMin = dist;
      }
    }
  }
  return varIdNext;
}

int
MarginCalculationLauncher::getLowestLIFailureVarId() const {
  for (int varId = 0; varId < discreteVars_.size(); ++varId)
      if (liDone(varId) && !results_[varId].getResult().getSuccess())
        return varId;
  return -1;
}

bool
MarginCalculationLauncher::launchLoadIncreaseWrapper(const boost::shared_ptr<LoadIncrease> & loadIncrease, int varId) {
  TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, discreteVars_[varId]) << Trace::endline;
  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  SimulationResult & result = results_[varId].getResult();
  launchLoadIncrease(loadIncrease, discreteVars_[varId], result);
  TraceInfo(logTag_) << Trace::endline;
  return result.getSuccess();
}

bool
MarginCalculationLauncher::launchScenarioWrapper(const boost::shared_ptr<LoadIncrease> & loadIncrease,
                                                 const std::string& baseJobsFile,
                                                 const boost::shared_ptr<Scenario> & scenario,
                                                 int scenId,
                                                 int varId) {
  if (results_[varId].getResult().getVariation() < 0)
    if (!launchLoadIncreaseWrapper(loadIncrease, varId))
      return false;

  int variation = discreteVars_[varId];

  std::string iidmFile = generateIDMFileNameForVariation(variation);
  if (inputsByIIDM_.count(iidmFile) == 0)  // read inputs only if not already existing with enough variants defined
    inputsByIIDM_[iidmFile].readInputs(workingDirectory_, baseJobsFile, iidmFile);

  SimulationResult & resultScen = results_[varId].getScenarioResult(scenId);
  launchScenario(inputsByIIDM_[iidmFile], scenario, variation, resultScen);
  TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, resultScen.getUniqueScenarioId(), getStatusAsString(resultScen.getStatus()))
                     << Trace::endline << Trace::endline;
  return resultScen.getSuccess();
}


void
MarginCalculationLauncher::finish(const std::vector<boost::shared_ptr<Scenario> > & events,
                                  boost::posix_time::ptime & t0,
                                  int marginGlobal,
                                  const std::vector<int> & marginScens) {
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, marginGlobal) << Trace::endline;
  for (int scenId = 0; scenId < marginScens.size(); ++scenId)
    TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, events[scenId]->getId(), marginScens[scenId]) << Trace::endline;
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;

  // purge unused results for readability of aggregatedResults.xml
  std::vector<LoadIncreaseResult> resultsTemp;
  resultsTemp.swap(results_);
  for (LoadIncreaseResult & result : resultsTemp)
    if (result.getResult().getVariation() >= 0) {
      for (int scenId = 0; scenId < events.size(); ++scenId) {
        SimulationResult & resultScen = result.getScenarioResult(scenId);
        if (resultScen.getScenarioId() == "") {
          resultScen.setScenarioId(events[scenId]->getId());
          resultScen.setVariation(result.getResult().getVariation());
          resultScen.setStatus(RESULT_FOUND_STATUS);
        }
      }
      results_.push_back(result);
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
