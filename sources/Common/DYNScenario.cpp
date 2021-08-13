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
 * @file DYNScenario.cpp
 * @brief Scenario description : implementation file
 *
 */

#include "DYNScenario.h"

namespace DYNAlgorithms {

void
Scenario::setId(const std::string& id) {
  id_ = id;
}

const std::string&
Scenario::getId() const {
  return id_;
}

void
Scenario::setDydFile(const std::string& file) {
  dydFile_ = file;
}

const std::string&
Scenario::getDydFile() const {
  return dydFile_;
}

void
Scenario::setCriteriaFile(const std::string& file) {
  criteriaFile_ = file;
}

const std::string&
Scenario::getCriteriaFile() const {
  return criteriaFile_;
}

}  // namespace DYNAlgorithms
