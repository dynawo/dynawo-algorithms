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

/**
 * @file  DYNDataInterfaceContainer.h
 *
 * @brief data interface container header file
 *
 */

#ifndef LAUNCHER_DYNDATAINTERFACECONTAINER_H_
#define LAUNCHER_DYNDATAINTERFACECONTAINER_H_

#include <DYNDataInterface.h>
#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {

/**
 * @brief data interface container
 *
 * This class aims to hide the multithreading management when it is supported to access to data interfaces
 * in a user friendly way. The hidding is performed using the PIMPL design pattern
 */
class DataInterfaceContainer {
 public:
  /**
   * @brief Constructor
   * @param iidmFile the iidm file to use for data interface reference
   * @param nbVariants the number of variants to use for the data interface
   */
  explicit DataInterfaceContainer(const std::string& iidmFile, unsigned int nbVariants);

  /// @brief Destructor
  ~DataInterfaceContainer();

  /**
   * @brief Init data interface for current thread
   *
   * @param variant variant number to use
   */
  void initDataInterface(unsigned int variant);

  /**
   * @brief retrieve data interface
   *
   * The data interface retrieves depends on the current running thread when multi threading is supported
   *
   * WARNING: it is not guaranteed that two consecutive return the same data interface
   * WARNING: calling this function before calling initDataInterface is undefined behaviour
   *
   * @returns data interface
   */
  boost::shared_ptr<DYN::DataInterface> getDataInterface() const;

 private:
  /// @brief forward declared implementation class
  class DataInterfaceContainerImpl;
  boost::shared_ptr<DataInterfaceContainerImpl> impl_;  ///< Implementation
};
}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNDATAINTERFACECONTAINER_H_
