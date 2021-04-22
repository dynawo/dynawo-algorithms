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

/**
 * @file DYNMultipleJobsFactory.h
 * @brief Multiple jobs factory : header file
 *
 */

#ifndef API_MULTIPLEJOBS_DYNMULTIPLEJOBSFACTORY_H_
#define API_MULTIPLEJOBS_DYNMULTIPLEJOBSFACTORY_H_

#include <boost/shared_ptr.hpp>

#include "DYNMultipleJobs.h"

namespace DYNAlgorithms {
class MarginCalculation;
}

namespace multipleJobs {

/**
 * @class MultipleJobsFactory
 * @brief MultipleJobs factory class
 *
 * MultipleJobsFactory encapsulate methods for creating new
 * @p MultipleJobs objects.
 */
class MultipleJobsFactory {
 public:
  /**
   * @brief Create new MultipleJobs instance
   *
   * @returns Shared pointer to a new empty @p MultipleJobs
   */
  static boost::shared_ptr<MultipleJobs> newInstance();

  /**
   * @brief Create new MultipleJobs instance as a clone of given instance
   *
   * @param[in] original MultipleJobs to be cloned
   *
   * @returns Shared pointer to a new @p MultipleJobs copied from original
   */
  static boost::shared_ptr<MultipleJobs> copyInstance(boost::shared_ptr<MultipleJobs> original);

  /**
   * @brief Create new MultipleJobs instance as a clone of given instance
   *
   * @param[in] original MultipleJobs to be cloned
   *
   * @returns Shared pointer to a new @p MultipleJobs copied from original
   */
  static boost::shared_ptr<MultipleJobs> copyInstance(const MultipleJobs& original);
};

}  // namespace multipleJobs

#endif  // API_MULTIPLEJOBS_DYNMULTIPLEJOBSFACTORY_H_

