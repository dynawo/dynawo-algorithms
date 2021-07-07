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
 * @file  DYNComputeSimulationLauncher.cpp
 *
 * @brief Unitary simulation : implementation of the algorithm and interaction with dynawo core
 *
 */

#include "DYNComputeSimulationLauncher.h"

#include "DYNAggrResXmlExporter.h"

#include <JOBXmlImporter.h>
#include <JOBIterators.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <DYNFileSystemUtils.h>

namespace DYNAlgorithms {

ComputeSimulationLauncher::~ComputeSimulationLauncher() {
}

void
ComputeSimulationLauncher::launch() {
  std::string outputFileFullPath = outputFile_;
  std::string workingDir = absolute(remove_file_name(inputFile_));
  if (outputFileFullPath.empty()) {
    outputFileFullPath = createAbsolutePath("results.xml", workingDir);
  }
  job::XmlImporter importer;
  boost::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(inputFile_);
  workingDirectory_ = workingDir;
  for (job::job_iterator itJobEntry = jobsCollection->begin();
      itJobEntry != jobsCollection->end();
      ++itJobEntry) {
    boost::shared_ptr<job::JobEntry>& job = *itJobEntry;
    std::cout << DYNLog(LaunchingJob, (*itJobEntry)->getName()) << std::endl;
    SimulationResult result;
    SimulationParameters params;
    result.setScenarioId(job->getName());
    boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);
    if (simulation) {
      simulate(simulation, result);
    }
    results_.push_back(result);
  }
  aggregatedResults::XmlExporter exporter;
  exporter.exportScenarioResultsToFile(results_, outputFileFullPath);
}

void
ComputeSimulationLauncher::createOutputs(std::map<std::string, std::string>& /*mapData*/, bool /*zipIt*/) const {
}

}  // namespace DYNAlgorithms
