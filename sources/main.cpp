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
 * @file  main.cpp
 *
 * @brief main program of Dynawo Algorithms
 *
 */
#include <iostream>
#include <boost/program_options.hpp>

#include <DYNExecUtils.h>
#include <DYNIoDico.h>
#include <DYNInitXml.h>

#include "config_algorithms.h"
#include "gitversion_algorithms.h"
#include "DYNSystematicAnalysisLauncher.h"
#include "DYNMarginCalculationLauncher.h"
#include "DYNComputeLoadVariationLauncher.h"
#include "DYNComputeSimulationLauncher.h"
#include "DYNCriticalTimeLauncher.h"
#include "DYNMultiProcessingContext.h"

using DYNAlgorithms::ComputeSimulationLauncher;
using DYNAlgorithms::SystematicAnalysisLauncher;
using DYNAlgorithms::MarginCalculationLauncher;
using DYNAlgorithms::ComputeLoadVariationLauncher;
using DYNAlgorithms::CriticalTimeLauncher;

namespace po = boost::program_options;

static void launchSimulation(const std::string& jobFile, const std::string& outputFile);
static void launchMarginCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory);
static void launchSystematicAnalysis(const std::string& inputFile, const std::string& outputFile, const std::string& directory);
static void launchLoadVariationCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory, int variation);
static void launchCriticalTimeCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory);

int main(int argc, char** argv) {
  DYNAlgorithms::multiprocessing::Context procContext;  // Should only be used once per process in the main thread
  static_cast<void>(procContext);  // we initialize the multiprocessing context here
  std::string simulationType = "";
  std::string inputFile = "";
  std::string outputFile = "";

  std::string simulationMC_SA_CS_CTC = "Set the simulation type to launch : MC (Margin calculation), SA (systematic analysis), CS (compute simulation)"
  " or CTC (critical time calculation)";

  std::vector<std::string> directoryVec;
  int variation = -1;
  try {
    // declare program options
    // -----------------------
    po::options_description desc;
    desc.add_options()
            ("help,h", "Produce help message")
            ("simulationType", po::value<std::string>(&simulationType)->required(),
             simulationMC_SA_CS_CTC.c_str())
            ("input", po::value<std::string>(&inputFile)->required(),
             "Set the input file of the simulation (*.zip or *.xml)")
            ("output", po::value<std::string>(&outputFile),
             "Set the output file of the simulation (*.zip or *.xml)")
            ("directory", po::value<std::vector<std::string> >(&directoryVec)->multitoken(),
             "Set the working directory of the simulation")
             ("variation", po::value<int>(&variation),
              "Specify a specific load increase variation to launch")
            ("version,v", "Print dynawoAlgorithms version");

    po::variables_map vm;
    // parse regular options
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    if (vm.count("version")) {
      std::cout << DYNAWO_ALGORITHMS_VERSION_STRING << " (rev:" << DYNAWO_ALGORITHMS_GIT_BRANCH << "-" << DYNAWO_ALGORITHMS_GIT_HASH << ")" << std::endl;
      return 0;
    }

    // should be there in order to display help or version without error ( required args not set )
    po::notify(vm);

    std::string directory;
    for (size_t i = 0, iEnd = directoryVec.size(); i < iEnd; ++i) {
      directory += directoryVec[i];
      if (i < iEnd - 1)
        directory += " ";
    }

    // launch simulation
    if (simulationType != "MC" && simulationType != "SA" && simulationType != "CS" && simulationType != "CTC") {
      std::cout << simulationType << " : unknown simulation type" << std::endl;
      std::cout << desc << std::endl;
      return 1;
    }

    if ((simulationType == "SA" || simulationType == "MC" || simulationType == "CTC") && outputFile == "") {
      std::cout << "An output file. (*.zip or *.xml) is required for SA, MC and CTC simulations." << std::endl;
      std::cout << desc << std::endl;
      return 1;
    }

    DYN::InitXerces xerces;
    DYN::InitLibXml2 libxml2;

    DYN::IoDicos& dicos = DYN::IoDicos::instance();
    dicos.addPath(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
    dicos.addDicos(getMandatoryEnvVar("DYNAWO_DICTIONARIES"),
        getMandatoryEnvVar("DYNAWO_ALGORITHMS_LOCALE"));

    if (simulationType == "MC" && variation < 0) {
      launchMarginCalculation(inputFile, outputFile, directory);
    } else if (simulationType == "MC") {
      launchLoadVariationCalculation(inputFile, outputFile, directory, variation);
    } else if (simulationType == "SA") {
      launchSystematicAnalysis(inputFile, outputFile, directory);
    } else if (simulationType == "CS") {
      launchSimulation(inputFile, outputFile);
    } else if (simulationType == "CTC") {
      launchCriticalTimeCalculation(inputFile, outputFile, directory);
    }
  }  catch (const char *s) {
    std::cerr << s << std::endl;
    return -1;
  }  catch (const std::string & Msg) {
    std::cerr << Msg << std::endl;
    return -1;
  }  catch (std::exception & exc) {
    std::cerr << exc.what() << std::endl;
    return -1;
  } catch (...) {
    std::cerr << __FILE__ << " " << __LINE__ << " " << "Unexpected error" << std::endl;
    return -1;
  }

  return 0;
}

void launchSimulation(const std::string& jobFile, const std::string& outputFile) {
  boost::shared_ptr<ComputeSimulationLauncher> simulationLauncher = boost::shared_ptr<ComputeSimulationLauncher>(new ComputeSimulationLauncher());
  simulationLauncher->setInputFile(jobFile);
  simulationLauncher->setOutputFile(outputFile);
  try {
    simulationLauncher->launch();
  }
  catch (...) {
    simulationLauncher->writeResults();
    throw;
  }
  simulationLauncher->writeResults();
}

void launchMarginCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory) {
  boost::shared_ptr<MarginCalculationLauncher> marginCalculationLauncher = boost::shared_ptr<MarginCalculationLauncher>(new MarginCalculationLauncher());
  marginCalculationLauncher->setInputFile(inputFile);
  marginCalculationLauncher->setOutputFile(outputFile);
  marginCalculationLauncher->setDirectory(directory);

  const bool initLog = true;
  marginCalculationLauncher->init(initLog);
  try {
    marginCalculationLauncher->launch();
  }
  catch (...) {
    marginCalculationLauncher->writeResults();
    throw;
  }
  marginCalculationLauncher->writeResults();
}

void launchLoadVariationCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory, int variation) {
  boost::shared_ptr<ComputeLoadVariationLauncher> loadVariationLauncher = boost::shared_ptr<ComputeLoadVariationLauncher>(new ComputeLoadVariationLauncher());
  loadVariationLauncher->setInputFile(inputFile);
  loadVariationLauncher->setOutputFile(outputFile);
  loadVariationLauncher->setDirectory(directory);
  loadVariationLauncher->setVariation(variation);

  loadVariationLauncher->init();
  loadVariationLauncher->launch();
}

void launchSystematicAnalysis(const std::string& inputFile, const std::string& outputFile, const std::string& directory) {
  boost::shared_ptr<SystematicAnalysisLauncher> analysisLauncher = boost::shared_ptr<SystematicAnalysisLauncher>(new SystematicAnalysisLauncher());
  analysisLauncher->setInputFile(inputFile);
  analysisLauncher->setOutputFile(outputFile);
  analysisLauncher->setDirectory(directory);

  const bool initLog = true;
  analysisLauncher->init(initLog);
  analysisLauncher->launch();
  analysisLauncher->writeResults();
}

void launchCriticalTimeCalculation(const std::string& inputFile, const std::string& outputFile, const std::string& directory) {
  boost::shared_ptr<CriticalTimeLauncher> criticalTimeLauncher = boost::shared_ptr<CriticalTimeLauncher>(new CriticalTimeLauncher());
  criticalTimeLauncher->setInputFile(inputFile);
  criticalTimeLauncher->setOutputFile(outputFile);
  criticalTimeLauncher->setDirectory(directory);

  const bool initLog = true;
  criticalTimeLauncher->init(initLog);
  criticalTimeLauncher->launch();
  criticalTimeLauncher->writeResults();
}
