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

#include <gtest_dynawo.h>

#include "DYNScenarios.h"
#include "DYNScenario.h"
#include "DYNMarginCalculation.h"
#include "DYNMultipleJobs.h"
#include "DYNMultipleJobsFactory.h"
#include "DYNMultipleJobsXmlHandler.h"

testing::Environment* initXmlEnvironment();

namespace multipleJobs {
testing::Environment* const env = initXmlEnvironment();

TEST(TestMultipleJobs, TestMultipleJobsClass) {
  MultipleJobs mj;
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> mc(new DYNAlgorithms::MarginCalculation());
  boost::shared_ptr<DYNAlgorithms::Scenarios> scenarios(new DYNAlgorithms::Scenarios());
  mj.setScenarios(scenarios);
  mj.setMarginCalculation(mc);
  ASSERT_EQ(mj.getScenarios(), scenarios);
  ASSERT_EQ(mj.getMarginCalculation(), mc);
}

TEST(TestMultipleJobs, TestMultipleJobsFactory) {
  boost::shared_ptr<MultipleJobs> mj = MultipleJobsFactory::newInstance();
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> mc(new DYNAlgorithms::MarginCalculation());
  boost::shared_ptr<DYNAlgorithms::Scenarios> scenarios(new DYNAlgorithms::Scenarios());
  mj->setScenarios(scenarios);
  mj->setMarginCalculation(mc);
  ASSERT_EQ(mj->getScenarios(), scenarios);
  ASSERT_EQ(mj->getMarginCalculation(), mc);
  boost::shared_ptr<MultipleJobs> mj2 = MultipleJobsFactory::copyInstance(mj);
  ASSERT_EQ(mj2->getScenarios(), scenarios);
  ASSERT_EQ(mj2->getMarginCalculation(), mc);
  boost::shared_ptr<MultipleJobs> mj3 = MultipleJobsFactory::copyInstance(*mj);
  ASSERT_EQ(mj3->getScenarios(), scenarios);
  ASSERT_EQ(mj3->getMarginCalculation(), mc);
}

TEST(TestMultipleJobs, TestMultipleJobsXmlHandlerMarginCalculation) {
  XmlHandler multipleJobsHandler;
  std::ifstream stream("res/marginCalculation.xml");

  xml::sax::parser::ParserFactory parser_factory;
  xml::sax::parser::ParserPtr parser = parser_factory.createParser();
  parser->addXmlSchema("multipleJobs.xsd");
  parser->parse(stream, multipleJobsHandler, true);
  boost::shared_ptr<MultipleJobs> mj = multipleJobsHandler.getMultipleJobs();
  assert(mj);
  assert(!mj->getScenarios());
  assert(mj->getMarginCalculation());
  boost::shared_ptr<DYNAlgorithms::MarginCalculation> mc = mj->getMarginCalculation();
  ASSERT_EQ(mc->getAccuracy(), 50);
  ASSERT_EQ(mc->getCalculationType(), DYNAlgorithms::MarginCalculation::LOCAL_MARGIN);
  assert(mc->getLoadIncrease());
  ASSERT_EQ(mc->getLoadIncrease()->getId(), "MyLoadIncrease");
  ASSERT_EQ(mc->getLoadIncrease()->getJobsFile(), "MyLoadIncrease.jobs");
  ASSERT_EQ(mc->getScenarios()->getScenarios().size(), 2);
  ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getId(), "MyScenario");
  ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getDydFile(), "MyScenario.dyd");
  ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getCriteriaFile(), "MyScenario.crt");
  ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getId(), "MyScenario2");
  ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getDydFile(), "MyScenario2.dyd");
  ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getCriteriaFile(), "MyScenario2.crt");
  ASSERT_EQ(mc->getScenarios()->getJobsFile(), "myScenarios.jobs");
}

TEST(TestMultipleJobs, TestMultipleJobsXmlHandlerScenarios) {
  XmlHandler multipleJobsHandler;
  std::ifstream stream("res/scenarios.xml");

  xml::sax::parser::ParserFactory parser_factory;
  xml::sax::parser::ParserPtr parser = parser_factory.createParser();
  parser->addXmlSchema("multipleJobs.xsd");
  parser->parse(stream, multipleJobsHandler, true);
  boost::shared_ptr<MultipleJobs> mj = multipleJobsHandler.getMultipleJobs();
  assert(mj);
  assert(mj->getScenarios());
  assert(!mj->getMarginCalculation());
  boost::shared_ptr<DYNAlgorithms::Scenarios> scenarios = mj->getScenarios();
  ASSERT_EQ(scenarios->getScenarios().size(), 2);
  ASSERT_EQ(scenarios->getScenarios()[0]->getId(), "MyScenario");
  ASSERT_EQ(scenarios->getScenarios()[0]->getDydFile(), "MyScenario.dyd");
  ASSERT_EQ(scenarios->getScenarios()[0]->getCriteriaFile(), "MyScenario.crt");
  ASSERT_EQ(scenarios->getScenarios()[1]->getId(), "MyScenario2");
  ASSERT_EQ(scenarios->getScenarios()[1]->getDydFile(), "MyScenario2.dyd");
  ASSERT_EQ(scenarios->getScenarios()[1]->getCriteriaFile(), "MyScenario2.crt");
  ASSERT_EQ(scenarios->getJobsFile(), "myScenarios.jobs");
}
}  // namespace multipleJobs
