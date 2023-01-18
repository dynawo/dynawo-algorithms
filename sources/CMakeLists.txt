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

set(DYN_ALGO_SOURCES
  main.cpp
  )

set(DYN_ALGO_INCLUDE_HEADERS
  config_algorithms.h
  gitversion_algorithms.h
  )

install(FILES ${DYN_ALGO_INCLUDE_HEADERS} DESTINATION ${INCLUDEDIR_NAME})

add_executable(dynawoAlgorithms ${DYN_ALGO_SOURCES})
target_link_libraries(dynawoAlgorithms
  dynawo_algorithms_Launcher
  LibXml2::LibXml2
  )

if(MSVC)
  install(PROGRAMS ${DYNAWO_ALGORITHMS_HOME}/util/windows/dynawo-algorithms.cmd DESTINATION .)
else()  # Linux
  install(PROGRAMS dynawo-algorithms.sh DESTINATION .)
endif()

set_target_properties(dynawoAlgorithms PROPERTIES VERSION ${DYNAWO_ALGORITHMS_VERSION_STRING})
install(TARGETS dynawoAlgorithms
  RUNTIME DESTINATION ${BINDIR_NAME}
  LIBRARY DESTINATION ${LIBDIR_NAME}
  ARCHIVE DESTINATION ${LIBDIR_NAME}
  )

# explicitly say that the executable depends on custom target
add_dependencies(dynawoAlgorithms revisiontag)

#obtain git revision
# a custom target that is always built
add_custom_target(revisiontag ALL)

# creates gitversion_algorithms.h using cmake script
add_custom_command(TARGET revisiontag
  COMMAND ${CMAKE_COMMAND}
    -DOUTPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DGIT_SOURCE_DIR=${DYNAWO_ALGORITHMS_HOME}
    -P ${DYNAWO_ALGORITHMS_CMAKE_MODULE_PATH}/getgitheadrevision.cmake
  )

add_subdirectory(API)
add_subdirectory(Common)
add_subdirectory(Launcher)

if(BUILD_TESTS OR BUILD_TESTS_COVERAGE)
  add_subdirectory(Test)

  if(MSVC)
    set(_lib_path "bin")
  else()
    set(_lib_path "lib")
  endif()
  set(_runtime_path
    $<TARGET_FILE_DIR:dynawo_algorithms_API_AggregatedResults>
    $<TARGET_FILE_DIR:dynawo_algorithms_API_MultipleJobs>
    $<TARGET_FILE_DIR:dynawo_algorithms_Common>
    $<TARGET_FILE_DIR:dynawo_algorithms_Launcher>
    $<TARGET_FILE_DIR:dynawo_algorithms_Test>
    $<$<TARGET_EXISTS:GTest::gmock_main>:$<TARGET_FILE_DIR:GTest::gmock_main>>
    $<$<TARGET_EXISTS:GTest::gtest_main>:$<TARGET_FILE_DIR:GTest::gtest_main>>
    ${GTEST_ROOT}/${_lib_path}
    ${GMOCK_HOME}/${_lib_path}
    ${DYNAWO_HOME}/${_lib_path}
  )
  if(DEFINED ENV{GTEST_ROOT})
    list(APPEND _runtime_path "$ENV{GTEST_ROOT}/${_lib_path}")
  endif()
  if(DEFINED ENV{GMOCK_HOME})
    list(APPEND _runtime_path "$ENV{GMOCK_HOME}/${_lib_path}")
  endif()
  list(REMOVE_DUPLICATES _runtime_path)
  unset(_lib_path)

  unset(runtime_tests_ENV)
  if(MSVC)
    list(APPEND _runtime_path
      ${DYNAWO_HOME}/OpenModelica/bin
      $ENV{PATH}
    )
    string(REPLACE ";" "\;" _runtime_path "${_runtime_path}")
    set(runtime_tests_ENV "PATH=${_runtime_path}")
  else()
    string(REPLACE ";" ":" _runtime_path "${_runtime_path}")
    set(runtime_tests_ENV
      "PATH=${DYNAWO_HOME}/OpenModelica/bin:$ENV{PATH}"
      "LD_LIBRARY_PATH=${_runtime_path}:$ENV{LD_LIBRARY_PATH}"
    )
  endif()
  unset(_runtime_path)

  add_subdirectory(API/aggregatedResults/test)
  add_subdirectory(API/multipleJobs/test)
  add_subdirectory(Common/test)
  add_subdirectory(Launcher/test)
endif()
