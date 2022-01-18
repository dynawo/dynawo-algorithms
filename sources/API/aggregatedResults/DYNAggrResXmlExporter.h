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

#ifndef API_AGGREGATEDRESULTS_DYNAGGRRESXMLEXPORTER_H_
#define API_AGGREGATEDRESULTS_DYNAGGRRESXMLEXPORTER_H_

#include <vector>

#include "DYNSimulationResult.h"
#include "DYNLoadIncreaseResult.h"
#include <xml/sax/formatter/Formatter.h>

namespace aggregatedResults {
/**
 * @class XmlExporter
 * @brief aggregated results XML exporter class
 *
 * XML export class for aggregated results
 */
class XmlExporter {
 public:
  /**
   * @brief Export Scenarios results into a file
   *
   * @param results aggregated results to export
   * @param filePath file where the simulation results must be exported
   */
  void exportScenarioResultsToFile(const std::vector<DYNAlgorithms::SimulationResult>& results, const std::string& filePath) const;

  /**
   * @brief Export Scenarios results into a stream
   *
   * @param results aggregated results to export
   * @param stream stream where the simulation results must be exported
   */
  void exportScenarioResultsToStream(const std::vector<DYNAlgorithms::SimulationResult>& results, std::ostream& stream) const;
  /**
   * @brief Export load increase results into a file
   *
   * @param results aggregated results to export
   * @param filePath file where the results must be exported
   */
  void exportLoadIncreaseResultsToFile(const std::vector<DYNAlgorithms::LoadIncreaseResult>& results, const std::string& filePath) const;

  /**
   * @brief Export load increase results into a steam
   *
   * @param results aggregated results to export
   * @param stream stream where the results must be exported
   */
  void exportLoadIncreaseResultsToStream(const std::vector<DYNAlgorithms::LoadIncreaseResult>& results, std::ostream& stream) const;

  /**
   * @brief Export critical time calculation results into a file
   *
   * @param results aggregated results to export
   * @param filePath file where the results must be exported
   */
  void exportCriticalTimeResultsToFile(double criticalTime, const std::string& messageCriticalTimeError, std::string filePath) const;

  /**
   * @brief Export critical time calculation results into a stream
   *
   * @param results aggregated results to export
   * @param stream stream where the results must be exported
   */
  void exportCriticalTimeResultsToStream(double CriticalTime, const std::string& messageCriticalTimeError, std::ostream& stream) const;

 private:
  /**
   * @brief append to the formatter a scenario results
   *
   * @param results aggregated results to export
   * @param formatter formatter to extend
   */
  void appendScenarioResultsElement(const std::vector<DYNAlgorithms::SimulationResult>& results, xml::sax::formatter::FormatterPtr& formatter) const;

  /**
   * @brief append to the formatter non-respected criteria
   *
   * @param result simulation result possibly containing failing criteria
   * @param formatter formatter to extend
   */
  void appendCriteriaNonRespected(const DYNAlgorithms::SimulationResult& result,
                                    xml::sax::formatter::FormatterPtr& formatter) const;
};

}  // namespace aggregatedResults

#endif  // API_AGGREGATEDRESULTS_DYNAGGRRESXMLEXPORTER_H_
