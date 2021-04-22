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

set(DYN_ALGO_TEST_UTILS_SOURCES
    initXml.cpp
    )

add_library(dynawo_algorithms_Test_utils SHARED ${DYN_ALGO_TEST_UTILS_SOURCES})

target_link_libraries(dynawo_algorithms_Test_utils
  PRIVATE
    XMLSAXParser${LibXML_LINK_SUFFIX}
)
if(CXX11_ENABLED)
  target_link_libraries(dynawo_algorithms_Test_utils
    PRIVATE
      LibXml2::LibXml2
  )
endif()

install(TARGETS dynawo_algorithms_Test_utils EXPORT dynawo-targets DESTINATION ${libdir})

