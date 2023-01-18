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

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel (CMake defaults)")

# Project dynawo-algorithms
project(dynawo-algorithms)
set(DYNAWO_ALGORITHMS_VERSION_MAJOR 1)
set(DYNAWO_ALGORITHMS_VERSION_MINOR 5)
set(DYNAWO_ALGORITHMS_VERSION_PATCH 0)
set(DYNAWO_ALGORITHMS_VERSION_STRING ${DYNAWO_ALGORITHMS_VERSION_MAJOR}.${DYNAWO_ALGORITHMS_VERSION_MINOR}.${DYNAWO_ALGORITHMS_VERSION_PATCH})

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)    # export all symbols from dll on Windows
set(CMAKE_ENABLE_EXPORTS ON)                # export all symbols from exe on Windows
set(CMAKE_POSITION_INDEPENDENT_CODE ON)     # set -fPIC on Linux

# Set repertory name
set(BINDIR_NAME bin)
set(SHAREDIR_NAME share)
set(INCLUDEDIR_NAME include)
set(LIBDIR_NAME lib)
set(DOCDIR_NAME doxygen)

get_filename_component(DYNAWO_HOME ${DYNAWO_HOME} ABSOLUTE)
get_filename_component(DYNAWO_ALGORITHMS_HOME ${DYNAWO_ALGORITHMS_HOME} ABSOLUTE)
get_filename_component(DYNAWO_ALGORITHMS_THIRD_PARTY_DIR ${DYNAWO_ALGORITHMS_THIRD_PARTY_DIR} ABSOLUTE)

list(APPEND CMAKE_MODULE_PATH "${DYNAWO_HOME}/share/cmake")

list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}/share/cmake")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}/share")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}")

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601)
else()
  set(FORCE_CXX11_ABI OFF CACHE BOOL "Choose either ON or OFF.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  if(FORCE_CXX11_ABI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
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
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-local-type-template-args")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-c++98-compat-pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated")
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

set(BUILD_SHARED_LIBS ON)
set(Boost_USE_STATIC_RUNTIME False)
add_definitions(-DBOOST_ALL_DYN_LINK)
# Tell BOOST to disable auto linking
add_definitions(-DBOOST_ALL_NO_LIB)

set(Boost_RequiredVersion  "1.70")
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
set(DYNAWO_USE_LIBXML2 "Y")
find_package(LibXml2 "2.9" REQUIRED)
find_package(LibIIDM "1.4.0" REQUIRED)

#LibArchive
find_package(LibArchive 2.8.0 REQUIRED)

#ZLIB
find_package(ZLIB 1.2.3 REQUIRED)

#DYNAWO
find_package(Dynawo 1.5.0 REQUIRED)

# Hack until libxml is not thread-safe
find_package(XercesC REQUIRED)
# end of hack

set(MPI_HOME ${DYNAWO_ALGORITHMS_THIRD_PARTY_DIR}/mpich)
find_package(MPI REQUIRED)
if(MPI_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_FLAGS}")
endif()

if(BUILD_TESTS_COVERAGE)
  include(TestCoverage)
endif()

if(BUILD_TESTS)
  include(UnitTest)
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
