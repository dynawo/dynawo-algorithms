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

/**
 * @file  DYNCriticalTimeCalculation.h
 *
 * @brief CriticalCalculation description : header file
 *
 */

#ifndef COMMON_DYNCRITICALTIMECALCULATION_H_
#define COMMON_DYNCRITICALTIMECALCULATION_H_

#include <string>
#include <DYNCommon.h>
#include "DYNScenarios.h"

namespace DYNAlgorithms {

/**
 * @brief Critical Time calculation class
 *
 * Class for Critical Time calculation element : input data
 */
class CriticalTimeCalculation{
 public:
   /**
   * Mode of calculation
   */
  typedef enum {
    SIMPLE = 0,  // Do a simple dichotomy
    COMPLEX = 1,  // Do a dichotomy until it meets a solver issue. it Reduces the range based on previous range and continue the dichotomy
  } mode_t;

  /**
   * constructor
   */
  CriticalTimeCalculation();

  /**
   * @brief set the scenarios of the critical time calculation
   * @param scenarios scenarios to associate to the critical time calculation
   */
  void setScenarios(const boost::shared_ptr<Scenarios>& scenarios);

  /**
   * @brief get the scenarios associated to the critical time calculation
   * @return scenarios associated to the critical time calculation
   */
  boost::shared_ptr<Scenarios> getScenarios() const;

  /**
   * @brief set the accuracy of the algorithm
   * @param accuracy accuracy of the algorithm
   */
  void setAccuracy(double accuracy);

  /**
   * @brief get the accuracy of the algorithm
   * @return accuracy of the algorithm
   */
  double getAccuracy() const;

  /**
   * @brief set the id parameter from the Dyd file.
   * @param dydId id parameter we will use
   */
  void setDydId(const std::string& dydId);

  /**
   * @brief get the id parameter from the Dyd file.
   * @return id parameter we will use
   */
  const std::string& getDydId() const;

  /**
   * @brief set the Name of end parameter used for the simulation
   * @param parName end parameter used for the simulation
   */
  void setParName(const std::string& parName);

  /**
   * @brief get the Name of end parameter used for the simulation
   * @return end parameter used for the simulation
   */
  const std::string& getParName() const;

  /**
   * @brief set the minimum value used for the simulation
   * @param minValue minimum value used for the simulation
   */
  void setMinValue(double minValue);

  /**
   * @brief get the minimum value used for the simulation
   * @return minimum value used for the simulation
   */
  double getMinValue() const;

  /**
   * @brief set the maximum value used for the simulation
   * @param maxValue maximum value used for the simulation
   */
  void setMaxValue(double maxValue);

  /**
   * @brief get the maximum value used for the simulation
   * @return maximum value used for the simulation
   */
  double getMaxValue();

  /**
   * @brief set the mode used for the simulation
   * @param mode mode used for the simulation
   */
  void setMode(mode_t mode);

  /**
   * @brief get the mode used for the simulation
   * @return mode used for the simulation
   */
  mode_t getMode() const;

  /**
   * @brief Check if the gap between min and max is at least two times the accuracy. Throw an error otherwise
   */
  void checkGapBetweenMinValueAndMaxValue() const;

  /**
   * @brief Check if the dydId_ is present the dydFile. Throw an error otherwise
   * @param workingDir working directory
   */
  void checkDydIdInDydFiles(std::string workingDir) const;

  /**
   * @brief Check min and max Value's coherence and presence of dydID in dydFile
   * @param workingDir working directory
   */
  void sanityChecks(std::string workingDir) const;

 private:
  boost::shared_ptr<Scenarios> scenarios_;  ///< description of the scenarios to apply
  double accuracy_;  ///< accuracy of the algorithm
  std::string jobsFile_;  ///< jobs file used for the simulation
  std::string dydId_;  ///< dyd id in the dyd file
  std::string parName_;  ///< name of the double that handles the fault end in the par file
  double minValue_;  ///< minimum value for the critical time
  double maxValue_;  ///< maximum value for the critical time
  mode_t mode_;   ///< mode for the calculation
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNCRITICALTIMECALCULATION_H_
