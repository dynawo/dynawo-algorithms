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
 * @file MultipleJobsFactory.cpp
 * @brief Multiple jobs factory : implementation file
 *
 */

#include "DYNMultipleJobsFactory.h"
#include "DYNMultipleJobs.h"

namespace multipleJobs {

boost::shared_ptr<MultipleJobs>
MultipleJobsFactory::newInstance() {
  return boost::shared_ptr<MultipleJobs>(new MultipleJobs());
}

boost::shared_ptr<MultipleJobs>
MultipleJobsFactory::copyInstance(boost::shared_ptr<MultipleJobs> original) {
  return boost::shared_ptr<MultipleJobs>(new MultipleJobs(dynamic_cast<MultipleJobs&> (*original)));
}

boost::shared_ptr<MultipleJobs>
MultipleJobsFactory::copyInstance(const MultipleJobs& original) {
  return boost::shared_ptr<MultipleJobs>(new MultipleJobs(dynamic_cast<const MultipleJobs&> (original)));
}

}  // namespace multipleJobs
