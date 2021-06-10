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
 * @file  DYNAnalysisContext.cpp
 *
 * @brief Analysis context for data interface
 *
 */

#include "DYNAnalysisContext.h"

#include <DYNDataInterfaceFactory.h>
#include <JOBJobsCollection.h>
#include <JOBXmlImporter.h>
#include <boost/make_shared.hpp>

namespace DYNAlgorithms {

void
AnalysisContext::init(const std::string& workingDirectory, const std::string& jobFile, unsigned int nbVariants, const std::string& iidmFile) {
  nbVariants_ = nbVariants;
  // job
  job::XmlImporter importer;
  boost::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(workingDirectory + "/" + jobFile);
  //  implicit : only one job per file
  jobEntry_ = *jobsCollection->begin();

  std::string iidmFilePath;
  if (!iidmFile.empty()) {
    if (jobEntry_->getModelerEntry()->getNetworkEntry()) {
      jobEntry_->getModelerEntry()->getNetworkEntry()->setIidmFile(iidmFile);
    }
    iidmFilePath = createAbsolutePath(iidmFile, workingDirectory);
  }

  if (!iidmFilePath.empty()) {
    // data interface
    // Create data interface and give it to simulation constructor
    boost::shared_ptr<DYN::DataInterface> dataInterface =
        DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, iidmFilePath, nbVariants_);
    dataInterfaceContainer_ = boost::make_shared<DataInterfaceContainer>(dataInterface);
  }
}

void
AnalysisContext::setCurrentVariant(unsigned int variant) {
  if (!dataInterfaceContainer_) {
    return;
  }

  dataInterfaceContainer_->initDataInterface();
  boost::shared_ptr<DYN::DataInterface> dataInterface = dataInterfaceContainer_->getDataInterface();

  if (dataInterface->canUseVariant()) {
#ifdef LANG_CXX11
    std::string name = std::to_string(variant);
#else
    std::stringstream ss;
    ss << variant;
    std::string name = ss.str();
#endif
    dataInterface->selectVariant(name);
  }
}

}  // namespace DYNAlgorithms
