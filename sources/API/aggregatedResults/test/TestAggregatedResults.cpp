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

#include <fstream>

#include <xml/sax/parser/ParserFactory.h>
#include <DYNExecUtils.h>
#include <gtest_dynawo.h>

#include "DYNSimulationResult.h"
#include "DYNAggrResXmlExporter.h"

namespace aggregatedResults {

TEST(TestAggregatedResults, TestAggregatedResultsScenariosResults) {
  std::vector<DYNAlgorithms::SimulationResult> results(3);
  results[0].setScenarioId("MyFirstScenario");
  results[0].setSuccess(true);
  results[0].setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  results[1].setScenarioId("MySecondScenario");
  results[1].setSuccess(false);
  results[1].setStatus(DYNAlgorithms::CRITERIA_NON_RESPECTED_STATUS);
  std::vector<std::pair<double, std::string> > failingCriteria;
  failingCriteria.push_back(std::make_pair(10, "MyCriteria"));
  results[1].setFailingCriteria(failingCriteria);
  results[2].setScenarioId("MyThirdScenario");
  results[2].setSuccess(false);
  results[2].setStatus(DYNAlgorithms::DIVERGENCE_STATUS);

  XmlExporter exporter;
  exporter.exportScenarioResultsToFile(results, "res/scenarioResults.xml");
  std::stringstream ssDiff;
#ifdef _WIN32
  std::string cmd = "fc res/scenarioResultsRef.xml res/scenarioResults.xml >NUL || fc res/scenarioResultsRef.xml res/scenarioResults.xml";
#else
  std::string cmd = "diff res/scenarioResultsRef.xml res/scenarioResults.xml";
#endif
  executeCommand(cmd, ssDiff);
  std::cout << ssDiff.str() << std::endl;
  std::stringstream ssCmd;
  ssCmd << "Executing command : " << cmd << std::endl;
  ASSERT_EQ(ssDiff.str(), ssCmd.str());
}

TEST(TestAggregatedResults, TestAggregatedResultsLoadIncreaseResults) {
  std::vector<DYNAlgorithms::LoadIncreaseResult> liResults(3);

  liResults[0].setLoadLevel(0.);
  liResults[0].resize(3);
  liResults[0].setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  liResults[0].getResult(0).setScenarioId("MyFirstScenario");
  liResults[0].getResult(0).setSuccess(true);
  liResults[0].getResult(0).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  liResults[0].getResult(1).setScenarioId("MySecondScenario");
  liResults[0].getResult(1).setSuccess(true);
  liResults[0].getResult(1).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  liResults[0].getResult(2).setScenarioId("MyThirdScenario");
  liResults[0].getResult(2).setSuccess(true);
  liResults[0].getResult(2).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);

  liResults[1].setLoadLevel(100.);
  liResults[1].resize(3);
  liResults[1].setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  liResults[1].getResult(0).setScenarioId("MyFirstScenario");
  liResults[1].getResult(0).setSuccess(true);
  liResults[1].getResult(0).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  liResults[1].getResult(1).setScenarioId("MySecondScenario");
  liResults[1].getResult(1).setSuccess(false);
  liResults[1].getResult(1).setStatus(DYNAlgorithms::CRITERIA_NON_RESPECTED_STATUS);
  std::vector<std::pair<double, std::string> > failingCriteria;
  failingCriteria.push_back(std::make_pair(10, "MyCriteria"));
  failingCriteria.push_back(std::make_pair(20, "MyCriteria2"));
  liResults[1].getResult(1).setFailingCriteria(failingCriteria);
  liResults[1].getResult(2).setScenarioId("MyThirdScenario");
  liResults[1].getResult(2).setSuccess(false);
  liResults[1].getResult(2).setStatus(DYNAlgorithms::DIVERGENCE_STATUS);

  liResults[2].setLoadLevel(50.);
  liResults[2].resize(3);
  liResults[2].setStatus(DYNAlgorithms::DIVERGENCE_STATUS);

  XmlExporter exporter;
  exporter.exportLoadIncreaseResultsToFile(liResults, "res/loadIncreaseResults.xml");
  std::stringstream ssDiff;
#ifdef _WIN32
  std::string cmd = "fc res/loadIncreaseResultsRef.xml res/loadIncreaseResults.xml >NUL || fc res/loadIncreaseResultsRef.xml res/loadIncreaseResults.xml";
#else
  std::string cmd = "diff res/loadIncreaseResultsRef.xml res/loadIncreaseResults.xml";
#endif
  executeCommand(cmd, ssDiff);
  std::cout << ssDiff.str() << std::endl;
  std::stringstream ssCmd;
  ssCmd << "Executing command : " << cmd << std::endl;
  ASSERT_EQ(ssDiff.str(), ssCmd.str());
}
}  // namespace aggregatedResults
