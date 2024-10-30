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
 * @file  DYNComputeLoadVariationLauncher.cpp
 *
 * @brief Launch a single load variation at a specific level based on margin calculation input files
 *
 */

#include "DYNComputeLoadVariationLauncher.h"

#include "DYNMarginCalculation.h"
#include "DYNMultipleJobs.h"
#include "MacrosMessage.h"
#include "DYNAggrResXmlExporter.h"

#include <JOBXmlImporter.h>
#include <JOBIterators.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <DYNFileSystemUtils.h>
#include <DYNExecUtils.h>
#include <DYNModelMulti.h>
#include <DYNSubModel.h>

namespace DYNAlgorithms {

void
ComputeLoadVariationLauncher::launch() {
  std::cout << "Launch loadIncrease of " << variation_ << "%" <<std::endl;
  boost::shared_ptr<MarginCalculation> marginCalculation = multipleJobs_->getMarginCalculation();
  const boost::shared_ptr<LoadIncrease>& loadIncrease = marginCalculation->getLoadIncrease();
  std::stringstream subDir;
  subDir << "step-" << variation_;
  std::string workingDir = createAbsolutePath(loadIncrease->getId(), createAbsolutePath(subDir.str(), workingDirectory_));
  if (!exists(workingDir))
    create_directory(workingDir);
  else if (!is_directory(workingDir))
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);

  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  SimulationParameters params;
  //  force simulation to dump final values (would be used as input to launch each events)
  params.activateDumpFinalState_ = true;
  params.activateExportIIDM_ = true;
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation_ << ".iidm";
  params.exportIIDMFile_ = createAbsolutePath(iidmFile.str(), workingDirectory_);
  std::stringstream dumpFile;
  dumpFile << "loadIncreaseFinalState-" << variation_ << ".dmp";
  params.dumpFinalStateFile_ = createAbsolutePath(dumpFile.str(), workingDirectory_);

  std::string DDBDir = getMandatoryEnvVar("DYNAWO_DDB_DIR");
  std::stringstream scenarioId;
  scenarioId << "loadIncrease-" << variation_;
  SimulationResult result;
  result.setScenarioId(scenarioId.str());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    auto subModels = modelMulti->findSubModelByLib(createAbsolutePath(std::string("DYNModelVariationArea") + DYN::sharedLibraryExtension(), DDBDir));
    double duration = 0;
    for (unsigned int i=0; i < subModels.size(); i++) {
      double startTime = subModels[i]->findParameterDynamic("startTime").getValue<double>();
      double stopTime = subModels[i]->findParameterDynamic("stopTime").getValue<double>();
      duration = stopTime - startTime;
      int nbLoads = subModels[i]->findParameterDynamic("nbLoads").getValue<int>();
      for (int k = 0; k < nbLoads; ++k) {
        std::stringstream deltaPName;
        deltaPName << "deltaP_load_" << k;
        double deltaP = subModels[i]->findParameterDynamic(deltaPName.str()).getValue<double>();
        subModels[i]->setParameterValue(deltaPName.str(), DYN::PAR, deltaP*variation_/100., false);

        std::stringstream deltaQName;
        deltaQName << "deltaQ_load_" << k;
        double deltaQ = subModels[i]->findParameterDynamic(deltaQName.str()).getValue<double>();
        subModels[i]->setParameterValue(deltaQName.str(), DYN::PAR, deltaQ*variation_/100., false);
      }
      // change of the stop time to keep the same ramp of variation.
      double originalDuration = stopTime - startTime;
      double newStopTime = startTime + originalDuration * variation_ / 100.;
      subModels[i]->setParameterValue("stopTime", DYN::PAR, newStopTime, false);
      subModels[i]->setSubModelParameters();  // update values stored in subModel
    }
    simulation->setStopTime(job->getSimulationEntry()->getStopTime() - (100. - variation_)/100. * duration);
    simulate(simulation, result);
  }
  LoadIncreaseResult loadIncreaseResult(0);
  loadIncreaseResult.getResult().setVariation(variation_);
  loadIncreaseResult.getResult().setStatus(result.getStatus());
  std::vector<LoadIncreaseResult> results;
  results.push_back(loadIncreaseResult);
  aggregatedResults::XmlExporter exporter;
  exporter.exportLoadIncreaseResultsToFile(results, outputFileFullPath_);
  std::cout << "End of load increase simulation status: " + getStatusAsString(result.getStatus()) << std::endl;
}

void
ComputeLoadVariationLauncher::createOutputs(std::map<std::string, std::string>& /*mapData*/, bool /*zipIt*/) const {
  // Only simulation outputs are needed
}

}  // namespace DYNAlgorithms
