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

#include <fstream>

#include <xml/sax/parser/ParserFactory.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/formatter/AttributeList.h>

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

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
#include <JOBIterators.h>
#include <JOBJobsCollection.h>
#include <DYNMacrosMessage.h>
#include <DYNDataInterfaceFactory.h>
#include <DYNTrace.h>

#include <config.h>
#include <gitversion.h>

#include "../config_algorithms.h"
#include "../gitversion_algorithms.h"
#include "DYNMultipleJobsXmlHandler.h"
#include "DYNMultipleJobs.h"
#include "MacrosMessage.h"

#include <boost/make_shared.hpp>
using DYN::Trace;
using multipleJobs::MultipleJobs;

namespace DYNAlgorithms {

RobustnessAnalysisLauncher::RobustnessAnalysisLauncher() :
logTag_("DYN-ALGO"),
nbThreads_(1) {
}

RobustnessAnalysisLauncher::~RobustnessAnalysisLauncher() {
}

void
RobustnessAnalysisLauncher::setInputFile(const std::string& inputFile) {
  inputFile_ = inputFile;
}

void
RobustnessAnalysisLauncher::setMultipleJobs(boost::shared_ptr<multipleJobs::MultipleJobs>& multipleJobs) {
  multipleJobs_ = multipleJobs;
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
RobustnessAnalysisLauncher::init(const bool doInitLog) {
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

  if (doInitLog)
    initLog();

  // build the name of the outputFile
  outputFileFullPath_ = createAbsolutePath(outputFile_, workingDirectory_);

  // If we already have received a definition for multipleJobs we have finished
  if (multipleJobs_) {
    return;
  }

  // Build a definition of multipleJobs from input file
  std::string inputFileFullPath;
  if (!isAbsolutePath(inputFile_))
    inputFileFullPath = createAbsolutePath(inputFile_, workingDirectory_);
  else
    inputFileFullPath = inputFile_;

  if (!exists(inputFileFullPath)) {
    throw DYNAlgorithmsError(FileDoesNotExist, inputFileFullPath);
  }

  std::string fileName = "";
  if (extensionEquals(inputFileFullPath, ".zip")) {
    fileName = unzipAndGetMultipleJobsFileName(inputFileFullPath);
  } else if (extensionEquals(inputFileFullPath, ".xml")) {
    fileName = inputFileFullPath;
  } else {
    throw DYNAlgorithmsError(InputFileFormatNotSupported, inputFileFullPath);
  }
  if (!exists(fileName))
    throw DYNAlgorithmsError(FileDoesNotExist, fileName);
  multipleJobs_ = readInputData(fileName);
}

struct Version {
  Version() {}
  Version(std::string n, std::string v, std::string b, std::string h) :
    projectName(n),
    versionString(v),
    gitBranch(b),
    gitHash(h) {
  }
  std::string projectName;
  std::string versionString;
  std::string gitBranch;
  std::string gitHash;
};

void
RobustnessAnalysisLauncher::initLog() {
  std::vector<Trace::TraceAppender> appenders;
  Trace::TraceAppender appender;
  std::string outputPath(createAbsolutePath("dynawo.log", workingDirectory_));

  appender.setFilePath(outputPath);
#if _DEBUG_
  appender.setLvlFilter(DYN::DEBUG);
#else
  appender.setLvlFilter(DYN::INFO);
#endif
  appender.setTag(logTag_);
  appender.setShowLevelTag(true);
  appender.setSeparator(" | ");
  appender.setShowTimeStamp(true);
  appender.setTimeStampFormat("%Y-%m-%d %H:%M:%S");
  appender.setPersistant(true);

  appenders.push_back(appender);

  Trace::addAppenders(appenders);

  // known projects versions
  std::vector<Version> versions;
  versions.push_back(Version("Dynawo", DYNAWO_VERSION_STRING, DYNAWO_GIT_BRANCH, DYNAWO_GIT_HASH));
  versions.push_back(Version("Dynawo-algorithms", DYNAWO_ALGORITHMS_VERSION_STRING, DYNAWO_ALGORITHMS_GIT_BRANCH, DYNAWO_ALGORITHMS_GIT_HASH));
  std::set<std::string> projects;
  projects.insert("DYNAWO");
  projects.insert("DYNAWO-ALGORITHMS");

  // get additional projects versions in versions.ini if exists
  std::string versions_ini = (boost::dll::program_location().parent_path() / "versions.ini").string();
  if (exists(versions_ini)) {
    namespace pt = boost::property_tree;
    pt::iptree tree;
    try {
      pt::read_ini(versions_ini, tree);
      BOOST_FOREACH(pt::iptree::value_type &section, tree) {
        std::string versionString = section.second.get("VERSION_STRING", "Unknown");
        std::string gitBranch = section.second.get("GIT_BRANCH", "Unknown");
        std::string gitHash = section.second.get("GIT_HASH", "0");
        std::string project = boost::algorithm::to_upper_copy(section.first);
        if (projects.count(project) == 0) {
          versions.push_back(Version(section.first, versionString, gitBranch, gitHash));
          projects.insert(project);
        }
      }
    } catch (pt::ptree_error &pe) {
      Trace::warn(logTag_) << pe.what() << Trace::endline;
      std::cerr << pe.what() << std::endl;
    }
  }

  Trace::info(logTag_) << " ============================================================ " << Trace::endline;
  BOOST_FOREACH(Version &version, versions) {
    Trace::info(logTag_) << "  " << version.projectName << " VERSION : " << version.versionString << Trace::endline;
    Trace::info(logTag_) << "  " << version.projectName << " REVISION: " << version.gitBranch << "-" << version.gitHash << Trace::endline;
  }
  Trace::info(logTag_) << " ============================================================ " << Trace::endline;
}

std::string
RobustnessAnalysisLauncher::unzipAndGetMultipleJobsFileName(const std::string& inputFileFullPath) const {
  // Unzip the input file in the working directory
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
  // When input is given as a zip file we are assuming the multiple jobs definition is always in a file named fic_MULTIPLE.xml
  return createAbsolutePath("fic_MULTIPLE.xml", parentDirectory(inputFileFullPath));
}

boost::shared_ptr<MultipleJobs>
RobustnessAnalysisLauncher::readInputData(const std::string& fileName) {
  multipleJobs::XmlHandler multipleJobsHandler;

  xml::sax::parser::ParserFactory parserFactory;
  xml::sax::parser::ParserPtr parser = parserFactory.createParser();
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
    boost::shared_ptr<job::JobEntry>& job, const SimulationParameters& params, SimulationResult& result, const MultiVariantInputs& analysisContext) {
  boost::shared_ptr<DYN::SimulationContext> context = boost::shared_ptr<DYN::SimulationContext>(new DYN::SimulationContext());
  context->setResourcesDirectory(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
  context->setLocale(getMandatoryEnvVar("DYNAWO_ALGORITHMS_LOCALE"));
  context->setInputDirectory(workingDirectory_);
  context->setWorkingDirectory(workingDir);

  boost::shared_ptr<DYN::DataInterface> dataInterface = analysisContext.dataInterfaceContainer()
    ? analysisContext.dataInterfaceContainer()->getDataInterface()
    : boost::shared_ptr<DYN::DataInterface>();
  boost::shared_ptr<DYN::Simulation> simulation =
    boost::shared_ptr<DYN::Simulation>(new DYN::Simulation(job, context, dataInterface));

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
    std::cerr << e.what() << std::endl;
    Trace::error() << e.what() << Trace::endline;
    result.setSuccess(false);
    if (e.type() == DYN::Error::SOLVER_ALGO || e.type() == DYN::Error::SUNDIALS_ERROR) {
      result.setStatus(DIVERGENCE_STATUS);
    } else {
      result.setStatus(EXECUTION_PROBLEM_STATUS);
    }
    return boost::shared_ptr<DYN::Simulation>();
  } catch (const DYN::MessageError& m) {
    std::cerr << m.what() << std::endl;
    Trace::error() << m.what() << Trace::endline;
    result.setSuccess(false);
    result.setStatus(EXECUTION_PROBLEM_STATUS);
    return boost::shared_ptr<DYN::Simulation>();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    Trace::error() << e.what() << Trace::endline;
    result.setSuccess(false);
    result.setStatus(EXECUTION_PROBLEM_STATUS);
    return boost::shared_ptr<DYN::Simulation>();
  } catch (...) {
    std::cerr << "Init simulation unknown error" << std::endl;
    Trace::error() << "Init simulation unknown error" << Trace::endline;
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
      std::cerr << e.what() << std::endl;
      Trace::error() << e.what() << Trace::endline;
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
    } catch (const DYN::MessageError& m) {
      std::cerr << m.what() << std::endl;
      Trace::error() << m.what() << Trace::endline;
      simulation->terminate();
      result.setSuccess(false);
      result.setStatus(EXECUTION_PROBLEM_STATUS);
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
      Trace::error() << e.what() << Trace::endline;
      simulation->terminate();
      result.setSuccess(false);
      result.setStatus(EXECUTION_PROBLEM_STATUS);
    } catch (...) {
      std::cerr << "Run simulation unknown error" << std::endl;
      Trace::error() << "Run simulation unknown error" << Trace::endline;
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
