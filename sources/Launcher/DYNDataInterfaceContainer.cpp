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

#include <DYNDataInterfaceFactory.h>
#include <boost/make_shared.hpp>
#include <string>

#ifdef USE_POWSYBL
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#endif

namespace DYNAlgorithms {

#ifdef USE_POWSYBL
class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const std::string& iidmFile, unsigned int nbVariants) :
      referenceDataInterface_(DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, iidmFile, nbVariants)) {}

  /**
   * @brief Initialize data interface for current thread
   *
   * Update the variant number in network and clone the data interface in order to be used afterwards when retrieved
   * during the main simulation. Update the map by thread id
   *
   * @param variant the variant number to select
   */
  void initDataInterface(unsigned int variant) {
    setReferenceVariant(variant);

    auto thread_id = std::this_thread::get_id();
    auto clone = referenceDataInterface_->clone();

    if (clone->canUseVariant()) {
      clone->selectVariant(std::to_string(variant));
    }

    std::unique_lock<std::mutex> lock(mutex_);
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
  void setReferenceVariant(unsigned int variant) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (referenceDataInterface_->canUseVariant()) {
      // Must be done before cloning as clone function will perform "get" actions on the network that
      // require the variant number
      // variant number selected is not relevant here
      referenceDataInterface_->selectVariant(std::to_string(variant));
    }
  }

 private:
  std::unordered_map<std::thread::id, boost::shared_ptr<DYN::DataInterface> > dataInterfaces_;
  const boost::shared_ptr<DYN::DataInterface> referenceDataInterface_;
  mutable std::mutex mutex_;
};
#else

class DataInterfaceContainer::DataInterfaceContainerImpl {
 public:
  explicit DataInterfaceContainerImpl(const std::string& iidmFile, unsigned int) : iidmFile_(iidmFile) {}

  void initDataInterface(unsigned int) {
    // do nothing
  }

  boost::shared_ptr<DYN::DataInterface> getDataInterface() const {
    // re-read the IIDM file every time
    return DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, iidmFile_);
  }

 private:
  std::string iidmFile_;
};
#endif

DataInterfaceContainer::DataInterfaceContainer(const std::string& iidmFile, unsigned int nbVariants) :
    impl_(boost::make_shared<DataInterfaceContainerImpl>(iidmFile, nbVariants)) {}

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
