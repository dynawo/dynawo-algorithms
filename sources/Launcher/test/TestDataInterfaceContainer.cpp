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

#include <DYNDataInterfaceFactory.h>
#include <gtest_dynawo.h>
#include <string>

#ifdef USE_POWSYBL
#include <thread>
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

#ifdef USE_POWSYBL

TEST(TestDataInterfaceContainer, multiThreading) {
  DataInterfaceContainer container("res/IEEE14.iidm", 2);

  ASSERT_FALSE(container.getDataInterface());

  boost::shared_ptr<DYN::DataInterface> dataInterface1;
  boost::shared_ptr<DYN::DataInterface> dataInterface2;

  std::thread thread1([&container, &dataInterface1]() {
    container.initDataInterface(0);
    dataInterface1 = container.getDataInterface();
  });

  std::thread thread2([&container, &dataInterface2]() {
    container.initDataInterface(1);
    dataInterface2 = container.getDataInterface();
  });

  thread1.join();
  thread2.join();

  ASSERT_TRUE(dataInterface1);
  ASSERT_TRUE(dataInterface2);
  ASSERT_NE(dataInterface2, dataInterface1);
}

#endif

}  // namespace DYNAlgorithms
