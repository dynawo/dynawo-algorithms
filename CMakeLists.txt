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

# Minimum required (for ExternalProject_Add)
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Add custom cmake modules to the path
set(DYNAWO_ALGORITHMS_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DYNAWO_ALGORITHMS_CMAKE_MODULE_PATH})

option(BUILD_TESTS "Choose to build unit tests")
option(BUILD_TESTS_COVERAGE "Choose to build tests coverage")

# Project dynawo-algorithms
project(dynawo-algorithms)
set(DYNAWO_ALGORITHMS_VERSION_MAJOR 1)
set(DYNAWO_ALGORITHMS_VERSION_MINOR 3)
set(DYNAWO_ALGORITHMS_VERSION_PATCH 0)
set(DYNAWO_ALGORITHMS_VERSION_STRING ${DYNAWO_ALGORITHMS_VERSION_MAJOR}.${DYNAWO_ALGORITHMS_VERSION_MINOR}.${DYNAWO_ALGORITHMS_VERSION_PATCH})

# Set repertory name
set(BINDIR_NAME bin)
set(SHAREDIR_NAME share)
set(INCLUDEDIR_NAME include)
set(LIBDIR_NAME lib)
set(DOCDIR_NAME doxygen)

get_filename_component(DYNAWO_HOME ${DYNAWO_HOME} ABSOLUTE)

# If C++11 is enabled, check flags availability
set(CXX11_ENABLED ${CXX11_ENABLED} CACHE BOOL "")
set(FORCE_CXX11_ABI OFF CACHE BOOL "Choose either ON or OFF.")
if(CXX11_ENABLED)
  # Test if a C++11 flag is supported
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
  check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
  if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DLANG_CXX11")
    if(FORCE_CXX11_ABI)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
    endif()
  elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -DLANG_CXX0X")
  else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Disabling C++11.")
    set(CXX11_ENABLED OFF CACHE BOOL "" FORCE)
  endif()
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
endif()


# Add specific options for compiler
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # See all warnings in Debug mode
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -fdiagnostics-show-option -Wno-unused-local-typedefs")
  # -Wswitch-enum : Warn whenever a switch statement has an index of enumerated type and lacks a case for one or more of the named codes of that enumeration
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wswitch-enum -Wunused")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Weverything")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-conditional-uninitialized")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-conversion")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-covered-switch-default")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-documentation")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-documentation-unknown-command")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-double-promotion")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-exit-time-destructors")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-extra-semi")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-float-conversion")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-float-equal")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-global-constructors")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-gnu-zero-variadic-macro-arguments")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-mismatched-tags")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-noreturn")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-prototypes")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-variable-declarations")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-non-virtual-dtor")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-old-style-cast")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-padded")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-reserved-id-macro")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-return-type")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-shadow")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-shorten-64-to-32")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-sign-conversion")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-sometimes-uninitialized")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-undef")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-undefined-func-template")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-break")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-loop-increment")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-return")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-exception-parameter")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-macros")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-private-field")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-weak-vtables")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  if(CXX11_ENABLED)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-local-type-template-args")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-variadic-macros")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++11-extensions")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++11-long-long")
  endif()
endif()

# Python Interpreter
find_package(PythonInterp REQUIRED)
if (DEFINED DYNAWO_PYTHON_COMMAND AND NOT DYNAWO_PYTHON_COMMAND STREQUAL "")
  set(PYTHON_EXECUTABLE ${DYNAWO_PYTHON_COMMAND})
  separate_arguments(PYTHON_EXECUTABLE)
endif()
message(STATUS "Python command used: ${PYTHON_EXECUTABLE}")

# Boost
if(BOOST_ROOT_DEFAULT STREQUAL "true")
  set(Boost_NO_SYSTEM_PATHS FALSE)
else()
  set(Boost_NO_SYSTEM_PATHS TRUE)
  # Use Boost_NO_BOOST_CMAKE to ensure that find_package uses MODULE mode and takes Boost_NO_SYSTEM_PATHS into account, instead of CONFIG mode
  set(Boost_NO_BOOST_CMAKE ON)
endif()
# Tell BOOST to disable auto linking
add_definitions(-DBOOST_ALL_NO_LIB)

if(CXX11_ENABLED)
  set(Boost_RequiredVersion  "1.70")
else()
  set(Boost_RequiredVersion  "1.64")
endif()
if(BUILD_TESTS OR BUILD_TESTS_COVERAGE)
  find_package(Boost "${Boost_RequiredVersion}" REQUIRED
    COMPONENTS
      filesystem
      program_options
      serialization
      system
      log
      log_setup
      iostreams
    )
else()
  find_package(Boost "${Boost_RequiredVersion}" REQUIRED
    COMPONENTS
      filesystem
      program_options
      serialization
      system
      log
      log_setup
    )
endif()

# KLU
find_package(SuiteSparse REQUIRED)

# NICSLU
find_package(NICSLU)

# Sundials
find_package(Sundials REQUIRED)

#libZIP
find_package(libZIP 1.3.0 REQUIRED)

# Transitive dependencies
find_package(XercesC 3.2.2 REQUIRED)

#libXML
find_package(libXML 0.2.4 REQUIRED)

#libIIDM
if(NOT CXX11_ENABLED)
  set(USE_LEGACY_IIDM YES)
else()
  file(STRINGS ${DYNAWO_HOME}/dynawoEnv.txt DYNAWO_ENV REGEX "^[ ]*DYNAWO_USE_LEGACY_IIDM[ ]*=")
  string(REGEX REPLACE "^[ ]*DYNAWO_USE_LEGACY_IIDM[ ]*=[ ]*\"([^\"]*)\"" "\\1" USE_LEGACY_IIDM ${DYNAWO_ENV})
  unset(DYNAWO_ENV)
endif()

if(NOT USE_LEGACY_IIDM)
  add_definitions(-DUSE_POWSYBL)
  find_package(LibXml2 "2.9" REQUIRED)
  find_package(LibIIDM "1.4.0" REQUIRED)
else()
  find_package(libIIDM 0.2.8 REQUIRED
    COMPONENTS
      core
      xml
      ext-activeseason
      ext-activeseason-xml
      ext-busbarsectionposition
      ext-busbarsectionposition-xml
      ext-connectableposition
      ext-connectableposition-xml
      ext-currentlimitsperseason
      ext-currentlimitsperseason-xml
      ext-generatoractivepowercontrol
      ext-generatoractivepowercontrol-xml
      ext-hvdcangledroopactivepowercontrol
      ext-hvdcangledroopactivepowercontrol-xml
      ext-hvdcoperatoractivepowerrange
      ext-hvdcoperatoractivepowerrange-xml
      ext-generatorentsoecategory
      ext-generatorentsoecategory-xml
      ext-generatorstartup
      ext-generatorstartup-xml
      ext-loaddetail
      ext-loaddetail-xml
      ext-standbyautomaton
      ext-standbyautomaton-xml
    )
endif()

#LibArchive
find_package(LibArchive 2.8.0 REQUIRED)

#ZLIB
find_package(ZLIB 1.2.3 REQUIRED)

#DYNAWO
find_package(Dynawo 1.3.0 REQUIRED)

# Hack until libxml is not thread-safe
find_package(XercesC REQUIRED)
# end of hack

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -DWITH_OPENMP")
endif()

if(BUILD_TESTS_COVERAGE)
  include(TestCoverage)
endif()

if(BUILD_TESTS)
  include(UnitTest)
endif()

if(BUILD_TESTS OR BUILD_TESTS_COVERAGE)
  find_package(GTest REQUIRED)

  if(NOT TARGET GTest::gtest AND TARGET GTest::GTest)
    add_library(GTest::gtest UNKNOWN IMPORTED)
    if(EXISTS "${GTEST_LIBRARY}")
      set_target_properties(GTest::gtest PROPERTIES
        IMPORTED_LOCATION "${GTEST_LIBRARY}")
    endif()
  endif()
  if(NOT TARGET GTest::gtest_main AND TARGET GTest::Main)
    add_library(GTest::gtest_main UNKNOWN IMPORTED)
    if(EXISTS "${GTEST_MAIN_LIBRARY}")
      set_target_properties(GTest::gtest_main PROPERTIES
        IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}")
    endif()
  endif()
  find_package(GMock REQUIRED)

  add_library(dynawo_algorithms_Test INTERFACE)
  target_link_libraries(dynawo_algorithms_Test
    INTERFACE
      Dynawo::dynawo_Common
      dynawo_algorithms_Test_utils
      GTest::gmock_main
      GTest::gtest_main
      Boost::iostreams
      Boost::filesystem
    )
endif()

set(CMAKE_SKIP_RPATH TRUE)

install(EXPORT dynawo_algorithms-targets
  NAMESPACE DynawoAlgorithms::
  FILE dynawoalgorithms-config.cmake
  DESTINATION ${SHAREDIR_NAME}
  )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/dynawoalgorithms-config-version.cmake
  VERSION ${DYNAWO_ALGORITHMS_VERSION_STRING}
  COMPATIBILITY AnyNewerVersion
  )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dynawoalgorithms-config-version.cmake
  DESTINATION ${SHAREDIR_NAME}
  )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/sources/config_algorithms.h.in ${CMAKE_CURRENT_SOURCE_DIR}/sources/config_algorithms.h @ONLY)

include(cpplint/cpplint.cmake)

add_subdirectory(sources)
add_subdirectory(doxygen)
