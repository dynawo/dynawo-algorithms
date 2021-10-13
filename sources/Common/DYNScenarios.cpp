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
 * @file  Scenarios.cpp
 *
 * @brief Scenarios description : implementation file
 *
 */

#include "DYNScenarios.h"

namespace DYNAlgorithms {

void
Scenarios::addScenario(const boost::shared_ptr<Scenario>& scenario) {
  scenarios_.push_back(scenario);
}

const std::vector<boost::shared_ptr<Scenario> >&
Scenarios::getScenarios() const {
  return scenarios_;
}

const std::string&
Scenarios::getJobsFile() const {
  return jobsFile_;
}

void
Scenarios::setJobsFile(const std::string& jobsFile) {
  jobsFile_ = jobsFile;
}

}  // namespace DYNAlgorithms
