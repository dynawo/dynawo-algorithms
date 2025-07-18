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
#include <limits>
#include <set>

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
#include <JOBSimulationEntryFactory.h>
#include <JOBJobsCollection.h>
#include <DYNMacrosMessage.h>
#include <DYNDataInterfaceFactory.h>
#include <DYNTrace.h>
#include <DYNCommon.h>

#include <config.h>
#include <gitversion.h>

#include "../config_algorithms.h"
#include "../gitversion_algorithms.h"
#include "DYNMultipleJobsXmlHandler.h"
#include "DYNMultipleJobs.h"
#include "MacrosMessage.h"
#include "DYNMultiProcessingContext.h"


using DYN::Trace;
using multipleJobs::MultipleJobs;

namespace DYNAlgorithms {

RobustnessAnalysisLauncher::RobustnessAnalysisLauncher() :
logTag_("DYN-ALGO") {
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
RobustnessAnalysisLauncher::init(const bool doInitLog) {
  // check if directory exists, if directory is not set, workingDirectory is the current directory
  workingDirectory_ = "";
  if (directory_.empty()) {
    workingDirectory_ = currentPath();
  } else if (!isAbsolutePath(directory_)) {
    workingDirectory_ = createAbsolutePath(directory_, currentPath());
  } else {
    workingDirectory_ = directory_;
  }

  if (!isDirectory(workingDirectory_))
    throw DYNAlgorithmsError(DirectoryDoesNotExist, workingDirectory_);

  if (doInitLog && multiprocessing::context().isRootProc())
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
  workingDirectory_ = parentDirectory(fileName);
  multipleJobs_ = readInputData(fileName);
}

struct Version {
  Version() {}
  Version(const std::string& n, const std::string& v, const std::string& b, const std::string& h) :
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
RobustnessAnalysisLauncher::initLog() const {
  std::vector<Trace::TraceAppender> appenders;
  Trace::TraceAppender appender;
  std::string outputPath(createAbsolutePath("dynawo.log", workingDirectory_));

  appender.setFilePath(outputPath);
#ifndef NDEBUG
  appender.setLvlFilter(DYN::DEBUG);
#else
  appender.setLvlFilter(DYN::INFO);
#endif
  appender.setTag(logTag_);
  appender.setShowLevelTag(true);
  appender.setSeparator(" | ");
  appender.setShowTimeStamp(true);
  appender.setTimeStampFormat("%Y-%m-%d %H:%M:%S");
  appender.setPersistent(true);

  appenders.push_back(appender);

  Trace::clearAndAddAppenders(appenders);

  // known projects versions
  std::vector<Version> versions;
  versions.emplace_back(Version("Dynawo", DYNAWO_VERSION_STRING, DYNAWO_GIT_BRANCH, DYNAWO_GIT_HASH));
  versions.emplace_back(Version("Dynawo-algorithms", DYNAWO_ALGORITHMS_VERSION_STRING, DYNAWO_ALGORITHMS_GIT_BRANCH, DYNAWO_ALGORITHMS_GIT_HASH));
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
          versions.emplace_back(Version(section.first, versionString, gitBranch, gitHash));
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
  auto& context = multiprocessing::context();
  std::string inputFileXml = "";

  // Only the main proc should open the archive
  // Unzip the input file in the working directory
  boost::shared_ptr<zip::ZipFile> archive = zip::ZipInputStream::read(inputFileFullPath);
  for (std::map<std::string, boost::shared_ptr<zip::ZipEntry> >::const_iterator itE = archive->getEntries().begin();
      itE != archive->getEntries().end(); ++itE) {
    std::string nom = itE->first;
    std::string data(itE->second->getData());
    std::string filePath = createAbsolutePath(nom, workingDirectory_).c_str();
    if (fileNameFromPath(filePath) == "fic_MULTIPLE.xml") {
      inputFileXml = filePath;
    }

    if (context.isRootProc()) {
      if (!exists(parentDirectory(filePath))) {
        createDirectory(parentDirectory(filePath));
      }
      std::ofstream file;
      file.open(filePath, std::ios::binary);
      if (file.is_open()) {
        file << data;
        file.close();
      }
    }
  }

  multiprocessing::Context::sync();  // To ensure that all procs can access the file

  // When input is given as a zip file we are assuming the multiple jobs definition is always in a file named fic_MULTIPLE.xml
  return inputFileXml;
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
RobustnessAnalysisLauncher::addDydFileToJob(const std::shared_ptr<job::JobEntry>& job, const std::string& dydFile) {
  if (!dydFile.empty()) {
    std::unique_ptr<job::DynModelsEntry> dynModels = job::DynModelsEntryFactory::newInstance();
    dynModels->setDydFile(dydFile);
    job->getModelerEntry()->addDynModelsEntry(std::move(dynModels));
  }
}

void
RobustnessAnalysisLauncher::setCriteriaFileForJob(const std::shared_ptr<job::JobEntry>& job, const std::string& criteriaFile) {
  if (!criteriaFile.empty()) {
    job->getSimulationEntry()->setCriteriaFile(criteriaFile);
  }
}

boost::shared_ptr<DYN::Simulation>
RobustnessAnalysisLauncher::createAndInitSimulation(const std::string& workingDir,
    const std::shared_ptr<job::JobEntry>& job, const SimulationParameters& params, SimulationResult& result, const MultiVariantInputs& analysisContext) {
  std::unique_ptr<DYN::SimulationContext> context = std::unique_ptr<DYN::SimulationContext>(new DYN::SimulationContext());
  context->setResourcesDirectory(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
  context->setLocale(getMandatoryEnvVar("DYNAWO_ALGORITHMS_LOCALE"));
  context->setInputDirectory(workingDirectory_);
  context->setWorkingDirectory(workingDir);

  boost::shared_ptr<DYN::DataInterface> dataInterface = !analysisContext.iidmPath().empty()
    ? DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, analysisContext.iidmPath().generic_string())
    : boost::shared_ptr<DYN::DataInterface>();

  boost::shared_ptr<DYN::Simulation> simulation =
    boost::shared_ptr<DYN::Simulation>(new DYN::Simulation(job, std::move(context), dataInterface));

  if (!params.InitialStateFile_.empty())
    simulation->setInitialStateFile(params.InitialStateFile_);

  if (!params.iidmFile_.empty())
    simulation->setIIDMFile(params.iidmFile_);

  if (!params.exportIIDMFile_.empty())
    simulation->setExportIIDMFile(params.exportIIDMFile_);

  if (!params.dumpFinalStateFile_.empty())
    simulation->setDumpFinalStateFile(params.dumpFinalStateFile_);

  if (!params.activateDumpFinalState_)
    simulation->disableDumpFinalState();

  if (!params.activateExportIIDM_)
    simulation->disableExportIIDM();

  if (params.startTime_ > 0. || DYN::doubleIsZero(params.startTime_))
    simulation->setStartTime(params.startTime_);

  if (params.stopTime_ > 0. || DYN::doubleIsZero(params.stopTime_))
    simulation->setStopTime(params.stopTime_);

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

  if (job->getOutputsEntry() && job->getOutputsEntry()->getTimelineEntry())
    result.setTimelineFileExtensionFromExportMode(job->getOutputsEntry()->getTimelineEntry()->getExportMode());
  if (job->getOutputsEntry() && job->getOutputsEntry()->getConstraintsEntry())
    result.setConstraintsFileExtensionFromExportMode(job->getOutputsEntry()->getConstraintsEntry()->getExportMode());
  if (job->getOutputsEntry() && job->getOutputsEntry()->getLostEquipmentsEntry())
    result.setLostEquipmentsFileExtensionFromExportMode("XML");

  if (job->getOutputsEntry() && job->getOutputsEntry()->getLogsEntry()) {
    const std::vector<std::shared_ptr<job::AppenderEntry> >& appendersEntry = job->getOutputsEntry()->getLogsEntry()->getAppenderEntries();
    for (const auto& appenderEntry : appendersEntry) {
      if (appenderEntry->getTag().empty()) {
        std::string file = createAbsolutePath(job->getOutputsEntry()->getOutputsDirectory(), workingDir);
        file = createAbsolutePath("logs", file);
        file = createAbsolutePath(appenderEntry->getFilePath(), file);
        result.setLogPath(file);
        break;
      }
    }
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
      simulation->disableExportIIDM();
      simulation->terminate();
      result.setSuccess(false);

      std::string e_str(e.what());
      std::replace(e_str.begin(), e_str.end(), '\n', ' ');
      result.setSimulationMessageError(e_str);

      if (e.type() == DYN::Error::SIMULATION) {
        result.setStatus(CRITERIA_NON_RESPECTED_STATUS);
        std::vector<std::pair<double, std::string> > failingCriteria;
        simulation->getFailingCriteria(failingCriteria);
        result.setFailingCriteria(failingCriteria);
      } else if (e.type() == DYN::Error::SOLVER_ALGO || e.type() == DYN::Error::SUNDIALS_ERROR
          || e.type() == DYN::Error::NUMERICAL_ERROR) {
        result.setStatus(DIVERGENCE_STATUS);
        std::vector<std::pair<double, std::string> > failingCriteria;
        simulation->getFailingCriteria(failingCriteria);
        result.setFailingCriteria(failingCriteria);
      } else {
        result.setStatus(EXECUTION_PROBLEM_STATUS);
      }
    } catch (const DYN::MessageError& m) {
      std::cerr << m.what() << std::endl;
      Trace::error() << m.what() << Trace::endline;

      std::string m_str(m.what());
      std::replace(m_str.begin(), m_str.end(), '\n', ' ');
      result.setSimulationMessageError(m_str);

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
    simulation->printLostEquipments(result.getLostEquipementsStream());
    simulation->dumpIIDMFile(result.getOutputIIDMStream());
    return result.getStatus();
}

void
RobustnessAnalysisLauncher::storeOutputs(const SimulationResult& result, std::map<std::string, std::string>& mapData) const {
#ifndef NDEBUG
  Trace::resetPersistentCustomAppender(logTag_, DYN::DEBUG);  // to force flush
#else
  Trace::resetPersistentCustomAppender(logTag_, DYN::INFO);  // to force flush
#endif
  if (!result.getTimelineStreamStr().empty()) {
    std::stringstream timelineName;
    timelineName << "timeLine/timeline_" << result.getUniqueScenarioId() << "." << result.getTimelineFileExtension();
    mapData[timelineName.str()] = result.getTimelineStreamStr();
  }

  if (!result.getConstraintsStreamStr().empty()) {
    std::stringstream constraintsName;
    constraintsName << "constraints/constraints_" << result.getUniqueScenarioId() << "." << result.getConstraintsFileExtension();
    mapData[constraintsName.str()] = result.getConstraintsStreamStr();
  }

  if (!result.getLostEquipementsStreamStr().empty()) {
    std::stringstream lostEquipmentsName;
    lostEquipmentsName << "lostEquipments/lostEquipments_" << result.getUniqueScenarioId() << "." << result.getLostEquipmentsFileExtension();
    mapData[lostEquipmentsName.str()] = result.getLostEquipementsStreamStr();
  }

  if (!result.getOutputIIDMStreamStr().empty()) {
    std::stringstream outputIIDMName;
    outputIIDMName << "finalState/outputIIDM_" << result.getUniqueScenarioId() << ".xml";
    mapData[outputIIDMName.str()] = result.getOutputIIDMStreamStr();
  }

  if (!result.getLogPath().empty()) {
    std::stringstream logName;
    logName << "logs/log_" << result.getUniqueScenarioId() << ".log";
    std::ifstream t(result.getLogPath());
    std::stringstream buffer;
    buffer << t.rdbuf();
    mapData[logName.str()] = buffer.str();
  }
}

void
RobustnessAnalysisLauncher::writeOutputs(const SimulationResult& result) const {
  Trace::resetCustomAppenders();  // to force flush
#ifndef NDEBUG
  Trace::resetPersistentCustomAppender(logTag_, DYN::DEBUG);  // to force flush
#else
  Trace::resetPersistentCustomAppender(logTag_, DYN::INFO);  // to force flush
#endif
  std::string constraintPath = createAbsolutePath("constraints", workingDirectory_);
  if (!isDirectory(constraintPath))
    createDirectory(constraintPath);
  std::string timelinePath = createAbsolutePath("timeLine", workingDirectory_);
  if (!isDirectory(timelinePath))
    createDirectory(timelinePath);
  std::string lostEquipmentsPath = createAbsolutePath("lostEquipments", workingDirectory_);
  if (!isDirectory(lostEquipmentsPath))
    createDirectory(lostEquipmentsPath);
  std::string finalStatePath = createAbsolutePath("finalState", workingDirectory_);
  if (!isDirectory(finalStatePath))
    createDirectory(finalStatePath);
  std::string logPath = createAbsolutePath("logs", workingDirectory_);
  if (!isDirectory(logPath))
    createDirectory(logPath);

  if (!result.getConstraintsStreamStr().empty()) {
    std::ofstream file;
    std::string filepath = createAbsolutePath("constraints_" + result.getUniqueScenarioId() + "." + result.getConstraintsFileExtension(), constraintPath);
    file.open(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath.c_str());
    }
    file << result.getConstraintsStreamStr();
    file.close();
  }

  if (!result.getTimelineStreamStr().empty()) {
    std::string filepath = createAbsolutePath("timeline_" + result.getUniqueScenarioId() + "." + result.getTimelineFileExtension(), timelinePath);
    std::ofstream file;
    file.open(filepath.c_str(), std::ios::binary);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath.c_str());
    }
    file << result.getTimelineStreamStr();
    file.close();
  }

  if (!result.getLostEquipementsStreamStr().empty()) {
    std::string filepath = createAbsolutePath("lostEquipments_" + result.getUniqueScenarioId() + "." + result.getLostEquipmentsFileExtension(),
                                              lostEquipmentsPath);
    std::fstream file;
    file.open(filepath.c_str(), std::fstream::out);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath.c_str());
    }
    file << result.getLostEquipementsStreamStr();
    file.close();
  }

  if (!result.getOutputIIDMStreamStr().empty()) {
    std::string filepath = createAbsolutePath("outputIIDM_" + result.getUniqueScenarioId() + ".xml", finalStatePath);
    std::ofstream file;
    file.open(filepath);
    if (!file.is_open()) {
      throw DYNError(DYN::Error::API, KeyError_t::FileGenerationFailed, filepath);
    }
    file << result.getOutputIIDMStreamStr();
    file.close();
  }

  if (!result.getLogPath().empty()) {
    boost::filesystem::path from(result.getLogPath());
    boost::filesystem::path to(createAbsolutePath("log_" + result.getUniqueScenarioId() + ".log", logPath));
    boost::filesystem::copy_file(from, to, boost::filesystem::copy_option::overwrite_if_exists);
  }
}

void
RobustnessAnalysisLauncher::writeResults() const {
  multiprocessing::Context::sync();  // To ensure that all procs have finished
  Trace::resetCustomAppenders();  // to force flush
  if (!multiprocessing::context().isRootProc()) {
    // only main proccessus is performing the archive
    return;
  }
  std::map<std::string, std::string > mapData;  // association between filename and data
  bool zipIt = extensionEquals(outputFileFullPath_, ".zip");
  createOutputs(mapData, zipIt);

  if (zipIt) {
    boost::shared_ptr<zip::ZipFile> archive = zip::ZipFileFactory::newInstance();

    for (const auto& mapPair : mapData)
      archive->addEntry(mapPair.first, mapPair.second);

    zip::ZipOutputStream::write(outputFileFullPath_, archive);
  }
}

boost::filesystem::path
RobustnessAnalysisLauncher::computeResultFile(const std::string& id) const {
  namespace fs = boost::filesystem;
  fs::path ret(createAbsolutePath(id, workingDirectory_));
  if (!fs::exists(ret)) {
    fs::create_directory(ret);
  }

  ret.append("result.save.txt");

  return ret;
}

SimulationResult
RobustnessAnalysisLauncher::importResult(const std::string& id) const {
  auto filepath = computeResultFile(id);
  SimulationResult ret;
  const char delimiter = ':';

  // Private type to modify the locale for ifstream
  // by default all white spaces are considered for '>>' operator
  class Delimiter : public std::ctype<char> {
   public:
    Delimiter(): std::ctype<char>(getTable()) {}
    static mask const* getTable() {
      static mask rc[table_size];
      rc['\n'] = std::ctype_base::space;
      return &rc[0];
    }
  };
  std::ifstream file(filepath.generic_string());
  if (file.fail()) {
    return ret;
  }
  file.precision(precisionResultFile_);
  file.imbue(std::locale(file.getloc(), new Delimiter));

  // scenario
  std::string tmpStr;
  file >> tmpStr;
  tmpStr = tmpStr.substr(tmpStr.find(delimiter)+1);
  ret.setScenarioId(tmpStr);

  // timeline
  file >> tmpStr;
  assert(tmpStr == "timeline:");
  auto& timeline = ret.getTimelineStream();
  while (tmpStr != "constraints:") {
    file >> tmpStr;
    if (tmpStr == "constraints:") {
      // case no timeline
      break;
    }
    timeline << tmpStr << std::endl;
  }

  // constraints
  auto& constraints = ret.getConstraintsStream();
  while (tmpStr.find("lostEquipments:") != 0) {
    file >> tmpStr;
    if (tmpStr.find("lostEquipments:") == 0) {
      // case no constraints
      break;
    }
    constraints << tmpStr << std::endl;
  }

  // lost equipments
  auto& lostEquipments = ret.getLostEquipementsStream();
  while (tmpStr.find("outputIIDM:") != 0) {
    file >> tmpStr;
    if (tmpStr.find("outputIIDM:") == 0) {
      // case no lost equipments
      break;
    }
    lostEquipments << tmpStr << std::endl;
  }

  // outputIIDM
  auto& outputIIDM = ret.getOutputIIDMStream();
  while (tmpStr.find("variation:") != 0) {
    file >> tmpStr;
    if (tmpStr.find("variation:") == 0) {
      // case no output IIDM
      break;
    }
    outputIIDM << tmpStr << std::endl;
  }

  // variation
  double variation;
  tmpStr = tmpStr.substr(tmpStr.find(delimiter)+1);
  std::stringstream ss(tmpStr);
  ss >> variation;
  ret.setVariation(variation);

  // success
  bool success;
  file >> tmpStr;
  assert(tmpStr.find("success:") == 0);
  tmpStr = tmpStr.substr(tmpStr.find(delimiter)+1);
  ss.clear();
  ss.str(tmpStr);
  ss >> std::boolalpha >> success;
  ret.setSuccess(success);

  // status
  unsigned int status;
  file >> tmpStr;
  assert(tmpStr.find("status:") == 0);
  tmpStr = tmpStr.substr(tmpStr.find(delimiter)+1);
  ss.clear();
  ss.str(tmpStr);
  ss >> status;
  ret.setStatus(static_cast<status_t>(status));

  // timeline extension:
  std::string timelineExtension;
  file >> timelineExtension;
  timelineExtension = timelineExtension.substr(timelineExtension.find(delimiter)+1);
  ret.setTimelineFileExtension(timelineExtension);

  // constraints extension:
  std::string cstrExtension;
  file >> cstrExtension;
  cstrExtension = cstrExtension.substr(cstrExtension.find(delimiter)+1);
  ret.setConstraintsFileExtension(cstrExtension);

  // lost equipments extension:
  std::string lostEquipmentsExtension;
  file >> lostEquipmentsExtension;
  lostEquipmentsExtension = lostEquipmentsExtension.substr(lostEquipmentsExtension.find(delimiter)+1);
  ret.setLostEquipmentsFileExtension(lostEquipmentsExtension);

  // log:
  std::string logPath;
  file >> logPath;
  logPath = logPath.substr(logPath.find(delimiter)+1);
  ret.setLogPath(logPath);

  // criteria
  file >> tmpStr;
  assert(tmpStr == "criteria:");
  tmpStr.clear();
  std::vector<std::pair<double, std::string>> criteria;
  // reading fail can happen at the end of the file if file ends with endline characters
  while (!file.eof() && (file >> tmpStr)) {
    if (tmpStr.find(delimiter) == std::string::npos) {
      break;
    }
    double time;
    auto timeStr = tmpStr.substr(0, tmpStr.find(delimiter));
    ss.clear();
    ss.str(timeStr);
    ss >> time;
    auto message = tmpStr.substr(tmpStr.find(delimiter)+1);
    criteria.emplace_back(std::make_pair(time, message));
  }
  ret.setFailingCriteria(criteria);

  return ret;
}

void
RobustnessAnalysisLauncher::exportResult(const SimulationResult& result) const {
  auto filepath = computeResultFile(result.getUniqueScenarioId());

  std::ofstream file(filepath.generic_string(), std::ios::binary);
  file.precision(precisionResultFile_);
  file << "scenario:" << result.getScenarioId() << std::endl;
  file << "timeline:" << std::endl << result.getTimelineStreamStr() << std::endl;
  file << "constraints:" << std::endl << result.getConstraintsStreamStr() << std::endl;
  file << "lostEquipments:" << std::endl << result.getLostEquipementsStreamStr() << std::endl;
  file << "outputIIDM:" << std::endl << result.getOutputIIDMStreamStr() << std::endl;
  file << "variation:" <<result.getVariation() << std::endl;
  file << "success:" << std::boolalpha << result.getSuccess() << std::endl;
  file << "status:" << static_cast<unsigned int>(result.getStatus()) << std::endl;
  file << "timeline extension:" << result.getTimelineFileExtension() << std::endl;
  file << "constraints extension:" << result.getConstraintsFileExtension() << std::endl;
  file << "lostEquipments extension:" << result.getLostEquipmentsFileExtension() << std::endl;
  file << "log:" << result.getLogPath() << std::endl;
  file << "criteria:" << std::endl;
  for (const auto& criteria : result.getFailingCriteria()) {
    file << criteria.first << ":" << criteria.second << std::endl;
  }
}


void
RobustnessAnalysisLauncher::cleanResult(const std::string& id) const {
  namespace fs = boost::filesystem;
  auto& context = multiprocessing::context();
  if (context.isRootProc()) {
    remove(computeResultFile(id));
    fs::path ret(createAbsolutePath(id, workingDirectory_));
    if (fs::is_empty(ret))
      remove(ret);
  }
}

bool
RobustnessAnalysisLauncher::findExportIIDM(const std::vector<std::shared_ptr<job::FinalStateEntry> >& finalStates) {
  for (const auto& finalState : finalStates) {
    if (!finalState->getTimestamp()) {
      // one without timestamp : it means that it concerns the final state
      return finalState->getExportIIDMFile();
    }
  }

  return false;
}

bool
RobustnessAnalysisLauncher::findExportDump(const std::vector<std::shared_ptr<job::FinalStateEntry> >& finalStates) {
  for (const auto& finalState : finalStates) {
    if (!finalState->getTimestamp()) {
      // one without timestamp : it means that it concerns the final state
      return finalState->getExportDumpFile();
    }
  }

  return false;
}

void
RobustnessAnalysisLauncher::initParametersWithJob(const std::shared_ptr<job::JobEntry>& job, SimulationParameters& params) {
  const std::vector<std::shared_ptr<job::FinalStateEntry> >& finalStateEntries = job->getOutputsEntry()->getFinalStateEntries();
  // It is considered that only the first final entry is relevant for parameters for systematic analysis
  if (!finalStateEntries.empty()) {
    params.activateExportIIDM_ = findExportIIDM(finalStateEntries);
    params.activateDumpFinalState_ = findExportDump(finalStateEntries);
  }
}

}  // namespace DYNAlgorithms
