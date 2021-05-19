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
 * @brief Systematic Analysis : implementation of the algorithm and interaction with dynamo core
 *
 */

#include "DYNSystematicAnalysisLauncher.h"

#ifdef WITH_OPENMP
#include <omp.h>
#endif

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

#include "DYNMultipleJobs.h"
#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNSimulationResult.h"
#include "DYNAggrResXmlExporter.h"
#include "MacrosMessage.h"

namespace DYNAlgorithms {

SystematicAnalysisLauncher::~SystematicAnalysisLauncher() {
}

void
SystematicAnalysisLauncher::launch() {
  boost::shared_ptr<Scenarios> scenarios = multipleJobs_->getScenarios();
  const std::string& baseJobsFile = scenarios->getJobsFile();
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios->getScenarios();

#ifdef WITH_OPENMP
  omp_set_num_threads(nbThreads_);
#endif
  results_.resize((unsigned int)(events.size()));

  for (unsigned int i=0; i < events.size(); i++) {
    std::string workingDir  = createAbsolutePath(events[i]->getId(), workingDirectory_);
    if (!exists(workingDir))
      create_directory(workingDir);
    else if (!is_directory(workingDir))
      throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDir);
  }

  updateAnalysisContext(baseJobsFile, events.size());

#ifdef WITH_OPENMP
#pragma omp parallel for schedule(dynamic, 1)
#endif
  for (unsigned int i=0; i < events.size(); i++) {
    if (context_.dataInterface && context_.dataInterface->canUseVariant()) {
#ifdef LANG_CXX11
      std::string name = std::to_string(i);
#else
      std::stringstream ss;
      ss << i;
      std::string name = ss.str();
#endif
      context_.dataInterface->useVariant(name);
    }
    results_[i] = launchScenario(events[i]);
  }
}

SimulationResult
SystematicAnalysisLauncher::launchScenario(const boost::shared_ptr<Scenario>& scenario) {
  std::stringstream ss;
  ss << " Launch scenario :" << scenario->getId() << " dydFile =" << scenario->getDydFile() << std::endl;
  std::cout << ss.str();
  ss.str("");
  ss.clear();

  std::string workingDir  = createAbsolutePath(scenario->getId(), workingDirectory_);
  boost::shared_ptr<job::JobEntry> job = boost::make_shared<job::JobEntry>(*context_.jobEntry);
  addDydFileToJob(job, scenario->getDydFile());
  SimulationParameters params;
  SimulationResult result;
  result.setScenarioId(scenario->getId());
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, context_.dataInterface);

  if (simulation) {
    simulate(simulation, result);
  }
  ss << " scenario :" << scenario->getId() << " final status: " << getStatusAsString(result.getStatus()) << std::endl;
  std::cout << ss.str();
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
