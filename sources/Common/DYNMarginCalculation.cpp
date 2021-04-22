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
 * @file MarginCalculation.cpp
 * @brief MarginCalculation description : implementation file
 *
 */

#include "DYNMarginCalculation.h"
#include "MacrosMessage.h"

namespace DYNAlgorithms {
MarginCalculation::MarginCalculation():
calculationType_(GLOBAL_MARGIN),
accuracy_(5.) {
}

void
MarginCalculation::setLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease) {
  loadIncrease_ = loadIncrease;
}

boost::shared_ptr<LoadIncrease>
MarginCalculation::getLoadIncrease() const {
  return loadIncrease_;
}

void
MarginCalculation::setScenarios(const boost::shared_ptr<Scenarios>& scenarios) {
  scenarios_ = scenarios;
}

boost::shared_ptr<Scenarios>
MarginCalculation::getScenarios() const {
  return scenarios_;
}

void
MarginCalculation::setAccuracy(int accuracy) {
  if (accuracy < 1 || accuracy > 100)
    throw DYNAlgorithmsError(IncoherentAccuracy, accuracy);
  accuracy_ = accuracy;
}

int
MarginCalculation::getAccuracy() const {
  return accuracy_;
}

void
MarginCalculation::setCalculationType(calculationType_t calculationType) {
  calculationType_ = calculationType;
}
MarginCalculation::calculationType_t
MarginCalculation::getCalculationType() const {
  return calculationType_;
}

}  // namespace DYNAlgorithms
