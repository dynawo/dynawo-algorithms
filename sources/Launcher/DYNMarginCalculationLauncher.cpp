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

#include <signal.h>

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

// special values in MPI_Send/Recv
static const int SERVER_ID = 0;
static const int MSG_STANDARD = 0;

// special values in workStatus
static const int SIMU_NOT_STARTED = -1;
static const int SIMU_FINISHED = -2;
static const int SIMU_ABORTED = -3;

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
}

inline int mid(int val) {return (val+1)/2;}  // general bias towards upper roundings (mounting to load increase ceiling)
inline bool isSingleThread() {return multiprocessing::context().nbProcs() == 1;}
inline bool isServerThread() {return !isSingleThread() &&  multiprocessing::context().isRootProc();}
inline bool isWorkerThread() {return !isSingleThread() && !multiprocessing::context().isRootProc();}

inline boost::shared_ptr<Scenario> MarginCalculationLauncher::getScen(int scenId) const {return mc_->getScenarios()->getScenarios()[scenId];}

inline int MarginCalculationLauncher::nbScens() const                 {return mc_->getScenarios()->getScenarios().size();}
inline int MarginCalculationLauncher::nbVars() const                  {return discreteVars_.size();}
inline int MarginCalculationLauncher::varId100() const                {return discreteVars_.size()-1;}
inline bool MarginCalculationLauncher::globMarginMode() const         {return mc_->getCalculationType() == MarginCalculation::GLOBAL_MARGIN;}
inline bool MarginCalculationLauncher::liStarted(int varId) const     {return workStatus_[varId][0] != SIMU_NOT_STARTED;}
inline bool MarginCalculationLauncher::liDone(int varId) const        {return workStatus_[varId][0] == SIMU_FINISHED;}
inline bool MarginCalculationLauncher::liOK(int varId) const          {return liDone(varId) && results_[varId].getResult().getSuccess();}
inline bool MarginCalculationLauncher::scenStarted(int scenId, int varId) const {return workStatus_[varId][scenId+1] != SIMU_NOT_STARTED;}
inline bool MarginCalculationLauncher::scenDone(int scenId, int varId) const {return workStatus_[varId][scenId+1] == SIMU_FINISHED;}
inline bool MarginCalculationLauncher::scenOK(int scenId, int varId) const {return scenDone(scenId, varId) &&
                                                                                   results_[varId].getScenarioResult(scenId).getSuccess();}

static const int PRIORITY_INVALID = 1000000;

#define PRINT(x) std::cout << "at " << (boost::posix_time::second_clock::local_time()-t0_).total_milliseconds()/1000 << "s : " << x << std::endl

void
MarginCalculationLauncher::initGlobals() {
  assert(multipleJobs_);

  t0_ = boost::posix_time::second_clock::local_time();

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

  workStatus_.resize(nbVars(), std::vector<int>((size_t)(nbScens()+1), SIMU_NOT_STARTED));

  int nbWorkers = std::max(multiprocessing::context().nbProcs()-1, 1u);
  initialLIs_.resize(nbWorkers);
  initialLIs_[0] = varId100();
  if (nbWorkers > 1) {  // equirepartition of initial LIs
    for (int i=0; i < nbWorkers-1; ++i)
      initialLIs_[i+1] = (i*varId100())/(nbWorkers-1);
  }
}

bool
MarginCalculationLauncher::workToDo() const {
  if (nbScens() == 0)
    return (highestLISuccessId()+1) != lowestLIFailureId();

  for (int scenId =0; scenId < nbScens(); ++scenId)
    if (marginScens_[scenId] <0)
      return true;
  return false;
}

void
MarginCalculationLauncher::launch() {
  initGlobals();

  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[4] = {getMachineId(), -1, -1, -1};
#ifndef _MSC_VER
    msg[1] = getpid();  // server needs to know worker PID to send interruption signals
#endif

    MPI_Send(msg, 4, MPI_INT, SERVER_ID, MSG_STANDARD, MPI_COMM_WORLD);  // worker declaration msg
    while (true) {
      MPI_Recv(msg, 2, MPI_INT, SERVER_ID, MSG_STANDARD, MPI_COMM_WORLD, MPI_STATUS_IGNORE);  // server command msg
      if (msg[0] < 0)  // invalid varId : termination msg
        return;
      launchTask(msg[0], msg[1], -1, &(msg[2]), &(msg[3]));
      MPI_Send(msg, 4, MPI_INT, SERVER_ID, MSG_STANDARD, MPI_COMM_WORLD);  // worker feedback msg
    }
#endif  // _MPI_
  }

  std::set<int> workersLeft;
  for (unsigned int i=1; i < multiprocessing::context().nbProcs(); ++i)
    workersLeft.insert(static_cast<int>(i));

  // server / single thread main loop
  while (workToDo()) {
    int workerId = SERVER_ID;
    if (isServerThread())
      waitForWorker(workerId);

    int varId, scenId;
    getNextTask(varId, scenId) ? launchTask(varId, scenId, workerId) : terminateWorker(workerId, workersLeft);
  }

  importResults();

  if (isServerThread())
    cleanupWorkers(workersLeft);
}

void
MarginCalculationLauncher::waitForWorker(int & workerId) {
  int msg[4];

#ifdef _MPI_
    MPI_Status senderInfo;
    MPI_Recv(msg, 4, MPI_INT, MPI_ANY_SOURCE, MSG_STANDARD, MPI_COMM_WORLD, &senderInfo);
    workerId = senderInfo.MPI_SOURCE;
#endif  // _MPI_

  if (pids_.find(workerId) != pids_.end())
    updateResults(msg[0], msg[1], msg[2] > 0, msg[3] > 0);
  else
    pids_[workerId] = (getMachineId() == msg[0]) ? msg[1] : -1;
}

bool
MarginCalculationLauncher::getNextTask(int & varIdRet, int & scenIdRet) const {
  if (liDone(0) && !liOK(0))  // LI failure at 0% : finished
    return false;

  int hls = highestLISuccessId();
  int llf = lowestLIFailureId();

  if ((hls+1) != llf) {  // LI search mode
    scenIdRet = -1;

    static unsigned int count = 0;
    if (count < initialLIs_.size()) {
      varIdRet = initialLIs_[count++];
      return true;
    }

    if (!liStarted(0)) {
      varIdRet = 0;
      return true;
    }

    varIdRet = -1;
    for (int varId = llf-1; varId >= hls+1; --varId)
      if (gapScoreLI(varId) > gapScoreLI(varIdRet))
        varIdRet = varId;

    if (gapScoreLI(varIdRet) > 0)
      return true;
  }

  if (hls < 0)
    return false;

  // at least one valid LI : search pulled by scenarios
  int lowestPriority = PRIORITY_INVALID;

  for (int scenId = 0; scenId < nbScens(); ++scenId) {
    int scenIdTask = scenId;
    int priority, varIdTask;
    getNextTaskForScen(scenIdTask, varIdTask, priority);
    if (priority < lowestPriority) {
      varIdRet = varIdTask;
      scenIdRet = scenIdTask;
      lowestPriority = priority;
    }
  }

  return (lowestPriority < PRIORITY_INVALID);
}

void
MarginCalculationLauncher::getNextTaskForScen(int & scenId, int & varIdRet, int & priority) const {
  priority = PRIORITY_INVALID;

  int hls = highestLISuccessId();
  int llf = lowestLIFailureId();
  int hss = highestScenSuccessId(scenId);
  int lsf = lowestScenFailureId(scenId);
  int lf = std::min(llf, lsf);

  if ((hss >= lf-1) || (globMarginMode() && (hss >= globalMarginVarId_)))
    return;

  // in global mode, priorize scens with low success and low failure, as they are more likely to determine global margin
  // in local mode, priorize scens with low success and high failure, as there is more work to do on them left
  priority = hss + (globMarginMode() ? lsf : -lf);

  if (hasActiveThreadScen(scenId))
    goto ANTICIPATION;

  if (globMarginMode()) {
    if (liOK(globalMarginVarId_) && !scenStarted(scenId, globalMarginVarId_)) {
      varIdRet = globalMarginVarId_;
      return;
    }
    if ((globalMarginVarId_ < varId100()) && liOK(globalMarginVarId_+1) && !scenStarted(scenId, globalMarginVarId_+1)) {
      varIdRet = globalMarginVarId_+1;
      return;
    }
  }

  // lsf > hls is sadly possible due to badly conditioned problems
  if (lsf <= hls) {  // at least one failure in pertinent range  : dichotomy
    varIdRet = getLiOKBetween(hss, lsf);
    if (varIdRet >= 0)
      return;
  } else {  // no failure yet : try as high as possible
    if (!scenStarted(scenId, hls)) {
      varIdRet = hls;
      return;
    }
  }

  if (!hasActiveThreadLI(scenId)) {
    scenId = -1;
    if (hss < 0)
      varIdRet = (lsf <= mid(varId100())) ? 0 :  mid(lsf);
    else
      varIdRet = mid(hss+lf);
    if (!liStarted(varIdRet))
      return;
  }

  ANTICIPATION:
  priority += 1000;

  varIdRet = 0;
  int bestScore = 0;
  for (int varId = lf-1; varId >= hss+1; --varId) {
    int score = std::max(gapScoreScen(varId, scenId), gapScoreLI(varId));
    if (score > bestScore) {
      bestScore = score;
      varIdRet = varId;
    }
  }

  // check whether selected task is a scen or a LI
  if (!liOK(varIdRet))
    scenId = -1;

  // priorize anticipation that covers the most ground among all scenarios rather than all anticipations on widest scenario
  if (!globMarginMode())
    priority = 1000 - bestScore;

  if (bestScore <= 0)
    priority = PRIORITY_INVALID;
}

void
MarginCalculationLauncher::updateResults(int varId, int scenId, bool complete, bool success) {
  workStatus_[varId][scenId+1] = complete ? SIMU_FINISHED : SIMU_NOT_STARTED;

  const char * successStr = (complete ? (success ? "success" : "failure") : "aborted");

  if (scenId >= 0) {
    PRINT("scen " << scenId << " at " << discreteVars_[varId] << "% " << successStr);
    TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioFeedback, scenId, discreteVars_[varId], successStr) << Trace::endline;
  } else {
    PRINT("LI at " << discreteVars_[varId] << "% " << successStr);
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseFeedback, discreteVars_[varId], successStr) << Trace::endline;
  }

  if (!complete)
    return;

  if (scenId < 0) {
    SimulationResult & result = results_[varId].getResult();
    result.setVariation(discreteVars_[varId]);
    result.setSuccess(success);
  } else {
    SimulationResult & result = results_[varId].getScenarioResult(scenId);
    result.setScenarioId(getScen(scenId)->getId());  // mark it as run for result collection
    result.setVariation(discreteVars_[varId]);
    result.setSuccess(success);
  }

  if (!success) {
    globalMarginVarId_ = std::min(globalMarginVarId_, varId-1);
    globalMarginVarId_ = std::max(globalMarginVarId_, 0);
  }

  for (int scenIdOther = 0; scenIdOther < nbScens(); ++scenIdOther) {
    int successId = highestScenSuccessId(scenIdOther);
    if ((successId == varId100()) ||
        (globMarginMode() && (successId >= globalMarginVarId_)) ||
        (liDone(successId+1) && (!liOK(successId+1))) ||
        scenDone(scenIdOther, successId+1))

      marginScens_[scenIdOther] = discreteVars_[std::max(successId, 0)];
  }

  if (isServerThread())
    abortObsoleteSimus();
}

void
MarginCalculationLauncher::abortObsoleteSimus() {
  for (int varId = 0; varId < nbVars(); ++varId)
    if (workStatus_[varId][0] >= SERVER_ID) {
      bool needed = false;
      for (int scenId = 0; scenId < nbScens(); ++scenId) {
        if (marginScens_[scenId] >= 0)
          continue;
        if (lowestScenFailureId(scenId) > varId100())
          continue;
        if (lowestScenFailureId(scenId) < varId)
          continue;
        if (highestScenSuccessId(scenId) > varId)
          continue;
        needed = true;
        break;
      }

      // do not cancel initial LIs as they may serve and are supposedly near to completion
      for (int protectedVarId : initialLIs_)
        if (varId == protectedVarId)
          needed = true;

      if ((highestLISuccessId()+1) != lowestLIFailureId()) {
        if (lowestLIFailureId() > varId100())
          needed = true;  // do not cancel LIs while varId100() has not been tested
        else if (varId > highestLISuccessId())
          needed = true;  // do not cancel crucial LIs in LI search mode motivated by LI failure
      }

      if (varId > lowestLIFailureId())
        needed = false;

      if (!needed)
        abortSimulation(varId, 0);
    }

  for (int varId = 0; varId < nbVars(); ++varId)
    for (int scenId = 0; scenId < nbScens(); ++scenId)
      if (workStatus_[varId][scenId+1] >= SERVER_ID) {
        if ((marginScens_[scenId] >= 0) || (lowestScenFailureId(scenId) < varId) || (highestScenSuccessId(scenId) > varId)) {  // useless computation
          abortSimulation(varId, scenId+1);
        } else if ((lowestScenFailureId(scenId) > varId100()) && (highestLISuccessId() > varId) && (varId < globalMarginVarId_)) {
          bool noHigherSimuRunning = true;
          for (int varIdBetter = varId+1; varIdBetter < nbVars(); ++varIdBetter)
            if (scenStarted(scenId, varIdBetter)) {
              noHigherSimuRunning = false;
              break;
            }
          if (noHigherSimuRunning)  // heuristic optim : most of the scenarios pass at the highest LI available
            abortSimulation(varId, scenId+1);
        }
      }
}

int
MarginCalculationLauncher::lowestLIFailureId() const {
  for (int varId = 0; varId < nbVars(); ++varId)
      if (liDone(varId) && !liOK(varId))
        return varId;

  return varId100()+1;
}

int
MarginCalculationLauncher::highestLISuccessId() const {
  for (int varId = lowestLIFailureId()-1; varId >= 0; --varId)
      if (liOK(varId))
        return varId;

  return -1;
}

int
MarginCalculationLauncher::lowestScenFailureId(int scenId) const {
  for (int varId = 0; varId < nbVars(); ++varId)
      if (scenDone(scenId, varId) && !scenOK(scenId, varId))
        return varId;

  return varId100()+1;
}

int
MarginCalculationLauncher::highestScenSuccessId(int scenId) const {
  for (int varId = lowestScenFailureId(scenId)-1; varId >= 0; --varId)
      if (scenDone(scenId, varId) && scenOK(scenId, varId))
        return varId;

  return -1;
}

bool
MarginCalculationLauncher::hasActiveThreadScen(int scenId) const {
  for (int varId = highestScenSuccessId(scenId)+1; varId < lowestScenFailureId(scenId); ++varId)
    if (scenStarted(scenId, varId))
      return true;
  return false;
}

bool
MarginCalculationLauncher::hasActiveThreadLI(int scenId) const {
  for (int varId = highestScenSuccessId(scenId)+1; varId < std::min(lowestScenFailureId(scenId), lowestLIFailureId()); ++varId)
    if (liStarted(varId))
      return true;
  return false;
}

int
MarginCalculationLauncher::getLiOKBetween(int varIdMin, int varIdMax) const {
  int varIdClosest = -1;
  int distMin = 1000;

  // find the already computed LI closest to the middle
  for (int varId = varIdMin+1; varId <= varIdMax-1; ++varId) {
    if (liOK(varId)) {
      int dist = std::abs(varId-mid(varIdMin+varIdMax));
      if (dist < distMin) {
        varIdClosest = varId;
        distMin = dist;
      }
    }
  }

  return varIdClosest;
}

int
MarginCalculationLauncher::gapScoreLI(int varId) const {
  if ((varId < 0) || (varId > varId100()) || liStarted(varId))
    return 0;

  int gapScore = 1;
  int gap = 1;
  bool limitReached = false;
  while (!limitReached) {
    if ((varId-gap < 0) || liStarted(varId-gap))
      limitReached = true;
    else
      ++gapScore;

    if ((varId+gap >= nbVars()) || liStarted(varId+gap))
      limitReached = true;
    else
      ++gapScore;

    ++gap;
  }

  return gapScore;
}

int
MarginCalculationLauncher::gapScoreScen(int varId, int scenId) const {
  if (scenStarted(scenId, varId))
    return 0;

  if (!liOK(varId))
    return 0;

  int gapScore = 1;
  int gap = 1;
  bool limitReached = false;
  while (!limitReached) {
    if ((varId-gap < 0) || scenStarted(scenId, varId-gap))
      limitReached = true;
    else
      ++gapScore;

    if ((varId+gap >= nbVars()) || scenStarted(scenId, varId+gap))
      limitReached = true;
    else
      ++gapScore;

    ++gap;
  }

  return gapScore;
}

void
MarginCalculationLauncher::launchTask(int varId, int scenId, int workerId, int * complete, int * success) {
  workStatus_[varId][scenId+1] = workerId;

  if (scenId >= 0)
    TraceInfo(logTag_) << DYNAlgorithmsLog(ScenarioLaunch, scenId, discreteVars_[varId]) << Trace::endline;
  else
    TraceInfo(logTag_) << DYNAlgorithmsLog(LoadIncreaseLaunch, discreteVars_[varId]) << Trace::endline;

  if (isServerThread()) {
#ifdef _MPI_
    int msg[2] = {varId, scenId};
    MPI_Send(msg, 2, MPI_INT, workerId, MSG_STANDARD, MPI_COMM_WORLD);
    return;
#endif  // _MPI_
  }

  DYN::SignalHandler::setExitSignal(false);  // not reset by dynawo ...
  SimulationResult & result = (scenId >= 0) ? results_[varId].getScenarioResult(scenId) :  results_[varId].getResult();

  if (scenId >= 0) {
    PRINT("launching scen " << scenId << " at " << discreteVars_[varId] << "% ");
    std::string iidmFile = generateIDMFileNameForVariation(discreteVars_[varId]);
    if (inputsByIIDM_.count(iidmFile) == 0)  // read inputs only if not already existing with enough variants defined
      inputsByIIDM_[iidmFile].readInputs(workingDirectory_, mc_->getScenarios()->getJobsFile(), iidmFile);

    launchScenario(inputsByIIDM_[iidmFile], getScen(scenId), discreteVars_[varId], result);
  } else {
    PRINT("launching LI at " << discreteVars_[varId] << "% ");
    const boost::shared_ptr<LoadIncrease> & loadIncrease = mc_->getLoadIncrease();
    inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());

    launchLoadIncrease(loadIncrease, discreteVars_[varId], result);
  }

  if (complete != nullptr)
    *complete = DYN::SignalHandler::gotExitSignal() ? 0 : 1;

  if (DYN::SignalHandler::gotExitSignal())
    return;

  if (isWorkerThread())
    exportResult(result);
  else
    updateResults(varId, scenId, true, result.getSuccess());

  if (success != nullptr)
    *success = result.getSuccess() ? 1 : 0;
}

void
MarginCalculationLauncher::abortSimulation(int varId, int taskId) {
  if (pids_[workStatus_[varId][taskId]] < 0) {
    PRINT("cannot abort task " << taskId << " at " << discreteVars_[varId] << "% as pid is invalid (remote host or windows ?)");
    return;
  }

#ifndef _MSC_VER
  kill(pids_[workStatus_[varId][taskId]], SIGINT);
  workStatus_[varId][taskId] = SIMU_ABORTED;

  if (taskId > 0) {
    PRINT("aborting scen " << taskId-1 << " at " << discreteVars_[varId] << "% ");
    TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenarioAbort, taskId-1, discreteVars_[varId]) << Trace::endline;
  } else {
    PRINT("aborting LI at " << discreteVars_[varId] << "% ");
    TraceInfo(logTag_) <<  DYNAlgorithmsLog(LoadIncreaseAbort, discreteVars_[varId]) << Trace::endline;
  }
#endif
}

void
MarginCalculationLauncher::terminateWorker(int workerId, std::set<int> & workersLeft) const {
#ifdef _MPI_
  int msg[2] = {-1, -1};
  MPI_Send(msg, 2, MPI_INT, workerId, MSG_STANDARD, MPI_COMM_WORLD);
  workersLeft.erase(workerId);
#endif  // _MPI_
}

void
MarginCalculationLauncher::cleanupWorkers(std::set<int> & workersLeft) {
  // abort everything still running
  for (int varId = 0; varId < nbVars(); ++varId)
    for (int taskId = 0; taskId <= nbScens(); ++taskId)
      if (workStatus_[varId][taskId] > SERVER_ID)
        abortSimulation(varId, taskId);

  // terminate cleanly each remaining worker when it reports to server
  while (workersLeft.size() > 0) {
#ifdef _MPI_
    MPI_Status senderInfo;
    int msg[4];
    MPI_Recv(msg, 4, MPI_INT, MPI_ANY_SOURCE, MSG_STANDARD, MPI_COMM_WORLD, &senderInfo);
    terminateWorker(senderInfo.MPI_SOURCE, workersLeft);
#endif  // _MPI_
  }
}

int
MarginCalculationLauncher::getMachineId() const {
  char hostname[256] = "windows_host";
#ifndef _MSC_VER
  gethostname(hostname, 256);
#endif
  hostname[255] = 0;
  int len = strlen(hostname);
  int machineId = 0;
  for (int i=1; i<= 4; ++i) {
    machineId += hostname[len-i];
    machineId = (machineId << 8);
  }
  return machineId;
}

void
MarginCalculationLauncher::importResults() {
  PRINT("finished with global margin " << discreteVars_[globalMarginVarId_] << "%");
  if (!globMarginMode()) {
    for (int scenId = 0; scenId < nbScens(); ++scenId)
      PRINT("local margin " << getScen(scenId)->getId() << " : " << marginScens_[scenId] << "%");
  }

  TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, discreteVars_[globalMarginVarId_]) << Trace::endline;
  if (!globMarginMode()) {
    for (int scenId = 0; scenId < nbScens(); ++scenId)
      TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, getScen(scenId)->getId(), marginScens_[scenId]) << Trace::endline;
  }
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0_;
  TraceInfo(logTag_) << DYNAlgorithmsLog(AlgorithmsWallTime, "Margin calculation", diff.total_milliseconds()/1000) << Trace::endline;
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;

  // purge unused results for readability of aggregatedResults.xml
  std::vector<LoadIncreaseResult> sparseResults;
  sparseResults.swap(results_);
  for (LoadIncreaseResult & result : sparseResults) {
    int variation = result.getResult().getVariation();
    if (variation < 0)
      continue;

    if (isServerThread()) {
      std::string liPath = std::string(LOAD_INCREASE) + "-" + std::to_string(variation);
      result.setResult(importResult(liPath));
      cleanResult(liPath);
    }

    std::vector<SimulationResult> denseScenResults;

    for (int scenId = 0; scenId < nbScens(); ++scenId) {
      SimulationResult & resultScen = result.getScenarioResult(scenId);
      if (resultScen.getScenarioId() == "")
        continue;
      if (isServerThread()) {
        std::string scenPath = SimulationResult::getUniqueScenarioId(getScen(scenId)->getId(), variation);
        resultScen = importResult(scenPath);
        cleanResult(scenPath);
      }
      denseScenResults.push_back(resultScen);
    }

    denseScenResults.swap(result.getScenariosResults());

    results_.push_back(result);
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
  params.iidmFile_ = generateIDMFileNameForVariation(variation);

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
MarginCalculationLauncher::generateIDMFileNameForVariation(double variation) const {
  std::stringstream iidmFile;
  iidmFile << "loadIncreaseFinalState-" << variation << ".iidm";
  return createAbsolutePath(iidmFile.str(), workingDirectory_);
}

}  // namespace DYNAlgorithms
