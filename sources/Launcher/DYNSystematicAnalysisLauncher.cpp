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
 * @file  SystematicAnalysis.cpp
 *
 * @brief Systematic Analysis : implementation of the algorithm and interaction with dynawo core
 *
 */

#include "DYNSystematicAnalysisLauncher.h"

#include <limits>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <libzip/ZipFile.h>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipEntry.h>
#include <libzip/ZipInputStream.h>
#include <libzip/ZipOutputStream.h>

#include <DYNFileSystemUtils.h>
#include <DYNSimulation.h>
#include <DYNError.h>
#include <JOBXmlImporter.h>
#include <JOBIterators.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBTimelineEntry.h>

#include "DYNMultipleJobs.h"
#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNSimulationResult.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"
#include "DYNMPIContext.h"

namespace DYNAlgorithms {

void
SystematicAnalysisLauncher::launch() {
  boost::shared_ptr<Scenarios> scenarios = multipleJobs_->getScenarios();
  if (!scenarios) {
    throw DYNAlgorithmsError(SystematicAnalysisTaskNotFound);
  }
  const std::string& baseJobsFile = scenarios->getJobsFile();
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();

  auto& context = mpi::context();

  if (context.isRootProc()) {
    // only required for root proc
    results_.resize(events.size());
  }

  mpi::forEach(0, events.size(), [this, &events](unsigned int i){
    std::string workingDir  = createAbsolutePath(events[i]->getId(), workingDirectory_);
    if (!exists(workingDir))
      create_directory(workingDir);
    else if (!is_directory(workingDir))
      throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
  });

  inputs_.readInputs(workingDirectory_, baseJobsFile);

  mpi::forEach(0, events.size(), [this, &events](unsigned int i){
      auto result = launchScenario(events[i]);
      exportResult(result);
  });

  mpi::Context::sync();

  // Update results for root proc
  if (context.isRootProc()) {
    for (unsigned int i = 0; i < events.size(); i++) {
      const auto& scenario = events.at(i);
      results_.at(i) = importResult(scenario->getId());
      cleanResult(scenario->getId());
    }
  }
}

SimulationResult
SystematicAnalysisLauncher::launchScenario(const boost::shared_ptr<Scenario>& scenario) {
  if (mpi::context().nbProcs() == 1)
    std::cout << " Launch scenario :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::string workingDir  = createAbsolutePath(scenario->getId(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
  if (job->getOutputsEntry()->getTimelineEntry()) {
    job->getOutputsEntry()->getTimelineEntry()->setMaxPriority(1);
  }
  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);

  SimulationResult result;
  result.setScenarioId(scenario->getId());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    simulate(simulation, result);
  }

  if (mpi::context().nbProcs() == 1)
    std::cout << " scenario :" << scenario->getId() << " final status: " << getStatusAsString(result.getStatus()) << std::endl;

  return result;
}

void
SystematicAnalysisLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportScenarioResultsToStream(results_, aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
  } else {
    exporter.exportScenarioResultsToFile(results_, outputFileFullPath_);
  }

  for (unsigned int i=0; i < results_.size(); i++) {
    if (zipIt) {
      storeOutputs(results_[i], mapData);
    } else {
      writeOutputs(results_[i]);
    }
  }
}

}  // namespace DYNAlgorithms
