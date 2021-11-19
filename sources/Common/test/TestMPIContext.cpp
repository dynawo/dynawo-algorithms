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

#include "DYNMPIContext.h"

#include <gtest_dynawo.h>

namespace DYNAlgorithms {

TEST(MPIContext, gatherBase) {
  auto& context = mpi::context();

  double test = 10.;
  std::vector<double> gathered;

  context.gather(test, gathered);
  ASSERT_EQ(gathered.size(), 1);
  ASSERT_EQ(gathered.front(), test);
}

TEST(MPIContext, gatherStr) {
  auto& context = mpi::context();

  std::string test = "Test gather";
  std::vector<std::string> gathered;

  context.gather(test, gathered);
  ASSERT_EQ(gathered.size(), 1);
  ASSERT_EQ(gathered.front(), test);
}

TEST(MPIContext, gatherBool) {
  auto& context = mpi::context();

  bool test = true;
  std::vector<bool> gathered;

  context.gather(test, gathered);
  ASSERT_EQ(gathered.size(), 1);
  ASSERT_EQ(gathered.front(), test);
}

TEST(MPIContext, gatherVectBool) {
  auto& context = mpi::context();

  std::vector<bool> test = {
      true,
      false,
  };
  std::vector<std::vector<bool> > gathered;

  context.gather(test, gathered);
  ASSERT_EQ(gathered.size(), 1);
  ASSERT_EQ(gathered.front(), test);
}

TEST(MPIContext, gatherVectEmpty) {
  auto& context = mpi::context();

  std::vector<unsigned int> test;
  std::vector<std::vector<unsigned int> > gathered;

  context.gather(test, gathered);
}

TEST(MPIContext, broadcast) {
  auto& context = mpi::context();

  unsigned int test = 1;

  context.broadcast(test);
  ASSERT_EQ(test, 1);

  std::vector<unsigned int> tests = {0, 1, 2};
  context.broadcast(tests);
  ASSERT_EQ(tests.size(), 3);
  ASSERT_EQ(tests.at(0), 0);
  ASSERT_EQ(tests.at(1), 1);
  ASSERT_EQ(tests.at(2), 2);
}

TEST(MPIContext, broadcastBool) {
  auto& context = mpi::context();

  bool test = true;

  context.broadcast(test);
  ASSERT_EQ(test, 1);

  std::vector<bool> tests = {true, false, true};
  context.broadcast(tests);
  ASSERT_EQ(tests.size(), 3);
  ASSERT_EQ(tests.at(0), true);
  ASSERT_EQ(tests.at(1), false);
  ASSERT_EQ(tests.at(2), true);
}

TEST(MPIContext, broadcastString) {
  auto& context = mpi::context();

  std::string test("Test broadcast");

  context.broadcast(test);
  ASSERT_EQ(test, "Test broadcast");
}

}  // namespace DYNAlgorithms
