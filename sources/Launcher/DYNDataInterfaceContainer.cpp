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
 * @file  DYNDataInterfaceContainer.cpp
 *
 * @brief Data interface container implementation file
 *
 */

#include "DYNDataInterfaceContainer.h"

#include <boost/make_shared.hpp>

#ifdef USE_POWSYBL
#include <mutex>
#include <thread>
#include <unordered_map>
#endif

namespace DYNAlgorithms {

#ifdef USE_POWSYBL
class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const boost::shared_ptr<DYN::DataInterface>& data) : referenceDataInterface_(data) {}

  /**
   * @brief Initialize data interface for current thread
   *
   * Update the variant number in network and clone the data interface in order to be used afterwards when retrieved
   * during the main simulation. Update the map by thread id
   *
   * @param variant the variant number to select
   */
  void initDataInterface(unsigned int variant) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (referenceDataInterface_->canUseVariant()) {
      // Must be done before cloning as clone function will perform "get" actions on the network that
      // require the variant number
      referenceDataInterface_->selectVariant(std::to_string(variant));
    }
    auto thread_id = std::this_thread::get_id();
    auto clone = referenceDataInterface_->clone();

    if (dataInterfaces_.count(thread_id) > 0) {
      // replace current element in case was already used
      dataInterfaces_.at(thread_id) = clone;
      return;
    }
    dataInterfaces_.insert({thread_id, clone});
  }

  boost::shared_ptr<DYN::DataInterface> getDataInterface() const {
    auto thread_id = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex_);
    auto found = dataInterfaces_.find(thread_id);
    return (found == dataInterfaces_.end()) ? nullptr : found->second;
  }

 private:
  std::unordered_map<std::thread::id, boost::shared_ptr<DYN::DataInterface> > dataInterfaces_;
  const boost::shared_ptr<DYN::DataInterface> referenceDataInterface_;
  mutable std::mutex mutex_;
};
#else

class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const boost::shared_ptr<DYN::DataInterface>& data) : referenceDataInterface_(data) {}

  void initDataInterface(unsigned int) {
    dataInterface_ = referenceDataInterface_->clone();
  }

  boost::shared_ptr<DYN::DataInterface> getDataInterface() const {
    return dataInterface_;
  }

 private:
  boost::shared_ptr<DYN::DataInterface> dataInterface_;
  const boost::shared_ptr<DYN::DataInterface> referenceDataInterface_;
};
#endif

DataInterfaceContainer::DataInterfaceContainer(const boost::shared_ptr<DYN::DataInterface>& data) :
    impl_(boost::make_shared<DataInterfaceContainerImpl>(data)) {}

DataInterfaceContainer::~DataInterfaceContainer() {}

void
DataInterfaceContainer::initDataInterface(unsigned int variant) {
  impl_->initDataInterface(variant);
}

boost::shared_ptr<DYN::DataInterface>
DataInterfaceContainer::getDataInterface() const {
  return impl_->getDataInterface();
}

}  // namespace DYNAlgorithms
