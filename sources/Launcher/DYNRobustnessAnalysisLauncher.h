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
 * @file  DYNRobustnessAnalysisLauncher.h
 *
 * @brief RobustnessAnalysis algorithm launcher: header file
 *
 */
#ifndef LAUNCHER_DYNROBUSTNESSANALYSISLAUNCHER_H_
#define LAUNCHER_DYNROBUSTNESSANALYSISLAUNCHER_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "DYNSimulation.h"
#include "JOBJobEntry.h"
#include "DYNSimulationResult.h"
#include "DYNMultiVariantInputs.h"
#include "DYNDataInterfaceContainer.h"

#include <DYNDataInterface.h>

namespace multipleJobs {
class MultipleJobs;
}

namespace DYNAlgorithms {
/**
 * @brief Robustness analysis launcher class
 *
 * Generic class for robustness calculation launched by cvg (margin calculation, systematic analysis, ...)
 */
class RobustnessAnalysisLauncher {
 public:
  /**
   * @brief default constructor
   */
  RobustnessAnalysisLauncher();

  /**
   * @brief default destructor
   */
  virtual ~RobustnessAnalysisLauncher();

  /**
   * @brief set the inputFile of the analysis
   * @param inputFile inputFile to use
   */
  void setInputFile(const std::string& inputFile);

  /**
   * @brief set the multiple jobs definition of the analysis
   * @param multipleJobs multiple jobs to use
   */
  void setMultipleJobs(boost::shared_ptr<multipleJobs::MultipleJobs>& multipleJobs);

  /**
   * @brief set the outputFile of the analysis
   * @param outputFile outputFile to fill
   */
  void setOutputFile(const std::string& outputFile);

  /**
   * @brief set the working directory of the analysis
   * @param directory working directory to use
   */
  void setDirectory(const std::string& directory);

  /**
   * @brief set the number of threads to use
   * @param nbThreads number of threads to use
   */
  void setNbThreads(const int nbThreads);

  /**
   * @brief initialize the algorithm
   * @param doInitLog True to initialize log
   */
  void init(const bool doInitLog = false);

  /**
   * @brief launch the systematic analysis
   */
  virtual void launch() = 0;

  /**
   * @brief write the results
   */
  void writeResults() const;

 protected:
  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  virtual void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const = 0;

 protected:
  /**
   * @brief Parameters for the simulation
   */
  struct SimulationParameters {
    bool activateExportIIDM_;  ///< if true the iidm will be exported at the end of the simulation
    bool activateDumpFinalState_;  ///< if true the final state will be exported at the end of the simulation
    std::string iidmFile_;  ///< path to the input iidm file, if empty the one defined in the job will be applied
    std::string InitialStateFile_;  ///< path to the initial state file, if empty the one defined in the job will be applied
    std::string dumpFinalStateFile_;  ///< path where to dump the final state file, if empty the one defined in the job will be applied
    std::string exportIIDMFile_;  ///< path where to dump the final iidm file, if empty the one defined in the job will be applied

    /**
     * @brief default constructor
     */
    SimulationParameters() :
      activateExportIIDM_(false),
      activateDumpFinalState_(false) {}
  };

  /**
   * @brief create and initialize a simulation
   * @param workingDir working directory
   * @param job job to simulate
   * @param params simulation parameters
   * @param result will be filled with simulation results if the simulation initialization failed
   * @param analysisContext the analysis context to use for data interface and job
   *
   * @return Initialized simulation or a null pointer if initialization failed
   */
  boost::shared_ptr<DYN::Simulation> createAndInitSimulation(const std::string& workingDir,
      boost::shared_ptr<job::JobEntry>& job, const SimulationParameters& params, SimulationResult& result,
      const MultiVariantInputs& analysisContext);

  /**
   * @brief create and initialize a simulation
   * @param job job to simulate
   * @param dydFile dyd file to add to the job, empty if none to add
   */
  void addDydFileToJob(boost::shared_ptr<job::JobEntry>& job, const std::string& dydFile);

  /**
   * @brief launch a simulation and collect results
   * @param simulation the simulation to launch
   * @param result will be filled with simulation results after call
   *
   * @return simulation result
   */
  status_t simulate(const boost::shared_ptr<DYN::Simulation>& simulation, SimulationResult& result);

  /**
   * @brief store outputs file contents for a result in a container
   * @param result result to dump
   * @param mapData map associating a fileName and the data contained in the file
   */
  void storeOutputs(const SimulationResult& result, std::map<std::string, std::string>& mapData) const;

  /**
   * @brief create outputs files for a result
   * @param result result to dump
   */
  void writeOutputs(const SimulationResult& result) const;

 protected:
  const std::string logTag_;  ///< tag string in dynawo.log
  std::string inputFile_;  ///< input data for the analysis
  std::string outputFile_;  ///< output results of the analysis
  std::string directory_;  ///< working directory as input
  std::string workingDirectory_;  ///< absolute path of the working directory
  std::string outputFileFullPath_;  ///< absolute path of the outputFile
  int nbThreads_;  ///< number of threads to use
  boost::shared_ptr<multipleJobs::MultipleJobs> multipleJobs_;  ///< multipleJobs description tu use for the systematic analysis

  MultiVariantInputs inputs_;  ///< basic analysis context, common to all

 private:
  /**
   * @brief Initialize algorithm log
   */
  void initLog();

  /**
   * read the input data for launching the systematic analysis
   * @param fileName path to the input file
   *
   * @return the multiple jobs instance create thanks to input data
   */
  boost::shared_ptr<multipleJobs::MultipleJobs> readInputData(const std::string& fileName);

  /**
   * unzip input file in working directory and obtain name of multiple jobs definition file
   * @param inputFileFullPath full path of the zip input file
   *
   * @return the full path of the unzipped file containing the multiple jobs definition
   */
  std::string unzipAndGetMultipleJobsFileName(const std::string& inputFileFullPath) const;
};

}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNROBUSTNESSANALYSISLAUNCHER_H_
