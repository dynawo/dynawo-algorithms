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

#ifndef COMMON_DYNCRITICALTIMERESULT_H_
#define COMMON_DYNCRITICALTIMERESULT_H_

#include <vector>
#include "DYNSimulationResult.h"
#include "DYNResultCommon.h"

namespace DYNAlgorithms {

/**
 * @brief Critical Time result class
 *
 * Class for the description of a Critical Time result
 */
class CriticalTimeResult {
 public:
   /**
   * @brief constructor
   */
  CriticalTimeResult() = default;

  /**
   * @brief default destructor
   */
  ~CriticalTimeResult() = default;

  /**
   * @brief copy constructor
   * @param result : result to copy
   */
  CriticalTimeResult(const CriticalTimeResult&)= default;

  /**
   * @brief copy operator
   * @param result result to copy
   * @return new instance of criticalTimeResult
   */
  CriticalTimeResult& operator=(const CriticalTimeResult&) = default;

  /**
   * @brief move constructor
   * @param result : result to move
   */
  CriticalTimeResult(CriticalTimeResult&&) = default;

  /**
   * @brief move operator
   * @param result result to move
   * @return new instance of criticalTimeResult
   */
  CriticalTimeResult& operator=(CriticalTimeResult&&) = default;

  /**
   * @brief Set Scenario id
   * @param id Scenario Id
   */
  void setId(const std::string& id);

  /**
   * @brief Get Scenario Id
   * @return Scenario Id
   */
  const std::string& getId() const;

  /**
   * @brief Set Scenario Status
   * @param status Scenario Status
   */
  void setStatus(const DYNAlgorithms::status_t& status);

  /**
   * @brief Get Scenario Status
   * @return Scenario Status
   */
  const DYNAlgorithms::status_t& getStatus() const;

  /**
   * @brief Set Critical Time Value for the scenario
   * @param criticalTime Critical Time Value for the scenario
   */
  void setCriticalTime(const double& criticalTime);

  /**
   * @brief Get Critical Time Value for the scenario
   * @return Critical Time Value for the scenario
   */
  const double& getCriticicalTime() const;

  /**
   * @brief Set Last Simulation Result
   * @param result Simulation Result
   */
  void setResult(SimulationResult& result);

  /**
   * @brief Get Last Simulation Result
   * @return Last Simulation Result
   */
  const SimulationResult& getResult() const;

  /**
   * @brief Get Last Simulation Result
   * @return Last Simulation Result
   */
  SimulationResult& getResult();

 private:
  std::string id_;  ///< Scenario Id
  double criticalTime_;  ///< Critical Time Value
  DYNAlgorithms::status_t status_;  ///< Scenario Status
  SimulationResult result_;  ///< Last Simulation Result
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNCRITICALTIMERESULT_H_
