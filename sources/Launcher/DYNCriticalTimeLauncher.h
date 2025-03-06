//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
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
#include "DYNError.h"

namespace DYNAlgorithms {
class SimulationResults;
class CriticalTimeCalculation;
/**
 * @brief Critical Time Calculation launcher class
 *
 * Class for critical time calculation launched by cvg
 */
class CriticalTimeLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @brief Search critical time with a specific default in the simulation. We follow
   * dichotomy's algorithm to find it.
   */
  void launch();

  /**
   * @brief Launch the simulation with the new value calculted in the critical time algorithm
   * @param nbSimulationsDone number of simulation done
   */
  void setParametersAndLaunchSimulation(boost::shared_ptr<CriticalTimeCalculation> criticalTimeCalculation);

  /**
   * @brief Round off the value with the same number of digits than the accuracy
   * @param value value to round off
   */
  double Round(double value);

 protected:
  double tSup_;  ///< value that will be updated until we find the critical time
  double accuracy_;  ///< accuracy of the critical time calculation
  DYNAlgorithms::status_t status_;  ///< final status of the simulation
  SimulationResult results_;  ///< results of the critical time calculation

 private:
  /**
   * @copydoc RobustnessAnalysisLauncher::createOutputs()
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNCRITICALTIMELAUNCHER_H_
