# -*- coding: utf-8 -*-

# Copyright (c) 2024, RTE (http://www.rte-france.com)
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
import sys
import shutil
import re


LOAD_INCREASE_FINAL_STATE_REGEX = re.compile('^loadIncreaseFinalState-[0-9]*.iidm$')

if not os.getenv("DYNAWO_ALGORITHMS_NRT_DIR"):
    print("environment variable DYNAWO_ALGORITHMS_NRT_DIR needs to be defined")
    sys.exit(1)

nrt_dir = os.path.join(os.getenv("DYNAWO_ALGORITHMS_NRT_DIR"), "data", "IEEE14")

zip_input_nrt_folders = list()
fic_multiple_nrt_folders = list()

for nrt_folder in os.listdir(nrt_dir):
    nrt_folder_path = os.path.join(nrt_dir, nrt_folder)
    if os.path.isdir(nrt_folder_path):
        for nrt_subfolder in os.listdir(nrt_folder_path):
            if nrt_subfolder == "files":
                zip_input_nrt_folders.append(nrt_folder_path)            
                break
        else:
            fic_multiple_nrt_folders.append(nrt_folder_path)

# Clean NRT folders which contains a 'files' folder
for zip_nrt_folder in zip_input_nrt_folders:
    for zip_nrt_content in os.listdir(zip_nrt_folder):
        zip_nrt_content_path = os.path.join(zip_nrt_folder, zip_nrt_content)
        if os.path.isfile(zip_nrt_content_path):
            os.remove(zip_nrt_content_path)
        elif (zip_nrt_content != "files" and zip_nrt_content != "reference"):
            shutil.rmtree(zip_nrt_content_path)
        else:
            pass  # don't remove the 'files' and 'references' folder

# Clean other NRT folders
for fic_multiple_nrt_folder in fic_multiple_nrt_folders:
    for fic_multiple_nrt_content in os.listdir(fic_multiple_nrt_folder):
        fic_multiple_nrt_content_path = os.path.join(fic_multiple_nrt_folder, fic_multiple_nrt_content)
        if os.path.isfile(fic_multiple_nrt_content_path):
            if fic_multiple_nrt_content in ["aggregatedResults.xml", "results.xml"] or \
                    fic_multiple_nrt_content.endswith(".dmp") or \
                    fic_multiple_nrt_content.endswith(".log") or \
                    LOAD_INCREASE_FINAL_STATE_REGEX.match(fic_multiple_nrt_content):
                os.remove(fic_multiple_nrt_content_path)
        elif fic_multiple_nrt_content != "reference":
            shutil.rmtree(fic_multiple_nrt_content_path)
        else:
            pass  # don't remove the 'reference' folder

# Delete the 'output' folder which contains the HTML report of the NRTs
nrt_output_dir = os.path.join(os.getenv("DYNAWO_ALGORITHMS_NRT_DIR"), "output")
if os.path.exists(nrt_output_dir):
    shutil.rmtree(nrt_output_dir)

# Delete every .pyc files
nrt_data_dir = os.path.join(os.getenv("DYNAWO_ALGORITHMS_NRT_DIR"), "data")
for nrt_subdir, _, _ in os.walk(nrt_data_dir):
    for nrt_file in os.listdir(nrt_subdir):
        if nrt_file.endswith(".pyc"):
            os.remove(os.path.join(nrt_subdir, nrt_file))

# Delete empty folders
for nrt_subdir, _, _ in os.walk(nrt_data_dir, topdown=False):
    if len(os.listdir(nrt_subdir)) == 0:
        os.rmdir(nrt_subdir)
