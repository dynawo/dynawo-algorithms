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

set(CPPLINT_PATH ${DYNAWO_ALGORITHMS_HOME}/cpplint/cpplint-wrapper.py)

if(MSVC)
  add_custom_target(cpplint ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CPPLINT_PATH} --modified --filter=-build/header_guard ${DYNAWO_ALGORITHMS_HOME})
else()
  add_custom_target(cpplint ALL
    COMMAND ${PYTHON_EXECUTABLE} ${CPPLINT_PATH} --modified ${DYNAWO_ALGORITHMS_HOME})
endif()
