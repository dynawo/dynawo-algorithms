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

#include "DYNCriticalTimeResult.h"
#include <iostream>

namespace DYNAlgorithms {

void
CriticalTimeResult::setId(const std::string& id) {
  id_ = id;
}

const std::string&
CriticalTimeResult::getId() const {
  return id_;
}

void
CriticalTimeResult::setStatus(const DYNAlgorithms::status_t& status) {
  status_ = status;
}

const DYNAlgorithms::status_t&
CriticalTimeResult::getStatus() const {
  return status_;
}

void
CriticalTimeResult::setCriticalTime(const double& criticalTime) {
  criticalTime_ = criticalTime;
}

const double&
CriticalTimeResult::getCriticicalTime() const {
  return criticalTime_;
}

void
CriticalTimeResult::setResult(SimulationResult& result) {
  result_ = result;
}

const SimulationResult&
CriticalTimeResult::getResult() const {
  return result_;
}

SimulationResult&
CriticalTimeResult::getResult() {
  return result_;
}

}  // namespace DYNAlgorithms
