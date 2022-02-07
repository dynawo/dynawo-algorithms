# Copyright (c) 2022, RTE (http://www.rte-france.com)
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
cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

project("dynawo-algorithms-3rd"
LANGUAGES C CXX
)

include(ExternalProject)

set(DOWNLOAD_DIR ${CMAKE_INSTALL_PREFIX}/src CACHE PATH "Directory where 3rd parties are downloaded.")
set(TMP_DIR ${CMAKE_INSTALL_PREFIX}/tmp CACHE PATH "Directory where 3rd parties temporary files are created.")

if(UNIX)
  set(GPERFTOOLS_URL https://github.com/gperftools/gperftools/releases/download/gperftools-2.6.1/gperftools-2.6.1.tar.gz)
  find_library(tcmalloc NAMES "tcmalloc" PATHS ${CMAKE_INSTALL_PREFIX}/gperftools/lib)
  if (NOT tcmalloc)
    ExternalProject_Add(gperftools
            INSTALL_DIR         ${CMAKE_INSTALL_PREFIX}/gperftools
            DOWNLOAD_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/gperftools
            TMP_DIR             ${TMP_DIR}
            STAMP_DIR           ${DOWNLOAD_DIR}/gperftools-stamp
            SOURCE_DIR          ${DOWNLOAD_DIR}/gperftools
            URL                 ${GPERFTOOLS_URL}
            BUILD_IN_SOURCE     1

            CONFIGURE_COMMAND   <SOURCE_DIR>/configure
                                "CC=${CMAKE_C_COMPILER}"
                                "CXX=${CMAKE_CXX_COMPILER}"
                                "CXXFLAGS=-fno-strict-aliasing -DTCMALLOC_LARGE_PAGES -O3"
                                "CFLAGS=-fno-strict-aliasing -DTCMALLOC_LARGE_PAGES -O3"
                                --prefix=<INSTALL_DIR>
                                --disable-static
                                --disable-libunwind
    )
  endif()
endif()
