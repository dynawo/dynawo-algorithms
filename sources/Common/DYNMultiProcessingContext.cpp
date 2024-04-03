//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
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

#include "DYNMultiProcessingContext.h"

#include <iostream>
#include <numeric>

namespace DYNAlgorithms {
namespace multiprocessing {

Context* Context::instance_ = nullptr;
bool Context::finalized_ = false;

Context&
Context::instance() {
  if (!instance_) {
    std::cerr << "No multiprocessing context has been instantiated" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (finalized_) {
    std::cerr << "Multiprocessing context has already been finalized" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  return *instance_;
}

Context::Context() {
  if (instance_) {
    std::cerr << "Multiprocessing context should only be instantiated once per process in the main thread" << std::endl;
    std::exit(EXIT_FAILURE);
  }

#ifdef _MPI_
  int ret = MPI_Init(NULL, NULL);
  if (ret != MPI_SUCCESS) {
    std::cerr << "MPI initialization error" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  ret = MPI_Comm_size(MPI_COMM_WORLD, &nbProcs_);
  if (ret != MPI_SUCCESS) {
    std::cerr << "Error acquiring MPI process count" << std::endl;
    MPI_Finalize();
    std::exit(EXIT_FAILURE);
  }
  ret = MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
  if (ret != MPI_SUCCESS) {
    std::cerr << "Error while retrieving rank of current MPI process" << std::endl;
    MPI_Finalize();
    std::exit(EXIT_FAILURE);
  }
#endif

  instance_ = this;
}

Context::~Context() {
  finalized_ = true;
#ifdef _MPI_
  MPI_Finalize();
#endif
}

void
forEach(unsigned int iStart, unsigned int size, const std::function<void(unsigned int)>& func) {
  for (unsigned int i = iStart; i < size; i++) {
#ifdef _MPI_
    if (i % multiprocessing::context().nbProcs() == multiprocessing::context().rank())
#endif
      func(i);
  }
}

#ifdef _MPI_
template<>
void
Context::gatherImpl(Tag<bool>, const bool& data, std::vector<bool>& recvData) const {
  std::vector<unsigned int> recvDataInt;
  gather(static_cast<unsigned int>(data), recvDataInt);
  if (isRootProc()) {
    recvData.assign(recvDataInt.begin(), recvDataInt.end());
  }
}

template<>
void
Context::gatherImpl(Tag<std::vector<bool> >, const std::vector<bool>& data, std::vector<std::vector<bool> >& recvData) const {
  std::vector<std::vector<unsigned int> > recvDataInt;
  std::vector<unsigned int> dataInt(data.begin(), data.end());
  gather(dataInt, recvDataInt);
  if (isRootProc()) {
    recvData.resize(recvDataInt.size());
    for (unsigned int i = 0; i < recvDataInt.size(); i++) {
      recvData.at(i).assign(recvDataInt.at(i).begin(), recvDataInt.at(i).end());
    }
  }
}

template<>
void
Context::gatherImpl(Tag<std::string>, const std::string& data, std::vector<std::string>& recvData) const {
  std::vector<unsigned char> dataStr(data.begin(), data.end());
  std::vector<std::vector<unsigned char> > ret;
  gather(dataStr, ret);
  if (isRootProc()) {
    recvData.resize(nbProcs_);
    for (unsigned int i = 0; i < ret.size(); i++) {
      recvData.at(i) = std::string(ret.at(i).begin(), ret.at(i).end());
    }
  }
}

template<>
void
Context::broadcastImpl(Tag<std::string>, std::string& data) const {
  std::vector<unsigned char> dataVect;
  if (isRootProc()) {
    dataVect.assign(data.begin(), data.end());
  }
  broadcast(dataVect);
  if (!isRootProc()) {
    data.assign(dataVect.begin(), dataVect.end());
  }
}

template<>
void
Context::broadcastImpl(Tag<bool>, bool& data) const {
  unsigned int dataInt = static_cast<unsigned int>(data);
  broadcast(dataInt);
  data = static_cast<bool>(dataInt);
}

template<>
void
Context::broadcastImpl(Tag<std::vector<bool> >, std::vector<bool>& data) const {
  std::vector<unsigned int> dataInt;
  if (isRootProc()) {
    dataInt.assign(data.begin(), data.end());
  }
  broadcast(dataInt);
  if (!isRootProc()) {
    std::transform(dataInt.begin(), dataInt.end(), std::back_inserter(data), [](unsigned int value) { return static_cast<bool>(value); });
  }
}
#endif

}  // namespace multiprocessing

}  // namespace DYNAlgorithms
