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

#ifdef LANG_CXX11
#include <thread>
#endif

namespace DYNAlgorithms {

static boost::shared_ptr<DYN::DataInterface>
getDataInterface() {
  std::string iidmFile = "res/IEEE14.iidm";
  return DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, iidmFile, 1);
}

TEST(TestDataInterfaceContainer, base) {
  boost::shared_ptr<DYN::DataInterface> dataInterface = getDataInterface();
  DataInterfaceContainer container(dataInterface);

  ASSERT_FALSE(container.getDataInterface());

  container.initDataInterface(0);
  // Clone is done : the retrieved data interface is supposed to be different
  boost::shared_ptr<DYN::DataInterface> dataInterface2 = container.getDataInterface();
  ASSERT_NE(dataInterface2, dataInterface);
}

#ifdef LANG_CXX11

TEST(TestDataInterfaceContainer, multiThreading) {
  boost::shared_ptr<DYN::DataInterface> dataInterface = getDataInterface();
  DataInterfaceContainer container(dataInterface);

  ASSERT_FALSE(container.getDataInterface());

  boost::shared_ptr<DYN::DataInterface> dataInterface1;
  boost::shared_ptr<DYN::DataInterface> dataInterface2;

  std::thread thread1([&container, &dataInterface1](){
      container.initDataInterface(0);
      dataInterface1 = container.getDataInterface();
  });

  std::thread thread2([&container, &dataInterface2](){
      container.initDataInterface(1);
      dataInterface2 = container.getDataInterface();
  });

  thread1.join();
  thread2.join();

  ASSERT_TRUE(dataInterface);
  ASSERT_TRUE(dataInterface1);
  ASSERT_TRUE(dataInterface2);
  ASSERT_NE(dataInterface1, dataInterface);
  ASSERT_NE(dataInterface2, dataInterface);
  ASSERT_NE(dataInterface2, dataInterface1);
}

#endif

}  // namespace DYNAlgorithms
