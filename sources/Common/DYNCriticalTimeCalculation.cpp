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
 * @file CriticalTimeCalculation.cpp
 * @brief CriticalTimeCalculation description : implementation file
 *
 */

#include "DYNCriticalTimeCalculation.h"
#include "MacrosMessage.h"

namespace DYNAlgorithms {

void
CriticalTimeCalculation::setAccuracy(double accuracy) {
  if (accuracy < 0 || accuracy > 1)
    throw DYNAlgorithmsError(IncoherentAccuracyCriticalTime, accuracy);
  accuracy_ = accuracy;
}

double
CriticalTimeCalculation::getAccuracy() const {
  return accuracy_;
}

void
CriticalTimeCalculation::setJobsFile(std::string jobsFile) {
  jobsFile_ = jobsFile;
}

const std::string&
CriticalTimeCalculation::getJobsFile() const {
  return jobsFile_;
}

void
CriticalTimeCalculation::setDydId(const std::string& dydId) {
  dydId_ = dydId;
}

std::string
CriticalTimeCalculation::getDydId() const {
  return dydId_;
}

void
CriticalTimeCalculation::setEndPar(std::string endPar) {
  endPar_ = endPar;
}

std::string
CriticalTimeCalculation::getEndPar() const {
  return endPar_;
}

void
CriticalTimeCalculation::setMinValue(double minValue) {
  minValue_ = minValue;
}

double
CriticalTimeCalculation::getMinValue() const {
  return minValue_;
}

void
CriticalTimeCalculation::setMaxValue(double maxValue) {
  maxValue_ = maxValue;
}

double
CriticalTimeCalculation::getMaxValue() {
  return maxValue_;
}

}  // namespace DYNAlgorithms
