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
  DYNAlgorithms::LoadIncreaseResult loadIncreaseResult1(3);
  loadIncreaseResult1.getResult().setVariation(0.);
  loadIncreaseResult1.getResult().setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  loadIncreaseResult1.getScenarioResult(0).setScenarioId("MyFirstScenario");
  loadIncreaseResult1.getScenarioResult(0).setSuccess(true);
  loadIncreaseResult1.getScenarioResult(0).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  loadIncreaseResult1.getScenarioResult(1).setScenarioId("MySecondScenario");
  loadIncreaseResult1.getScenarioResult(1).setSuccess(true);
  loadIncreaseResult1.getScenarioResult(1).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  loadIncreaseResult1.getScenarioResult(2).setScenarioId("MyThirdScenario");
  loadIncreaseResult1.getScenarioResult(2).setSuccess(true);
  loadIncreaseResult1.getScenarioResult(2).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);

  DYNAlgorithms::LoadIncreaseResult loadIncreaseResult2(3);
  loadIncreaseResult2.getResult().setVariation(100.);
  loadIncreaseResult2.getResult().setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  loadIncreaseResult2.getScenarioResult(0).setScenarioId("MyFirstScenario");
  loadIncreaseResult2.getScenarioResult(0).setSuccess(true);
  loadIncreaseResult2.getScenarioResult(0).setStatus(DYNAlgorithms::CONVERGENCE_STATUS);
  loadIncreaseResult2.getScenarioResult(1).setScenarioId("MySecondScenario");
  loadIncreaseResult2.getScenarioResult(1).setSuccess(false);
  loadIncreaseResult2.getScenarioResult(1).setStatus(DYNAlgorithms::CRITERIA_NON_RESPECTED_STATUS);
  std::vector<std::pair<double, std::string> > failingCriteria;
  failingCriteria.push_back(std::make_pair(10, "MyCriteria"));
  failingCriteria.push_back(std::make_pair(20, "MyCriteria2"));
  loadIncreaseResult2.getScenarioResult(1).setFailingCriteria(failingCriteria);
  loadIncreaseResult2.getScenarioResult(2).setScenarioId("MyThirdScenario");
  loadIncreaseResult2.getScenarioResult(2).setSuccess(false);
  loadIncreaseResult2.getScenarioResult(2).setStatus(DYNAlgorithms::DIVERGENCE_STATUS);

  DYNAlgorithms::LoadIncreaseResult loadIncreaseResult3(3);
  loadIncreaseResult3.getResult().setVariation(50.);
  loadIncreaseResult3.getResult().setStatus(DYNAlgorithms::DIVERGENCE_STATUS);

  std::vector<DYNAlgorithms::LoadIncreaseResult> liResults;
  liResults.push_back(loadIncreaseResult1);
  liResults.push_back(loadIncreaseResult2);
  liResults.push_back(loadIncreaseResult3);

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
