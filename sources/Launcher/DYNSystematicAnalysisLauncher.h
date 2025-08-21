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
 * @file  DYNSystematicAnalysisLauncher.h
 *
 * @brief SystematicAnalysis algorithm launcher: header file
 *
 */
#ifndef LAUNCHER_DYNSYSTEMATICANALYSISLAUNCHER_H_
#define LAUNCHER_DYNSYSTEMATICANALYSISLAUNCHER_H_

#include "DYNRobustnessAnalysisLauncher.h"

#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include <vector>

namespace DYNAlgorithms {
class Scenario;
class SimulationResult;
/**
 * @brief Systematic analysis launcher class
 *
 * Class for systematic analysis calculation launched by cvg
 */
class SystematicAnalysisLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @copydoc RobustnessAnalysisLauncher::launch()
   */
  void launch();

 private:
  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;

  /**
   * launch the calculation of one scenario
   * @param scenario scenario to launch
   * @return result of the scenario
   */
  SimulationResult launchScenario(const boost::shared_ptr<Scenario>& scenario);

  /**
   * creates the appropriate working directory for a given scenario, if it does not already exist
   * @param path pathname to create
   */
  void createWorkingDir(const std::string & path);

  /**
   * obtain the ID of the next scenario to simulate, either by incrementing it locally or requesting it remotely
   * @param scenId current ID to update
   * @param maxId first invalid ID on the scenario list 
   * @return true if the updated ID is valid and to be processed, false otherwise
   */
  bool getNextScenId(int & scenId, int maxId);

  /**
   * imports all simulation results from (possibly network) disk
   * @param events the list of scenarios whose simulations results are to be collected
   */
  void collectResults(const std::vector<boost::shared_ptr<Scenario> > & events);

  /**
   * distributes scenario IDs by answering requests and incrementing a local counter, returns when all workers
   * have been dealt an invalid ID and thus terminated
   * @param maxId first invalid ID on the scenario list 
   */
  void serverLoop(int maxId);

 private:
  std::vector<SimulationResult> results_;  ///< results of the systematic analysis
};
}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNSYSTEMATICANALYSISLAUNCHER_H_
