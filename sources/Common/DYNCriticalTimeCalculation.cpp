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

#include <DYNCommon.h>
#include "DYNCriticalTimeCalculation.h"
#include "MacrosMessage.h"

namespace DYNAlgorithms {

void
CriticalTimeCalculation::setAccuracy(double accuracy) {
  if (accuracy < 0 || accuracy > 1 || DYN::doubleIsZero(accuracy))
    throw DYNAlgorithmsError(IncoherentAccuracyCriticalTime, accuracy);
  accuracy_ = accuracy;
}

double
CriticalTimeCalculation::getAccuracy() const {
  return accuracy_;
}

void
CriticalTimeCalculation::setJobsFile(const std::string& jobsFile) {
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

const std::string&
CriticalTimeCalculation::getDydId() const {
  return dydId_;
}

void
CriticalTimeCalculation::setParName(const std::string& parName) {
  parName_ = parName;
}

const std::string&
CriticalTimeCalculation::getParName() const {
  return parName_;
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

void
CriticalTimeCalculation::setMode(CriticalTimeCalculation::mode_t mode) {
  mode_ = mode;
}

CriticalTimeCalculation::mode_t
CriticalTimeCalculation::getMode() const {
  return mode_;
}

void
CriticalTimeCalculation::checkMinValueInferiorMaxValue() {
  if (minValue_ > maxValue_)
    throw DYNAlgorithmsError(IncoherentMinAndMaxValue, minValue_, maxValue_);
}

}  // namespace DYNAlgorithms
