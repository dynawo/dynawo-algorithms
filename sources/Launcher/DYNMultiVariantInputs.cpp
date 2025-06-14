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
 * @file  DYNMultiVariantInputs.cpp
 *
 * @brief Implementation for multi variant inputs management
 *
 */

#include "DYNMultiVariantInputs.h"

#include "MacrosMessage.h"

#include <DYNDataInterfaceFactory.h>
#include <JOBJobsCollection.h>
#include <JOBXmlImporter.h>


namespace DYNAlgorithms {

void
MultiVariantInputs::readInputs(const std::string& workingDirectory, const std::string& jobFile, const std::string& iidmFile) {
  // job
  job::XmlImporter importer;
  std::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(createAbsolutePath(jobFile, workingDirectory));
  //  implicit : only one job per file
  jobEntry_ = jobsCollection->getJobs()[0];

  // Compute the iidm file path according to the criteria:
  // - priority to the file given in parameter
  // - if not given, use the one in the network entry of the job
  std::string iidmFilePath;
  if (!iidmFile.empty()) {
    if (jobEntry_->getModelerEntry()->getNetworkEntry()) {
      jobEntry_->getModelerEntry()->getNetworkEntry()->setIidmFile(iidmFile);
    }
    iidmFilePath = createAbsolutePath(iidmFile, workingDirectory);
  } else {
    if (jobEntry_->getModelerEntry()->getNetworkEntry()) {
      iidmFilePath = jobEntry_->getModelerEntry()->getNetworkEntry()->getIidmFile();
      if (!iidmFilePath.empty()) {
        iidmFilePath = createAbsolutePath(iidmFilePath, workingDirectory);
      }
    }
  }
  iidmPath_ = iidmFilePath;
}

std::shared_ptr<job::JobEntry>
MultiVariantInputs::cloneJobEntry() const {
  return jobEntry_ ? std::make_shared<job::JobEntry>(*jobEntry_) : std::shared_ptr<job::JobEntry>();
}

}  // namespace DYNAlgorithms
