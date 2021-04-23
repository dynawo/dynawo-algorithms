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
CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)

# Add custom cmake modules to the path
set(DYNAWO_ALGORITHMS_CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${DYNAWO_ALGORITHMS_CMAKE_MODULE_PATH})
set(LIBRARY_TYPE SHARED CACHE STRING "Choose either STATIC or SHARED.")

option(BUILD_TESTS "Choose to build unit tests")
option(BUILD_TESTS_COVERAGE "Choose to build tests coverage")

# Project dynawo-algorithms
PROJECT(dynawo-algorithms)
SET(DYNAWO_ALGORITHMS_VERSION_MAJOR 1)
SET(DYNAWO_ALGORITHMS_VERSION_MINOR 3)
SET(DYNAWO_ALGORITHMS_VERSION_PATCH 0)
SET(DYNAWO_ALGORITHMS_VERSION_STRING ${DYNAWO_ALGORITHMS_VERSION_MAJOR}.${DYNAWO_ALGORITHMS_VERSION_MINOR}.${DYNAWO_ALGORITHMS_VERSION_PATCH})

# Set repertory name
SET(BINDIR_NAME bin)
SET(SHAREDIR_NAME share)
SET(SBINDIR_NAME sbin)
SET(INCLUDEDIR_NAME include)
SET(LIBDIR_NAME lib)
SET(DOCDIR_NAME doxygen)

# Set some install directories variables
SET(INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
SET(libdir ${INSTALL_PREFIX}/${LIBDIR_NAME})
SET(includedir ${INSTALL_PREFIX}/${INCLUDEDIR_NAME})
SET(docdir ${INSTALL_PREFIX}/${DOCDIR_NAME})
SET(bindir ${INSTALL_PREFIX}/${BINDIR_NAME})
SET(sharedir ${INSTALL_PREFIX}/${SHAREDIR_NAME})
SET(sbindir ${INSTALL_PREFIX}/${SBINDIR_NAME})

# If C++11 is enabled, check flags availability
SET(CXX11_ENABLED ${CXX11_ENABLED} CACHE BOOL "")
IF(CXX11_ENABLED)
    # Test if a C++11 flag is supported
    INCLUDE(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    IF(COMPILER_SUPPORTS_CXX11)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DLANG_CXX11")
    ELSEIF(COMPILER_SUPPORTS_CXX0X)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -DLANG_CXX0X")
    ELSE()
        MESSAGE(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Disabling C++11.")
    ENDIF()
ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
ENDIF()

# Add specific options for compiler
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # See all warnings in Debug mode
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -fdiagnostics-show-option -Wno-unused-local-typedefs")
  # -Wswitch-enum : Warn whenever a switch statement has an index of enumerated type and lacks a case for one or more of the named codes of that enumeration
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wswitch-enum -Wunused")
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Weverything")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-conditional-uninitialized")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-conversion")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-covered-switch-default")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-documentation")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-documentation-unknown-command")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-double-promotion")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-exit-time-destructors")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-extra-semi")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-float-conversion")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-float-equal")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-global-constructors")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-gnu-zero-variadic-macro-arguments")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-mismatched-tags")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-noreturn")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-prototypes")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-missing-variable-declarations")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-non-virtual-dtor")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-old-style-cast")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-padded")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-reserved-id-macro")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-return-type")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-shadow")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-shorten-64-to-32")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-sign-conversion")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-sometimes-uninitialized")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-undef")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-undefined-func-template")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-break")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-loop-increment")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unreachable-code-return")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-exception-parameter")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-macros")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-private-field")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-weak-vtables")
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
  IF(CXX11_ENABLED)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-local-type-template-args")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-pedantic")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated")
  ELSE()
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-pedantic")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-variadic-macros")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++11-extensions")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++11-long-long")
  ENDIF()
ENDIF()

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
endif()
# Tell BOOST to disable auto linking
add_definitions(-DBOOST_ALL_NO_LIB)

if(BUILD_TESTS OR BUILD_TESTS_COVERAGE)
  find_package(Boost 1.64 REQUIRED
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
  if(CXX11_ENABLED)
    set(Boost_RequiredVersion  "1.70")
  else()
    set(Boost_RequiredVersion  "1.64")
  endif()
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
FIND_PACKAGE(SuiteSparse REQUIRED)

# NICSLU
FIND_PACKAGE(NICSLU)

# Sundials
FIND_PACKAGE(Sundials REQUIRED)

#libZIP
FIND_PACKAGE(libZIP 1.3.0 REQUIRED)

# Transitive dependencies
FIND_PACKAGE(XercesC 3.2.2 REQUIRED)

#libXML
FIND_PACKAGE(libXML 0.2.4 REQUIRED)

#libIIDM
if(CXX11_ENABLED)
  find_package(LibXml2 "2.9" REQUIRED)
  find_package(LibIIDM "1.3.2" REQUIRED)
else()
FIND_PACKAGE(libIIDM 0.2.8 REQUIRED
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
FIND_PACKAGE(Dynawo 1.3.0 REQUIRED)

# Hack until libxml is not thread-safe
FIND_PACKAGE(XercesC REQUIRED)
# end of hack

FIND_PACKAGE(OpenMP)
IF (OPENMP_FOUND)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -DWITH_OPENMP")
ENDIF()

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
  target_link_libraries(dynawo_algorithms_Test INTERFACE
          Dynawo::dynawo_Common
          dynawo_algorithms_Test_tools
          GTest::gmock_main
          GTest::gtest_main
          Boost::iostreams)
endif()

SET(CMAKE_SKIP_RPATH TRUE)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/sources/config_algorithms.h.in ${CMAKE_CURRENT_SOURCE_DIR}/sources/config_algorithms.h @ONLY)

INCLUDE(cpplint/cpplint.cmake)

ADD_SUBDIRECTORY(sources)
ADD_SUBDIRECTORY(doxygen)
