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

#ifndef COMMON_DYNMPICONTEXT_HPP_
#define COMMON_DYNMPICONTEXT_HPP_

#include <algorithm>
#include <numeric>
#include <type_traits>

namespace DYNAlgorithms {
namespace mpi {

/**
 * @brief Traits to specialize the MPI data types according to the input c++ type
 *
 * If one wants to send an unusal datatype throught MPI, one must implement these traits
 *
 */
namespace traits {

/**
 * @brief Default trait for data
 *
 * by default, data is considered as a byte. By default, the data will be send as a sequence of bytes (unsigned char)
 *
 * @tparam T c++ data type
 */
template<class T>
struct MPIType {
  static constexpr MPI_Datatype type = MPI_UNSIGNED_CHAR;  ///< Corresponding base MPI data type
  static constexpr size_t ratio = 1;                       ///< size ratio of the data to the MPI data type
};

/// @brief Specialization for char
template<>
struct MPIType<char> {
  static constexpr MPI_Datatype type = MPI_CHAR;  ///< Corresponding base MPI data type
  static constexpr size_t ratio = 1;              ///< size ratio of the data to the MPI data type
};

/// @brief Specialization for int
template<>
struct MPIType<int> {
  static constexpr MPI_Datatype type = MPI_INT;  ///< Corresponding base MPI data type
  static constexpr size_t ratio = sizeof(int);   ///< size ratio of the data to the MPI data type
};

/// @brief Specialization for unsigned int
template<>
struct MPIType<unsigned int> {
  static constexpr MPI_Datatype type = MPI_UNSIGNED;     ///< Corresponding base MPI data type
  static constexpr size_t ratio = sizeof(unsigned int);  ///< size ratio of the data to the MPI data type
};

/// @brief Specialization for double
template<>
struct MPIType<double> {
  static constexpr MPI_Datatype type = MPI_DOUBLE;  ///< Corresponding base MPI data type
  static constexpr size_t ratio = sizeof(double);   ///< size ratio of the data to the MPI data type
};

/// @brief Specialization for float
template<>
struct MPIType<float> {
  static constexpr MPI_Datatype type = MPI_FLOAT;  ///< Corresponding base MPI data type
  static constexpr size_t ratio = sizeof(float);   ///< size ratio of the data to the MPI data type
};

}  // namespace traits

template<class T>
void
Context::gatherImpl(Tag<T>, const T& data, std::vector<T>& recvData) const {
  if (isRootProc()) {
    recvData.resize(nbProcs_);
  }
  MPI_Gather(&data, sizeof(T) / traits::MPIType<T>::ratio, traits::MPIType<T>::type, recvData.data(), sizeof(T), MPI_UNSIGNED_CHAR, rootRank_, MPI_COMM_WORLD);
}

template<class T>
void
Context::gatherImpl(Tag<std::vector<T> >, const std::vector<T>& data, std::vector<std::vector<T> >& recvData) const {
  std::vector<int> sizes;
  std::vector<T> total;
  std::vector<int> displacements;
  if (isRootProc()) {
    sizes.resize(nbProcs_);
  }
  int size = static_cast<int>(data.size());
  MPI_Gather(&size, 1, MPI_INT, sizes.data(), 1, MPI_INT, rootRank_, MPI_COMM_WORLD);

  // Update size
  if (isRootProc()) {
    recvData.resize(nbProcs_);
    displacements.resize(nbProcs_);
    displacements[0] = 0;
    for (int i = 0; i < nbProcs_; i++) {
      recvData.at(i).resize(sizes.at(i));
      if (i != rootRank_) {
        displacements.at(i) = displacements.at(i - 1) + sizes.at(i - 1);
      }
    }
    auto nbElem = std::accumulate(recvData.begin(), recvData.end(), size_t{0},
                                  [](size_t sum, const std::vector<T>& dataLambda) { return sum + dataLambda.size(); });
    total.resize(nbElem);
  }

  // Must take into account the case empty vector
  if (!data.empty()) {
    MPI_Gatherv(data.data(), size * sizeof(T) / traits::MPIType<T>::ratio, traits::MPIType<T>::type, total.data(), sizes.data(), displacements.data(),
                traits::MPIType<T>::type, rootRank_, MPI_COMM_WORLD);
  }

  if (isRootProc()) {
    auto it = total.begin();
    for (int i = 0; i < nbProcs_; i++) {
      recvData.at(i) = std::vector<T>(it, it + sizes[i]);
      it = it + sizes[i];
    }
  }
}

template<class T>
void
Context::broadcastImpl(Tag<T>, T& data) const {
  MPI_Bcast(&data, sizeof(T) / traits::MPIType<T>::ratio, traits::MPIType<T>::type, rootRank_, MPI_COMM_WORLD);
}

template<class T>
void
Context::broadcastImpl(Tag<std::vector<T> >, std::vector<T>& data) const {
  unsigned int size = static_cast<unsigned int>(data.size());
  broadcast(size);
  if (size == 0) {
    // nothing to broadcast
    return;
  }

  if (!isRootProc()) {
    data.resize(size);
  }
  MPI_Bcast(data.data(), size * sizeof(T) / traits::MPIType<T>::ratio, traits::MPIType<T>::type, rootRank_, MPI_COMM_WORLD);
}

}  // namespace mpi
}  // namespace DYNAlgorithms

#endif  // COMMON_DYNMPICONTEXT_HPP_
