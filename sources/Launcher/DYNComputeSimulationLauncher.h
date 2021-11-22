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
 * @file  DYNComputeSimulationLauncher.h
 *
 * @brief unitary simulation launcher: header file
 *
 */
#ifndef LAUNCHER_DYNCOMPUTESIMULATIONLAUNCHER_H_
#define LAUNCHER_DYNCOMPUTESIMULATIONLAUNCHER_H_

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <JOBFinalStateEntry.h>
#include "DYNRobustnessAnalysisLauncher.h"

namespace DYNAlgorithms {
class Scenario;
class SimulationResult;
/**
 * @brief Unitary simulation : implementation of the algorithm and interaction with dynawo core
 */
class ComputeSimulationLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @copydoc RobustnessAnalysisLauncher::launch()
   */
  void launch();

 private:
  /**
   * @brief Find in the final state entries if the final state IIDM export is required
   *
   * Find the first final state entry without timestamp, if it exists, and uses it as base to determine if
   * IIDM export for final state is required
   *
   * @param finalStates the list of final state entries to check
   * @return true if IIDM export is required, false if not
   */
  static bool findExportIIDM(const std::vector<boost::shared_ptr<job::FinalStateEntry> >& finalStates);

  /**
   * @brief Find in the final state entries if the final state dump export is required
   *
   * Find the first final state entry without timestamp, if it exists, and uses it as base to determine if
   * dump export for final state is required
   *
   * @param finalStates the list of final state entries to check
   * @return true if dump export is required, false if not
   */
  static bool findExportDump(const std::vector<boost::shared_ptr<job::FinalStateEntry> >& finalStates);

  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;

 private:
  std::vector<SimulationResult> results_;  ///< results of the simulation of the jobs file
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNCOMPUTESIMULATIONLAUNCHER_H_
