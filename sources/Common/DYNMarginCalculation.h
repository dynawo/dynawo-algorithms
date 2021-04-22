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
 * @file  DYNMarginCalculation.h
 *
 * @brief MarginCalculation description : header file
 *
 */

#ifndef COMMON_DYNMARGINCALCULATION_H_
#define COMMON_DYNMARGINCALCULATION_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "DYNLoadIncrease.h"
#include "DYNScenarios.h"

namespace DYNAlgorithms {

/**
 * @brief Margin calculation class
 *
 * Class for margin calculation element : input data
 */
class MarginCalculation {
 public:
  /**
   * type of margin calculation algorithm
   */
  typedef enum {
    GLOBAL_MARGIN = 0,
    LOCAL_MARGIN
  } calculationType_t;

  /**
   * constructor
   */
  MarginCalculation();

  /**
   * destructor
   */
  virtual ~MarginCalculation() {}

  /**
   * @brief set the load increase event to the margin calculation
   * @param loadIncrease load increase event
   */
  void setLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease);

  /**
   * @brief set the scenarios of the margin calculation
   * @param scenarios scenarios to associate to the margin calculation
   */
  void setScenarios(const boost::shared_ptr<Scenarios>& scenarios);

  /**
   * @brief set the type of the algorithm to use
   * @param calculationType type of the algorithm, could be either @b ALL_DEFAULTS or @b MAX_LOAD
   */
  void setCalculationType(calculationType_t calculationType);

  /**
   * @brief get the calculation type
   * @return calculation type
   */
  calculationType_t getCalculationType() const;

  /**
   * @brief set the accuracy of the algorithm
   * @param accuracy accuracy of the algorithm
   */
  void setAccuracy(int accuracy);

  /**
   * @brief get the accuracy of the algorithm
   * @return accuracy of the algorithm
   */
  int getAccuracy() const;

  /**
   * @brief get the load increase event associated to the margin calculation
   * @return load increase event associated to the margin calculation
   */
  boost::shared_ptr<LoadIncrease> getLoadIncrease() const;

  /**
   * @brief get the scenarios associated to the margin calculation
   * @return scenarios associated to the margin calculation
   */
  boost::shared_ptr<Scenarios> getScenarios() const;

 private:
  boost::shared_ptr<Scenarios> scenarios_;  ///< description of the scenarios to apply after the load increase
  boost::shared_ptr<LoadIncrease> loadIncrease_;  ///< description of the load increase event to apply to the original situation
  calculationType_t calculationType_;  ///< type of the algorithm, could be either @b GLOBAL_MARGIN or @b LOCAL_MARGIN
  double accuracy_;  ///< accuracy of the algorithm
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNMARGINCALCULATION_H_
