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

#ifndef COMMON_DYNMULTIPROCESSINGCONTEXT_H_
#define COMMON_DYNMULTIPROCESSINGCONTEXT_H_

#include <functional>
#include <string>
#include <vector>

#ifdef _MPI_
#include <mpi.h>
#endif

namespace DYNAlgorithms {

/// @brief namespace for multiprocessing wrapping
namespace multiprocessing {

/**
 * @brief Multiprocessing Context
 *
 * Singleton multiprocessing context to use multiprocessing functions
 *
 */
class Context {
 public:
  /**
   * @brief Function to retrieve the singleton
   *
   * @return the single instance of context
   */
  static Context& instance();

  /**
   * @brief Constructor
   *
   * Should only be used once per process in the main thread.
   * RAII ensures proper initialization/finalization.
   */
  Context();

  /// @brief Destructor
  ~Context();

  /// @brief no copy constructor
  Context(const Context&) = delete;
  /// @brief no assignment
  Context& operator=(const Context&) = delete;
  /// @brief no copy constructor
  Context(Context&&) = delete;
  /// @brief no assignment
  Context& operator=(Context&&) = delete;

  /**
   * @brief Retrieve the number of processes
   *
   * @return number of processes
   */
  unsigned int nbProcs() const {
#ifdef _MPI_
    return static_cast<unsigned int>(nbProcs_);
#else
    return 1;
#endif
  }

  /**
   * @brief Determines if the current process is the root process
   *
   * @return true if it is the root process, false if not
   */
  bool isRootProc() const {
#ifdef _MPI_
    return rank_ == rootRank_;
#else
    return true;
#endif
  }

  /// @brief Synchronize all process
  static void sync() {
#ifdef _MPI_
    MPI_Barrier(MPI_COMM_WORLD);
#endif
  }

 private:
  static Context* instance_;  ///< Unique instance
  static bool finalized_;  ///< Instance is already finalized

 public:
#ifdef _MPI_
  /**
   * @brief Gather all data into root rank
   *
   * @tparam T The data type to gather
   * @param data the data to send to root rank
   * @param recvData the vector of gathered data (relevant only for root process)
   */
  template<class T>
  void gather(const T& data, std::vector<T>& recvData) const {
    gatherImpl(Tag<T>(), data, recvData);
  }
#endif

  /**
   * @brief Broadcast data from root rank to all process
   *
   * @tparam T the data type to broadcast
   * @param data the data to broadcast
   */
  template<class T>
  void broadcast(T& data) const {
#ifdef _MPI_
    broadcastImpl(Tag<T>(), data);
#endif
  }

#ifdef _MPI_
  /**
   * @brief Retrieve the rank of the current process
   *
   * @return unsigned int
   */
  unsigned int rank() const {
    return static_cast<unsigned int>(rank_);
  }

 private:
  /// @brief Private structure to allow specilization of mpi *_impl with std::vector<> as data input
  template<class T>
  struct Tag {};

 private:
  /**
   * @brief Gather implementation
   *
   * @tparam T data type
   * @param tag unused
   * @param data data to gather
   * @param recvData the vector of gathered data (relevant only for root process)
   */
  template<class T>
  void gatherImpl(Tag<T> tag, const T& data, std::vector<T>& recvData) const;

  /**
   * @brief Gather implementation for vector of data
   *
   * @tparam T data type
   * @param tag unused
   * @param data vector of data to gather
   * @param recvData the vector of gathered vectors of data (relevant only for root process)
   */
  template<class T>
  void gatherImpl(Tag<std::vector<T> > tag, const std::vector<T>& data, std::vector<std::vector<T> >& recvData) const;

  /**
   * @brief Broadcast implementation
   *
   * @tparam T data type to broadcast
   * @param tag unused
   * @param data data to broadcast
   */
  template<class T>
  void broadcastImpl(Tag<T> tag, T& data) const;

  /**
   * @brief Broadcast implementation for vector of data
   *
   * @tparam T data type to broadcast
   * @param tag unused
   * @param data data to broadcast
   */
  template<class T>
  void broadcastImpl(Tag<std::vector<T> > tag, std::vector<T>& data) const;

 private:
  static constexpr int rootRank_ = 0;  ///< Root rank

 private:
  int nbProcs_;  ///< number of process
  int rank_;     ///< Rank of the current process
#endif
};

/**
 * @brief Retrieve the context instance
 *
 * @return Multiprocessing context single instance
 */
inline Context&
context() {
  return Context::instance();
}

/**
 * @brief Perform a operation by distributing into process
 *
 * For index range i in [ @a iStart, @a size [, a process will execute the function @a func if "i mod nbProcs == rank"
 *
 * @param iStart index range start index
 * @param size index range size of range
 * @param func functor to call for each index of the range according to the process
 */
void forEach(unsigned int iStart, unsigned int size, const std::function<void(unsigned int)>& func);

#ifdef _MPI_
/**
 * @brief Specialization for string (implemented as vector of unsigned char)
 *
 * @param tag unused
 * @param data vector of data to gather
 * @param recvData the vector of gathered vectors of data (relevant only for root process)
 */
template<>
void Context::gatherImpl(Tag<std::string> tag, const std::string& data, std::vector<std::string>& recvData) const;

/**
 * @brief Specialization for string (implemented as vector of unsigned char)
 *
 * @param tag unused
 * @param data data to broadcast
 */
template<>
void Context::broadcastImpl(Tag<std::string> tag, std::string& data) const;

/**
 * @brief Specialization for bool (implemented as unsigned int)
 *
 * @param tag unused
 * @param data vector of data to gather
 * @param recvData the vector of gathered vectors of data (relevant only for root process)
 */
template<>
void Context::gatherImpl(Tag<bool> tag, const bool& data, std::vector<bool>& recvData) const;
/**
 * @brief Specialization for vector<bool> (implemented as vector of unsigned int)
 *
 * @param tag unused
 * @param data vector of data to gather
 * @param recvData the vector of gathered vectors of data (relevant only for root process)
 */
template<>
void Context::gatherImpl(Tag<std::vector<bool> > tag, const std::vector<bool>& data, std::vector<std::vector<bool> >& recvData) const;
/**
 * @brief Specialization for bool (implemented as unsigned int)
 *
 * @param tag unused
 * @param data data to broadcast
 */
template<>
void Context::broadcastImpl(Tag<bool> tag, bool& data) const;
/**
 * @brief Specialization for vector<bool> (implemented as vector of unsigned int)
 *
 * @param tag unused
 * @param data data to broadcast
 */
template<>
void Context::broadcastImpl(Tag<std::vector<bool> > tag, std::vector<bool>& data) const;
#endif

}  // namespace multiprocessing

}  // namespace DYNAlgorithms

#ifdef _MPI_
#include "DYNMultiProcessingContext.hpp"
#endif

#endif  // COMMON_DYNMULTIPROCESSINGCONTEXT_H_
