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
  std::string generateIDMFileNameForVariation(double variation) const;

  /**
   * @brief initializes global variables and arrays
   */
  void initGlobals();

  /**
   * @brief launch a computation, either locally or remotely depending on multiprocessing configuration
   * @param varId the variation level at which to run it
   * @param scenId the integer ID of the scenario to run, or -1 if load increase
   * @param workerId the integer ID of the worker thread to delegate the computation to, if higher than 0
   * @param complete will be set to true the computation could be run to its completion without interruption, false otherwise
   * @param success only valid if complete is true, indicates whether the computation result satisfies all dynawo success criteria
   */
  void launchTask(int varId, int scenId, int workerId = 0,  int * complete = nullptr, int * success = nullptr);

  /**
   * @brief wrap-up with synthetic log writing, and importing detailed results from output files in case of multithread
   */
  void importResults();

  /**
   * @brief get the lowest known variation level at which the load increase is known to fail
   * @returns said varId (i.e, variation level), or the search space upper bound if none is known to fail
   */
  int lowestLIFailureId() const;

  /**
   * @brief get the highest known variation level at which the load increase is known to succeed, below the lowest known at which it is known to fail
   * @returns said varId (i.e, variation level), or the search space lower bound if none is known to succeed
   */
  int highestLISuccessId() const;

  /**
   * @brief get the lowest known variation level at which the given scenario is known to fail
   * @param scenId the integer id of queried scenario
   * @returns said varId (i.e, variation level), or the search space upper bound if none is known to fail
   */
  int lowestScenFailureId(int scenId) const;

  /**
   * @brief get the highest known variation level at which the given scenario is known to succeed, below the lowest known at which it is known to fail
   * @param scenId the integer id of queried scenario
   * @returns said varId (i.e, variation level), or the search space lower bound if none is known to succeed
   */
  int highestScenSuccessId(int scenId) const;

  /**
   * @brief checks whether a load increase computation that is of interest to the given scenario is already in progress
   * @param scenId the integer id of queried scenario
   * @returns true if there is, false if there is not
   */
  bool hasActiveThreadLI(int scenId) const;

  /**
   * @brief checks whether a scenario computation that is of interest to the given scenario is already in progress
   * @param scenId the integer id of queried scenario
   * @returns true if there is, false if there is not
   */
  bool hasActiveThreadScen(int scenId) const;

  /**
   * @brief checks whether there are still computations to be launched to finish the job requested by user
   * @returns true if there is, false if there is not
   */
  bool workToDo() const;

  /**
   * @brief send a special message to a given idle worker thread ordering it to exit cleanly as it is not needed anymore
   * @param workerId the integer id of the worker thread to terminate
   * @param workersLeft set of remaining worker IDs, in which to remove given workerId
   */
  void terminateWorker(int workerId, std::set<int> & workersLeft) const;

  /**
   * @brief forcefully abort a computation in progress, either a load increase if scenId == 0, or a scenario otherwise. 
   * It does so by sending a SIGINT to the relevant process, which is listened to by the main loop of dynawo
   * @param varId the variation level of the computation to abord
   * @param taskId the ID of the computation, 0 if a load incrase, scenId+1 otherwise
   */
  void abortSimulation(int varId, int taskId);

  /**
   * @brief select the task with highest priority among all possible ones
   * @param varIdRet the variation level of the computation to run
   * @param scenId the ID of the scenario to run, or -1 if the computation to run is a load increase
   * @returns true if a valid task was found, false otherwise
   */
  bool getNextTask(int & varIdRet, int & scenId) const;

  /**
   * @brief subfonction of getNextTask, returns the computation with highest priority amond all computations
   * of interest for a given scenario
   * @param scenId as input, the scenario to explore, and as output, either the same value or -1, depending on
   * if the computation to run is a scenario or a load increase
   * @param varId the variation level of the computation to run
   * @param priority the priority of the computation to run, a lower value meaning higher priority
   */
  void getNextTaskForScen(int & scenId, int & varId, int & priority) const;

  /**
   * @brief find an already computed and successful load increase whose varId is strictly between lower and upper bounds, 
   * if it exists. If more than one does, select the one closest to the mean between bounds
   * @param varIdMin lower bound, returned value has to be strictly higher than this
   * @param varIdMax upper bound, returned value has to be strictly lower than this
   * @returns a varId such as varIdMin < varId < varIdMax, or -1 if no such one exists for which a load increase result is
   * ready to be used
   */
  int getLiOKBetween(int varIdMin, int varIdMax) const;

  /**
   * @brief compute the distance to already computed load increases both lower and higher. 
   * A higher score means a more interesting candidate for uncertainty reduction purposes (principle of dichotomy)
   * @param varId the starting point for which the distance is computed
   * @returns the score for given varId : 0 if invalid, 1 if isolated value, 2 if no imediate value juste above or below, etc.
   */
  int gapScoreLI(int varId) const;

  /**
   * @brief same principle as gapScoreLI, but for scenarios instead of load increases
   * @param varId the starting point for which the distance is computed
   * @param scenId the ID of the scenario for which we are computing the gap
   * @returns the score for given varId : 0 if invalid, 1 if isolated value, 2 if no imediate value juste above or below, etc.
   */
  int gapScoreScen(int varId, int scenId) const;

   /**
   * @brief update global overseeing matrix with a single computation result
   * @param varId the variation level of the finished task
   * @param scenId the integer ID of the finished task (either scenId or -1 for load increase)
   * @param complete true if the computation has run its whole course without interruption, false otherwise
   * @param success valid only if complete, whether the computation was a success or a failure in the sense of dynawo criteria
   */
  void updateResults(int varId, int scenId, bool complete, bool success);

  /**
   * @brief analyze on the global overseeing matrix which computations are not needed anymore due to
   * recent results, and try to abort them
   */
  void abortObsoleteSimus();

  /**
   * @brief blocking serverside wait for a worker to declare itself idle, possibly sending feedback at the same time
   * @param workerId the ID of the worker thread waking up the server and ready to take on a new task
   */
  void waitForWorker(int & workerId);

  /**
   * @brief immediately send an abort signal to all remaining worker threads, then send them a clean termination
   * message on an individual basis as they declare themselves idle
   * @param workersLeft
   */
  void cleanupWorkers(std::set<int> & workersLeft);

  /**
   * @brief build a - hopefully unique - id from 4 last bytes of hostname
   * @returns a 32-bits ID specific to each machine
   */
  int getMachineId() const;

  /**
   * @brief quick accessor to a given scenario
   * @param scenId the ID of the requested scenario
   * @returns the scenario queried by its ID
   */
  inline boost::shared_ptr<Scenario> getScen(int scenId) const;

  /**
   * @brief shorthand for scenarios.size()
   * @returns the number of scenarios, i.e, the first invalid scenId
   */
  inline int nbScens() const;

  /**
   * @brief shorthand for discreteVars_.size()
   * @returns the number of variations, i.e, the first invalid varId
   */
  inline int nbVars() const;

  /**
   * @brief shorthand for discreteVars_.size()-1
   * @returns the highest admissible varId, i.e the index for 100% variation
   */
  inline int varId100() const;

  /**
   * @brief whether the MC is in global or local margin mode
   * @returns true if global margin mode, false if local margin mode
   */
  inline bool globMarginMode() const;

  /**
   * @brief whether a load increase has already been attempted at this variation level
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool liStarted(int varId) const;

  /**
   * @brief whether a load increase has already been completed at this variation level
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool liDone(int varId) const;

  /**
   * @brief whether a load increase has already been completed with success at this variation level
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool liOK(int varId) const;

  /**
   * @brief whether a given scenario has already been attempted at this variation level
   * @param scenId the id of the scenario to check
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool scenStarted(int scenId, int varId) const;

  /**
   * @brief whether a given scenario has already been completed at this variation level
   * @param scenId the id of the scenario to check
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool scenDone(int scenId, int varId) const;

  /**
   * @brief whether a given scenario has already been completed with success at this variation level
   * @param scenId the id of the scenario to check
   * @param varId the variation level to check
   * @returns true if it has been, false if not
   */
  inline bool scenOK(int scenId, int varId) const;

 private:
  boost::shared_ptr<MarginCalculation> mc_;  ///< the input data that was built priori to instanciation, specialized for MC for convenience
  std::map<std::string, MultiVariantInputs> inputsByIIDM_;  ///< For scenarios, the contexts to use, by IIDM file
  double tLoadIncrease_;  ///< maximum stop time for the load increase part
  double tScenario_;  ///< stop time for the scenario part
  boost::posix_time::ptime t0_;  ///< time at which the program was launched, for computation time measurement and display
  int globalMarginVarId_;  ///< varId of the global margin, that is, the maximum variation level at which all scenarios can be run successfully
  std::vector<int> discreteVars_;  ///< variation levels, discretized depending on target accuracy ; varIds are indexes of this global fixed array
  std::vector<int> marginScens_;  ///< the final margins of each scenario, in percentage
  std::vector<LoadIncreaseResult> results_;  ///< global result matrix
  std::vector<std::vector<int> > workStatus_;  ///< array memorizing which computations have already been launched
  std::map<int, int> pids_;  ///< table mapping MPI workerIds to Linux pids (process ids)
  std::vector<int> initialLIs_;  ///< varIds of the load increases to run during the initial round of computations
};

}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
