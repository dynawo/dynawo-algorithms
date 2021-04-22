//
// Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
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

#include <string>

#ifndef COMMON_DYNRESULTCOMMON_H_
#define COMMON_DYNRESULTCOMMON_H_

namespace DYNAlgorithms {
/**
 * @brief task status
 */
typedef enum {
  CONVERGENCE_STATUS,  ///< simulation reached its end with no error nor criterion not respected
  DIVERGENCE_STATUS,  ///< divergence occurs during the simulation (solver error)
  EXECUTION_PROBLEM_STATUS,  ///< something wrong happened during the simulation (solver error, data error, etc...)
  CRITERIA_NON_RESPECTED_STATUS  ///< one criterion was not respected (0.8 Un, etc...)
}status_t;

static inline std::string getStatusAsString(status_t status) {
  switch (status) {
    case CONVERGENCE_STATUS:
      return "CONVERGENCE";
    case EXECUTION_PROBLEM_STATUS:
      return "EXECUTION_PROBLEM";
    case CRITERIA_NON_RESPECTED_STATUS:
      return "CRITERIA_NON_RESPECTED";
    case DIVERGENCE_STATUS:
      return "DIVERGENCE";
  }
  return "";  // to avoid compiler warning, should not appear
}

}  // namespace DYNAlgorithms
#endif  // COMMON_DYNRESULTCOMMON_H_


