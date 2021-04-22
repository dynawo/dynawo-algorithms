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

#include "DYNLoadIncreaseResult.h"

namespace DYNAlgorithms {

LoadIncreaseResult::LoadIncreaseResult():
status_(EXECUTION_PROBLEM_STATUS),
loadLevel_(-1.) { }

LoadIncreaseResult::~LoadIncreaseResult() { }

double
LoadIncreaseResult::getLoadLevel() const {
  return loadLevel_;
}

void
LoadIncreaseResult::setLoadLevel(double loadLevel) {
  loadLevel_ = loadLevel;
}

status_t
LoadIncreaseResult::getStatus() const {
  return status_;
}

void
LoadIncreaseResult::setStatus(status_t status) {
  status_ = status;
}

void
LoadIncreaseResult::resize(size_t nbScenarios) {
  results_.resize(nbScenarios);
}

SimulationResult&
LoadIncreaseResult::getResult(size_t idx) {
  assert(idx < results_.size());
  return results_[idx];
}

const std::vector<SimulationResult>&
LoadIncreaseResult::getResults() const {
  return results_;
}

std::vector<SimulationResult>::const_iterator LoadIncreaseResult::begin() const {
  return results_.begin();
}

std::vector<SimulationResult>::const_iterator LoadIncreaseResult::end() const {
  return results_.end();
}

}  // namespace DYNAlgorithms
