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
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBTimelineEntry.h>
#include <DYNTrace.h>

#include "DYNMultipleJobs.h"
#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNSimulationResult.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"
#include "DYNMultiProcessingContext.h"

using DYN::Trace;

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
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
  inputs_.readInputs(workingDirectory_, baseJobsFile);

  bool isSingleThread = multiprocessing::context().nbProcs() == 1;
  bool isServerThread = !isSingleThread &&  multiprocessing::context().isRootProc();
  bool isWorkerThread = !isSingleThread && !multiprocessing::context().isRootProc();

  if (isSingleThread || isWorkerThread) {  // worker mode : actually run the simulations
    int scenId = -1;
    while (getNextScenId(scenId, events.size())) {
      createWorkingDir(events[scenId]->getId());
      exportResult(launchScenario(events[scenId]));
    }
  } else {  // isServerThread
    serverLoop(events.size());
  }

  if (isSingleThread || isServerThread)
    collectResults(events);

  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Systematic analysis", diff.total_milliseconds()/1000) << Trace::endline;
}

void
SystematicAnalysisLauncher::createWorkingDir(const std::string & path) {
  std::string workingDir  = createAbsolutePath(path, workingDirectory_);
  if (!exists(workingDir))
    createDirectory(workingDir);
  else if (!isDirectory(workingDir))
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
}

bool
SystematicAnalysisLauncher::getNextScenId(int &scenId, int maxId) {
  ++scenId;  // monothread : simply increment ID

#ifdef _MPI_
  if (multiprocessing::context().nbProcs() > 1) {  // multithread : request ID to server (root process) instead, who centralizes incrementations
    MPI_Send(nullptr, 0, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&scenId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
#endif  // _MPI_

  return scenId < maxId;
}

void
SystematicAnalysisLauncher::serverLoop(int maxId) {
#ifdef _MPI_
  std::set<int> workersLeft;
  for (int i=1; i < multiprocessing::context().nbProcs(); ++i)
    workersLeft.insert(i);

  int scenId = 0;
  while (workersLeft.size() > 0) {
    MPI_Status senderInfo;
    MPI_Recv(nullptr, 0, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &senderInfo);
    MPI_Send(&scenId, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
    if (scenId >= maxId)
      workersLeft.erase(senderInfo.MPI_SOURCE);
    ++scenId;
  }
#endif  // _MPI_
}

void
SystematicAnalysisLauncher::collectResults(const std::vector<boost::shared_ptr<Scenario> >& events) {
  results_.resize(events.size());
  for (unsigned int i = 0; i < events.size(); i++) {
    const auto& scenario = events.at(i);
    results_.at(i) = importResult(scenario->getId());
    cleanResult(scenario->getId());
  }
}
SimulationResult
SystematicAnalysisLauncher::launchScenario(const boost::shared_ptr<Scenario>& scenario) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Launch scenario :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::string workingDir  = createAbsolutePath(scenario->getId(), workingDirectory_);
  std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);

  SimulationResult result;
  result.setScenarioId(scenario->getId());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    simulation->setTimelineOutputFile("");
    simulation->setConstraintsOutputFile("");
    simulation->setLostEquipmentsOutputFile("");
    simulate(simulation, result);
  }

  if (multiprocessing::context().nbProcs() == 1)
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
