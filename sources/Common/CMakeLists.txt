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

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/DYNAlgorithmsError_keys.cpp PROPERTIES GENERATED 1)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/DYNAlgorithmsLog_keys.cpp PROPERTIES GENERATED 1)

set(DICTIONARIES
  Dictionaries/DYNAlgorithmsError_en_GB.dic
  Dictionaries/DYNAlgorithmsLog_en_GB.dic
  )

set(DICTIONARIES_MAPPING
  Dictionaries/dictionaries_mapping.dic
  )

# custom target to test and generate files associated to dictionnaries
add_custom_target(create_keys_files ALL
  DEPENDS ${DICTIONARIES}
  )

add_custom_command(TARGET create_keys_files
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND ${PYTHON_EXECUTABLE} ${DYNAWO_HOME}/sbin/validateDictionaries.py --inputDir=${CMAKE_CURRENT_SOURCE_DIR}/Dictionaries --outputDir=${CMAKE_CURRENT_SOURCE_DIR} --namespace=DYNAlgorithms
  COMMENT "Test dictionaries files and generate keys file"
  )

# Find keys.cpp files (generated files)
set(CPP_KEYS
  DYNAlgorithmsError_keys.cpp
  DYNAlgorithmsLog_keys.cpp
  )

# Find keys.h files  (generated files)
set(INCLUDE_KEYS
  DYNAlgorithmsError_keys.h
  DYNAlgorithmsLog_keys.h
  )

set(DYN_ALGO_COMMON_SOURCES
  DYNMarginCalculation.cpp
  DYNScenarios.cpp
  DYNScenario.cpp
  DYNLoadIncrease.cpp
  DYNSimulationResult.cpp
  DYNLoadIncreaseResult.cpp
  DYNMPIContext.cpp
  ${CPP_KEYS}
  )

set(DYN_ALGO_COMMON_HEADERS
  DYNResultCommon.h
  DYNScenario.h
  DYNScenarios.h
  DYNSimulationResult.h
  DYNLoadIncreaseResult.h
  DYNMPIContext.h
  DYNMPIContext.hpp
  ${INCLUDE_KEYS}
  )

add_library(dynawo_algorithms_Common SHARED ${DYN_ALGO_COMMON_SOURCES})
add_dependencies(dynawo_algorithms_Common create_keys_files)
install(FILES ${DYN_ALGO_COMMON_HEADERS} DESTINATION ${INCLUDEDIR_NAME})

target_include_directories(dynawo_algorithms_Common
  PUBLIC
    $<TARGET_PROPERTY:Boost::boost,INTERFACE_INCLUDE_DIRECTORIES>
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDEDIR_NAME}>
  )

target_link_libraries(dynawo_algorithms_Common
  Dynawo::dynawo_Common
  MPI::MPI_CXX
  )

install(TARGETS dynawo_algorithms_Common EXPORT dynawo_algorithms-targets DESTINATION ${LIBDIR_NAME})
install(FILES ${DICTIONARIES} ${DICTIONARIES_MAPPING} DESTINATION ${SHAREDIR_NAME})
