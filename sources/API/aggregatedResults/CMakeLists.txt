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

set(API_AGGREGATEDRES_SHARED
  xsd/aggregatedResults.xsd
  )

install(FILES ${API_AGGREGATEDRES_SHARED} DESTINATION ${sharedir}/xsd/)

set(API_AGGREGATEDRES_SOURCES
  DYNAggrResXmlExporter.cpp
  )

add_library(dynawo_algorithms_API_AggregatedResults SHARED ${API_AGGREGATEDRES_SOURCES})

target_include_directories(dynawo_algorithms_API_AggregatedResults
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDEDIR_NAME}>
  )

target_link_libraries(dynawo_algorithms_API_AggregatedResults
  dynawo_algorithms_Common
  Dynawo::dynawo_Common
  XMLSAXFormatter${LibXML_LINK_SUFFIX}
  XMLSAXParser${LibXML_LINK_SUFFIX}
  )

install(TARGETS dynawo_algorithms_API_AggregatedResults EXPORT dynawo_algorithms-targets DESTINATION ${libdir})
