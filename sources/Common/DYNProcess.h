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
 * @file  DYNProcess.h
 *
 * @brief Process data : header file
 *
 */

#ifndef COMMON_DYNPROCESS_H_
#define COMMON_DYNPROCESS_H_

namespace DYNAlgorithms {

/**
 * @brief Determines if the simulation is run with a single process
 * @return true if the simulation is run with a single process, false if not
 */
bool isSingleProcess();

/**
 * @brief Determines if the current process is the root process
 * @return true if it is the root process, false if not
 */
bool isRootProcess();

}  // namespace DYNAlgorithms

#endif  // COMMON_DYNPROCESS_H_
