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

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "DYNRobustnessAnalysisLauncher.h"

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
   * @brief default destructor
   */
  virtual ~SystematicAnalysisLauncher();

  /**
   * @copydoc RobustnessAnalysisLauncher::launch()
   */
  void launch();

 protected:
  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;

 private:
  /**
   * launch the calculation of one scenario
   * @param scenario scenario to launch
   * @return result of the scenario
   */
  SimulationResult launchScenario(const boost::shared_ptr<Scenario>& scenario);

 private:
  std::vector<SimulationResult> results_;  ///< results of the systematic analysis
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNSYSTEMATICANALYSISLAUNCHER_H_
