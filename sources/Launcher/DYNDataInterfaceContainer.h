//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
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

/**
 * @file  DYNDataInterfaceContainer.h
 *
 * @brief data interface container
 *
 */

#ifndef LAUNCHER_DYNDATAINTERFACECONTAINER_H_
#define LAUNCHER_DYNDATAINTERFACECONTAINER_H_

#include <DYNDataInterface.h>
#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {
class DataInterfaceContainer {
 public:
  explicit DataInterfaceContainer(const boost::shared_ptr<DYN::DataInterface>& data);
  ~DataInterfaceContainer();

  void initDataInterface();
  boost::shared_ptr<DYN::DataInterface> getDataInterface() const;

 private:
  class DataInterfaceContainerImpl;
  boost::shared_ptr<DataInterfaceContainerImpl> impl_;
};
}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNDATAINTERFACECONTAINER_H_
