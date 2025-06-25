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
  void launchScenario(const MultiVariantInputs& context, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result);

  /**
   * @brief generates the IIDM file path for the corresponding variation
   * @param variation the variation of the scenario
   * @returns the corresponding IIDM file path
   */
  std::string generateIDMFileNameForVariation(double variation) const;

  /**
   * @brief read the initial jobs file to set the different normal start and stop times
   * @param jobFileLoadIncrease job file for the loadIncrease
   * @param jobFileScenario job file for the scenario
   */
  void readTimes(const std::string& jobFileLoadIncrease, const std::string& jobFileScenario);

  void initArrays(int accuracy, int nbScenarios);
  int findMaxLoadIncrease(const boost::shared_ptr<LoadIncrease> & loadIncrease);
  bool launchLoadIncreaseWrapper(const boost::shared_ptr<LoadIncrease> & loadIncrease, int varId);
  bool launchScenarioWrapper(const boost::shared_ptr<LoadIncrease> & loadIncrease,
                             const std::string& baseJobsFile,
                             const boost::shared_ptr<Scenario> & scenario,
                             int scenId,
                             int varId);
  void finish(const std::vector<boost::shared_ptr<Scenario> > & events,
              boost::posix_time::ptime & t0,
              int marginGlobal,
              const std::vector<int> & marginsLocal);


  int getVarIdStart() const;
  int getVarIdNext(int varIdMin, int varIdMax) const;
  int getLowestLIFailureVarId() const;
  inline bool liDone(int varId) const {return results_[varId].getResult().getVariation() >= 0;}

 private:
  std::vector<LoadIncreaseResult> results_;  ///< results of the systematic analysis
  std::map<std::string, MultiVariantInputs> inputsByIIDM_;  ///< For scenarios, the contexts to use, by IIDM file
  double tLoadIncrease_;  ///< maximum stop time for the load increase part
  double tScenario_;  ///< stop time for the scenario part
  std::vector<double> discreteVars_;
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNMARGINCALCULATIONLAUNCHER_H_
