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
CriticalTimeLauncher::setParametersAndLaunchSimulation(boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation, int& nbSimulationsDone) {
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
    simulate(simulation, results_);
    nbSimulationsDone++;
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

void
CriticalTimeLauncher::launch() {
  boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation = multipleJobs_->getCriticalTimeCalculation();
  if (!criticalTimeCalculation)
    throw DYNAlgorithmsError(CriticalTimeCalculationTaskNotFound);

  const double accuracy = criticalTimeCalculation->getAccuracy();
  double curAccuracy = 1.;
  const double multiplierRound = 1. / accuracy;

  tSup_ = criticalTimeCalculation->getMaxValue();
  const double tInf = criticalTimeCalculation->getMinValue();
  double tPrevious = tSup_;
  int nbSimulationsDone = 0;
  int nbSimulationsFailed = 0;

  // First simulation case.
  setParametersAndLaunchSimulation(criticalTimeCalculation, nbSimulationsDone);
  if (results_.getSuccess()) {
    throw DYNAlgorithmsError(CriticalTimeOutOfRangeMax);
  } else {
    tSup_ -= std::round(((tSup_ - tInf) / 2.) * multiplierRound) / multiplierRound;
    nbSimulationsFailed++;
  }

  while (curAccuracy > accuracy) {
    setParametersAndLaunchSimulation(criticalTimeCalculation, nbSimulationsDone);
    updateIndexes(tPrevious, curAccuracy, multiplierRound, nbSimulationsFailed);
  }

  // Check if the final result is calculated with a failed simulation
  if (!results_.getSuccess())
    tSup_ -= accuracy;

  // Check if all simulations failed (if yes, min range might be the problem)
  if (nbSimulationsDone == nbSimulationsFailed)
    throw DYNAlgorithmsError(CriticalTimeOutOfRangeMin);

  tSup_ = std::round(tSup_ * multiplierRound) / multiplierRound;
}

void
CriticalTimeLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportCriticalTimeResultsToStream(tSup_, results_.getSimulationMessageError(), aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
    storeOutputs(results_, mapData);
  } else {
    exporter.exportCriticalTimeResultsToFile(tSup_, results_.getSimulationMessageError(), outputFileFullPath_);
    writeOutputs(results_);
  }
}

}  // namespace DYNAlgorithms
