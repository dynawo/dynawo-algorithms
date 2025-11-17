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
#include "MacrosMessage.h"

#include <DYNFileSystemUtils.h>
#include <JOBJobEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBJobsCollection.h>
#include <JOBXmlImporter.h>
#include <DYNTrace.h>

#include "boost/date_time/posix_time/posix_time.hpp"

using DYN::Trace;

namespace DYNAlgorithms {

void
ComputeSimulationLauncher::launch() {
  boost::posix_time::ptime t0 = boost::posix_time::second_clock::local_time();
  std::string outputFileFullPath = outputFile_;
  std::string workingDir = absolute(removeFileName(inputFile_));
  workingDirectory_ = workingDir;
  initLog();
  if (outputFileFullPath.empty()) {
    outputFileFullPath = createAbsolutePath("results.xml", workingDir);
  }
  job::XmlImporter importer;
  std::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(inputFile_);
  workingDirectory_ = workingDir;
  for (auto& job : jobsCollection->getNonCstJobs()) {
    std::cout << DYNLog(LaunchingJob, job->getName()) << std::endl;
    SimulationResult result;
    SimulationParameters params;
    initParametersWithJob(job, params);
    result.setScenarioId(job->getName());
    boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_, "");
    if (simulation) {
      simulate(simulation, result);
    }
    results_.push_back(result);
  }
  aggregatedResults::XmlExporter exporter;
  exporter.exportScenarioResultsToFile(results_, outputFileFullPath);
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  Trace::info(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Simulation", diff.total_milliseconds()/1000) << Trace::endline;
}

void
ComputeSimulationLauncher::createOutputs(std::map<std::string, std::string>& /*mapData*/, bool /*zipIt*/) const {
  // Only simulation outputs are needed
}

}  // namespace DYNAlgorithms
