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
   * @brief default constructor
   */
  LoadIncreaseResult();

  /**
   * @brief get the load level of the load increase
   * @return load level of the load increase
   */
  double getLoadLevel() const;

  /**
   * @brief set the load level of the load increase
   * @param loadLevel value to apply
   */
  void setLoadLevel(double loadLevel);

  /**
   * @brief get the status of the load increase
   * @return status of the load increase
   */
  status_t getStatus() const;

  /**
   * @brief set the status of the load increase
   * @param status value to apply
   */
  void setStatus(status_t status);

  /**
   * @brief specify the number of scenarios
   * @param nbScenarios number of scenarios
   */
  void resize(size_t nbScenarios);

  /**
   * @brief getter of the number of scenarios
   * @return the number of scenarios
   */
  size_t size() const {return results_.size();}

  /**
   * @brief result getter
   * @param idx id of the result
   * @return result object with index idx associated to this load increase
   */
  SimulationResult& getResult(size_t idx);

  /**
   * @brief getter for the results
   * @return results associated to this load increase
   */
  const std::vector<SimulationResult>& getResults() const;

  /**
   * @brief begin iterator on simulation results
   * @return begin iterator on the first result
   */
  std::vector<SimulationResult>::const_iterator begin() const;

  /**
   * @brief end iterator on simulation results
   * @return end iterator on the first result
   */
  std::vector<SimulationResult>::const_iterator end() const;

 private:
  std::vector<SimulationResult> results_;  ///< list of scenario results
  status_t status_;  ///< status of the load increase
  double loadLevel_;  ///< value of the variation of the load increase
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNLOADINCREASERESULT_H_
