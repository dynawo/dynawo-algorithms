# -*- coding: utf-8 -*-

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

import os

test_cases = []
standardReturnCode = [0]
standardReturnCodeType = "ALLOWED"
forbiddenReturnCodeType = "FORBIDDEN"

#########################################
#   IEEE14 - Margin Calculation         #
#########################################

case_name = "IEEE14_MC"
case_description = "IEEE14 - Margin Calculation"
job_file = os.path.join(os.path.dirname(__file__), "MC", "IEEE14_MC.zip")

test_cases.append((case_name, case_description, "MC", job_file, -1, 10, True, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_file_div"
case_description = "IEEE14 - Margin Calculation input file with divergence"
job_file = os.path.join(os.path.dirname(__file__), "MC_file_div", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, -1, 10, False, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_file"
case_description = "IEEE14 - Margin Calculation input file"
job_file = os.path.join(os.path.dirname(__file__), "MC_file", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, -1, 10, False, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_file_local"
case_description = "IEEE14 - Local margin Calculation"
job_file = os.path.join(os.path.dirname(__file__), "MC_file_local", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, -1, 10, False, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_FAIL_AT_0"
case_description = "IEEE14 - Margin Calculation fails at 0"
job_file = os.path.join(os.path.dirname(__file__), "MC_Fault", "IEEE14_MC_Fault.zip")

test_cases.append((case_name, case_description, "MC", job_file, -1, 1, True, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_specific_load_var"
case_description = "IEEE14 - Load variation at a specific level"
job_file = os.path.join(os.path.dirname(__file__), "MC_specific_load_var", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, 10, 1, False, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC_LOCAL_FAIL_AT_0"
case_description = "IEEE14 - Margin Calculation fails at 0"
job_file = os.path.join(os.path.dirname(__file__), "MC_Fault_local", "IEEE14_MC_Fault.zip")

test_cases.append((case_name, case_description, "MC", job_file, -1, 10, True, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC empty scenario list"
case_description = "IEEE14 - empty scenario list"
job_file = os.path.join(os.path.dirname(__file__), "MC_no_scenario", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, -1, 1, False, standardReturnCodeType, standardReturnCode))

case_name = "IEEE14_MC failing load increase"
case_description = "IEEE14 - failing load increase"
job_file = os.path.join(os.path.dirname(__file__), "MC_failing_load_increase", "fic_MULTIPLE.xml")

test_cases.append((case_name, case_description, "MC", job_file, -1, 1, False, standardReturnCodeType, standardReturnCode))

#########################################
#   IEEE14 - Systematic Analysis        #
#########################################

case_name = "IEEE14_SA"
case_description = "IEEE14 - Systematic Analysis"
job_file = os.path.join(os.path.dirname(__file__), "SA", "IEEE14_SA.zip")

test_cases.append((case_name, case_description, "SA", job_file, -1, 5, True, standardReturnCodeType, standardReturnCode))

#########################################
#        IEEE14 - Simulation            #
#########################################

case_name = "IEEE14_CS"
case_description = "IEEE14 - Simulation"
job_file = os.path.join(os.path.dirname(__file__), "IEEE14_BlackBoxModels", "IEEE14.jobs")

test_cases.append((case_name, case_description, "CS", job_file, -1, 5, False, standardReturnCodeType, standardReturnCode))
