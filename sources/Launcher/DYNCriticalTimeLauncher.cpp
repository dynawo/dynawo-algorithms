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
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  SimulationParameters params;
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDirectory_, job, params, results_, inputs_);
  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    const std::string& dydId = criticalTimeCalculation->getDydId();
    const std::string& endPar = criticalTimeCalculation->getEndPar();
    if (modelMulti->findSubModelByName(dydId) != NULL) {
      boost::shared_ptr<DYN::SubModel> subModel_ = modelMulti->findSubModelByName(dydId);
      subModel_->setParameterValue(endPar, DYN::PAR, tSup_, false);
      subModel_->setSubModelParameters();
    }
    std::cout << "tSup_ " << tSup_ << std::endl;
    simulate(simulation, results_);
  }
}

void
CriticalTimeLauncher::updateIndexes(double& tPrevious, double& curAccuracy, const double& multiplierRound, int& nbSimulationsFailed) {
  curAccuracy = std::abs(tSup_ - tPrevious);
  double midDichotomy = std::round((std::abs(tSup_ - tPrevious) / 2) * multiplierRound) / multiplierRound;
  double tmp = tSup_;

  if (results_.getSuccess()) {
    tSup_ += midDichotomy;
  } else {
    tSup_ -= midDichotomy;
    nbSimulationsFailed++;
  }

  tPrevious = tmp;
}
double
CriticalTimeLauncher::Round(double value, const double multiplierRound) {
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

  const double accuracy = criticalTimeCalculation->getAccuracy();
  const double multiplierRound = 1. / accuracy;
  double curAccuracy = 1.;
  tSup_ = criticalTimeCalculation->getMaxValue();
  const double tInf = criticalTimeCalculation->getMinValue();
  double tPrevious = tSup_;
  int nbSimulationsDone = 0;
  int nbSimulationsFailed = 0;

  // First simulation case.
  setParametersAndLaunchSimulation(criticalTimeCalculation);
  nbSimulationsDone++;
  if (results_.getSuccess()) {
    status_ = DYNAlgorithms::CT_ABOVE_MAX_BOUND;
    return;
  } else {
    tSup_ -= std::round(((tSup_ - tInf) / 2.) * multiplierRound) / multiplierRound;
    nbSimulationsFailed++;
  }

  while (curAccuracy > accuracy) {
    setParametersAndLaunchSimulation(criticalTimeCalculation);
    nbSimulationsDone++;
    updateIndexes(tPrevious, curAccuracy, multiplierRound, nbSimulationsFailed);
  }

  // Check if the final result is calculated with a failed simulation
  if (!results_.getSuccess())
  tSup_ -= accuracy;

  // Check if all simulations failed (if yes, min range might be the problem)
  if (nbSimulationsDone == nbSimulationsFailed) {
    status_ = DYNAlgorithms::CT_BELOW_MIN_BOUND;
    return;
  }

  tSup_ = std::round(tSup_ * multiplierRound) / multiplierRound;
  status_ = DYNAlgorithms::RESULT_FOUND;
}

// void
// CriticalTimeLauncher::launch() {
//   boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation = multipleJobs_->getCriticalTimeCalculation();
//   if (!criticalTimeCalculation)
//     throw DYNAlgorithmsError(CriticalTimeCalculationTaskNotFound);

//   const double accuracy = criticalTimeCalculation->getAccuracy();
//   const double multiplierRound = 1. / accuracy;
//   int nbSimulationsDone = 0;
//   int nbSimulationsFailed = 0;
//   tSup_ = criticalTimeCalculation->getMaxValue();  // time use in simulation
//   double tMin = criticalTimeCalculation->getMinValue();  // Bound min of the time
//   double tMax = criticalTimeCalculation->getMaxValue();  // bound max of the time
//   double gap = tMax-tMin;
//   double tLowestFailed = tMax;  // min time where all times higher lead to a failed simulation
//   std::map<double, status_t> tFailedValues;  // stores every tested tSup_ which lead to a failed Simulation

//   // While difference between lowest time of fail and Highest time of Success (tMin) is higher than the accuracy then continue loop
//   while (std::abs(Round(tLowestFailed, multiplierRound)-Round(tMin, multiplierRound)-accuracy) > 1e-9) {
//     // Set tMax et tMin
//     if (nbSimulationsDone > 0) {
//       if (results_.getSuccess()) {
//         if (nbSimulationsDone == 1) {
//           status_ = DYNAlgorithms::CT_ABOVE_MAX_BOUND;
//           return;
//         }
//         tMin = tMax;
//         tMax = std::min(tLowestFailed, tMax+gap/2);
//       } else {
//         std::cout<< "Simulation Failed" << std::endl;
//         nbSimulationsFailed++;
//         tFailedValues.insert({tSup_, results_.getStatus()});
//         if (results_.getStatus() == CRITERIA_NON_RESPECTED_STATUS || results_.getStatus() == DIVERGENCE_STATUS) {
//           // In case of solver issue, tMax become the previous value of tMax (before the issue) minus the accuracy
//           if (tMax == tLowestFailed - accuracy) {
//             tLowestFailed = tMax;
//           }
//           tMax = tLowestFailed - accuracy;
//           if (tMin > tMax) {std::swap(tMin, tMax);}  // By lowering tMax this way, tMin might become greater than tMax
//         } else {
//           tLowestFailed = tMax;
//           tMax -= gap/2;
//         }
//       }
//     }
//     std::cout<< "tMax: " << tMax << "; tMin: " << tMin << std::endl;

//     // Set tSup_
//     gap = tMax-tMin;
//     tSup_ = Round(tMax, multiplierRound);

//     // Launch Simulation
//     if (tFailedValues.find(tSup_)== tFailedValues.end()) {
//       setParametersAndLaunchSimulation(criticalTimeCalculation);
//     } else {  // if failed values already detected, no need to launch the simulation
//       results_.setSuccess(false);
//       results_.setStatus(tFailedValues[tSup_]);
//     }
//     nbSimulationsDone++;
//   }

//   // Check if all simulations failed (if yes, min range might be the problem)
//   if (nbSimulationsDone == nbSimulationsFailed) {
//     status_ = DYNAlgorithms::CT_BELOW_MIN_BOUND;
//     return;
//   }
//   // Set final tSup_
//   tSup_ = std::floor(tMax * multiplierRound) / multiplierRound;
//   status_ = DYNAlgorithms::RESULT_FOUND;
// }

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
