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
 * @file  DYNMarginCalculationLauncher.h
 *
 * @brief MarginCalculation algorithm launcher: header file
 *
 */
#ifndef LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
#define LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <DYNCommon.h>
#include "DYNRobustnessAnalysisLauncher.h"
#include "DYNLoadIncreaseResult.h"

namespace DYNAlgorithms {
class LoadIncrease;
class Scenario;
class LoadIncreaseResult;
/**
 * @brief Margin Calculation launcher class
 *
 * Class for margin calculation launched by cvg
 */
class MarginCalculationLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @brief destructor
   */
  virtual ~MarginCalculationLauncher();

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
   * @brief Research of the maximum variation value for which all the scenarios pass
   * try to find the maximum load increase between 0 and 100%. Only simulate events that crashes at 100% of load increase
   * at each iteration, keep only the events that crashes with the latest load increase
   * stops iteration when the interval of research is less than a parameter
   *
   * @param loadIncrease scenario to simulate the load increase
   * @param baseJobsFile jobs file to use as basis for the events
   * @param events list of scenarios to launch
   * @param maximumVariationPassing maximum variation passing found so far
   * @param tolerance maximum difference between the real value of the maximum variation and the value returned
   *
   * @return maximum variation value for which all the scenarios pass
   */
  double computeGlobalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
      const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events,
      std::vector<double >& maximumVariationPassing, double tolerance);
  /**
   * @brief Research of the maximum variation value for all the scenarios
   * try to find the maximum load increase between 0 and 100% for each scenario.
   * stops iteration when the interval of research is less than a parameter
   *
   * @param loadIncrease scenario to simulate the load increase
   * @param baseJobsFile jobs file to use as basis for the events
   * @param events list of scenarios to launch
   * @param tolerance maximum difference between the real value of the maximum variation and the value returned
   * @param results adter execution, contains the maximum variation value for the events
   *
   * @return maximum variation value for which the loadIncrease scenario pass
   */
  double computeLocalMargin(const boost::shared_ptr<LoadIncrease>& loadIncrease,
      const std::string& baseJobsFile, const std::vector<boost::shared_ptr<Scenario> >& events, double tolerance,
      std::vector<double >& results);

  /**
   * @brief Find if the variation load-increase was already done
   * otherwise, launch as many load increase as possible in multi-threading, including the variation one
   *
   * @param loadIncrease scenario to simulate the load increase
   * @param variation percentage of launch variation to perform
   * @param tolerance maximum difference between the real value of the maximum variation and the value returned
   * @param result result of the load increase
   *
   */
  void findOrLaunchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease, const double variation,
      const double tolerance,
      SimulationResult& result);

  /**
   * @brief launch the load increase scenario
   * Warning: must remain thread-safe!
   *
   * @param loadIncrease scenario to simulate the load increase
   * @param variation percentage of launch variation to perform
   * @param result result of the load increase
   *
   */
  void launchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease, const double variation, SimulationResult& result);

  /**
   * @brief launch the calculation of one scenario
   * Warning: must remain thread-safe!
   *
   * @param scenario scenario to launch
   * @param baseJobsFile base jobs file
   * @param variation percentage of launch variation
   * @param result result of the simulation
   *
   */
  void launchScenario(const boost::shared_ptr<Scenario>& scenario, const std::string& baseJobsFile, const double variation, SimulationResult& result);

  /**
   * @brief Create the working directory of a scenario
   * @param scenarioId scenario id
   * @param variation percentage of launch variation
   */
  void createScenarioWorkingDir(const std::string& scenarioId, double variation) const;

  /**
   * @brief Initialize margin calculation log
   */
  void initLog();

 private:
  /**
   * @brief double comparison with tolerance
   */
  struct dynawoDoubleLess : public std::binary_function<double, SimulationResult, bool> {
    /**
     * @brief double comparison with tolerance
     * @param left first real to compare
     * @param right second real to compare
     * @return true if left < right
     */
    bool operator() (const double left, const double right) const {
      return !DYN::doubleEquals(left, right) && left < right;
    }
  };

  std::vector<LoadIncreaseResult> results_;  ///< results of the systematic analysis
  std::map<double, SimulationResult, dynawoDoubleLess> loadIncreaseCache_;  ///< contains available load increase simulation results
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
