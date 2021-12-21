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
 * @file  DYNCriticalTimeCalculation.h
 *
 * @brief CriticalCalculation description : header file
 *
 */

#ifndef COMMON_DYNCRITICALTIMECALCULATION_H_
#define COMMON_DYNCRITICALTIMECALCULATION_H_

#include <string>

namespace DYNAlgorithms {

/**
 * @brief Margin calculation class
 *
 * Class for margin calculation element : input data
 */
class CriticalTimeCalculation {
 public:
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
   * @brief set the jobs file used for the simulation
   * @param jobsFile jobs file used for the simulation
   */
  void setJobsFile(std::string jobsFile);

  /**
   * @brief get the jobs file used for the simulation
   * @return jobs file used for the simulation
   */
  const std::string& getJobsFile() const;

  /**
   * @brief set the id parameter from the Dyd file.
   * @param parSetId id parameter we will use
   */
  void setDydId(const std::string& dydId);

  /**
   * @brief get the id parameter from the Dyd file.
   * @return id parameter we will use
   */
  std::string getDydId() const;

  /**
   * @brief set the end parameter used for the simulation
   * @param endPar end parameter used for the simulation
   */
  void setEndPar(std::string endPar);

  /**
   * @brief get the end parameter used for the simulation
   * @return end parameter used for the simulation
   */
  std::string getEndPar() const;

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

 private:
  double accuracy_;  ///< accuracy of the algorithm
  std::string jobsFile_;  ///< jobs file used for the simulation
  std::string dydId_;  ///< dyd id in the dyd file
  std::string endPar_;  ///< end parameter used for the simulation
  double minValue_;  ///< minimum value for the critical time
  double maxValue_;  ///< maximum value for the critical time
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNCRITICALTIMECALCULATION_H_
