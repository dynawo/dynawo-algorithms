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
#include "boost/date_time/posix_time/posix_time.hpp"
#include <DYNCommon.h>
#include "DYNRobustnessAnalysisLauncher.h"
#include "DYNLoadIncreaseResult.h"
#include <map>

namespace DYNAlgorithms {
class LoadIncrease;
class Scenario;
class LoadIncreaseResult;
class MarginCalculation;
/**
 * @brief Margin Calculation launcher class
 *
 * Class for margin calculation launched by cvg
 */
class MarginCalculationLauncher : public RobustnessAnalysisLauncher {
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
   * @param context the analysis context to use
   * @param scenario scenario to launch
   * @param variation percentage of launch variation
   * @param result result of the simulation
   *
   */
  void launchScenario(const MultiVariantInputs& context,
                      const boost::shared_ptr<Scenario>& scenario,
                      const double variation,
                      SimulationResult& result);

  /**
   * @brief generates the IIDM file path for the corresponding variation
   * @param variation the variation of the scenario
   * @returns the corresponding IIDM file path
   */
  std::string idmFileNameFromVariation(double variation) const;

  /**
   * @brief generates the load increase working dir path for given variation
   * @param variation the variation of the load increase simulation
   * @returns the corresponding dir path
   */
  std::string loadIncreaseExportDir(double variation) const;

  /**
   * @brief initializes global variables and arrays
   */
  void initGlobals();

  /**
   * @brief server task : centralize results and decisions, attribute simulations to workers
   */
  void serverLoop();

  /**
   * @brief worker task : run the simulations as ordered by server, report results
   */
  void workerLoop();

  /**
   * @brief find the maximal load increase variation level that passes by monothreaded dichotomic search
   * @returns the variation ID of to the maximum load increase level that passes simulation
   */
  int findMaxLoadIncrease();

  /**
   * @brief imports appropriate inputs for a given load increase simulation, run it, and if in a worker thread, reports results to server
   * @param varId the variation level of the load increase simulation
   * @returns the corresponding dir path
   */
  bool launchLoadIncreaseWrapper(int varId);


  /**
   * @brief checks that the load increase works for that variation level, then run the scenario with outputs from load increase if it does
   * @param scenId the integer ID of the scenario to run
   * @param varId the variation level at which to run it
   * @returns the corresponding dir path
   */
  bool launchScenarioWrapper(int scenId, int varId);

  /**
   * @brief wrap-up with synthetic log and files outputting
   * @param t0 posix time at which the computations started, for duration output
   */
  void finish(boost::posix_time::ptime & t0);


  int getNextScenId(int & scenId) const;
  int getVarIdStart() const;
  int getVarIdBetween(int varIdMin, int varIdMax) const;
  int getLowestLIFailureVarId() const;
  int getHighestLISuccessVarId() const;
  int getHighestAllScensSuccessVarId() const;
  int getGlobalMarginVarId() const;
  bool checkLoadIncreaseStatus(int varId, bool & success, int & liVarIdToLaunch);
  int getAnticipatedLoadIncreaseVarId() const;
  void limitVarIdSup(int & varIdSup) const;
  void updateScenMargin(int scenId, int scenMarginVarId);
  bool allScensFinished() const;
  void updateGlobalMargin(int varIdFail);

  inline boost::shared_ptr<Scenario> getScen(int scenId) const;
  inline int nbScens() const;
  inline int nbVars() const;
  inline int varId100() const;
  inline bool searchingGlobalMargin() const;
  inline bool liStarted(int varId) const;
  inline bool liDone(int varId) const;
  inline bool liOK(int varId) const;

 private:
  boost::shared_ptr<MarginCalculation> mc_;
  std::map<std::string, MultiVariantInputs> inputsByIIDM_;  ///< For scenarios, the contexts to use, by IIDM file
  double tLoadIncrease_;  ///< maximum stop time for the load increase part
  double tScenario_;  ///< stop time for the scenario part
  int globalMarginVarId_;
  std::vector<int> discreteVars_;
  std::vector<int> marginScens_;  ///< the final margins of each scenario, in percentage
  std::vector<LoadIncreaseResult> results_;  ///< results of the systematic analysis, by variation level
  std::vector<boost::posix_time::ptime> startingTimes_;  ///< times at which the server ordered a worker to run a load incease, by variation level
};

}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
