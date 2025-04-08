//
// Copyright (c) 2025, RTE (http://www.rte-france.com)
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
 * @file  DYNCriticalTimeLauncher.h
 *
 * @brief CriticalTime algorithm launcher: header file
 *
 */

#ifndef LAUNCHER_DYNCRITICALTIMELAUNCHER_H_
#define LAUNCHER_DYNCRITICALTIMELAUNCHER_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <DYNCommon.h>
#include "DYNRobustnessAnalysisLauncher.h"
#include "DYNCriticalTimeCalculation.h"
#include "DYNCriticalTimeResult.h"
#include "DYNError.h"

namespace DYNAlgorithms {
class SimulationResults;
class CriticalTimeCalculation;
class CriticalTimeResult;
/**
 * @brief Critical Time launcher class
 *
 * Class for critical time calculation
 */
class CriticalTimeLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @brief launch the calculation of critical time for all scenarios
   */
  void launch();

  /**
   * launch the calculation of one scenario
   * @param scenario scenario to launch
   * @param criticalTimeCalculation critical time calculation
   * @return result of the scenario
   */
  CriticalTimeResult launchScenario(const boost::shared_ptr<Scenario>& scenario, std::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation);

  /**
   * @brief Launch the simulation with the new value calculted in the critical time algorithm
   * @param workingDir working directory
   * @param criticalTimeCalculation critical time calculation
   * @param dydFile dyd file
   * @param result result of a simulation
   * @param tSup time to test in the simulation
   */
  void setParametersAndLaunchSimulation(const std::string& workingDir, std::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation,
    const std::string dydFile, SimulationResult& result, double& tSup);

  /**
   * @brief round off the value with the same number of digits than the accuracy
   * @param value value to round off
   * @param accuracy give the number of decimal (ex: accuracy = 0.001)
   * @return rounded value
   */
  double round(double value, double accuracy);

  /**
   * @brief get the status of the Calculation according to the number of simulation done and the ones failed
   * @param nbSimulationsDone number of simulation Done
   * @param nbSimulationsFailed number of simulation Failed
   * @return status of the calculation
   */
  status_t getFinalStatus(int nbSimulationsDone, int nbSimulationsFailed) const;

  /**
   * @brief Export a save result file
   * @param result the Critical time result to export
   */
  void exportResult(const CriticalTimeResult& result) const;

  /**
   * @brief Import simulation result from a save file
   * @param id the scenario id
   * @return CriticalTimeResult from this scenario
   */
  CriticalTimeResult importResult(const std::string& id) const;

 protected:
  std::vector<CriticalTimeResult> results_;  ///< results of all scenarios of the critical time calculation

 private:
  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNCRITICALTIMELAUNCHER_H_
