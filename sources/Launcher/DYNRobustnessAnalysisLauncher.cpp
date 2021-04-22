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
 * @file  RobustnessAnalysis.cpp
 *
 * @brief Robustness Analysis : implementation file
 *
 */
#include "DYNRobustnessAnalysisLauncher.h"

#include <limits>
#include <fstream>

#include <xml/sax/parser/ParserFactory.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/formatter/AttributeList.h>


#include <libzip/ZipFile.h>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipEntry.h>
#include <libzip/ZipInputStream.h>
#include <libzip/ZipOutputStream.h>

#include <DYNFileSystemUtils.h>
#include <DYNSimulationContext.h>
#include <DYNExecUtils.h>
#include <JOBDynModelsEntry.h>
#include <JOBDynModelsEntryFactory.h>
#include <JOBModelerEntry.h>
#include <DYNMacrosMessage.h>

#include "DYNMultipleJobsXmlHandler.h"
#include "DYNMultipleJobs.h"
#include "MacrosMessage.h"

using multipleJobs::MultipleJobs;

namespace DYNAlgorithms {

RobustnessAnalysisLauncher::RobustnessAnalysisLauncher() :
nbThreads_(1) {
}

RobustnessAnalysisLauncher::~RobustnessAnalysisLauncher() {
}

void
RobustnessAnalysisLauncher::setInputFile(const std::string& inputFile) {
  inputFile_ = inputFile;
}

void
RobustnessAnalysisLauncher::setOutputFile(const std::string& outputFile) {
  outputFile_ = outputFile;
}

void
RobustnessAnalysisLauncher::setDirectory(const std::string& directory) {
  directory_ = directory;
}

void
RobustnessAnalysisLauncher::setNbThreads(const int nbThreads) {
  nbThreads_ = nbThreads;
}

void
RobustnessAnalysisLauncher::init() {
  // check if directory exists, if directory is not set, workingDirectory is the current directory
  workingDirectory_ = "";
  if ( directory_ == "" ) {
    workingDirectory_ = current_path();
  } else if (!isAbsolutePath(directory_)) {
    workingDirectory_ = createAbsolutePath(directory_, current_path());
  } else {
    workingDirectory_ = directory_;
  }

  if ( !is_directory(workingDirectory_) )
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDirectory_);
  workingDirectory_ += '/';  // to be sure to have an '/' at the end of the path

  std::string inputFileFullPath;
  if (!isAbsolutePath(inputFile_))
    inputFileFullPath = createAbsolutePath(inputFile_, workingDirectory_);
  else
    inputFileFullPath = inputFile_;

  if (!exists(inputFileFullPath))
    throw DYNAlgorithmsError(FileDoesNotExist, inputFileFullPath);

  // build the name of the outputFile
  outputFileFullPath_ = createAbsolutePath(outputFile_, workingDirectory_);

  std::string fileName = "";
  if (extensionEquals(inputFileFullPath, ".zip")) {
    // read zip input file
    boost::shared_ptr<zip::ZipFile> archive = zip::ZipInputStream::read(inputFileFullPath);
    for (std::map<std::string, boost::shared_ptr<zip::ZipEntry> >::const_iterator itE = archive->getEntries().begin();
        itE != archive->getEntries().end(); ++itE) {
      std::string nom = itE->first;
      std::string data(itE->second->getData());
      std::fstream file;
      file.open((workingDirectory_ + nom).c_str(), std::ios::out);
      file << data;
      file.close();
    }
    fileName = createAbsolutePath("fic_MULTIPLE.xml", parentDirectory(inputFileFullPath));
  } else if (extensionEquals(inputFileFullPath, ".xml")) {
    fileName = inputFileFullPath;
  } else {
    throw DYNAlgorithmsError(InputFileFormatNotSupported, inputFileFullPath);
  }
  if (!exists(fileName))
    throw DYNAlgorithmsError(FileDoesNotExist, fileName);
  multipleJobs_ = readInputData(fileName);
}

boost::shared_ptr<MultipleJobs>
RobustnessAnalysisLauncher::readInputData(const std::string& fileName) {
  multipleJobs::XmlHandler multipleJobsHandler;

  xml::sax::parser::ParserFactory parser_factory;
  xml::sax::parser::ParserPtr parser = parser_factory.createParser();
  bool xsdValidation = false;
  if (getEnvVar("DYNAWO_USE_XSD_VALIDATION") == "true") {
    const std::string xsdPath = createAbsolutePath("multipleJobs.xsd", getMandatoryEnvVar("DYNAWO_ALGORITHMS_XSD_DIR"));
    parser->addXmlSchema(xsdPath);
    xsdValidation = true;
  }
  try {
    parser->parse(fileName, multipleJobsHandler, xsdValidation);
  }  catch (const xml::sax::parser::ParserException& exp) {
    throw DYNAlgorithmsError(XmlParsingError, fileName, exp.what());
  }
  return multipleJobsHandler.getMultipleJobs();
}

void
RobustnessAnalysisLauncher::addDydFileToJob(boost::shared_ptr<job::JobEntry>& job,
    const std::string& dydFile) {
  if (!dydFile.empty()) {
    boost::shared_ptr<job::DynModelsEntry> dynModels = job::DynModelsEntryFactory::newInstance();
    dynModels->setDydFile(dydFile);
    job->getModelerEntry()->addDynModelsEntry(dynModels);
  }
}
boost::shared_ptr<DYN::Simulation>
RobustnessAnalysisLauncher::createAndInitSimulation(const std::string& workingDir,
    boost::shared_ptr<job::JobEntry>& job, const SimulationParameters& params, SimulationResult& result) {
  boost::shared_ptr<DYN::SimulationContext> context = boost::shared_ptr<DYN::SimulationContext>(new DYN::SimulationContext());
  context->setResourcesDirectory(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
  context->setLocale(getMandatoryEnvVar("DYNAWO_ALGORITHMS_LOCALE"));
  context->setInputDirectory(workingDirectory_);
  context->setWorkingDirectory(workingDir);
  boost::shared_ptr<DYN::Simulation> simulation = boost::shared_ptr<DYN::Simulation>(new DYN::Simulation(job, context));

  if (!params.InitialStateFile_.empty())
    simulation->setInitialStateFile(params.InitialStateFile_);

  if (!params.iidmFile_.empty())
    simulation->setIIDMFile(params.iidmFile_);

  if (!params.exportIIDMFile_.empty())
    simulation->setExportIIDMFile(params.exportIIDMFile_);

  if (!params.dumpFinalStateFile_.empty())
    simulation->setDumpFinalStateFile(params.dumpFinalStateFile_);

  if (params.activateDumpFinalState_)
    simulation->activateDumpFinalState(true);

  if (params.activateExportIIDM_)
    simulation->activateExportIIDM(true);
  try {
    simulation->init();
  } catch (const DYN::Error& e) {
    result.setSuccess(false);
    if (e.type() == DYN::Error::SOLVER_ALGO || e.type() == DYN::Error::SUNDIALS_ERROR) {
      result.setStatus(DIVERGENCE_STATUS);
    } else {
      result.setStatus(EXECUTION_PROBLEM_STATUS);
    }
    return boost::shared_ptr<DYN::Simulation>();
  } catch (...) {
    result.setSuccess(false);
    result.setStatus(EXECUTION_PROBLEM_STATUS);
    return boost::shared_ptr<DYN::Simulation>();
  }
  return simulation;
}


status_t
RobustnessAnalysisLauncher::simulate(const boost::shared_ptr<DYN::Simulation>& simulation, SimulationResult& result) {
  try {
      simulation->simulate();
      simulation->terminate();
      result.setSuccess(true);
      result.setStatus(CONVERGENCE_STATUS);
    } catch (const DYN::Error& e) {
      // Needed as otherwise terminate might crash due to badly formed model
      simulation->activateExportIIDM(false);
      simulation->terminate();
      result.setSuccess(false);
      if (e.type() == DYN::Error::SIMULATION) {
        result.setStatus(CRITERIA_NON_RESPECTED_STATUS);
        std::vector<std::pair<double, std::string> > failingCriteria;
        simulation->getFailingCriteria(failingCriteria);
        result.setFailingCriteria(failingCriteria);
      } else if (e.type() == DYN::Error::SOLVER_ALGO || e.type() == DYN::Error::SUNDIALS_ERROR) {
        result.setStatus(DIVERGENCE_STATUS);
      } else {
        result.setStatus(EXECUTION_PROBLEM_STATUS);
      }
    } catch (...) {
      simulation->terminate();
      result.setSuccess(false);
      result.setStatus(EXECUTION_PROBLEM_STATUS);
    }
    simulation->printTimeline(result.getTimelineStream());
    simulation->printConstraints(result.getConstraintsStream());
    return result.getStatus();
}

void
RobustnessAnalysisLauncher::storeOutputs(const SimulationResult& result, std::map<std::string, std::string>& mapData) const {
  if (!result.getTimelineStreamStr().empty()) {
    std::stringstream timelineName;
    timelineName << "timeLine/timeline_" << result.getUniqueScenarioId() << ".xml";
    mapData[timelineName.str()] = result.getTimelineStreamStr();
  }

  if (!result.getConstraintsStreamStr().empty()) {
    std::stringstream constraintsName;
    constraintsName << "constraints/constraints_" << result.getUniqueScenarioId() << ".xml";
    mapData[constraintsName.str()] = result.getConstraintsStreamStr();
  }
}

void
RobustnessAnalysisLauncher::writeOutputs(const SimulationResult& result) const {
  std::string constraintPath;
  std::string timelinePath;
  constraintPath = createAbsolutePath("constraints", workingDirectory_);
  if (!is_directory(constraintPath))
    create_directory(constraintPath);
  timelinePath = createAbsolutePath("timeLine", workingDirectory_);
  if (!is_directory(timelinePath))
    create_directory(timelinePath);
  if (!result.getConstraintsStreamStr().empty()) {
    std::fstream file;
    std::string filepath = createAbsolutePath("constraints_" + result.getUniqueScenarioId() + ".xml", constraintPath);
    file.open(filepath.c_str(), std::fstream::out);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath.c_str());
    }
    file << result.getConstraintsStreamStr();
    file.close();
  }

  if (!result.getTimelineStreamStr().empty()) {
    std::string filepath = createAbsolutePath("timeline_" + result.getUniqueScenarioId() + ".xml", timelinePath);
    std::fstream file;
    file.open(filepath.c_str(), std::fstream::out);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath.c_str());
    }
    file << result.getTimelineStreamStr();
    file.close();
  }
}

void
RobustnessAnalysisLauncher::writeResults() const {
  std::map<std::string, std::string > mapData;  // association between filename and data
  bool zipIt = extensionEquals(outputFileFullPath_, ".zip");
  createOutputs(mapData, zipIt);

  if (zipIt) {
    boost::shared_ptr<zip::ZipFile> archive = zip::ZipFileFactory::newInstance();

    for (std::map<std::string, std::string>::const_iterator it = mapData.begin();
        it != mapData.end();
        it++) {
      archive->addEntry(it->first, it->second);
    }

    zip::ZipOutputStream::write(outputFileFullPath_, archive);
  }
}
}  // namespace DYNAlgorithms
