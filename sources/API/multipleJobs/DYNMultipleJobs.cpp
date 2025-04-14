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
 * @file MultipleJobs.cpp
 * @brief Multiple jobs description : implementation file
 *
 */

#include "DYNMultipleJobs.h"

#include "DYNScenarios.h"
#include "DYNMarginCalculation.h"
#include "DYNCriticalTimeCalculation.h"

using DYNAlgorithms::Scenarios;
using DYNAlgorithms::MarginCalculation;
using DYNAlgorithms::CriticalTimeCalculation;

namespace multipleJobs {

MultipleJobs::MultipleJobs() {
}

void
MultipleJobs::setScenarios(const boost::shared_ptr<Scenarios>& scenarios) {
  scenarios_ = scenarios;
}

void
MultipleJobs::setMarginCalculation(const boost::shared_ptr<MarginCalculation>& marginCalculation) {
  marginCalculation_ = marginCalculation;
}

void
MultipleJobs::setCriticalTimeCalculation(const std::shared_ptr<DYNAlgorithms::CriticalTimeCalculation>& criticalTimeCalculation) {
  criticalTimeCalculation_ = criticalTimeCalculation;
}

boost::shared_ptr<Scenarios>
MultipleJobs::getScenarios() const {
  return scenarios_;
}

boost::shared_ptr<MarginCalculation>
MultipleJobs::getMarginCalculation() const {
  return marginCalculation_;
}

const std::shared_ptr<CriticalTimeCalculation>&
MultipleJobs::getCriticalTimeCalculation() {
  return criticalTimeCalculation_;
}
}  // namespace multipleJobs
