//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
// This file is part of Dynawo, an hybrid C++/Modelica open source suite of simulation tools for power systems.
//

#include "DYNMultiVariantInputs.h"

#include <boost/filesystem.hpp>
#include <gtest_dynawo.h>

#ifdef USE_POWSYBL
#include <powsybl/PowsyblException.hpp>
#include <thread>
#endif

testing::Environment* initXmlEnvironment();

namespace DYNAlgorithms {

testing::Environment* const env = initXmlEnvironment();

TEST(MultiVariant, base) {
  MultiVariantInputs inputs;

  std::string workingDir = "res";
  std::string iidmFile = "IEEE14.iidm";
  std::string jobFile = "MyJobs.jobs";
  unsigned int nbVariants = 2;

  inputs.readInputs(workingDir, jobFile, nbVariants);
  ASSERT_FALSE(inputs.dataInterfaceContainer());

  jobFile = "MyJobsWithIIDM.jobs";
  inputs.readInputs(workingDir, jobFile, nbVariants);
  ASSERT_TRUE(inputs.dataInterfaceContainer());
  inputs.setCurrentVariant(0);
  ASSERT_TRUE(inputs.dataInterfaceContainer()->getDataInterface());
  inputs.setCurrentVariant(1);
  ASSERT_TRUE(inputs.dataInterfaceContainer()->getDataInterface());
#ifdef USE_POWSYBL
  // variant not existing
  ASSERT_THROW(inputs.setCurrentVariant(2), powsybl::PowsyblException);
#endif
  boost::shared_ptr<job::JobEntry> job1 = inputs.cloneJobEntry();
  boost::shared_ptr<job::JobEntry> job2 = inputs.cloneJobEntry();
  ASSERT_TRUE(job1);
  ASSERT_TRUE(job2);
  ASSERT_NE(job1, job2);
  ASSERT_EQ(job1->getName(), "My Jobs IIDM");

  jobFile = "MyJobs.jobs";
  inputs.readInputs(workingDir, jobFile, nbVariants, iidmFile);
  ASSERT_TRUE(inputs.dataInterfaceContainer());
  inputs.setCurrentVariant(0);
  ASSERT_TRUE(inputs.dataInterfaceContainer()->getDataInterface());

  boost::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();
  ASSERT_TRUE(job);
  ASSERT_EQ(job->getName(), "My Jobs");
}

#ifdef USE_POWSYBL
TEST(MultiVariant, multi) {
  MultiVariantInputs inputs;
  constexpr unsigned int nbVariants = 2;

  std::string workingDir = "res";
  std::string iidmFile = "IEEE14.iidm";
  std::string jobFile = "MyJobs.jobs";

  inputs.readInputs(workingDir, jobFile, nbVariants, iidmFile);

  boost::shared_ptr<DYN::DataInterface> dataInterface1;
  boost::shared_ptr<DYN::DataInterface> dataInterface2;

  std::thread thread1([&inputs, &dataInterface1]() {
    inputs.setCurrentVariant(0);
    ASSERT_TRUE(inputs.dataInterfaceContainer()->getDataInterface());
    dataInterface1 = inputs.dataInterfaceContainer()->getDataInterface();
  });
  std::thread thread2([&inputs, &dataInterface2]() {
    inputs.setCurrentVariant(1);
    ASSERT_TRUE(inputs.dataInterfaceContainer()->getDataInterface());
    dataInterface2 = inputs.dataInterfaceContainer()->getDataInterface();
  });

  thread1.join();
  thread2.join();

  ASSERT_NE(dataInterface1, dataInterface2);
}
#endif

}  // namespace DYNAlgorithms
