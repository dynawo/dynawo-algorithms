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
 * @file  MarginCalculationLauncher.cpp
 *
 * @brief Margin Calculation launcher: implementation of the algorithm and interaction with dynawo core
 *
 */

#include <iostream>
#include <cmath>
#include <ctime>
#include <iomanip>

#include <libzip/ZipFile.h>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipEntry.h>
#include <libzip/ZipInputStream.h>
#include <libzip/ZipOutputStream.h>

#include <DYNFileSystemUtils.h>
#include <DYNExecUtils.h>
#include <DYNSimulation.h>
#include <DYNSubModel.h>
#include <DYNTrace.h>
#include <DYNModel.h>
#include <DYNParameter.h>
#include <DYNModelMulti.h>
#include <JOBXmlImporter.h>
#include <JOBJobsCollection.h>
#include <JOBJobEntry.h>
#include <JOBSimulationEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBTimelineEntry.h>

#include "DYNMarginCalculationLauncher.h"
#include "DYNMultipleJobs.h"
#include "DYNMarginCalculation.h"
#include "DYNScenario.h"
#include "MacrosMessage.h"
#include "DYNScenarios.h"
#include "DYNAggrResXmlExporter.h"
#include "DYNMultiProcessingContext.h"

using DYN::Trace;

static const char LOAD_INCREASE[] = "loadIncrease";

enum MsgId {
  REQ_SCEN_ID,
  REQ_GLOBAL_MARGIN,
  REQ_LOAD_INC_LOWEST_FAIL,
  REQ_LOAD_INC_HIGHEST_SUCCESS,
  REQ_LOAD_INC_DONE,
  REQ_LOAD_INC_STATUS,
  FDB_LOAD_INC,
  FDB_SCEN_SINGLE,
  FDB_SCEN_MARGIN,
};

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
}

inline bool isSingleThread() {return multiprocessing::context().nbProcs() == 1;}
inline bool isServerThread() {return !isSingleThread() &&  multiprocessing::context().isRootProc();}
inline bool isWorkerThread() {return !isSingleThread() && !multiprocessing::context().isRootProc();}

inline const std::vector<boost::shared_ptr<Scenario> > & MarginCalculationLauncher::getScens()   const {return mc_->getScenarios()->getScenarios();}

inline int MarginCalculationLauncher::nbScens() const                 {return getScens().size();}
inline int MarginCalculationLauncher::nbVars() const                  {return discreteVars_.size();}
inline int MarginCalculationLauncher::varId100() const                {return discreteVars_.size()-1;}
inline bool MarginCalculationLauncher::searchingGlobalMargin() const  {return mc_->getCalculationType() == MarginCalculation::GLOBAL_MARGIN;}
inline bool MarginCalculationLauncher::liStarted(int varId) const     {return !startingTimes_[varId].is_not_a_date_time();}
inline bool MarginCalculationLauncher::liDone(int varId) const        {return results_[varId].getResult().getVariation() >= 0;}
inline bool MarginCalculationLauncher::liOK(int varId) const          {return results_[varId].getResult().getSuccess();}

void
MarginCalculationLauncher::launch() {
  boost::posix_time::ptime t0 = boost::posix_time::second_clock::local_time();

  initGlobals();

  // fringe case that might actually be used in prod, do a monothreaded dichotomy on load increases
  if (nbScens() == 0) {
    if (!isWorkerThread()) {
      globalMarginVarId_ = findMaxLoadIncrease();
      finish(t0);
    }
    return;
  }

  if (isServerThread())
    serverLoop();
  else
    workerLoop();

  if (!isWorkerThread())
    finish(t0);
}

void
MarginCalculationLauncher::initGlobals() {
  assert(multipleJobs_);
  mc_ = multipleJobs_->getMarginCalculation();
  if (!mc_) {
    throw DYNAlgorithmsError(MarginCalculationTaskNotFound);
  }
  if (!mc_->getScenarios()) {
    throw DYNAlgorithmsError(SystematicAnalysisTaskNotFound);
  }

  job::XmlImporter importer;

  std::shared_ptr<job::JobsCollection> jobsCollection = importer.importFromFile(createAbsolutePath(mc_->getLoadIncrease()->getJobsFile(), workingDirectory_));
  tLoadIncrease_ = jobsCollection->getJobs()[0]->getSimulationEntry()->getStopTime();  //  implicit : only one job per file

  jobsCollection = importer.importFromFile(createAbsolutePath(mc_->getScenarios()->getJobsFile(), workingDirectory_));
  const auto& job = jobsCollection->getJobs()[0];  //  implicit : only one job per file
  tScenario_ = job->getSimulationEntry()->getStopTime() - job->getSimulationEntry()->getStartTime();

  discreteVars_.clear();
  results_.clear();
  int step = 0;
  while (step < 100) {
    discreteVars_.push_back(step);
    results_.emplace_back(nbScens());
    step += mc_->getAccuracy();
  }
  discreteVars_.push_back(100);
  results_.emplace_back(nbScens());

  globalMarginVarId_ = varId100();
  marginScens_ = std::vector<int>((size_t)nbScens(), -1);

  startingTimes_.resize(nbVars(), boost::posix_time::ptime());
}

void
MarginCalculationLauncher::workerLoop() {
  int scenId = -1;
  while (getNextScenId(scenId)) {
    int varIdInf = 0;
    int varIdSup = getVarIdStart();

    if (launchScenarioWrapper(scenId, varIdSup)) {
      if (searchingGlobalMargin() || (varIdSup == varId100())) {
        // max possible varID success : scenario finished
        updateScenMargin(scenId, varIdSup);
        continue;
      } else {  // local margin computation and possible valid load increases above
        varIdInf = varIdSup;
        varIdSup == varId100();
      }
    }

    limitVarIdSup(varIdSup);

    while (varIdSup > varIdInf+1) {
      int varIdNext = getVarIdBetween(varIdInf, varIdSup);
      if (launchScenarioWrapper(scenId, varIdNext)) {
        varIdInf = varIdNext;
      } else {
        varIdSup = varIdNext;
        // heuristic optim : test zero only if 50% fails
        if ((varIdInf == 0) && (results_[0].getScenarioResult(scenId).getScenarioId() == "")) {
          if (!launchScenarioWrapper(scenId, 0)) {
            varIdInf = -1;
            break;
          }
        }  // if 0% succeeds, we are right back on track
      }
      limitVarIdSup(varIdSup);
    }
    updateScenMargin(scenId, varIdInf);
  }
}

void
MarginCalculationLauncher::serverLoop() {
  std::set<int> workersLeft;
  for (int i=1; i < multiprocessing::context().nbProcs(); ++i)
    workersLeft.insert(i);

  std::map<int, std::vector<int> > delayedAnswers;

  int scenId = 0;

#ifdef _MPI_
  while (workersLeft.size() > 0) {
    MPI_Status senderInfo;
    int msg[4];
    MPI_Recv(msg, 4, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &senderInfo);
    if (msg[0] == REQ_SCEN_ID) {
      MPI_Send(&scenId, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
      if (scenId >= nbScens())
        workersLeft.erase(senderInfo.MPI_SOURCE);
      else
        std::cout << "attributing scen " << scenId << " to thread " << senderInfo.MPI_SOURCE << std::endl;

      ++scenId;
    } else if (msg[0] == REQ_GLOBAL_MARGIN) {
      msg[0] = globalMarginVarId_;
      MPI_Send(msg, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
    } else if (msg[0] == REQ_LOAD_INC_LOWEST_FAIL) {
      msg[0] = getLowestLIFailureVarId();
      MPI_Send(msg, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
    } else if (msg[0] == REQ_LOAD_INC_HIGHEST_SUCCESS) {
      msg[0] = getHighestLISuccessVarId();
      MPI_Send(msg, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
    } else if (msg[0] == REQ_LOAD_INC_DONE) {
      msg[0] = getVarIdBetween(msg[1], msg[2]);
      MPI_Send(msg, 1, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
    } else if (msg[0] == REQ_LOAD_INC_STATUS) {
      int liVarIdToCheck = msg[1];
      bool liSuccess;
      int liVarIdToLaunch;
      bool resKnown = checkLoadIncreaseStatus(liVarIdToCheck, liSuccess, liVarIdToLaunch);
      msg[0] = resKnown;
      if (resKnown) {
        msg[1] = liSuccess;
        MPI_Send(msg, 2, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
      } else {
        if (liVarIdToLaunch >= 0) {
          msg[1] = liVarIdToLaunch;
          MPI_Send(msg, 2, MPI_INT, senderInfo.MPI_SOURCE, 0, MPI_COMM_WORLD);
          if (liVarIdToLaunch != liVarIdToCheck) {
            std::cout << "telling thread " << senderInfo.MPI_SOURCE << " to compute LI " << discreteVars_[liVarIdToLaunch]
                      << "% while waiting for "  << discreteVars_[liVarIdToCheck] << "%" << std::endl;
          } else {
            std::cout << "telling thread " << senderInfo.MPI_SOURCE << " to compute LI " << discreteVars_[liVarIdToLaunch] << "%" << std::endl;
          }
        } else {
          // if (delayedAnswers.find(liVarIdToCheck) == delayedAnswers.end())
          //   delayedAnswers[liVarIdToCheck] = std::vector<int>();
          delayedAnswers[liVarIdToCheck].push_back(senderInfo.MPI_SOURCE);
          std::cout << "putting thread " << senderInfo.MPI_SOURCE << " on hold for LI " << discreteVars_[liVarIdToCheck] << "% result" << std::endl;
        }
      }
    } else if (msg[0] == FDB_LOAD_INC) {
      int varId = msg[1];
      int liSuccess = msg[2];
      std::cout << "LI " << discreteVars_[varId] << "% by thread " << senderInfo.MPI_SOURCE
                  << " " << (liSuccess ? "success" : "failure") << std::endl;

      SimulationResult & result = results_[varId].getResult();
      result.setVariation(discreteVars_[varId]);
      result.setSuccess(liSuccess);
      if (delayedAnswers.find(varId) != delayedAnswers.end()) {
        msg[0] = 1;
        msg[1] = liSuccess;
        for (int threadId : delayedAnswers[varId]) {
          MPI_Send(msg, 2, MPI_INT, threadId, 0, MPI_COMM_WORLD);
          std::cout << "delayed answer for LI " << discreteVars_[varId] << "% to thread " << threadId << std::endl;
        }
        delayedAnswers.erase(delayedAnswers.find(varId));
      }
    } else if (msg[0] == FDB_SCEN_SINGLE) {
      int scenId = msg[1];
      int varId = msg[2];
      bool scenSuccess = msg[3] > 0;
      results_[varId].getScenarioResult(scenId).setScenarioId(getScens()[scenId]->getId());  // mark it as run for result collection
      std::cout << "Scen " << scenId << " at " << discreteVars_[varId] << "% by thread " << senderInfo.MPI_SOURCE
                    << " " << (scenSuccess ? "success" : "failure") << std::endl;

    } else if (msg[0] == FDB_SCEN_MARGIN) {
      updateScenMargin(msg[1], msg[2]);
    } else {
      // ToDo : log error
    }
  }
#endif  // _MPI_
}

int
MarginCalculationLauncher::findMaxLoadIncrease() {
  if (launchLoadIncreaseWrapper(varId100()))
    return varId100();

  int varIdInf = 0;  // lowest known that succeeds
  int varIdSup = varId100();  // highest known that fails;

  while (varIdSup > varIdInf+1) {
    int varIdNext = (varIdInf+varIdSup+1)/2;  // bias towards upper in integer roundings
    if (launchLoadIncreaseWrapper(varIdNext)) {
      varIdInf = varIdNext;
    } else {
      varIdSup = varIdNext;
      if ((varIdInf == 0) && !liDone(0)) {  // heuristic optim : try 50% before 0%
        if (!launchLoadIncreaseWrapper(0))
          return -1;
      }
    }
  }

  return varIdInf;
}

int
MarginCalculationLauncher::getVarIdStart() const {
  if (searchingGlobalMargin())
    return getGlobalMarginVarId();

  int highestLIOKVarId = getHighestLISuccessVarId();
  if (highestLIOKVarId >= 0)
    return highestLIOKVarId;

  int lowestLINOKVarId = getLowestLIFailureVarId();
  if (lowestLINOKVarId > 0)
    return lowestLINOKVarId-1;
  else
    return 0;
}

int
MarginCalculationLauncher::getVarIdBetween(int varIdMin, int varIdMax) const {
  if (isWorkerThread()) {
    int msg[3] = {REQ_LOAD_INC_DONE, varIdMin, varIdMax};
  #ifdef _MPI_
    MPI_Send(msg, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  #endif  // _MPI_
    return msg[0];
  }

  int varIdMid = (varIdMin+varIdMax+1)/2;
  int varIdClosest  = varIdMid;
  int distMin = 1000;

  for (int varId = varIdMin+1; varId <= varIdMax-1; ++varId) {
    if (liDone(varId)) {
      int dist = std::abs(varId-varIdMid);
      if (dist < distMin) {
        varIdClosest = varId;
        distMin = dist;
      }
    }
  }
  return varIdClosest;
}

int
MarginCalculationLauncher::getGlobalMarginVarId() const {
  if (isWorkerThread()) {
    int msg = REQ_GLOBAL_MARGIN;
  #ifdef _MPI_
    MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  #endif  // _MPI_
    return msg;
  }

  return globalMarginVarId_;
}

int
MarginCalculationLauncher::getLowestLIFailureVarId() const {
  if (isWorkerThread()) {
    int msg = REQ_LOAD_INC_LOWEST_FAIL;
  #ifdef _MPI_
    MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  #endif  // _MPI_
    return msg;
  }

  for (int varId = 0; varId < discreteVars_.size(); ++varId)
      if (liDone(varId) && !liOK(varId))
        return varId;

  return varId100()+1;
}

int
MarginCalculationLauncher::getHighestLISuccessVarId() const {
  if (isWorkerThread()) {
    int msg = REQ_LOAD_INC_HIGHEST_SUCCESS;
#ifdef _MPI_
    MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#endif  // _MPI_
    return msg;
  }

  for (int varId = discreteVars_.size()-1; varId >= 0; --varId)
      if (liDone(varId) && liOK(varId))
        return varId;

  return -1;
}

void
MarginCalculationLauncher::limitVarIdSup(int & varIdSup) const {
  varIdSup = std::min(varIdSup, getLowestLIFailureVarId());

  if (searchingGlobalMargin())
    varIdSup = std::min(varIdSup, getGlobalMarginVarId()+1);
}

int
MarginCalculationLauncher::getNextScenId(int & scenId) const {
  ++scenId;  // monothread : simply increment ID

  if (isWorkerThread()) {  // multithread : request ID to server (root process) instead, who centralizes incrementations
    int msg = REQ_SCEN_ID;
#ifdef _MPI_
    MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#endif  // _MPI_
    scenId = msg;
  }

  return scenId < nbScens();
}

int
MarginCalculationLauncher::getAnticipatedLoadIncreaseVarId() const {
  assert(isServerThread());

  if (!liStarted(nbVars()/2))
    return nbVars()/2;

  if (!liStarted(0))
    return 0;

  int varIdMem = varId100();
  limitVarIdSup(varIdMem);

  int widestGap = 0;
  int varIdWidest = varIdMem;

  for (int varId = varIdMem - 1; varId >= 0; --varId) {
    if (liStarted(varId)) {
      int gap = varIdMem - varId;
      varIdMem = varId;
      if (gap > widestGap) {
        varIdWidest = varId;
        widestGap = gap;
      }
    }
  }

  return (widestGap > 1) ? varIdWidest + widestGap/2 : -1;
}

bool
MarginCalculationLauncher::checkLoadIncreaseStatus(int varId, bool & liSuccess, int & liVarIdToLaunch) {
  if (liDone(varId)) {
    liSuccess = liOK(varId);
    return true;
  }

  if (isSingleThread()) {
    liVarIdToLaunch = varId;
    return false;
  }

  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[2] = {REQ_LOAD_INC_STATUS, varId};
    MPI_Send(msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
    MPI_Recv(msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    liSuccess = msg[1] > 0;
    liVarIdToLaunch = msg[1];
    return msg[0] > 0;
#endif  // _MPI_
  }

  assert(isServerThread());

  if (!liStarted(varId))
    liVarIdToLaunch = varId;
  else if ((boost::posix_time::second_clock::local_time() - startingTimes_[varId]).total_milliseconds() < 1000)
    liVarIdToLaunch = getAnticipatedLoadIncreaseVarId();
  else
    liVarIdToLaunch = -1;

  if (liVarIdToLaunch >= 0)
    startingTimes_[liVarIdToLaunch] = boost::posix_time::second_clock::local_time();
  return false;
}

void
MarginCalculationLauncher::updateScenMargin(int scenId, int scenMarginVarId) {
  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[3] = {FDB_SCEN_MARGIN, scenId, scenMarginVarId};
    MPI_Send(msg, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
#endif  // _MPI_
    return;
  }

  globalMarginVarId_ = std::min(globalMarginVarId_, scenMarginVarId);
  marginScens_[scenId] = discreteVars_[scenMarginVarId];
}

bool
MarginCalculationLauncher::launchLoadIncreaseWrapper(int varId) {
  TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, discreteVars_[varId]) << Trace::endline;

  const boost::shared_ptr<LoadIncrease> & loadIncrease = mc_->getLoadIncrease();
  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  SimulationResult & result = results_[varId].getResult();
  launchLoadIncrease(loadIncrease, discreteVars_[varId], result);

  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[3] = {FDB_LOAD_INC, varId, result.getSuccess() ? 1 : 0};
    MPI_Send(&msg, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    exportResult(result);
#endif  // _MPI_
  }

  TraceInfo(logTag_) << Trace::endline;
  return result.getSuccess();
}

bool
MarginCalculationLauncher::launchScenarioWrapper(int scenId, int varId) {
  bool liSuccess;
  int liVarIdToLaunch;
  while (!checkLoadIncreaseStatus(varId, liSuccess, liVarIdToLaunch))
    launchLoadIncreaseWrapper(liVarIdToLaunch);

  if (!liSuccess)
    return false;

  int variation = discreteVars_[varId];

  std::string iidmFile = idmFileNameFromVariation(variation);
  if (inputsByIIDM_.count(iidmFile) == 0)  // read inputs only if not already existing with enough variants defined
    inputsByIIDM_[iidmFile].readInputs(workingDirectory_, mc_->getScenarios()->getJobsFile(), iidmFile);

  SimulationResult & resultScen = results_[varId].getScenarioResult(scenId);
  launchScenario(inputsByIIDM_[iidmFile], getScens()[scenId], variation, resultScen);
  TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, resultScen.getUniqueScenarioId(), getStatusAsString(resultScen.getStatus()))
                     << Trace::endline << Trace::endline;

  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[4] = {FDB_SCEN_SINGLE, scenId, varId, resultScen.getSuccess() ? 1 : 0};
    MPI_Send(&msg, 4, MPI_INT, 0, 0, MPI_COMM_WORLD);
    exportResult(resultScen);
#endif  // _MPI_
  }

  return resultScen.getSuccess();
}

void
MarginCalculationLauncher::finish(boost::posix_time::ptime & t0) {
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, discreteVars_[globalMarginVarId_]) << Trace::endline;
  if (!searchingGlobalMargin()) {
    for (int scenId = 0; scenId < nbScens(); ++scenId)
      TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, getScens()[scenId]->getId(), marginScens_[scenId]) << Trace::endline;
  }
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;

  // purge unused results for readability of aggregatedResults.xml
  std::vector<LoadIncreaseResult> resultsTemp;
  resultsTemp.swap(results_);
  for (LoadIncreaseResult & result : resultsTemp) {
    int variation = result.getResult().getVariation();
    if (variation>= 0) {
      if (isServerThread() && (nbScens() > 0))  // if nbScens = 0, the server thread has done the computations itself
        result.setResult(importResult(loadIncreaseExportDir(variation)));
      for (int scenId = 0; scenId < nbScens(); ++scenId) {
        SimulationResult & resultScen = result.getScenarioResult(scenId);
        if (resultScen.getScenarioId() == "") {
          resultScen.setScenarioId(getScens()[scenId]->getId());
          resultScen.setVariation(variation);
          resultScen.setStatus(RESULT_FOUND_STATUS);
        } else if (isServerThread()) {
          resultScen = importResult(SimulationResult::getUniqueScenarioId(getScens()[scenId]->getId(), variation));
        }
      }
      results_.push_back(result);
    }
  }
}

void
MarginCalculationLauncher::launchScenario(const MultiVariantInputs& inputs, const boost::shared_ptr<Scenario>& scenario,
    const double variation, SimulationResult& result) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Launch task :" << scenario->getId() << " dydFile =" << scenario->getDydFile()
              << " criteriaFile =" << scenario->getCriteriaFile() << std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation;
  std::string workingDir = createAbsolutePath(scenario->getId(), createAbsolutePath(subDir.str(), workingDirectory_));
  std::shared_ptr<job::JobEntry> job = inputs.cloneJobEntry();

  addDydFileToJob(job, scenario->getDydFile());
  setCriteriaFileForJob(job, scenario->getCriteriaFile());

  SimulationParameters params;
  initParametersWithJob(job, params);
  std::stringstream dumpFile;
  dumpFile << "loadIncreaseFinalState-" << variation << ".dmp";
  //  force simulation to load previous dump and to use final values
  params.InitialStateFile_ = createAbsolutePath(dumpFile.str(), workingDirectory_);
  params.iidmFile_ = idmFileNameFromVariation(variation);

  // startTime and stopTime are adapted depending on the variation length
  double startTime = tLoadIncrease_ - (100. - variation)/100. * inputs_.getTLoadIncreaseVariationMax();
  params.startTime_ = startTime;
  params.stopTime_ = startTime + tScenario_;

  result.setScenarioId(scenario->getId());
  result.setVariation(variation);
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs);

  if (simulation) {
    simulation->setTimelineOutputFile("");
    simulation->setConstraintsOutputFile("");
    // The event time should be adapted (the list of events models supported currently corresponds to events really used)
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getEnvVar("DYNAWO_DDB_DIR");
    decltype(modelMulti->findSubModelByLib("")) subModels;
    auto addSubModelsByLib = [&](const std::string& libName) {
      auto subModelsToAdd = modelMulti->findSubModelByLib(createAbsolutePath(libName + DYN::sharedLibraryExtension(), DDBDir));
      subModels.insert(subModels.end(), subModelsToAdd.begin(), subModelsToAdd.end());
    };
    addSubModelsByLib("EventQuadripoleDisconnection");
    addSubModelsByLib("EventConnectedStatus");
    addSubModelsByLib("EventSetPointBoolean");
    addSubModelsByLib("SetPoint");
    addSubModelsByLib("EventSetPointReal");
    addSubModelsByLib("EventSetPointDoubleReal");
    addSubModelsByLib("EventSetPointGenerator");
    addSubModelsByLib("EventSetPointLoad");
    addSubModelsByLib("LineTrippingEvent");
    addSubModelsByLib("TfoTrippingEvent");
    addSubModelsByLib("EventQuadripoleConnection");
    for (const auto& subModel : subModels) {
      double tEvent = subModel->findParameterDynamic("event_tEvent").getValue<double>();
      subModel->setParameterValue("event_tEvent", DYN::PAR, tEvent - (100. - variation) * inputs_.getTLoadIncreaseVariationMax() / 100., false);
      subModel->setSubModelParameters();
    }
    simulate(simulation, result);
  }

  if (multiprocessing::context().nbProcs() == 1)
    std::cout << " Task :" << scenario->getId() << " status =" << getStatusAsString(result.getStatus()) << std::endl;
}

void
MarginCalculationLauncher::launchLoadIncrease(const boost::shared_ptr<LoadIncrease>& loadIncrease,
    const double variation, SimulationResult& result) {
  if (multiprocessing::context().nbProcs() == 1)
    std::cout << "Launch loadIncrease of " << variation << "%" <<std::endl;

  std::stringstream subDir;
  subDir << "step-" << variation;
  std::string workingDir = createAbsolutePath(loadIncrease->getId(), createAbsolutePath(subDir.str(), workingDirectory_));
  std::shared_ptr<job::JobEntry> job = inputs_.cloneJobEntry();

  SimulationParameters params;
  //  force simulation to dump final values (would be used as input to launch each event)
  params.activateDumpFinalState_ = true;
  params.activateExportIIDM_ = true;
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation << ".iidm";
  params.exportIIDMFile_ = createAbsolutePath(iidmFile.str(), workingDirectory_);
  std::stringstream dumpFile;
  dumpFile << "loadIncreaseFinalState-" << variation << ".dmp";
  params.dumpFinalStateFile_ = createAbsolutePath(dumpFile.str(), workingDirectory_);

  result.setScenarioId(LOAD_INCREASE);
  result.setVariation(variation);
  boost::shared_ptr<DYN::Simulation> simulation = createAndInitSimulation(workingDir, job, params, result, inputs_);

  if (simulation) {
    std::shared_ptr<DYN::ModelMulti> modelMulti = std::dynamic_pointer_cast<DYN::ModelMulti>(simulation->getModel());
    std::string DDBDir = getMandatoryEnvVar("DYNAWO_DDB_DIR");
    auto subModels = modelMulti->findSubModelByLib(createAbsolutePath(std::string("DYNModelVariationArea") + DYN::sharedLibraryExtension(), DDBDir));
    for (const auto& subModel : subModels) {
      double startTime = subModel->findParameterDynamic("startTime").getValue<double>();
      double stopTime = subModel->findParameterDynamic("stopTime").getValue<double>();
      inputs_.setTLoadIncreaseVariationMax(stopTime - startTime);
      int nbLoads = subModel->findParameterDynamic("nbLoads").getValue<int>();
      for (int k = 0; k < nbLoads; ++k) {
        std::stringstream deltaPName;
        deltaPName << "deltaP_load_" << k;
        double deltaP = subModel->findParameterDynamic(deltaPName.str()).getValue<double>();
        subModel->setParameterValue(deltaPName.str(), DYN::PAR, deltaP*variation/100., false);

        std::stringstream deltaQName;
        deltaQName << "deltaQ_load_" << k;
        double deltaQ = subModel->findParameterDynamic(deltaQName.str()).getValue<double>();
        subModel->setParameterValue(deltaQName.str(), DYN::PAR, deltaQ*variation/100., false);
      }
      // change of the stop time to keep the same ramp of variation.
      double originalDuration = stopTime - startTime;
      double newStopTime = startTime + originalDuration * variation / 100.;
      subModel->setParameterValue("stopTime", DYN::PAR, newStopTime, false);
      subModel->setSubModelParameters();  // update values stored in subModel
      // Limitation for this log : will only be printed for root process
      TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseModelParameter, subModel->name(), newStopTime, variation/100.) << Trace::endline;
    }
    simulation->setStopTime(tLoadIncrease_ - (100. - variation)/100. * inputs_.getTLoadIncreaseVariationMax());
    simulate(simulation, result);
  }
}

void
MarginCalculationLauncher::createOutputs(std::map<std::string, std::string>& mapData, bool zipIt) const {
  Trace::resetCustomAppenders();  // to force flush
#ifndef NDEBUG
  Trace::resetPersistentCustomAppender(logTag_, DYN::DEBUG);  // to force flush
#else
  Trace::resetPersistentCustomAppender(logTag_, DYN::INFO);  // to force flush
#endif

  aggregatedResults::XmlExporter exporter;
  if (zipIt) {
    std::stringstream aggregatedResults;
    exporter.exportLoadIncreaseResultsToStream(results_, aggregatedResults);
    mapData["aggregatedResults.xml"] = aggregatedResults.str();
    std::ifstream inFile(createAbsolutePath("dynawo.log", workingDirectory_).c_str());
    if (inFile.is_open()) {
      std::string line;
      std::stringstream strStream;
      while (getline(inFile, line)) {
        strStream << line << "\n";
      }
      mapData["dynawo.log"] = strStream.str();
      inFile.close();
    }
  } else {
    exporter.exportLoadIncreaseResultsToFile(results_, outputFileFullPath_);
  }

  std::map<std::string, SimulationResult> bestResults;
  std::map<std::string, SimulationResult> worstResults;

  for (const auto& loadIncreaseResult : results_) {
    const double loadLevel = loadIncreaseResult.getResult().getVariation();
    const std::vector<SimulationResult>& allScenariosResults = loadIncreaseResult.getScenariosResults();
    size_t numberOfSuccessfulScenarios = 0;
    for (const SimulationResult& scenarioResult : allScenariosResults) {
      const std::string& scenarioId = scenarioResult.getScenarioId();
      if (scenarioResult.getSuccess()) {
        ++numberOfSuccessfulScenarios;
        const std::map<std::string, SimulationResult>::const_iterator itBest = bestResults.find(scenarioId);
        if (itBest == bestResults.end() || (loadLevel > itBest->second.getVariation()))
          bestResults[scenarioId] = scenarioResult;
      } else {
        const std::map<std::string, SimulationResult>::const_iterator itWorst = worstResults.find(scenarioId);
        if (itWorst == worstResults.end() || loadLevel < itWorst->second.getVariation())
          worstResults[scenarioId] = scenarioResult;
      }
    }
    if (loadIncreaseResult.getResult().getSuccess() && numberOfSuccessfulScenarios == allScenariosResults.size()) {
      const std::map<std::string, SimulationResult>::const_iterator loadIncreaseBestIt = bestResults.find(LOAD_INCREASE);
      if (loadIncreaseBestIt == bestResults.end() || loadLevel > loadIncreaseBestIt->second.getVariation()) {
        bestResults[LOAD_INCREASE] = loadIncreaseResult.getResult();
      }
    } else {
      const std::map<std::string, SimulationResult>::const_iterator loadIncreaseWorstIt = worstResults.find(LOAD_INCREASE);
      if (loadIncreaseWorstIt == worstResults.end() || loadLevel < loadIncreaseWorstIt->second.getVariation()) {
        worstResults[LOAD_INCREASE] = loadIncreaseResult.getResult();
      }
    }
  }

  for (const auto& bestResult : bestResults) {
    if (zipIt) {
      storeOutputs(bestResult.second, mapData);
    } else {
      writeOutputs(bestResult.second);
    }
  }
  for (const auto& worstResult : worstResults) {
    if (zipIt) {
      storeOutputs(worstResult.second, mapData);
    } else {
      writeOutputs(worstResult.second);
    }
  }
}

std::string
MarginCalculationLauncher::loadIncreaseExportDir(double variation) const {
  std::stringstream ss;
  ss << LOAD_INCREASE << "-" << variation;
  return ss.str();
}

std::string
MarginCalculationLauncher::idmFileNameFromVariation(double variation) const {
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation << ".iidm";
  return createAbsolutePath(iidmFile.str(), workingDirectory_);
}

}  // namespace DYNAlgorithms
