//
// Copyright (c) 2025, RTE (http://www.rte-france.com)
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
 * @file  CriticalTimeLauncher.cpp
 *
 * @brief Critical Time launcher: implementation of the algorithm and interaction with dynawo core
 *
 */

#include <DYNExecUtils.h>
#include <DYNSimulation.h>
#include <DYNSubModel.h>
#include <DYNModelMulti.h>
#include <DYNTrace.h>

#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNCriticalTimeLauncher.h"
#include "DYNMultipleJobs.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"
#include "DYNMultiProcessingContext.h"



using multipleJobs::MultipleJobs;

namespace DYNAlgorithms {

double
CriticalTimeLauncher::round(double value, double accuracy) {
  const double multiplierRound = 1. / accuracy;
  if (std::abs(value * multiplierRound - std::floor(value * multiplierRound)-0.5) < 1e-9)
    return std::floor((value) * multiplierRound) / multiplierRound;
  else
    return std::round((value) * multiplierRound) / multiplierRound;
}

void
CriticalTimeLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportCriticalTimeResultsToStream(results_, aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
  } else {
    exporter.exportCriticalTimeResultsToFile(results_, outputFileFullPath_);
  }

  for (unsigned int i=0; i < results_.size(); i++) {
    if (zipIt) {
      storeOutputs(results_[i].getResult(), mapData);
    } else {
      writeOutputs(results_[i].getResult());
    }
  }
}

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? DYN::Trace::info(tag) : DYN::TraceStream();
}

void
CriticalTimeLauncher::setParametersAndLaunchSimulation(std::string workingDir, boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation,
   const std::string dydFile, SimulationResult& result, double& tSup) {
  std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  addDydFileToJob(job, dydFile);

  SimulationParameters params;
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);
  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    const std::string& dydId = criticalTimeCalculation->getDydId();
    const std::string& parName = criticalTimeCalculation->getParName();
    if (modelMulti->findSubModelByName(dydId) != NULL) {
      boost::shared_ptr<DYN::SubModel> subModel_ = modelMulti->findSubModelByName(dydId);
      subModel_->setParameterValue(parName, DYN::PAR, tSup, false);
      subModel_->setSubModelParameters();
    }
    simulate(simulation, result);
  }
}

void
CriticalTimeLauncher::launch() {
  boost::posix_time::ptime t0 = boost::posix_time::second_clock::local_time();
  boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation = multipleJobs_->getCriticalTimeCalculation();
  if (!criticalTimeCalculation)
    throw DYNAlgorithmsError(CriticalTimeCalculationTaskNotFound);

  const boost::shared_ptr<Scenarios>& scenarios = criticalTimeCalculation->getScenarios();
  const std::string& baseJobsFile = scenarios->getJobsFile();
  if (!scenarios) {
    throw DYNAlgorithmsError(SystematicAnalysisTaskNotFound);
  }
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();

  // Check Inputs
  criticalTimeCalculation->checkMinValueInferiorMaxValue();
  criticalTimeCalculation->checkDydIdInDydFiles(workingDirectory_);

  results_.clear();
  results_.resize(events.size());
  auto& context = multiprocessing::context();

  if (context.isRootProc()) {
    // only required for root proc
    results_.resize(events.size());
  }

  multiprocessing::forEach(0, events.size(), [this, &events](unsigned int i) {
    std::string workingDir  = createAbsolutePath(events[i]->getId(), workingDirectory_);
    if (!exists(workingDir))
      create_directory(workingDir);
    else if (!is_directory(workingDir))
      throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
  });

  inputs_.readInputs(workingDirectory_, baseJobsFile);

  multiprocessing::forEach(0, events.size(), [this, &events, criticalTimeCalculation](unsigned int i){
    CriticalTimeResult ret = launchScenario(events[i], criticalTimeCalculation);
    exportResult(ret);
  });

  multiprocessing::Context::sync();

  // Update results for root proc
  if (context.isRootProc()) {
    for (unsigned int i = 0; i < events.size(); i++) {
      const auto& scenario = events.at(i);
      results_.at(i) = importResult(scenario->getId());
      cleanResult(scenario->getId());
    }
  }

  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  TraceInfo(logTag_) << "============================================================ " << DYN::Trace::endline;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Critical Time Calculation", diff.total_milliseconds()/1000) << DYN::Trace::endline;
  TraceInfo(logTag_) << "============================================================ " << DYN::Trace::endline;
}

CriticalTimeResult
CriticalTimeLauncher::launchScenario(const boost::shared_ptr<Scenario>& scenario, boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Launch scenario: " << scenario->getId() << " - dydFile: " << scenario->getDydFile() << std::endl;
  TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioLaunch, scenario->getId()) << DYN::Trace::endline;

  status_t status;
  SimulationResult result;
  std::string workingDir  = createAbsolutePath(scenario->getId(), workingDirectory_);

  result.setScenarioId(scenario->getId());

  const mode_t mode = criticalTimeCalculation->getMode();
  const double accuracy = criticalTimeCalculation->getAccuracy();
  double tMin = criticalTimeCalculation->getMinValue();  // Bound min of the time and also max time where all times lower lead to a succeeded simulation
  double tMax = criticalTimeCalculation->getMaxValue();  // Bound max of the time
  double tSup = tMax;  // time used in simulation
  double tLowestFailed = tMax;  // min time where all times higher lead to a failed simulation
  double gap;
  int nbSimulationsDone = 0, nbSimulationsFailed = 0;
  std::map<double, std::pair<bool, status_t>> tTestedValues;  // stores every tested tSup

  // While difference between lowest time of fail and Highest time of Success (tMin) is higher than the accuracy then continue loop
  while (std::abs(DYN::doubleNotEquals((round(tLowestFailed, accuracy)-round(tMin, accuracy)), accuracy))) {
    TraceInfo(logTag_) << DYNAlgorithmsLog(CriticalTimeValues, nbSimulationsDone, tMin, tMax, tSup) << DYN::Trace::endline;

    // Launch Simulation
    if (tTestedValues.find(tSup) == tTestedValues.end()) {
      setParametersAndLaunchSimulation(workingDir, criticalTimeCalculation, scenario->getDydFile(), result, tSup);
    } else {  // if tested values already detected, no need to launch the simulation
      result.setSuccess(tTestedValues[tSup].first);
      result.setStatus(tTestedValues[tSup].second);
    }
    nbSimulationsDone++;
    tTestedValues[tSup] = {result.getSuccess(), result.getStatus()};

    // Set tSup, tMax et tMin
    gap = tMax - tMin;
    if (result.getSuccess()) {
      // std::cout<< "Simulation Succeeded" << std::endl;
      tMin = tMax;
      tMax = std::min(tLowestFailed, tMax + gap/2);
      if (nbSimulationsDone == 1) {
        break;
      }
    } else {
      // std::cout<< "Simulation Failed" << std::endl;
      nbSimulationsFailed++;

      if (mode == CriticalTimeCalculation::COMPLEX &&
        (result.getStatus() == CRITERIA_NON_RESPECTED_STATUS || result.getStatus() == DIVERGENCE_STATUS)) {
        // In case of solver issue, tMax become the previous lowest time with an issue minus the accuracy
        if (tMax == tLowestFailed - accuracy) {tLowestFailed = tMax;}
        tMax = tLowestFailed - accuracy;
        if (tMin > tMax) {std::swap(tMin, tMax);}  // By lowering tMax this way, tMin might become greater than tMax
      } else {
        tLowestFailed = tMax;
        tMax = tMax - gap/2;
      }
    }
    tSup = round(tMax, accuracy);
  }

  // Set final Critical time and status
  tSup = round(tMin, accuracy);
  status = getFinalStatus(nbSimulationsDone, nbSimulationsFailed);

  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " scenario: " << scenario->getId() << " - final status: " << getStatusAsString(status) << "\n" << std::endl;

  CriticalTimeResult criticalTimeResult;
  criticalTimeResult.setId(scenario->getId());
  criticalTimeResult.setCriticalTime(tSup);
  criticalTimeResult.setResult(result);
  criticalTimeResult.setStatus(status);
  return criticalTimeResult;
}

status_t
CriticalTimeLauncher::getFinalStatus(double nbSimulationsDone, double nbSimulationsFailed) {
  if (nbSimulationsDone == nbSimulationsFailed) {
    // Check if all simulations failed (if yes, min range might be the problem)
    return DYNAlgorithms::CT_BELOW_MIN_BOUND;
  } else if (nbSimulationsDone == 1) {
    // If the first one succeed, max range might be the problem
    return DYNAlgorithms::CT_ABOVE_MAX_BOUND;
  } else {
    return DYNAlgorithms::RESULT_FOUND;
  }
}

void
CriticalTimeLauncher::exportResult(const CriticalTimeResult& result) const {
  DYNAlgorithms::RobustnessAnalysisLauncher::exportResult(result.getResult());

  namespace fs = boost::filesystem;
  fs::path filepath(createAbsolutePath(result.getId(), workingDirectory_));
  filepath.append("result.save.txt");

  std::ofstream file(filepath.generic_string(), std::ios::binary | std::ios::app);
  file.precision(precisionResultFile_);

  file << "criticalTime:" << result.getCriticicalTime() << std::endl;
  file << "calculationStatus:" << static_cast<unsigned int>(result.getStatus()) << std::endl;
  file << "lastMessageError:" << result.getResult().getSimulationMessageError() << std::endl;
}

CriticalTimeResult
CriticalTimeLauncher::importResult(const std::string& id) const {
  SimulationResult ret = DYNAlgorithms::RobustnessAnalysisLauncher::importResult(id);

  CriticalTimeResult result;
  result.setResult(ret);
  result.setId(id);
  auto filepath = computeResultFile(id);
  const char delimiter = ':';

  // Private type to modify the locale for ifstream
  // by default all white spaces are considered for '>>' operator
  class Delimiter : public std::ctype<char> {
   public:
     Delimiter(): std::ctype<char>(getTable()) {}
     static mask const* getTable() {
       static mask rc[table_size];
       rc['\n'] = std::ctype_base::space;
       return &rc[0];
     }
  };
  std::ifstream file(filepath.generic_string());
  if (file.fail()) {
    return result;
  }
  file.precision(precisionResultFile_);
  file.imbue(std::locale(file.getloc(), new Delimiter));

  // critical time
  std::string tmpStr;
  double criticalTime;
  while (file >> tmpStr) {
    if (tmpStr.find("criticalTime:") == 0) {
      tmpStr = tmpStr.substr(tmpStr.find(delimiter) + 1);
      std::stringstream ss(tmpStr);
      ss >> criticalTime;
      result.setCriticalTime(criticalTime);
      break;
    }
  }

  // Calculation Status
  unsigned int calculationStatus;
  std::stringstream ss;
  file >> tmpStr;
  assert(tmpStr.find("calculationStatus:") == 0);
  tmpStr = tmpStr.substr(tmpStr.find(delimiter)+1);
  ss.clear();
  ss.str(tmpStr);
  ss >> calculationStatus;
  result.setStatus(static_cast<status_t>(calculationStatus));

  // Message Error
  std::string lastMessageError;
  file >> tmpStr;
  assert(tmpStr.find("lastMessageError:") == 0);
  lastMessageError = tmpStr.substr(tmpStr.find(delimiter) + 1);
  lastMessageError.erase(0, lastMessageError.find_first_not_of(" "));
  result.getResult().setSimulationMessageError(lastMessageError);

  return result;
}

}  // namespace DYNAlgorithms
