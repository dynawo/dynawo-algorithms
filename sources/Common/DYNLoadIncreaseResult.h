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

#ifndef COMMON_DYNLOADINCREASERESULT_H_
#define COMMON_DYNLOADINCREASERESULT_H_

#include <vector>
#include "DYNSimulationResult.h"
#include "DYNResultCommon.h"


namespace DYNAlgorithms {

/**
 * @brief load increase result class
 *
 * Class for the description of a load increase result
 */
class LoadIncreaseResult {
 public:
  /**
   * @brief constructor
   * @param nbScenarios number of scenarios result
   */
  explicit LoadIncreaseResult(const size_t nbScenarios);

  /**
   * @brief get the result of the load increase
   * @return result of the load increase
   */
  const SimulationResult& getResult() const {
    return result_;
  }

  /**
   * @brief get the result of the load increase
   * @return result of the load increase
   */
  SimulationResult& getResult() {
    return result_;
  }

  /**
   * @brief set the result of the load increase
   * @param result result of the load increase
   */
  void setResult(const SimulationResult& result) {
    result_ = result;
  }

  /**
   * @brief result getter
   * @param idx id of the result
   * @return result object with index idx associated to this load increase
   */
  SimulationResult& getScenarioResult(size_t idx);

  /**
   * @brief getter for the scenarios results
   * @return scenarios results associated to this load increase
   */
  const std::vector<SimulationResult>& getScenariosResults() const {
    return scenariosResults_;
  }

 private:
  SimulationResult result_;  ///< load increase result
  std::vector<SimulationResult> scenariosResults_;  ///< list of scenarios results
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNLOADINCREASERESULT_H_
