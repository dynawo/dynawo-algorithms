# Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Dynawo, an hybrid C++/Modelica open source suite
# of simulation tools for power systems.

set(DYN_ALGO_LAUNCHER_SOURCES
  DYNComputeLoadVariationLauncher.cpp
  DYNComputeSimulationLauncher.cpp
  DYNSystematicAnalysisLauncher.cpp
  DYNMarginCalculationLauncher.cpp
  DYNRobustnessAnalysisLauncher.cpp
  DYNDataInterfaceContainer.cpp
  DYNMultiVariantInputs.cpp
  )

set(DYN_ALGO_LAUNCHER_HEADERS
  DYNComputeLoadVariationLauncher.h
  DYNComputeSimulationLauncher.h
  DYNSystematicAnalysisLauncher.h
  DYNMarginCalculationLauncher.h
  DYNRobustnessAnalysisLauncher.h
  DYNDataInterfaceContainer.h
  DYNMultiVariantInputs.h
  )

add_library(dynawo_algorithms_Launcher SHARED ${DYN_ALGO_LAUNCHER_SOURCES})

install(FILES ${DYN_ALGO_LAUNCHER_HEADERS} DESTINATION ${includedir})

target_include_directories(dynawo_algorithms_Launcher
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDEDIR_NAME}>
  )

target_link_libraries(dynawo_algorithms_Launcher
  dynawo_algorithms_API_MultipleJobs
  dynawo_algorithms_API_AggregatedResults
  Dynawo::dynawo_Common
  Dynawo::dynawo_API_JOB
  Dynawo::dynawo_ModelerCommon
  Dynawo::dynawo_Simulation
  Dynawo::dynawo_SimulationCommon
  Boost::system
  Boost::program_options
  libZIP::libZIP
  )
install(TARGETS dynawo_algorithms_Launcher EXPORT dynawo_algorithms-targets DESTINATION ${libdir})
