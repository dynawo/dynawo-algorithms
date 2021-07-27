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

#include "DYNDataInterfaceContainer.h"

#include <gtest_dynawo.h>
#include <string>

#ifdef WITH_OPENMP
#include <omp.h>
#endif

namespace DYNAlgorithms {

TEST(TestDataInterfaceContainer, base) {
  DataInterfaceContainer container("res/IEEE14.iidm", 1);

  boost::shared_ptr<DYN::DataInterface> dataInterface = container.getDataInterface();
#ifdef USE_POWSYBL
  ASSERT_FALSE(dataInterface);
#else
  ASSERT_TRUE(dataInterface);
#endif

  container.initDataInterface(0);

  // case powsybl: Clone is done
  // case legacy: IIDM file is read
  // the retrieved data interface is supposed to be different
  boost::shared_ptr<DYN::DataInterface> dataInterface2 = container.getDataInterface();
  ASSERT_NE(dataInterface2, dataInterface);
}

#ifdef WITH_OPENMP

TEST(TestDataInterfaceContainer, multiThreading) {
  DataInterfaceContainer container("res/IEEE14.iidm", 2);

  ASSERT_FALSE(container.getDataInterface());

  const unsigned int nbThreads = 2;
  omp_set_num_threads(nbThreads);
  std::vector<boost::shared_ptr<DYN::DataInterface> > dataInterfaces(nbThreads);
#pragma omp parallel for schedule(dynamic, 1)
  for (unsigned int i = 0; i < nbThreads; i++) {
    container.initDataInterface(i);
    dataInterfaces[i] = container.getDataInterface();
  }

  ASSERT_TRUE(dataInterfaces.at(0));
  ASSERT_TRUE(dataInterfaces.at(1));
  ASSERT_NE(dataInterfaces.at(0), dataInterfaces.at(1));
}

#endif

}  // namespace DYNAlgorithms
