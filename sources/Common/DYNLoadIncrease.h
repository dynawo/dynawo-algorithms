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

#ifndef COMMON_DYNLOADINCREASE_H_
#define COMMON_DYNLOADINCREASE_H_

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {

/**
 * @brief LoadIncrease class
 *
 * Class for the description of a load increase scenario
 */
class LoadIncrease {
 public:
  /**
   * @brief set the id of the scenario
   * @param id id of the scenario
   */
  void setId(const std::string& id);

  /**
   * @brief set the jobs file to describe the scenario
   * @param jobsFile jobs file to describe the scenario
   */
  void setJobsFile(const std::string& jobsFile);

  /**
   * @brief get the id of the scenario
   * @return id of the scenario
   */
  const std::string& getId() const;

  /**
   * @brief get the jobs file of the scenario
   * @return jobs file of the scenario
   */
  const std::string& getJobsFile() const;

 private:
  std::string id_;  ///< id of the load increase
  std::string jobsFile_;  ///< jobs file to use for the scenario
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNLOADINCREASE_H_
