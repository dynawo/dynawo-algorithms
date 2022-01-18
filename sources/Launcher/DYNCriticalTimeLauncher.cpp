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

#ifdef WITH_OPENMP
#include <omp.h>
#endif

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
CriticalTimeLauncher::SetParametersAndLaunchSimulation() {
  inputs_.readInputs(workingDirectory_, jobsFile_, 1);
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  SimulationParameters params;
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDirectory_, job, params, results_, inputs_);

  if (simulation) {
    boost::shared_ptr<DYN::ModelMulti> modelMulti = boost::dynamic_pointer_cast<DYN::ModelMulti>(simulation->model_);

    const std::string& dydId = criticalTimeCalculation_->getDydId();
    const std::string& endPar = criticalTimeCalculation_->getEndPar();

    if (modelMulti->findSubModelByName(dydId) != NULL) {
      subModel_ = modelMulti->findSubModelByName(dydId);
      subModel_->setParameterValue(endPar, DYN::PAR, tSup_, false);
      subModel_->setSubModelParameters();
    }
    simulate(simulation, results_);
  }
}

void
CriticalTimeLauncher::updateIndexes(double& tPrevious, double& curAccuracy, const double& multiplierRound) {
  curAccuracy = std::abs(tSup_ - tPrevious);
  double midDichotomy = std::round((std::abs(tSup_ - tPrevious) / 2) * multiplierRound) / multiplierRound;
  double tmp = tSup_;

  if (results_.getSuccess())
    tSup_ += midDichotomy;
  else
    tSup_ -= midDichotomy;

  tPrevious = tmp;
}

void
CriticalTimeLauncher::launch() {
  criticalTimeCalculation_ = multipleJobs_->getCriticalTimeCalculation();
  if (!criticalTimeCalculation_)
    throw DYNAlgorithmsError(CriticalTimeCalculationTaskNotFound);

  const double accuracy = criticalTimeCalculation_->getAccuracy();
  double curAccuracy = 1;
  const double multiplierRound = 1 / accuracy;

  tSup_ = criticalTimeCalculation_->getMaxValue();
  jobsFile_ = criticalTimeCalculation_->getJobsFile();
  double tInf = criticalTimeCalculation_->getMinValue();
  double tPrevious = tSup_;

  // First simulation case.
  SetParametersAndLaunchSimulation();
  if (results_.getSuccess())
    tSup_ += std::round(((tSup_ - tInf) / 2) * multiplierRound) / multiplierRound;
  else
    tSup_ -= std::round(((tSup_ - tInf) / 2) * multiplierRound) / multiplierRound;

  while (curAccuracy > accuracy) {
    SetParametersAndLaunchSimulation();
    updateIndexes(tPrevious, curAccuracy, multiplierRound);
  }

  // Check if the final result is calculated with a failed simulation
  if (!results_.getSuccess())
    tSup_ -= accuracy;

  tSup_ = std::round(tSup_ * multiplierRound) / multiplierRound;;
}

void
CriticalTimeLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportCriticalTimeResultsToStream(tSup_, results_.getCriticalTimeMessageError(), aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
    storeOutputs(results_, mapData);
  } else {
    exporter.exportCriticalTimeResultsToFile(tSup_, results_.getCriticalTimeMessageError(), outputFileFullPath_);
    writeOutputs(results_);
  }
}

}  // namespace DYNAlgorithms
