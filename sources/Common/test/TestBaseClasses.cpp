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
#include "DYNCriticalTimeCalculation.h"
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

TEST(TestBaseClasses, testCriticalTimeCalculation) {
  CriticalTimeCalculation ct;
  ct.setAccuracy(0.01);
  ct.setJobsFile("Myjobs.jobs");
  ct.setDydId("MyDydId");
  ct.setEndPar("MyEndPar");
  ct.setMinValue(1);
  ct.setMaxValue(2);
  ASSERT_EQ(ct.getAccuracy(), 0.01);
  ASSERT_EQ(ct.getJobsFile(), "Myjobs.jobs");
  ASSERT_EQ(ct.getDydId(), "MyDydId");
  ASSERT_EQ(ct.getEndPar(), "MyEndPar");
  ASSERT_EQ(ct.getMinValue(), 1);
  ASSERT_EQ(ct.getMaxValue(), 2);

  ASSERT_THROW_DYNAWO(ct.setAccuracy(-1), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::IncoherentAccuracyCriticalTime);
  ASSERT_THROW_DYNAWO(ct.setAccuracy(2), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::IncoherentAccuracyCriticalTime);
}

TEST(TestBaseClasses, testSimulationResult) {
  SimulationResult sr;
  ASSERT_TRUE(sr.getScenarioId().empty());
  ASSERT_FALSE(sr.getSuccess());
  ASSERT_EQ(sr.getStatus(), EXECUTION_PROBLEM_STATUS);
  ASSERT_TRUE(sr.getFailingCriteria().empty());
  ASSERT_EQ(sr.getConstraintsFileExtension(), "xml");
  ASSERT_EQ(sr.getTimelineFileExtension(), "xml");
  ASSERT_EQ(sr.getLostEquipmentsFileExtension(), "xml");
  sr.setScenarioId("MyId");
  sr.setVariation(50.);
  sr.setSuccess(true);
  sr.setStatus(CONVERGENCE_STATUS);
  sr.getConstraintsStream() << "Test Constraints";
  sr.getTimelineStream() << "Test Timeline";
  sr.getLostEquipementsStream() << "Test LostEquipements";
  sr.getOutputIIDMStream() << "Test OutputIIDM";
  sr.setConstraintsFileExtension("log");
  sr.setTimelineFileExtension("log");
  sr.setLostEquipmentsFileExtension("log");
  sr.setLogPath("Test LogPath");
  std::vector<std::pair<double, std::string> > failingCriteria;
  failingCriteria.push_back(std::make_pair(10, "MyCriteria"));
  sr.setFailingCriteria(failingCriteria);
  ASSERT_EQ(sr.getScenarioId(), "MyId");
  ASSERT_EQ(sr.getUniqueScenarioId(), "MyId-50");
  ASSERT_TRUE(sr.getSuccess());
  ASSERT_EQ(sr.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(sr.getConstraintsStreamStr(), "Test Constraints");
  ASSERT_EQ(sr.getTimelineStreamStr(), "Test Timeline");
  ASSERT_EQ(sr.getLostEquipementsStreamStr(), "Test LostEquipements");
  ASSERT_EQ(sr.getOutputIIDMStreamStr(), "Test OutputIIDM");
  ASSERT_EQ(sr.getVariation(), 50);
  ASSERT_EQ(sr.getConstraintsFileExtension(), "log");
  ASSERT_EQ(sr.getTimelineFileExtension(), "log");
  ASSERT_EQ(sr.getLostEquipmentsFileExtension(), "log");
  ASSERT_EQ(sr.getLogPath(), "Test LogPath");
  ASSERT_EQ(sr.getFailingCriteria().size(), 1);
  ASSERT_EQ(sr.getFailingCriteria()[0].second, "MyCriteria");
  ASSERT_EQ(sr.getFailingCriteria()[0].first, 10);

  SimulationResult srCopy(sr);
  ASSERT_EQ(srCopy.getScenarioId(), "MyId");
  ASSERT_TRUE(srCopy.getSuccess());
  ASSERT_EQ(srCopy.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(srCopy.getConstraintsStreamStr(), "Test Constraints");
  ASSERT_EQ(srCopy.getTimelineStreamStr(), "Test Timeline");
  ASSERT_EQ(srCopy.getLostEquipementsStreamStr(), "Test LostEquipements");
  ASSERT_EQ(srCopy.getOutputIIDMStreamStr(), "Test OutputIIDM");
  ASSERT_EQ(srCopy.getVariation(), 50);
  ASSERT_EQ(srCopy.getConstraintsFileExtension(), "log");
  ASSERT_EQ(srCopy.getTimelineFileExtension(), "log");
  ASSERT_EQ(srCopy.getLostEquipmentsFileExtension(), "log");
  ASSERT_EQ(srCopy.getLogPath(), "Test LogPath");
  ASSERT_EQ(srCopy.getFailingCriteria().size(), 1);
  ASSERT_EQ(srCopy.getFailingCriteria()[0].first, 10);
  ASSERT_EQ(srCopy.getFailingCriteria()[0].second, "MyCriteria");

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
  ASSERT_EQ(srCopy2.getConstraintsStreamStr(), "Test Constraints");
  ASSERT_EQ(srCopy2.getTimelineStreamStr(), "Test Timeline");
  ASSERT_EQ(srCopy2.getLostEquipementsStreamStr(), "Test LostEquipements");
  ASSERT_EQ(srCopy2.getOutputIIDMStreamStr(), "Test OutputIIDM");
  ASSERT_EQ(srCopy2.getVariation(), 50);
  ASSERT_EQ(srCopy2.getConstraintsFileExtension(), "log");
  ASSERT_EQ(srCopy2.getTimelineFileExtension(), "log");
  ASSERT_EQ(srCopy2.getLostEquipmentsFileExtension(), "log");
  ASSERT_EQ(srCopy2.getLogPath(), "Test LogPath");
  ASSERT_EQ(srCopy2.getFailingCriteria().size(), 1);
  ASSERT_EQ(srCopy2.getFailingCriteria()[0].first, 10);
  ASSERT_EQ(srCopy2.getFailingCriteria()[0].second, "MyCriteria");

  SimulationResult srMove(std::move(sr));
  ASSERT_EQ(srMove.getScenarioId(), "MyId");
  ASSERT_TRUE(srMove.getSuccess());
  ASSERT_EQ(srMove.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(srMove.getConstraintsStreamStr(), "Test Constraints");
  ASSERT_EQ(srMove.getTimelineStreamStr(), "Test Timeline");
  ASSERT_EQ(srMove.getLostEquipementsStreamStr(), "Test LostEquipements");
  ASSERT_EQ(srMove.getOutputIIDMStreamStr(), "Test OutputIIDM");
  ASSERT_EQ(srMove.getVariation(), 50);
  ASSERT_EQ(srMove.getConstraintsFileExtension(), "log");
  ASSERT_EQ(srMove.getTimelineFileExtension(), "log");
  ASSERT_EQ(srMove.getLostEquipmentsFileExtension(), "log");
  ASSERT_EQ(srMove.getLogPath(), "Test LogPath");
  ASSERT_EQ(srMove.getFailingCriteria().size(), 1);
  ASSERT_EQ(srMove.getFailingCriteria()[0].first, 10);
  ASSERT_EQ(srMove.getFailingCriteria()[0].second, "MyCriteria");

  SimulationResult srMove2;
  srMove2 = std::move(srMove);
  ASSERT_EQ(srMove2.getScenarioId(), "MyId");
  ASSERT_TRUE(srMove2.getSuccess());
  ASSERT_EQ(srMove2.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(srMove2.getConstraintsStreamStr(), "Test Constraints");
  ASSERT_EQ(srMove2.getTimelineStreamStr(), "Test Timeline");
  ASSERT_EQ(srMove2.getLostEquipementsStreamStr(), "Test LostEquipements");
  ASSERT_EQ(srMove2.getOutputIIDMStreamStr(), "Test OutputIIDM");
  ASSERT_EQ(srMove2.getVariation(), 50);
  ASSERT_EQ(srMove2.getConstraintsFileExtension(), "log");
  ASSERT_EQ(srMove2.getTimelineFileExtension(), "log");
  ASSERT_EQ(srMove2.getLostEquipmentsFileExtension(), "log");
  ASSERT_EQ(srMove2.getLogPath(), "Test LogPath");
  ASSERT_EQ(srMove2.getFailingCriteria().size(), 1);
  ASSERT_EQ(srMove2.getFailingCriteria()[0].first, 10);
  ASSERT_EQ(srMove2.getFailingCriteria()[0].second, "MyCriteria");
}

TEST(TestBaseClasses, testLoadIncreaseResult) {
  LoadIncreaseResult lir(2);
  lir.getResult().setScenarioId("MyId1");
  lir.getResult().setVariation(82.);
  lir.getResult().setSuccess(true);
  lir.getResult().setStatus(CONVERGENCE_STATUS);
  SimulationResult& sr1 = lir.getScenarioResult(0);
  sr1.setScenarioId("MyId2");
  sr1.setSuccess(true);
  sr1.setStatus(CONVERGENCE_STATUS);
  SimulationResult& sr2 = lir.getScenarioResult(1);
  sr2.setScenarioId("MyId3");
  sr2.setVariation(65.2);
  sr2.setSuccess(false);
  sr2.setStatus(CRITERIA_NON_RESPECTED_STATUS);
  ASSERT_EQ(lir.getResult().getScenarioId(), "MyId1");
  ASSERT_EQ(lir.getResult().getUniqueScenarioId(), "MyId1-82");
  ASSERT_TRUE(lir.getResult().getSuccess());
  ASSERT_EQ(lir.getResult().getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(sr1.getScenarioId(), "MyId2");
  ASSERT_EQ(sr1.getUniqueScenarioId(), "MyId2");
  ASSERT_TRUE(sr1.getSuccess());
  ASSERT_EQ(sr1.getStatus(), CONVERGENCE_STATUS);
  ASSERT_EQ(sr2.getScenarioId(), "MyId3");
  ASSERT_EQ(sr2.getUniqueScenarioId(), "MyId3-65.2");
  ASSERT_FALSE(sr2.getSuccess());
  ASSERT_EQ(sr2.getStatus(), CRITERIA_NON_RESPECTED_STATUS);
}
}  // namespace DYNAlgorithms
