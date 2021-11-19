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
  boost::filesystem::path expectedIIDM(boost::filesystem::current_path());
  expectedIIDM.append(workingDir);
  expectedIIDM.append(iidmFile);

  inputs.readInputs(workingDir, jobFile);
  ASSERT_TRUE(inputs.iidmPath().empty());

  jobFile = "MyJobsWithIIDM.jobs";
  inputs.readInputs(workingDir, jobFile);
  ASSERT_EQ(inputs.iidmPath(), expectedIIDM);
  boost::shared_ptr<job::JobEntry> job1 = inputs.cloneJobEntry();
  boost::shared_ptr<job::JobEntry> job2 = inputs.cloneJobEntry();
  ASSERT_TRUE(job1);
  ASSERT_TRUE(job2);
  ASSERT_NE(job1, job2);
  ASSERT_EQ(job1->getName(), "My Jobs IIDM");

  jobFile = "MyJobs.jobs";
  inputs.readInputs(workingDir, jobFile, iidmFile);
  ASSERT_EQ(inputs.iidmPath(), expectedIIDM);

  boost::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();
  ASSERT_TRUE(job);
  ASSERT_EQ(job->getName(), "My Jobs");
}

}  // namespace DYNAlgorithms
