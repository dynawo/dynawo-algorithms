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
 * @file  DYNScenarios.h
 *
 * @brief Scenarios description : header file
 *
 */

#ifndef COMMON_DYNSCENARIOS_H_
#define COMMON_DYNSCENARIOS_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include "DYNScenario.h"

namespace DYNAlgorithms {

/**
 * @brief Scenarios class
 *
 * Class for the description of several scenario
 */
class Scenarios {
 public:
  /**
   * @brief add a scenario to the list
   * @param scenario scenario to add
   */
  void addScenario(const boost::shared_ptr<Scenario>& scenario);

  /**
   * @brief get the list of scenarios to launch
   * @return list of scenarios to launch
   */
  const std::vector<boost::shared_ptr<Scenario> >& getScenarios() const;

  /**
   * @brief get the number of scenarios to launch
   * @return number of scenarios to launch
   */
  size_t size() const {return scenarios_.size();}

  /**
   * @brief get the jobs file associated to those scenarios
   * @return jobs file associated to those scenarios
   */
  const std::string& getJobsFile() const;

  /**
   * @brief set the jobs file associated to those scenarios
   * @param jobsFile jobs file to associate to those scenarios
   */
  void setJobsFile(const std::string& jobsFile);

 private:
  std::vector<boost::shared_ptr<Scenario> > scenarios_;  ///< list of scenarios to launch
  std::string jobsFile_;  ///< jobs file used as base for the scenarios
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNSCENARIOS_H_
