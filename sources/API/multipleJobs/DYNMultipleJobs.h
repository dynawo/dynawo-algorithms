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
 * @file  DYNMultipleJobs.h
 *
 * @brief Multiple jobs description : header file
 *
 */
#ifndef API_MULTIPLEJOBS_DYNMULTIPLEJOBS_H_
#define API_MULTIPLEJOBS_DYNMULTIPLEJOBS_H_

#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {
class Scenarios;
class MarginCalculation;
class CriticalTimeCalculation;
}

namespace multipleJobs {
/**
 * @brief Multiple jobs class
 *
 * Class for multiple jobs data input
 */
class MultipleJobs {
 public:
  /**
   * @brief default constructor
   */
  MultipleJobs();

  /**
   * @brief Set the scenarios description to launch
   * @param scenarios scenarios description to launch
   */
  void setScenarios(const boost::shared_ptr<DYNAlgorithms::Scenarios>& scenarios);

  /**
   * @brief Set the margin calculation input data to launch
   * @param marginCalculation margin calculation input data to launch
   */
  void setMarginCalculation(const boost::shared_ptr<DYNAlgorithms::MarginCalculation>& marginCalculation);

  /**
   * @brief Set the critical time calculation input data to launch
   * @param criticalTimeCalculation critical time calculation input data to launch
   */
  void setCriticalTimeCalculation(const boost::shared_ptr<DYNAlgorithms::CriticalTimeCalculation>& criticalTimeCalculation);

  /**
   * @brief get the list of scenarios to launch
   * @return list of scenarios to launch
   */
  boost::shared_ptr<DYNAlgorithms::Scenarios> getScenarios() const;

  /**
   * @brief get the margin calculation to launch
   * @return margin calculation to launch
   */
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> getMarginCalculation() const;

  /**
   * @brief get the critical time calculation to launch
   * @return critical time calculation to launch
   */
  boost::shared_ptr<DYNAlgorithms::CriticalTimeCalculation> getCriticalTimeCalculation() const;

 private:
  boost::shared_ptr<DYNAlgorithms::Scenarios> scenarios_;  ///< scenarios to launch
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> marginCalculation_;  ///< margin calculation to launch
  boost::shared_ptr<DYNAlgorithms::CriticalTimeCalculation> criticalTimeCalculation_;  ///< critical time calculation to launch
};

}  // namespace multipleJobs

#endif  // API_MULTIPLEJOBS_DYNMULTIPLEJOBS_H_

