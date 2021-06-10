//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
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
 * @file  DYNAnalysisContext.h
 *
 * @brief Analysis context for data interface
 *
 */

#ifndef LAUNCHER_DYNANALYSISCONTEXT_H_
#define LAUNCHER_DYNANALYSISCONTEXT_H_

#include "DYNDataInterfaceContainer.h"

#include <JOBJobEntry.h>
#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {

/**
 * @brief Analysis context for data interface re-use
 */
class AnalysisContext {
 public:
  /**
   * @brief Update analysis context
   *
   * @param workingDirectory working directory of current run
   * @param jobFile the job file to use
   * @param nbVariants the number of variants to use
   * @param iidmFile the iidm file to use instead of the reference in the job
   */
  void update(const std::string& workingDirectory, const std::string& jobFile, unsigned int nbVariants, const std::string& iidmFile = "");

  /**
   * @brief Update variant for current run
   *
   * The variant name will be the string representation of the integer
   *
   * @param variant the variant number
   */
  void updateCurrentRun(unsigned int variant);

  /**
   * @brief Retrieve the job entry
   * @returns the job entry for the context
   */
  inline const boost::shared_ptr<job::JobEntry>& jobEntry() const {
    return jobEntry_;
  }

  /**
   * @brief Retrieve the data interface container
   * @returns the data interface container for the context
   */
  inline const boost::shared_ptr<DataInterfaceContainer>& dataInterfaceContainer() const {
    return dataInterfaceContainer_;
  }

 private:
  boost::shared_ptr<job::JobEntry> jobEntry_;                         ///< job entry to use
  boost::shared_ptr<DataInterfaceContainer> dataInterfaceContainer_;  ///< data interface container to use
};
}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNANALYSISCONTEXT_H_
