//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
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
#include "DYNCriticalTimeLauncher.h"
#include "DYNMultipleJobs.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"

using multipleJobs::MultipleJobs;

namespace DYNAlgorithms {

void
CriticalTimeLauncher::setParametersAndLaunchSimulation(boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation) {
  inputs_.readInputs(workingDirectory_, criticalTimeCalculation->getJobsFile());
  std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  SimulationParameters params;
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDirectory_, job, params, results_, inputs_);
  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    const std::string& dydId = criticalTimeCalculation->getDydId();
    const std::string& parName = criticalTimeCalculation->getParName();
    if (modelMulti->findSubModelByName(dydId) != NULL) {
      boost::shared_ptr<DYN::SubModel> subModel_ = modelMulti->findSubModelByName(dydId);
      subModel_->setParameterValue(parName, DYN::PAR, tSup_, false);
      subModel_->setSubModelParameters();
    }
    std::cout << "tSup_ " << tSup_ << std::endl;
    simulate(simulation, results_);
  }
}

double
CriticalTimeLauncher::Round(double value) {
  const double multiplierRound = 1. / accuracy_;
  if (std::abs(value * multiplierRound - std::floor(value * multiplierRound)-0.5) < 1e-9)
    return std::floor((value) * multiplierRound) / multiplierRound;
  else
    return std::round((value) * multiplierRound) / multiplierRound;
}

void
CriticalTimeLauncher::launch() {
  boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation = multipleJobs_->getCriticalTimeCalculation();
  if (!criticalTimeCalculation)
    throw DYNAlgorithmsError(CriticalTimeCalculationTaskNotFound);

  const mode_t mode = criticalTimeCalculation->getMode();
  accuracy_ = criticalTimeCalculation->getAccuracy();

  int nbSimulationsDone = 0, nbSimulationsFailed = 0;
  tSup_ = criticalTimeCalculation->getMaxValue();  // time use in simulation
  double tMax = criticalTimeCalculation->getMaxValue();  // time use in simulation
  double tMin = criticalTimeCalculation->getMinValue();  // Bound min of the time
  double gap;
  double tLowestFailed = tSup_;  // min time where all times higher lead to a failed simulation
  std::map<double, std::pair<bool, status_t>> tTestedValues;  // stores every tested tSup_

  // While difference between lowest time of fail and Highest time of Success (tMin) is higher than the accuracy then continue loop
  while (std::abs((Round(tLowestFailed)-Round(tMin))-accuracy_) > 1e-9) {
    std::cout<< "tMax: " << tMax << "; tMin: " << tMin << std::endl;

    // Launch Simulation
    if (tTestedValues.find(tSup_) == tTestedValues.end()) {
      setParametersAndLaunchSimulation(criticalTimeCalculation);
    } else {  // if tested values already detected, no need to launch the simulation
      results_.setSuccess(tTestedValues[tSup_].first);
      results_.setStatus(tTestedValues[tSup_].second);
    }
    nbSimulationsDone++;
    tTestedValues[tSup_] = {results_.getSuccess(), results_.getStatus()};

    // Set tSup_, tMax et tMin
    gap = tMax - tMin;
    if (results_.getSuccess()) {
      if (nbSimulationsDone == 1) {
        status_ = DYNAlgorithms::CT_ABOVE_MAX_BOUND;
        return;
      }
      tMin = tMax;
      tMax = std::min(tLowestFailed, tMax + gap/2);
    } else {
      std::cout<< "Simulation Failed" << std::endl;
      nbSimulationsFailed++;

      if (mode == CriticalTimeCalculation::COMPLEX &&
        (results_.getStatus() == CRITERIA_NON_RESPECTED_STATUS || results_.getStatus() == DIVERGENCE_STATUS)) {
        // In case of solver issue, tMax become the previous lowest time with an issue minus the accuracy
        if (tMax == tLowestFailed - accuracy_) {tLowestFailed = tMax;}
        tMax = tLowestFailed - accuracy_;
        if (tMin > tMax) {std::swap(tMin, tMax);}  // By lowering tMax this way, tMin might become greater than tMax
      } else {
        tLowestFailed = tMax;
        tMax = tMax - gap/2;
      }
    }
    tSup_ = Round(tMax);
  }

  // Check if all simulations failed (if yes, min range might be the problem)
  if (nbSimulationsDone == nbSimulationsFailed) {
    status_ = DYNAlgorithms::CT_BELOW_MIN_BOUND;
    return;
  }
  // Set final Status and Critical time
  tSup_ = Round(tMin);
  status_ = DYNAlgorithms::RESULT_FOUND;
}

void
CriticalTimeLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportCriticalTimeResultsToStream(status_, tSup_, results_.getSimulationMessageError(), aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
    storeOutputs(results_, mapData);
  } else {
    exporter.exportCriticalTimeResultsToFile(status_, tSup_, results_.getSimulationMessageError(), outputFileFullPath_);
    writeOutputs(results_);
  }
}

}  // namespace DYNAlgorithms
