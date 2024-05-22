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
 * @file  DYNSimulationResult.h
 *
 * @brief Simulation result : header file
 *
 */
#ifndef COMMON_DYNSIMULATIONRESULT_H_
#define COMMON_DYNSIMULATIONRESULT_H_

#include <string>
#include <sstream>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "DYNResultCommon.h"

namespace DYNAlgorithms {

/**
 * @brief Simulation result class
 *
 * Class for the description of a simulation result
 */
class SimulationResult {
 public:
  /**
   * @brief default constructor
   */
  SimulationResult();

  /**
   * @brief default destructor
   */
  ~SimulationResult() = default;

  /**
   * @brief copy constructor
   * @param result : result to copy
   */
  SimulationResult(const SimulationResult& result);

  /**
   * @brief copy operator
   * @param result result to copy
   * @return new instance of simulationResult
   */
  SimulationResult& operator=(const SimulationResult& result);

  /**
   * @brief set the id of the scenario
   * @param id id of the scenario
   */
  void setScenarioId(const std::string& id);

  /**
   * @brief set the variation of the scenario
   * @param variation variation of the scenario
   */
  void setVariation(const double variation);

  /**
   * @brief set whether the simulation succeed or not
   * @param success @b true if the simulation succeed, @b false otherwise
   */
  void setSuccess(const bool success);

  /**
   * @brief set the detailed output status of the simulation
   * @param status detailed output status of the simulation
   */
  void setStatus(status_t status);

  /**
   * @brief getter of the timeline stream associated to the scenario
   * @return timeline stream associated to the scenario
   */
  std::stringstream& getTimelineStream();

  /**
   * @brief getter of the timeline associated to the scenario
   * @return timeline associated to the scenario
   */
  std::string getTimelineStreamStr() const;

  /**
   * @brief getter of the constraints stream associated to the scenario
   * @return constraints stream associated to the scenario
   */
  std::stringstream& getConstraintsStream();

  /**
   * @brief getter of the constraints associated to the scenario
   * @return constraints associated to the scenario
   */
  std::string getConstraintsStreamStr() const;

  /**
   * @brief getter of the lost equipments stream associated to the scenario
   * @return lost equipments stream associated to the scenario
   */
  std::stringstream& getLostEquipementsStream();

  /**
   * @brief getter of the lost equipments associated to the scenario
   * @return lost equipments associated to the scenario
   */
  std::string getLostEquipementsStreamStr() const;

  /**
   * @brief getter of the scenario id associated to the simulation
   * @return the scenario id associated to the simulation
   */
  std::string getScenarioId() const;

  /**
   * @brief getter of the variation of the scenario
   * @return the variation of the scenario
   */
  double getVariation() const;

  /**
   * @brief generate a unique scenario id associating the scenario id and the variation
   * @return unique scenario id associated to the simulation
   */
  std::string getUniqueScenarioId() const;

  /**
   * @brief Get the Unique Scenario Id
   *
   * @param scenarioId scenario id to use
   * @param variation variation to use
   * @return unique scenario id associated to a simulation
   */
  static std::string getUniqueScenarioId(const std::string& scenarioId, double variation);

  /**
   * @brief getter of the indicator of success of the simulation
   * @return indicator of success of the simulation
   */
  bool getSuccess() const;

  /**
   * @brief getter of the detailed output status of the simulation
   * @return detailed output status of the simulation
   */
  status_t getStatus() const;

  /**
   * @brief getter of the failing criteria ids
   * @return failing criteria ids
   */
  const std::vector<std::pair<double, std::string> >& getFailingCriteria() const;

  /**
   * @brief setter of the failing criteria ids
   * @param failingCriteria new failing criteria ids
   */
  void setFailingCriteria(const std::vector<std::pair<double, std::string> >& failingCriteria);

  /**
   * @brief getter of the constraints file extension
   * @return constraints file extension
   */
  const std::string& getConstraintsFileExtension() const;

  /**
   * @brief setter of the constraints file extension based on constraints export mode from the job
   * @param constraintsExportMode constraints export mode
   */
  void setConstraintsFileExtensionFromExportMode(const std::string& constraintsExportMode);

  /**
   * @brief setter of the constraints file extension
   * @param constraintsFileExtension constraints export mode
   */
  void setConstraintsFileExtension(const std::string& constraintsFileExtension);

  /**
   * @brief getter of the timeline file extension
   * @return timeline file extension
   */
  const std::string& getTimelineFileExtension() const;

  /**
   * @brief setter of the timeline file extension based on timeline export mode from the job
   * @param timelineExportMode timeline export mode
   */
  void setTimelineFileExtensionFromExportMode(const std::string &timelineExportMode);

  /**
   * @brief setter of the timeline file extension
   * @param timelineFileExtension timeline export mode
   */
  void setTimelineFileExtension(const std::string &timelineFileExtension);

  /**
   * @brief getter of the lost equipments file extension
   * @return lost equipments file extension
   */
  const std::string& getLostEquipmentsFileExtension() const;

  /**
   * @brief setter of the lost equipments file extension based on lost equipments export mode from the job
   * @param lostEquipmentsExportMode lost equipments export mode
   */
  void setLostEquipmentsFileExtensionFromExportMode(const std::string& lostEquipmentsExportMode);

  /**
   * @brief setter of the lost equipments file extension
   * @param lostEquipmentsFileExtension lost equipments export mode
   */
  void setLostEquipmentsFileExtension(const std::string& lostEquipmentsFileExtension);

  /**
   * @brief getter of the general dynawo log path
   * @return general dynawo log path
   */
  const std::string& getLogPath() const;

  /**
   * @brief setter of the  general dynawo log path
   * @param logPath new general dynawo log path
   */
  void setLogPath(const std::string& logPath);

 private:
  std::stringstream timelineStream_;  ///< stream for the timeline associated to the scenario
  std::stringstream constraintsStream_;  ///< stream for the constraints associated to the scenario
  std::stringstream lostEquipmentsStream_;  ///< stream for the lost equipments associated to the scenario
  std::string scenarioId_;  ///< id of the scenario
  double variation_;  ///< variation of the scenario (aka loadLevel when associated to a load increase)
  bool success_;  ///< @b true if the simulation reached its end, @b false otherwise
  status_t status_;  ///< detailed output status of the simulation
  std::vector<std::pair<double, std::string> > failingCriteria_;  ///< failing criteria ids
  std::string timelineFileExtension_;  ///< timeline export mode for this result
  std::string constraintsFileExtension_;  ///< constraints export mode for this result
  std::string lostEquipmentsFileExtension_;  ///< lost equipments export mode for this result
  std::string logPath_;   ///< Path to the general dynawo log file associated to this result
};

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNSIMULATIONRESULT_H_
