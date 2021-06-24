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
#include <queue>
#include <boost/shared_ptr.hpp>
#include <DYNCommon.h>
#include "DYNRobustnessAnalysisLauncher.h"
#include "DYNLoadIncreaseResult.h"
#include <boost/unordered_map.hpp>

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
   * @brief Description of a set of scenarios to run
   */
  struct task_t{
    double minVariation_;  ///< minimal variation that passes
    double maxVariation_;  ///< maximal variation that fails
    std::vector<size_t> ids_;  ///< indexes of the scenarios to run

    /**
     * @brief constructor
     *
     * @param minVariation minimal variation that passes
     * @param maxVariation maximal variation that fails
     * @param ids indexes of the scenarios to run
     *
     */
    task_t(double minVariation, double maxVariation, const std::vector<size_t>& ids) {
      minVariation_ = minVariation;
      maxVariation_ = maxVariation;
      ids_ = ids;
    }


    /**
     * @brief constructor
     *
     * @param minVariation minimal variation that passes
     * @param maxVariation maximal variation that fails
     *
     */
    task_t(double minVariation, double maxVariation) {
      minVariation_ = minVariation;
      maxVariation_ = maxVariation;
    }
  };

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
   * @brief Find if the scenarios associated to this variation were already done
   * otherwise, launch as many load scenarios as possible in multi-threading, including the variation one
   *
   * @param baseJobsFile base jobs file
   * @param events complete list of scenarios
   * @param toRun scenarios that needs to be run
   * @param result result of the load increase
   *
   */
  void findOrLaunchScenarios(const std::string& baseJobsFile,
      const std::vector<boost::shared_ptr<Scenario> >& events,
      std::queue< task_t >& toRun,
      LoadIncreaseResult& result);


  /**
   * @brief Fill the vector with as many levels of variation you can run based on the number of available threads
   *
   * @param requestedTask the level of reference
   * @param toRun scenarios that needs to be run
   * @param events2Run will be filled with the scenario index and the level that can be run
   *
   */
  void prepareEvents2Run(const task_t& requestedTask,
      std::queue< task_t >& toRun,
      std::vector<std::pair<size_t, double> >& events2Run);

  /**
   * @brief launch the calculation of one scenario
   * Warning: must remain thread-safe!
   *
   * @param context the analysis context to use
   * @param scenario scenario to launch
   * @param variation percentage of launch variation
   * @param result result of the simulation
   *
   */
  void launchScenario(const MultiVariantInputs& context, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result);

  /**
   * @brief fill the queue with the possible levels that could be run with the number of threads available
   *
   * @param minVariation minimal variation that passes
   * @param maxVariation maximal variation that fails
   * @param tolerance maximum difference between the real value of the maximum variation and the value returned
   * @param eventIdxs events that should be run
   * @param toRun queue that will be filled with the task to run after this method
   *
   */
  void findAllLevelsBetween(const double minVariation, const double maxVariation, const double tolerance,
      const std::vector<size_t>& eventIdxs, std::queue< task_t >& toRun);

  /**
   * @brief Create the working directory of a scenario
   * @param scenarioId scenario id
   * @param variation percentage of launch variation
   */
  void createScenarioWorkingDir(const std::string& scenarioId, double variation) const;

  /**
   * @brief generates the IIDM file path for the corresponding variation
   * @param variation the variation of the scenario
   * @returns the corresponding IIDM file path
   */
  std::string generateIDMFileNameForVariation(double variation) const;

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
  std::map<double, LoadIncreaseResult, dynawoDoubleLess> scenariosCache_;  ///< contains available scenarios simulation results
  boost::unordered_map<std::string, MultiVariantInputs> inputsByIIDM_;  ///< For scenarios, the contexts to use, by IIDM file
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
