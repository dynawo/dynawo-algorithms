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

#ifdef LANG_CXX11
#include <mutex>
#include <thread>
#include <unordered_map>
#endif

namespace DYNAlgorithms {

#ifdef LANG_CXX11
class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const boost::shared_ptr<DYN::DataInterface>& data) : referenceDataInterface(data) {}

  void initDataInterface(unsigned int variant) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (referenceDataInterface->canUseVariant()) {
      referenceDataInterface->selectVariant(std::to_string(variant));
    }
    auto thread_id = std::this_thread::get_id();
    auto clone = referenceDataInterface->clone();
    if (dataInterfaces.count(thread_id) > 0) {
      // replace current element in case was used
      dataInterfaces.at(thread_id) = clone;
      return;
    }
    dataInterfaces.insert({thread_id, clone});
  }

  boost::shared_ptr<DYN::DataInterface> getDataInterface() const {
    auto thread_id = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex_);
    auto found = dataInterfaces.find(thread_id);
    return (found == dataInterfaces.end()) ? nullptr : found->second;
  }

 private:
  std::unordered_map<std::thread::id, boost::shared_ptr<DYN::DataInterface> > dataInterfaces;
  const boost::shared_ptr<DYN::DataInterface> referenceDataInterface;
  mutable std::mutex mutex_;
};
#else

class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const boost::shared_ptr<DYN::DataInterface>& data) : referenceDataInterface(data) {}

  void initDataInterface(unsigned int variant) {
    if (referenceDataInterface->canUseVariant()) {
      std::stringstream ss;
      ss << variant;
      referenceDataInterface->selectVariant(ss.str());
    }
    dataInterface = referenceDataInterface->clone();
  }

  boost::shared_ptr<DYN::DataInterface> getDataInterface() const {
    return dataInterface;
  }

 private:
  boost::shared_ptr<DYN::DataInterface> dataInterface;
  boost::shared_ptr<DYN::DataInterface> referenceDataInterface;
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
