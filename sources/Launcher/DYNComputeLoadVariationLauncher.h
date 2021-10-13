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
 * @file  DYNComputeLoadVariationLauncher.h
 *
 * @brief Launch a single load variation at a specific level based on margin calculation input files
 *
 */
#ifndef LAUNCHER_DYNCOMPUTELOADVARIATIONLAUNCHER_H_
#define LAUNCHER_DYNCOMPUTELOADVARIATIONLAUNCHER_H_

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include "DYNRobustnessAnalysisLauncher.h"

namespace DYNAlgorithms {
class Scenario;
class SimulationResult;
/**
 * @brief Launch a single load variation at a specific level based on margin calculation input files
 */
class ComputeLoadVariationLauncher : public RobustnessAnalysisLauncher {
 public:
  /**
   * @copydoc RobustnessAnalysisLauncher::launch()
   */
  void launch();

  /**
   * @brief set the variation to run
   * @param variation variation to run
   */
  void setVariation(double variation) {
    variation_ = variation;
  }

 private:
  /**
   * @brief create outputs file for each job
   * @param mapData map associating a fileName and the data contained in the file
   * @param zipIt true if we want to fill mapData to create a zip, false if we want to write the files on the disk
   */
  void createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const;

 private:
  double variation_;  ///< variation to launch
};
}  // namespace DYNAlgorithms


#endif  // LAUNCHER_DYNCOMPUTELOADVARIATIONLAUNCHER_H_
