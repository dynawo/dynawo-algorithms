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
 * @file  DYNMultiVariantInputs.h
 *
 * @brief Manager for multi variant inputs
 *
 */

#ifndef LAUNCHER_DYNMULTIVARIANTINPUTS_H_
#define LAUNCHER_DYNMULTIVARIANTINPUTS_H_

#include <JOBJobEntry.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

namespace DYNAlgorithms {

/**
 * @brief Multi variant inputs
 *
 * This class aims to manage the data that are factorized in case of multivariant use in data interface in order to use
 * during multithreading simulations
 */
class MultiVariantInputs {
 public:
  /**
   * @brief Read inputs files to initialize the inputs
   *
   * @param workingDirectory working directory of current run
   * @param jobFile the job file to use
   * @param iidmFile the iidm file to use instead of the reference in the job
   */
  void readInputs(const std::string& workingDirectory, const std::string& jobFile, const std::string& iidmFile = "");

  /**
   * @brief Retrieve a copy of the job entry
   * @returns job entry copy or null pointer if empty
   */
  boost::shared_ptr<job::JobEntry> cloneJobEntry() const;

  /**
   * @brief Retrieve the IIDM path to use
   *
   * @return IIDM path to use
   */
  inline const boost::filesystem::path& iidmPath() const {
    return iidmPath_;
  }

  /**
   * @brief set maximum time duration of the variation during the load increase part
   * @param t time duration of the variation during the load increase part
   */
  inline void setTLoadIncreaseVariationMax(double t) {
    tLoadIncreaseVariationMax_ = t;
  }

  /**
   * @brief get maximum time duration of the variation during the load increase part
   * @return the maximum time duration of the variation during the load increase part
   */
  inline double getTLoadIncreaseVariationMax() const {
    return tLoadIncreaseVariationMax_;
  }

 private:
  boost::shared_ptr<job::JobEntry> jobEntry_;  ///< job entry to use
  boost::filesystem::path iidmPath_;           ///< IIDM path to use
  double tLoadIncreaseVariationMax_;                                  ///< maximum time duration of the variation during the load increase part
};
}  // namespace DYNAlgorithms

#endif  // LAUNCHER_DYNMULTIVARIANTINPUTS_H_
