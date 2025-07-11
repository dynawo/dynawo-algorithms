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

namespace DYNAlgorithms {

static DYN::TraceStream TraceInfo(const std::string& tag = "") {
  return multiprocessing::context().isRootProc() ? Trace::info(tag) : DYN::TraceStream();
}

inline int mid(int val) {return (val+1)/2;}  // general biais towards upper roundings (mounting to load increase ceiling)
inline bool isSingleThread() {return multiprocessing::context().nbProcs() == 1;}
inline bool isServerThread() {return !isSingleThread() &&  multiprocessing::context().isRootProc();}
inline bool isWorkerThread() {return !isSingleThread() && !multiprocessing::context().isRootProc();}

inline boost::shared_ptr<Scenario> MarginCalculationLauncher::getScen(int scenId) const {return mc_->getScenarios()->getScenarios()[scenId];}

inline int MarginCalculationLauncher::nbScens() const                 {return mc_->getScenarios()->getScenarios().size();}
inline int MarginCalculationLauncher::nbVars() const                  {return discreteVars_.size();}
inline int MarginCalculationLauncher::varId100() const                {return discreteVars_.size()-1;}
inline bool MarginCalculationLauncher::searchingGlobalMargin() const  {return mc_->getCalculationType() == MarginCalculation::GLOBAL_MARGIN;}
inline bool MarginCalculationLauncher::liStarted(int varId) const     {return !startingTimesLI_[varId].is_not_a_date_time();}
inline bool MarginCalculationLauncher::liDone(int varId) const        {return results_[varId].getResult().getVariation() >= 0;}
inline bool MarginCalculationLauncher::liOK(int varId) const          {return liDone(varId) && results_[varId].getResult().getSuccess();}
inline bool MarginCalculationLauncher::scenStarted(int scenId, int varId) const {return !startingTimesScens_[scenId][varId].is_not_a_date_time();;}
inline bool MarginCalculationLauncher::scenClosed(int scenId) const {return marginScens_[scenId] >= 0;}
inline bool MarginCalculationLauncher::scenDone(int scenId, int varId) const {return (results_[varId].getScenarioResult(scenId).getScenarioId() != "");}
inline bool MarginCalculationLauncher::scenOK(int scenId, int varId) const {return scenDone(scenId, varId) &&
                                                                                   results_[varId].getScenarioResult(scenId).getSuccess();}

static const int PRIORITY_INVALID = 1000000;

int
MarginCalculationLauncher::lowestLIFailureId() const {
  for (int varId = 0; varId < nbVars(); ++varId)
      if (liDone(varId) && !liOK(varId))
        return varId;

  return varId100()+1;
}

int
MarginCalculationLauncher::highestLISuccessId() const {
  for (int varId = varId100(); varId >= 0; --varId)
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
  for (int varId = varId100(); varId >= 0; --varId)
      if (scenDone(scenId, varId) && scenOK(scenId, varId))
        return varId;

  return -1;
}

int
MarginCalculationLauncher::scenNbThreads(int scenId) const {
  int count = 0;
  for (int varId = 0; varId < nbVars(); ++varId)
    if (scenStarted(scenId, varId) && !scenDone(scenId, varId))
      ++count;

  return count;
}

// bool
// MarginCalculationLauncher::allScensFinished() const {
//   for (int scenId = 0; scenId < nbScens(); ++scenId)
//     if (!scenClosed(scenId))
//       return false;

//   return true;
// }

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

  startingTimesLI_.resize(nbVars(), boost::posix_time::ptime());
  startingTimesScens_.resize(nbScens(), std::vector<boost::posix_time::ptime>((size_t)nbVars(), boost::posix_time::ptime()));
}

void
MarginCalculationLauncher::launch() {
  initGlobals();

  // fringe case that might actually be used in prod, do a monothreaded dichotomy on load increases
  if (nbScens() == 0) {
    if (!isWorkerThread()) {
      globalMarginVarId_ = findMaxLoadIncrease();
      finish();
    }
    return;
  }

  if (isWorkerThread()) {
#ifdef _MPI_
    int msg[3] = {-1, -1, -1};
    MPI_Send(msg, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    while (true) {
      MPI_Recv(msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      int varId = msg[0];
      if (varId < 0)
        return;
      int scenId = msg[1];
      msg[2] = (scenId >= 0) ? launchScenarioWrapper(scenId, varId) : launchLoadIncreaseWrapper(varId);
      MPI_Send(msg, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
#endif  // _MPI_
  }

  std::set<int> workersLeft;
  for (int i=1; i < multiprocessing::context().nbProcs(); ++i)
    workersLeft.insert(i);

  while (workersLeft.size() > 0) {
    int workerId = -1;
    if (isServerThread()) {
  #ifdef _MPI_
      MPI_Status senderInfo;
      int msg[3];
      MPI_Recv(msg, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &senderInfo);
      updateResults(msg[0], msg[1], msg[2] > 0);
      workerId = senderInfo.MPI_SOURCE;
  #endif  // _MPI_
    }

    int varId, scenId;
    if (getNextTask(varId, scenId)) {
      if (scenId >= 0)
        launchScenarioWrapper(scenId, varId, workerId);
      else
        launchLoadIncreaseWrapper(varId, workerId);
    } else {
      if (isServerThread()) {
#ifdef _MPI_
        int msg[2] = {-1, -1};
        MPI_Send(msg, 2, MPI_INT, workerId, 0, MPI_COMM_WORLD);
        workersLeft.erase(workerId);
#endif  // _MPI_
      } else {
        workersLeft.clear();
      }
    }
  }
  finish();
}

// bool
// MarginCalculationLauncher::getJobGlobal(int & varIdRet, int & scenId) const {
//   if (highestLISuccessId() < 0)
//     return false;

//   if (searchingGlobalMargin()) {
//     // priority 1 : among the scenarios that we know fail at globalMargin+1, the one with the lower known success
//     if (globalMarginVarId_ < varId100()) {
//       int bestCandidate = -1;
//       int biggestGap = -1;
//       for (scenId = 0; scenId < nbScens(); ++scenId) {
//         if (scenClosed(scenId) || (scenNbThreads(scenId) > 0) || (lowestScenFailureId(scenId) != (globalMarginVarId_+1)))
//           continue;
//         int gap = lowestScenFailureId(scenId)-highestScenSuccessId(scenId);
//         if (gap > biggestGap) {
//           bestCandidate = scenId;
//           biggestGap = gap;
//         }
//       }
//       if (bestCandidate >= 0) {
//         scenId = bestCandidate;
//         varIdRet = getLiOKBetween(highestScenSuccessId(scenId), lowestScenFailureId(scenId));
//         if (varIdRet >=0)
//           return true;
//       }
//     }

//     // priority 2 : virgin scenarios
//     for (scenId = 0; scenId < nbScens(); ++scenId)
//       if (scenNbThreads(scenId) > 0)
//         continue;
//       if ((highestScenSuccessId(scenId) == -1) && (lowestScenFailureId(scenId) == nbVars())) {
//         for (int varId = globalMarginVarId_; varId < nbVars(); ++varId)
//           if (liOK(varId)) {
//             varIdRet = varId;
//             return true;
//           }
//       }

//     // priority 3 : scenarios that fail above globalMargin+1 ?

//     return false;
//   }

//   return false;
// }

bool
MarginCalculationLauncher::getNextTask(int & varIdRet, int & scenIdRet) const {
  if (!liStarted(varId100())) {
    varIdRet = varId100();
    scenIdRet = -1;
    return true;
  }

  int lowestPriority = PRIORITY_INVALID;

  for (int scenId = 0; scenId < nbScens(); ++scenId)
    if (!scenClosed(scenId)) {
      int scenIdTask = scenId;
      int priority, varIdTask;
      getNextTaskScen(scenIdTask, varIdTask, priority);
      if (priority < lowestPriority) {
        varIdRet = varIdTask;
        scenIdRet = scenIdTask;
        lowestPriority = priority;
      }
    }

  if (lowestPriority < PRIORITY_INVALID)
    return true;

  if (!liDone(varId100())) {
    scenIdRet = -1;

    if (!liStarted(0)) {
      varIdRet = 0;
      return true;
    }

    varIdRet = 0;
    for (int varId = varId100() -1 ; varId > 0; --varId)
      if (gapScoreLI(varId) > gapScoreLI(varIdRet))
        varIdRet = varId;
    return true;
  }

  return false;
}

void
MarginCalculationLauncher::getNextTaskScen(int & scenId, int & varIdRet, int & priority) const {
  int varIdSup = lowestScenFailureId(scenId);
  varIdSup = std::min(varIdSup, highestLISuccessId()+1);
  if (searchingGlobalMargin())
    varIdSup = std::min(varIdSup, globalMarginVarId_+1);

  int varIdInf = highestScenSuccessId(scenId);

  if ((scenNbThreads(scenId) == 0) && (varIdSup > varIdInf+1)) {
    // scen available and idle
    priority = 1000-(varIdSup-varIdInf);
    if ((highestScenSuccessId(scenId) == -1) && (lowestScenFailureId(scenId) == nbVars())) {  // virgin scenario
      varIdRet = highestLISuccessId();
      return;
    } else if ((highestScenSuccessId(scenId) >= 0) && (lowestScenFailureId(scenId) < nbVars())) {  // properly bounded dichotomy
      varIdRet = getLiOKBetween(highestScenSuccessId(scenId), lowestScenFailureId(scenId));
      if (varIdRet >= 0)
        return;
    } else if (highestScenSuccessId(scenId) >= 0) {  // only successes
      if (highestLISuccessId() > highestScenSuccessId(scenId)) {
        varIdRet = highestLISuccessId();
        return;
      }
    } else {  // only failures
      if (liOK(0) && !scenDone(scenId, 0)) {
        varIdRet = 0;
        return;
      }
    }

    // LI with 100% chance to be used
    bool noWorkAtAll = true;
    for (int varId = varIdInf+1; varId <= varIdSup-1; ++varId)
      if (liStarted(varId))
        noWorkAtAll = false;
    if (noWorkAtAll) {
      scenId = -1;
      varIdRet = mid(varIdInf+varIdSup);
      priority = 2000 - gapScoreLI(varIdRet);
      return;
    }
  }

  // widest available scen anticipation
  int bestScore = 0;
  for (int varId = varIdSup-1; varId >= varIdInf+1; --varId)
    if (gapScoreScen(varId, scenId) > bestScore) {
      varIdRet = varId;
      bestScore = gapScoreScen(varId, scenId);
    }
  if (bestScore > 0) {
    priority = 3000-bestScore;
    return;
  }

  // widest LI anticipation
  int bestCandidate = -1;
  for (int varId = varIdSup-1; varId >= varIdInf+1; --varId)
    if (gapScoreLI(varId) > gapScoreLI(bestCandidate))
      bestCandidate = varId;

  if (bestCandidate >= 0) {
    scenId = -1;
    varIdRet = bestCandidate;
    priority = 4000-gapScoreLI(varIdRet);
    return;
  }

  priority = PRIORITY_INVALID;
  return;
}

int
MarginCalculationLauncher::gapScoreLI(int varId) const {
  if ((varId < 0) || liStarted(varId))
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

  if (!liDone(varId))
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
MarginCalculationLauncher::updateResults(int varId, int scenId, bool success) {
  if (varId < 0)
    return;

  if (scenId < 0) {
    std::cout << "at " << (boost::posix_time::second_clock::local_time() - t0_).total_milliseconds()/1000 << "s : ";
    std::cout << "LI " << discreteVars_[varId] << "% " << (success ? "success" : "failure") << std::endl;

    SimulationResult & result = results_[varId].getResult();
    result.setVariation(discreteVars_[varId]);
    result.setSuccess(success);
  } else {
    std::cout << "at " << (boost::posix_time::second_clock::local_time() - t0_).total_milliseconds()/1000 << "s : ";
    std::cout << "scen " << scenId << " at " << discreteVars_[varId] << "% " << (success ? "success" : "failure") << std::endl;

    SimulationResult & result = results_[varId].getScenarioResult(scenId);
    result.setScenarioId(getScen(scenId)->getId());  // mark it as run for result collection
    result.setVariation(discreteVars_[varId]);
    result.setSuccess(success);
  }

  if (!success) {
    globalMarginVarId_ = std::min(globalMarginVarId_, varId-1);
    globalMarginVarId_ = std::max(globalMarginVarId_, 0);
  }

  for (int scenId = 0; scenId < nbScens(); ++scenId) {
    int successId = highestScenSuccessId(scenId);
    if ((successId == varId100()) ||
        (searchingGlobalMargin() && (successId >= globalMarginVarId_)) ||
        liDone(successId+1) && (!liOK(successId+1)) ||
        scenDone(scenId, successId+1))

      marginScens_[scenId] = discreteVars_[std::max(successId, 0)];
  }
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
MarginCalculationLauncher::findMaxLoadIncrease() {
  if (launchLoadIncreaseWrapper(varId100()))
    return varId100();

  if (!launchLoadIncreaseWrapper(0))
    return 0;

  int varIdInf = 0;  // lowest known that succeeds
  int varIdSup = varId100();  // highest known that fails

  while (varIdSup > varIdInf+1)
    (launchLoadIncreaseWrapper(mid(varIdInf+varIdSup)) ? varIdInf : varIdSup) = mid(varIdInf+varIdSup);

  return varIdInf;
}

bool
MarginCalculationLauncher::launchLoadIncreaseWrapper(int varId, int workerId) {
  startingTimesLI_[varId] = boost::posix_time::second_clock::local_time();

  if (isServerThread()) {
#ifdef _MPI_
    int msg[2] = {varId, -1};
    MPI_Send(msg, 2, MPI_INT, workerId, 0, MPI_COMM_WORLD);
    return false;
#endif  // _MPI_
  }

  std::cout << "at " << (boost::posix_time::second_clock::local_time() - t0_).total_milliseconds()/1000 << "s : ";
  std::cout << "launching LI " << discreteVars_[varId] << "%" << std::endl;

  // TraceInfo(logTag_) << DYNAlgorithmsLog(VariationValue, discreteVars_[varId]) << Trace::endline;

  const boost::shared_ptr<LoadIncrease> & loadIncrease = mc_->getLoadIncrease();
  inputs_.readInputs(workingDirectory_, loadIncrease->getJobsFile());
  SimulationResult & result = results_[varId].getResult();
  launchLoadIncrease(loadIncrease, discreteVars_[varId], result);

  if (isWorkerThread())
    exportResult(result);
  else
    updateResults(varId, -1, result.getSuccess());

  // TraceInfo(logTag_) << Trace::endline;
  return result.getSuccess();
}

bool
MarginCalculationLauncher::launchScenarioWrapper(int scenId, int varId, int workerId) {
  startingTimesScens_[scenId][varId] = boost::posix_time::second_clock::local_time();

  if (isServerThread()) {
#ifdef _MPI_
    int msg[2] = {varId, scenId};
    MPI_Send(msg, 2, MPI_INT, workerId, 0, MPI_COMM_WORLD);
    return false;
#endif  // _MPI_
  }

  std::cout << "at " << (boost::posix_time::second_clock::local_time() - t0_).total_milliseconds()/1000 << "s : ";
  std::cout << "launching scen " << scenId << " at " << discreteVars_[varId] << "%" << std::endl;

  std::string iidmFile = idmFileNameFromVariation(discreteVars_[varId]);
  if (inputsByIIDM_.count(iidmFile) == 0)  // read inputs only if not already existing with enough variants defined
    inputsByIIDM_[iidmFile].readInputs(workingDirectory_, mc_->getScenarios()->getJobsFile(), iidmFile);

  SimulationResult & resultScen = results_[varId].getScenarioResult(scenId);
  launchScenario(inputsByIIDM_[iidmFile], getScen(scenId), discreteVars_[varId], resultScen);
  TraceInfo(logTag_) <<  DYNAlgorithmsLog(ScenariosEnd, resultScen.getUniqueScenarioId(), getStatusAsString(resultScen.getStatus()))
                     << Trace::endline << Trace::endline;

  if (isWorkerThread())
    exportResult(resultScen);
  else
    updateResults(varId, scenId, resultScen.getSuccess());


  return resultScen.getSuccess();
}

void
MarginCalculationLauncher::finish() {
  TraceInfo(logTag_) << "============================================================ " << Trace::endline;
  TraceInfo(logTag_) << DYNAlgorithmsLog(GlobalMarginValue, discreteVars_[globalMarginVarId_]) << Trace::endline;
  if (!searchingGlobalMargin()) {
    for (int scenId = 0; scenId < nbScens(); ++scenId)
      TraceInfo(logTag_) << DYNAlgorithmsLog(LocalMarginValueScenario, getScen(scenId)->getId(), marginScens_[scenId]) << Trace::endline;
  }
  boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
  boost::posix_time::time_duration diff = t1 - t0_;
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
          resultScen.setScenarioId(getScen(scenId)->getId());
          resultScen.setVariation(variation);
          resultScen.setStatus(RESULT_FOUND_STATUS);
        } else if (isServerThread()) {
          resultScen = importResult(SimulationResult::getUniqueScenarioId(getScen(scenId)->getId(), variation));
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
