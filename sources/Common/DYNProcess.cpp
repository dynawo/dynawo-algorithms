//
// Copyright (c) 2024, RTE (http://www.rte-france.com)
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
 * @file  Process.cpp
 *
 * @brief Process data : implementation file
 *
 */

#include "DYNProcess.h"

#ifdef _MPI_
#include "DYNMPIContext.h"
#endif

namespace DYNAlgorithms {

bool isSingleProcess() {
#ifdef _MPI_
  return mpi::context().nbProcs() == 1;
#else
  return true;
#endif
}

bool isRootProcess() {
#ifdef _MPI_
  return mpi::context().isRootProc();
#else
  return true;
#endif
}

}  // namespace DYNAlgorithms
