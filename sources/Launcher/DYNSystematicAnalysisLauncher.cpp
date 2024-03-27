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
#include "boost/date_time/posix_time/posix_time.hpp"

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
#include <DYNTrace.h>

#include "DYNMultipleJobs.h"
#include "DYNProcess.h"
#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNSimulationResult.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"
#ifdef _MPI_
#include "DYNMPIContext.h"
#endif

using DYN::Trace;

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return isRootProcess() ? Trace::info(tag) : DYN::TraceStream();
}

void
SystematicAnalysisLauncher::launch() {
  boost::posix_time::ptime t0 = boost::posix_time::second_clock::local_time();
  boost::shared_ptr<Scenarios> scenarios = multipleJobs_->getScenarios();
  if (!scenarios) {
    throw DYNAlgorithmsError(SystematicAnalysisTaskNotFound);
  }
  const std::string& baseJobsFile = scenarios->getJobsFile();
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();

  if (isRootProcess()) {
    // only required for root proc
    results_.resize(events.size());
  }

  auto createScenariosWorkingDir = [this, &events](unsigned int i) {
    std::string workingDir  = createAbsolutePath(events[i]->getId(), workingDirectory_);
    if (!exists(workingDir))
      create_directory(workingDir);
    else if (!is_directory(workingDir))
      throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
  };

#ifdef _MPI_
  mpi::forEach(0, events.size(), createScenariosWorkingDir);
#else
  for (unsigned int idx1 = 0; idx1 < events.size(); idx1++) {
    createScenariosWorkingDir(idx1);
  }
#endif

  inputs_.readInputs(workingDirectory_, baseJobsFile);

  auto launchScenarioAndExportResult = [this, &events](unsigned int i) {
    auto result = launchScenario(events[i]);
    exportResult(result);
  };

#ifdef _MPI_
  mpi::forEach(0, events.size(), launchScenarioAndExportResult);
  mpi::Context::sync();
#else
  for (unsigned int idx2 = 0; idx2 < events.size(); idx2++) {
    launchScenarioAndExportResult(idx2);
  }
#endif

  // Update results for root proc
  if (isRootProcess()) {
    for (unsigned int i = 0; i < events.size(); i++) {
      const auto& scenario = events.at(i);
      results_.at(i) = importResult(scenario->getId());
      cleanResult(scenario->getId());
    }
  }
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Systematic analysis", diff.total_milliseconds()/1000) << Trace::endline;
}

SimulationResult
SystematicAnalysisLauncher::launchScenario(const boost::shared_ptr<Scenario>& scenario) {
#ifdef _MPI_
  if (mpi::context().nbProcs() == 1)
#endif
    std::cout << " Launch scenario :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::string workingDir  = createAbsolutePath(scenario->getId(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);

  SimulationResult result;
  result.setScenarioId(scenario->getId());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);
  simulation->setTimelineOutputFile("");
  simulation->setConstraintsOutputFile("");
  simulation->setLostEquipmentsOutputFile("");

  if (simulation) {
    simulate(simulation, result);
  }

#ifdef _MPI_
  if (mpi::context().nbProcs() == 1)
#endif
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
