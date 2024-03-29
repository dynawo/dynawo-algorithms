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

set(SRC_DIR ${PROJECT_SOURCE_DIR})
get_filename_component(DOC_DIR ${CMAKE_INSTALL_PREFIX}/${DOCDIR_NAME} ABSOLUTE)

# add a target to generate API documentation with Doxygen
find_package(Doxygen 1.8)
if(DOXYGEN_FOUND)
    configure_file(doxygen.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)
    add_custom_target(doc
                      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating dynawo-algorithms documentation with Doxygen" VERBATIM
                      )
endif(DOXYGEN_FOUND)
