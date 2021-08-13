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

#include <gtest_dynawo.h>

#include "DYNCommon.h"
#include "DYNScenario.h"
#include "DYNScenarios.h"
#include "DYNMarginCalculation.h"
#include "DYNSimulationResult.h"
#include "DYNLoadIncreaseResult.h"
#include "MacrosMessage.h"

using DYN::doubleEquals;
namespace DYNAlgorithms {

TEST(TestBaseClasses, testScenario) {
  Scenario t;
  ASSERT_EQ(t.getId(), "");
  ASSERT_EQ(t.getDydFile(), "");
  ASSERT_EQ(t.getCriteriaFile(), "");
  t.setId("MyId");
  t.setDydFile("MyDydFile");
  t.setCriteriaFile("MyCrtFile");
  ASSERT_EQ(t.getId(), "MyId");
  ASSERT_EQ(t.getDydFile(), "MyDydFile");
  ASSERT_EQ(t.getCriteriaFile(), "MyCrtFile");
}

TEST(TestBaseClasses, testScenarios) {
  Scenarios s;
  ASSERT_TRUE(s.getScenarios().empty());
  ASSERT_TRUE(s.getJobsFile().empty());
  boost::shared_ptr<Scenario> t1(new Scenario());
  t1->setId("MyId1");
  t1->setDydFile("MyDydFile1");
  t1->setCriteriaFile("MyCrtFile1");
  boost::shared_ptr<Scenario> t2(new Scenario());
  t2->setId("MyId2");
  t2->setDydFile("MyDydFile2");
  t2->setCriteriaFile("MyCrtFile2");
  s.addScenario(t1);
  s.addScenario(t2);
  s.setJobsFile("myJobsFile");
  ASSERT_EQ(s.getScenarios().size(), 2);
  ASSERT_EQ(s.getScenarios()[0]->getId(), "MyId1");
  ASSERT_EQ(s.getScenarios()[0]->getDydFile(), "MyDydFile1");
  ASSERT_EQ(s.getScenarios()[0]->getCriteriaFile(), "MyCrtFile1");
  ASSERT_EQ(s.getScenarios()[1]->getId(), "MyId2");
  ASSERT_EQ(s.getScenarios()[1]->getDydFile(), "MyDydFile2");
  ASSERT_EQ(s.getScenarios()[1]->getCriteriaFile(), "MyCrtFile2");
  ASSERT_EQ(s.getJobsFile(), "myJobsFile");
}

TEST(TestBaseClasses, testMarginCalculation) {
  MarginCalculation mc;
  assert(!mc.getScenarios());
  assert(!mc.getLoadIncrease());
  ASSERT_EQ(mc.getAccuracy(), 5.);
  ASSERT_EQ(mc.getCalculationType(), MarginCalculation::GLOBAL_MARGIN);
  boost::shared_ptr<LoadIncrease> t1(new LoadIncrease());
  t1->setId("MyId1");
  t1->setJobsFile("MyJobsFile1");
  boost::shared_ptr<Scenario> t2(new Scenario());
  t2->setId("MyId2");
  t2->setDydFile("MyDydFile2");
  t2->setCriteriaFile("MyCrtFile2");
  boost::shared_ptr<Scenario> t3(new Scenario());
  t3->setId("MyId3");
  t3->setDydFile("MyDydFile3");
  t3->setCriteriaFile("MyCrtFile3");
  boost::shared_ptr<Scenarios> scenarios(new Scenarios());
  mc.setLoadIncrease(t1);
  scenarios->addScenario(t2);
  scenarios->addScenario(t3);
  mc.setScenarios(scenarios);
  mc.setAccuracy(52);
  mc.setCalculationType(MarginCalculation::LOCAL_MARGIN);
  ASSERT_EQ(mc.getLoadIncrease()->getId(), "MyId1");
  ASSERT_EQ(mc.getLoadIncrease()->getJobsFile(), "MyJobsFile1");
  ASSERT_EQ(mc.getScenarios()->getScenarios().size(), 2);
  ASSERT_EQ(mc.getScenarios()->getScenarios()[0]->getId(), "MyId2");
  ASSERT_EQ(mc.getScenarios()->getScenarios()[0]->getDydFile(), "MyDydFile2");
  ASSERT_EQ(mc.getScenarios()->getScenarios()[0]->getCriteriaFile(), "MyCrtFile2");
  ASSERT_EQ(mc.getScenarios()->getScenarios()[1]->getId(), "MyId3");
  ASSERT_EQ(mc.getScenarios()->getScenarios()[1]->getDydFile(), "MyDydFile3");
  ASSERT_EQ(mc.getScenarios()->getScenarios()[1]->getCriteriaFile(), "MyCrtFile3");
  ASSERT_EQ(mc.getAccuracy(), 52);
  ASSERT_EQ(mc.getCalculationType(), MarginCalculation::LOCAL_MARGIN);

  ASSERT_THROW_DYNAWO(mc.setAccuracy(-1), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::IncoherentAccuracy);
  ASSERT_THROW_DYNAWO(mc.setAccuracy(101), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::IncoherentAccuracy);
  ASSERT_THROW_DYNAWO(mc.setAccuracy(0), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::IncoherentAccuracy);
}

TEST(TestBaseClasses, testSimulationResult) {
  SimulationResult sr;
  ASSERT_TRUE(sr.getScenarioId().empty());
  ASSERT_FALSE(sr.getSuccess());
  ASSERT_EQ(sr.getStatus(), EXECUTION_PROBLEM_STATUS);
  ASSERT_TRUE(sr.getFailingCriteria().empty());
  sr.setScenarioId("MyId");
  sr.setVariation("50");
  sr.setSuccess(true);
  sr.setStatus(CONVERGENCE_STATUS);
  std::vector<std::pair<double, std::string> > failingCriteria;
  failingCriteria.push_back(std::make_pair(10, "MyCriteria"));
  sr.setFailingCriteria(failingCriteria);
  ASSERT_EQ(sr.getScenarioId(), "MyId");
  ASSERT_EQ(sr.getUniqueScenarioId(), "MyId-50");
  ASSERT_TRUE(sr.getSuccess());
  ASSERT_EQ(sr.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(sr.getFailingCriteria().size(), 1);
  ASSERT_EQ(sr.getFailingCriteria()[0].second, "MyCriteria");
  ASSERT_EQ(sr.getFailingCriteria()[0].first, 10);

  SimulationResult srCopy(sr);
  ASSERT_EQ(srCopy.getScenarioId(), "MyId");
  ASSERT_TRUE(srCopy.getSuccess());
  ASSERT_EQ(srCopy.getStatus(), CONVERGENCE_STATUS);

  SimulationResult srCopy2;
  srCopy2 = sr;
  ASSERT_EQ(srCopy2.getScenarioId(), "MyId");
  ASSERT_TRUE(srCopy2.getSuccess());
  ASSERT_EQ(srCopy2.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(getStatusAsString(srCopy2.getStatus()), "CONVERGENCE");
  srCopy2.setStatus(DIVERGENCE_STATUS);
  ASSERT_EQ(getStatusAsString(srCopy2.getStatus()), "DIVERGENCE");
  srCopy2.setStatus(EXECUTION_PROBLEM_STATUS);
  ASSERT_EQ(getStatusAsString(srCopy2.getStatus()), "EXECUTION_PROBLEM");
  srCopy2.setStatus(CRITERIA_NON_RESPECTED_STATUS);
  ASSERT_EQ(getStatusAsString(srCopy2.getStatus()), "CRITERIA_NON_RESPECTED");
}

TEST(TestBaseClasses, testLoadIncreaseResult) {
  LoadIncreaseResult lir;
  ASSERT_TRUE(lir.begin() == lir.end());
  ASSERT_EQ(lir.getStatus(), EXECUTION_PROBLEM_STATUS);
  ASSERT_DOUBLE_EQUALS_DYNAWO(lir.getLoadLevel(), -1.);
  lir.setStatus(CONVERGENCE_STATUS);
  lir.resize(2);
  SimulationResult& sr = lir.getResult(0);
  sr.setScenarioId("MyId");
  sr.setSuccess(true);
  sr.setStatus(CONVERGENCE_STATUS);
  SimulationResult& sr2 = lir.getResult(1);
  sr2.setScenarioId("MyId2");
  sr2.setVariation("65.2");
  sr2.setSuccess(false);
  sr2.setStatus(CRITERIA_NON_RESPECTED_STATUS);
  lir.setLoadLevel(10.);
  ASSERT_EQ(lir.getStatus(), CONVERGENCE_STATUS);
  ASSERT_DOUBLE_EQUALS_DYNAWO(lir.getLoadLevel(), 10.);
  SimulationResult test = *lir.begin();
  ASSERT_EQ(test.getScenarioId(), "MyId");
  ASSERT_EQ(test.getUniqueScenarioId(), "MyId");
  ASSERT_TRUE(test.getSuccess());
  ASSERT_EQ(test.getStatus(), CONVERGENCE_STATUS);
  SimulationResult& test2 = lir.getResult(1);
  ASSERT_EQ(test2.getScenarioId(), "MyId2");
  ASSERT_EQ(test2.getUniqueScenarioId(), "MyId2-65.2");
  ASSERT_FALSE(test2.getSuccess());
  ASSERT_EQ(test2.getStatus(), CRITERIA_NON_RESPECTED_STATUS);
}

}  // namespace DYNAlgorithms
