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

set(MODULE_NAME dynawo_algorithms_Launcher_unittest)

set(MODULE_SOURCES
  TestRobustnessAnalysisLauncher.cpp
  TestMultiVariantInputs.cpp
  )

add_executable(${MODULE_NAME} ${MODULE_SOURCES})

target_include_directories(${MODULE_NAME}
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
  )

target_link_libraries(${MODULE_NAME}
  dynawo_algorithms_Launcher
  dynawo_algorithms_Test

  Dynawo::dynawo_DataInterfaceFactory
  )

add_custom_target(${MODULE_NAME}-pre-tests
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/res
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)

add_custom_target(${MODULE_NAME}-tests
  COMMAND ${CMAKE_COMMAND} -E env ${runtime_tests_ENV}
    "DYNAWO_RESOURCES_DIR=${DYNAWO_HOME}/share"
    "DYNAWO_XSD_DIR=${DYNAWO_HOME}/share/xsd/"
    "DYNAWO_ALGORITHMS_LOCALE=en_GB"
    "DYNAWO_ALGORITHMS_XSD_DIR=${CMAKE_SOURCE_DIR}/sources/API/multipleJobs/xsd"
    "DYNAWO_INSTALL_DIR=${DYNAWO_HOME}"
    "DYNAWO_SCRIPTS_DIR=${DYNAWO_HOME}/sbin"
    "DYNAWO_DICTIONARIES=dictionaries_mapping"
    "DYNAWO_ADEPT_INSTALL_DIR=${DYNAWO_HOME}"
    "DYNAWO_INSTALL_OPENMODELICA=${DYNAWO_HOME}/OpenModelica"
    "OPENMODELICAHOME=${DYNAWO_HOME}/OpenModelica"  # enable building models with pre-compiled OpenModelica
    "DYNAWO_XERCESC_INSTALL_DIR=${DYNAWO_HOME}"
    "DYNAWO_SUITESPARSE_INSTALL_DIR=${DYNAWO_HOME}"
    "DYNAWO_SUNDIALS_INSTALL_DIR=${DYNAWO_HOME}"
    "DYNAWO_BOOST_HOME=${DYNAWO_HOME}"
    "DYNAWO_LIBXML_HOME=${DYNAWO_HOME}"
    "DYNAWO_NICSLU_INSTALL_DIR=${DYNAWO_HOME}"
    $<TARGET_FILE:${MODULE_NAME}>
  DEPENDS
    ${MODULE_NAME}-pre-tests
    ${MODULE_NAME}
  COMMENT "Running ${MODULE_NAME}...")

if(BUILD_TESTS_COVERAGE)
  set(EXTRACT_PATTERNS "'*/sources/Launcher/DYN*'")

  add_test_coverage(${MODULE_NAME}-tests "${EXTRACT_PATTERNS}")
endif()

if(BUILD_TESTS)
  add_test(${MODULE_NAME}-tests)
endif()
