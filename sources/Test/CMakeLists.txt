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

cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# Avoid custom flags warning to be used to compile googletest
set(CMAKE_CXX_FLAGS_SAVE "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG_INITIAL}")

## Googletest dependency
set(GTEST_ROOT ${DYNAWO_HOME} CACHE PATH "Path where an installation of GoogleTest may already exist")
set(GMOCK_HOME ${DYNAWO_HOME} CACHE PATH "Path where an installation of GoogleTest may already exist")
find_package(GTest)
find_package(GMock)
if (NOT GTEST_FOUND OR NOT GMOCK_FOUND)
  message(STATUS "GTest or GMock not found : they will be downloaded")

  set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")
  set(gtest_force_shared_crt ON CACHE INTERNAL "Force shared crt")

  if(DEFINED ENV{DYNAWO_ALGORITHMS_GOOGLETEST_DOWNLOAD_URL})
    set(googletest_prefix_url $ENV{DYNAWO_ALGORITHMS_GOOGLETEST_DOWNLOAD_URL})
  else()
    set(googletest_prefix_url https://github.com/google/googletest/archive)
  endif()

  if(('${CMAKE_CXX_COMPILER_ID}' STREQUAL 'GNU') AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 11))
    set(googletest_version   1.12.1) # last version that supports CXX11
    set(googletest_md5       e82199374acdfda3f425331028eb4e2a)
  else()
    set(googletest_version   1.8.1)
    set(googletest_md5       2E6FBEB6A91310A16EFE181886C59596)
  endif()
  set(googletest_url       ${googletest_prefix_url}/release-${googletest_version}.tar.gz)

  FetchContent_Declare(
    googletest

    URL            ${googletest_url}
    URL_HASH       MD5=${googletest_md5}
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
  )

  FetchContent_GetProperties(googletest)
  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    # we do not want googletest to be installed with the rest of Dynawo-algo as googletest is used only for unit tests
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  add_library(GTest::gtest_main ALIAS gtest_main)
  add_library(GTest::gmock_main ALIAS gmock_main)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_SAVE}")

if(NOT TARGET GTest::gtest AND TARGET GTest::GTest)
  add_library(GTest::gtest UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_LIBRARY_DEBUG}")
    set_target_properties(GTest::gtest PROPERTIES
            IMPORTED_LOCATION "${GTEST_LIBRARY_DEBUG}")
  elseif(EXISTS "${GTEST_LIBRARY}")
    set_target_properties(GTest::gtest PROPERTIES
      IMPORTED_LOCATION "${GTEST_LIBRARY}")
  endif()
endif()
if(NOT TARGET GTest::gtest_main AND TARGET GTest::Main)
  add_library(GTest::gtest_main UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_MAIN_LIBRARY_DEBUG}")
    set_target_properties(GTest::gtest_main PROPERTIES
            IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY_DEBUG}")
  elseif(EXISTS "${GTEST_MAIN_LIBRARY}")
    set_target_properties(GTest::gtest_main PROPERTIES
      IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}")
  endif()
endif()

set(DYN_ALGO_TEST_SOURCES
  initXml.cpp
)

add_library(dynawo_algorithms_Test SHARED ${DYN_ALGO_TEST_SOURCES})
target_link_libraries(dynawo_algorithms_Test
  INTERFACE
    Dynawo::dynawo_Common
    Boost::iostreams
    Boost::filesystem
  PUBLIC
    GTest::gmock_main
    GTest::gtest_main
  PRIVATE
    XMLSAXParser${LibXML_LINK_SUFFIX}
    LibXml2::LibXml2
)
