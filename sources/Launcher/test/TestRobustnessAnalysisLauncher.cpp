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

#include <DYNFileSystemUtils.h>
#include <JOBXmlImporter.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <JOBModelerEntry.h>
#include <JOBDynModelsEntry.h>

#include <gtest_dynawo.h>
#include <boost/make_shared.hpp>
#include <libzip/ZipFile.h>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipOutputStream.h>

#include "DYNRobustnessAnalysisLauncher.h"
#include "DYNResultCommon.h"
#include "MacrosMessage.h"
#include "DYNMultipleJobs.h"
#include "DYNMarginCalculation.h"
#include "DYNMultiProcessingContext.h"

testing::Environment* initXmlEnvironment();

namespace DYNAlgorithms {
testing::Environment* const env = initXmlEnvironment();

multiprocessing::Context procContext;

class MyLauncher : public RobustnessAnalysisLauncher {
 public:
  void launch() {
    inputs_.readInputs(workingDirectory_, "MyJobs.jobs");
    std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();
    addDydFileToJob(job, "MyDydFile.dyd");
    ASSERT_EQ(job->getModelerEntry()->getDynModelsEntries().size(), 2);
    ASSERT_EQ(job->getModelerEntry()->getDynModelsEntries()[0]->getDydFile(), "EmptyDydFile.dyd");
    ASSERT_EQ(job->getModelerEntry()->getDynModelsEntries()[1]->getDydFile(), "MyDydFile.dyd");
    setCriteriaFileForJob(job, "MyCrtFile.crt");
    ASSERT_EQ(job->getSimulationEntry()->getCriteriaFiles().size(), 1);
    ASSERT_EQ(job->getSimulationEntry()->getCriteriaFiles()[0], "MyCrtFile.crt");

    SimulationParameters params;
    result_.setScenarioId("MyScenario");
    boost::shared_ptr<DYN::Simulation> simu = createAndInitSimulation("res", job, params, result_, inputs_);
    ASSERT_TRUE(simu);
    status_t status = simulate(simu, result_);
    ASSERT_EQ(status, CONVERGENCE_STATUS);
    ASSERT_EQ(result_.getStatus(), CONVERGENCE_STATUS);
    ASSERT_TRUE(result_.getSuccess());

    writeResults();
    ASSERT_TRUE(exists("res/MyOutputFile.zip"));
    ASSERT_FALSE(exists("res/constraints/constraints_MyScenario.xml"));
    ASSERT_FALSE(exists("res/timeLine/timeline_MyScenario.log"));
    ASSERT_FALSE(exists("res/timeLine/timeline_MyScenario.xml"));
    ASSERT_FALSE(exists("res/finalState/outputIIDM_MyScenario.xml"));
    ASSERT_TRUE(exists("res/logs/log_MyScenario.log"));

    result_.getConstraintsStream() << "Test Constraints";
    result_.getTimelineStream() << "Test Timeline";
    result_.getOutputIIDMStream() << "Test final state";

    writeResults();
    ASSERT_TRUE(exists("res/MyOutputFile.zip"));
    ASSERT_TRUE(exists("res/constraints/constraints_MyScenario.xml"));
    ASSERT_TRUE(exists("res/timeLine/timeline_MyScenario.log"));
    ASSERT_FALSE(exists("res/timeLine/timeline_MyScenario.xml"));
    ASSERT_TRUE(exists("res/finalState/outputIIDM_MyScenario.xml"));
    ASSERT_TRUE(exists("res/logs/log_MyScenario.log"));
  }

  void testInputFile(const std::string& inputFile) {
    ASSERT_EQ(inputFile_, inputFile);
  }

  void testOutputFile(const std::string& outputFile) {
    ASSERT_EQ(outputFile_, outputFile);
  }

  void testDirectory(const std::string& directory) {
    ASSERT_EQ(directory_, directory);
  }

  void testWorkingDirectory(const std::string& workingDirectory) {
    ASSERT_EQ(workingDirectory_, workingDirectory);
  }

  void testOutputFileFullPath(const std::string& outputFileFullPath) {
    ASSERT_EQ(outputFileFullPath_, outputFileFullPath);
  }

  void testMultipleJobs() {
    assert(multipleJobs_);
    assert(!multipleJobs_->getScenarios());
    assert(multipleJobs_->getMarginCalculation());
    boost::shared_ptr<DYNAlgorithms::MarginCalculation> mc = multipleJobs_->getMarginCalculation();
    ASSERT_EQ(mc->getAccuracy(), 50);
    ASSERT_EQ(mc->getCalculationType(), DYNAlgorithms::MarginCalculation::LOCAL_MARGIN);
    assert(mc->getLoadIncrease());
    ASSERT_EQ(mc->getLoadIncrease()->getId(), "MyLoadIncrease");
    ASSERT_EQ(mc->getLoadIncrease()->getJobsFile(), "MyLoadIncrease.jobs");
    ASSERT_EQ(mc->getScenarios()->getScenarios().size(), 2);
    ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getId(), "MyScenario");
    ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getDydFile(), "MyScenario.dyd");
    ASSERT_EQ(mc->getScenarios()->getScenarios()[0]->getCriteriaFile(), "MyScenario.crt");
    ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getId(), "MyScenario2");
    ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getDydFile(), "MyScenario2.dyd");
    ASSERT_EQ(mc->getScenarios()->getScenarios()[1]->getCriteriaFile(), "MyScenario2.crt");
    ASSERT_EQ(mc->getScenarios()->getJobsFile(), "myScenarios.jobs");
  }

 protected:
  void createOutputs(std::map<std::string, std::string>& mapData, bool) const {
    storeOutputs(result_, mapData);
    writeOutputs(result_);
  }

 private:
  SimulationResult result_;
};


TEST(TestLauncher, TestRobustnessAnalysisLauncher) {
  MyLauncher launcher;

  launcher.testInputFile("");
  std::string absoluteFile = createAbsolutePath("MyAbsoluteFile", currentPath());
  launcher.setInputFile(absoluteFile);
  launcher.testInputFile(absoluteFile);

  launcher.testOutputFile("");
  launcher.setOutputFile("MyOutputFile");
  launcher.testOutputFile("MyOutputFile");

  launcher.testDirectory("");
  launcher.setDirectory("MyDirectory");
  launcher.testDirectory("MyDirectory");

  ASSERT_THROW_DYNAWO(launcher.init(), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::DirectoryDoesNotExist);
  launcher.testWorkingDirectory(createAbsolutePath("MyDirectory", currentPath()));

  std::string absoluteDir = createAbsolutePath("MyAbsoluteDirectory", currentPath());
  launcher.setDirectory(absoluteDir);
  ASSERT_THROW_DYNAWO(launcher.init(), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::DirectoryDoesNotExist);
  launcher.testWorkingDirectory(absoluteDir);

  launcher.setDirectory("");
  ASSERT_THROW_DYNAWO(launcher.init(), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::FileDoesNotExist);
  launcher.testWorkingDirectory(currentPath());

  launcher.setInputFile("res/MyDummyInputFile.xml");
  ASSERT_THROW_DYNAWO(launcher.init(), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::FileDoesNotExist);

  launcher.setInputFile("res/MyInputFile.txt");
  ASSERT_THROW_DYNAWO(launcher.init(), DYN::Error::GENERAL, DYNAlgorithms::KeyAlgorithmsError_t::InputFileFormatNotSupported);

  boost::shared_ptr<zip::ZipFile> archive = zip::ZipFileFactory::newInstance();
  boost::shared_ptr<zip::ZipEntry> entry(new zip::ZipEntry("res/fic_MULTIPLE.xml"));
  archive->addEntry("fic_MULTIPLE.xml", entry->getData());
  zip::ZipOutputStream::write("res/MyInputFile.zip", archive);
  launcher.setInputFile("MyInputFile.zip");
  launcher.setDirectory("res");
  launcher.setOutputFile("MyOutputFile.zip");
  ASSERT_NO_THROW(launcher.init());
  launcher.testOutputFileFullPath(createAbsolutePath("MyOutputFile.zip", createAbsolutePath("res", currentPath())));
  launcher.testMultipleJobs();
  launcher.launch();
}

}  // namespace DYNAlgorithms
