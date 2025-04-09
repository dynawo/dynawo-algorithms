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

/**
 * @file CriticalTimeCalculation.cpp
 * @brief CriticalTimeCalculation description : implementation file
 *
 */

#include <DYNCommon.h>
#include <DYNFileSystemUtils.h>
#include "DYNCriticalTimeCalculation.h"
#include "MacrosMessage.h"
#include "DYNDynamicData.h"
#include "DYNModelDescription.h"
#include <algorithm>

using DYN::DynamicData;
using DYN::ModelDescription;

namespace DYNAlgorithms {
CriticalTimeCalculation::CriticalTimeCalculation():
mode_(SIMPLE) {
}

void
CriticalTimeCalculation::setScenarios(const boost::shared_ptr<Scenarios>& scenarios) {
  scenarios_ = scenarios;
}

boost::shared_ptr<Scenarios>
CriticalTimeCalculation::getScenarios() const {
  return scenarios_;
}

void
CriticalTimeCalculation::setAccuracy(double accuracy) {
  if (accuracy < 0 || DYN::doubleIsZero(accuracy))
    throw DYNAlgorithmsError(IncoherentAccuracyCriticalTime, accuracy);
  accuracy_ = accuracy;
}

double
CriticalTimeCalculation::getAccuracy() const {
  return accuracy_;
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
CriticalTimeCalculation::checkGapBetweenMinValueAndMaxValue() const {
  if (minValue_ > maxValue_ - 2 * accuracy_)
    throw DYNAlgorithmsError(IncoherentMinAndMaxValue, minValue_, maxValue_);
}

void
CriticalTimeCalculation::checkDydIdInDydFiles(std::string workingDir) const {
  const std::vector<boost::shared_ptr<Scenario> >& events = scenarios_->getScenarios();

  std::for_each(events.begin(), events.end(), [this, workingDir](const boost::shared_ptr<Scenario>& scenario) {
    std::string dydFile = scenario->getDydFile();
    std::vector<std::string> dydFiles;
    dydFiles.push_back(createAbsolutePath(dydFile, workingDir));
    boost::shared_ptr<DYN::DynamicData> dyd(new DYN::DynamicData());
    dyd->setRootDirectory(workingDir);
    dyd->initFromDydFiles(dydFiles);

    bool missingDydId = true;
    std::map<std::string, std::shared_ptr<DYN::ModelDescription>> blackboxes = dyd->getBlackBoxModelDescriptions();
    for (auto it = blackboxes.begin(); it != blackboxes.end(); ++it) {
      if (it->first == dydId_) {
        missingDydId = false;
        break;
      }
    }
    if (missingDydId)
      throw DYNAlgorithmsError(DydIdNotInDydFile, dydId_, dydFile);
  });
}

void
CriticalTimeCalculation::sanityChecks(std::string workingDir) const {
  checkGapBetweenMinValueAndMaxValue();
  checkDydIdInDydFiles(workingDir);
}

}  // namespace DYNAlgorithms
