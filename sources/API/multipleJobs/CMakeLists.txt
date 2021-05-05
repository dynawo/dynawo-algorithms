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

set(API_MULTIPLEJOBS_SHARED
  xsd/multipleJobs.xsd
  )

install(FILES ${API_MULTIPLEJOBS_SHARED} DESTINATION ${sharedir}/xsd/)

set(API_MULTIPLEJOBS_SOURCES
  DYNMultipleJobs.cpp
  DYNMultipleJobsFactory.cpp
  DYNMultipleJobsXmlHandler.cpp
  )

add_library(dynawo_algorithms_API_MultipleJobs ${LIBRARY_TYPE} ${API_MULTIPLEJOBS_SOURCES})

target_include_directories(dynawo_algorithms_API_MultipleJobs
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDEDIR_NAME}>
  )

target_link_libraries(dynawo_algorithms_API_MultipleJobs
  dynawo_algorithms_Common
  XMLSAXFormatter${LibXML_LINK_SUFFIX}
  XMLSAXParser${LibXML_LINK_SUFFIX}
  )

install(TARGETS dynawo_algorithms_API_MultipleJobs EXPORT dynawo_algorithms-targets DESTINATION ${libdir})
