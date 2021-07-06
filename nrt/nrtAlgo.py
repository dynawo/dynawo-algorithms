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
import sys
import shutil
import time
from optparse import OptionParser
import subprocess
import signal
import datetime
from zipfile import ZipFile
import contextlib

nrt_dir = os.environ["DYNAWO_HOME"]
sys.path.append(os.path.join(nrt_dir,"sbin","nrt"))
import nrt
nrtDiff_dir = os.environ["DYNAWO_NRT_DIFF_DIR"]
sys.path.append(nrtDiff_dir)
import nrtDiff
    
class TestCaseAlgo:
    def __init__(self, case, case_name, case_description, job_type, jobs_file, variation, estimated_computation_time, zip_inputs, return_code_type, expected_return_codes):
        self.case_ = case
        self.jobs_file_ = jobs_file
        self.job_type = job_type
        self.estimated_computation_time_ = estimated_computation_time # rough computation time estimate (in s) in order to sort test cases
        self.time_ = 0
        self.code_ = 0
        self.ok_ = True
        self.tooLong_ = False
        self.command_ = ""
        self.name_ = case_name
        self.description_ = case_description
        self.jobs_ = []
        self.return_code_type_ = return_code_type # ALLOWED or FORBIDDEN
        self.process_return_codes_ = expected_return_codes
        self.diff_ = None
        self.diff_messages_ = None
        self.zip_inputs = zip_inputs
        self.variation = variation

    def launch(self, timeout):
        start_time = time.time()
        command = []
        
        if self.zip_inputs :
            with contextlib.closing(ZipFile(self.jobs_file_, 'w')) as zipObj:
                path_to_zip = os.path.join(os.path.dirname(self.jobs_file_), "files")
                for file in os.listdir(path_to_zip):
                    if ".dyd" in file or ".jobs" in file or ".par" in file or ".iidm" in file or ".crv" in file or "fic_MULTIPLE.xml" in file:
                        zipObj.write(os.path.join(path_to_zip, file), file)
        

        if os.getenv("DYNAWO_ENV_DYNAWO") is None:
            print("environment variable DYNAWO_ENV_DYNAWO needs to be defined")
            sys.exit(1)
        env_dynawo = os.environ["DYNAWO_ENV_DYNAWO"]

        if self.job_type == "SA" or self.job_type == "MC":
            directory = os.path.dirname(self.jobs_file_)
            if ".zip" in self.jobs_file_:
                output_file = os.path.join(directory,"result.zip")
            else:
                output_file = os.path.join(directory,"aggregatedResults.xml")
            variation = "-1"
            if self.variation > 0:
                command = [env_dynawo, self.job_type, "--input", os.path.basename(self.jobs_file_), "--directory", directory, "--output", os.path.basename(output_file), "--variation", str(self.variation)]
            else:
                command = [env_dynawo, self.job_type, "--input", os.path.basename(self.jobs_file_), "--directory", directory, "--output", os.path.basename(output_file)]
        else:
            command = [env_dynawo, self.job_type, "--input", self.jobs_file_]

        commandStr = " ".join(command) # for traces
        self.command_= commandStr
        print (self.name_)

        if(timeout > 0):
            signal.signal(signal.SIGALRM, alarm_handler)
            signal.alarm(int(timeout)*60) # in seconds

        code = 0
        errorTooLong = -150
        p = subprocess.Popen(commandStr, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        try:
            output, errors = p.communicate()
            signal.alarm(0)
            code = p.wait()
        except nrt.Alarm:
            kill_subprocess(p.pid)
            code = errorTooLong

        self.code_ = code
        if code == errorTooLong:
            self.tooLong_ = True
            self.ok_ = False
        else:
            self.tooLong_ = False
            self.ok_ = True
            if (self.return_code_type_ == "FORBIDDEN") and (self.code_ in self.process_return_codes_):
                self.ok_ = False

            if (self.return_code_type_ == "ALLOWED") and (not self.code_ in self.process_return_codes_):
                self.ok_ = False

        if self.ok_ and ".zip" in self.jobs_file_ and (self.job_type == "SA" or self.job_type == "MC"):
            with contextlib.closing(ZipFile(output_file, 'r')) as zipObj:
                zipObj.extractall(directory)
            
        end_time = time.time()
        self.time_ = end_time - start_time

        # Conversion from csv to html if curves file csv/xml
        for job in self.jobs_:
            try:
                curvesOutput = os.path.join(os.path.dirname(job.curves_), "curvesOutput")
                if job.curvesType_ == CURVES_TYPE_CSV:
                    readCsvToHtml(job.curves_,curvesOutput,True, False)
                elif job.curvesType_ == CURVES_TYPE_XML:
                    readXmlToHtml(job.curves_,curvesOutput,True, False)
                job.hasCurves_ = True
            except:
                job.hasCurves_ = False
                pass

    ##
    # Check whether a test case led to satisfactory results
    def gives_satisfactory_results(self):
        if not self.ok_:
            return False

        if self.diff_ in nrtDiff.diff_error_statuses (True):
            return False

        return True


##
# The main function usually called for non-regression tests
def main():
    usage=u""" Usage: %prog

    Script to launch non-regression test
    """

    options = {}
    options[('-t', '--timeout')] = {'dest': 'timeout',
                                    'help': 'Timeout of a NRT (in minutes)'}

    options[('-n', '--name')] = {'dest': 'directory_names', 'action':'append',
                                    'help': 'name filter to only run some non-regression tests'}

    options[('-p', '--pattern')] = {'dest': 'directory_patterns', 'action' : 'append',
                                    'help': 'regular expression filter to only run some non-regression tests'}

    parser = OptionParser(usage)
    for param, option in options.items():
        parser.add_option(*param, **option)
    (options, args) = parser.parse_args()

    global NRT

    log_message = "Running non-regression tests"

    timeout = 0
    if options.timeout > 0:
        timeout = options.timeout
        log_message += " with " + timeout + "s timeout"

    with_directory_name = False
    directory_names = []
    if (options.directory_names is not None) and (len(options.directory_names) > 0):
        with_directory_name = True
        directory_names = options.directory_names
        if options.timeout > 0:
            log_message += " and"
        else:
            log_message += " with"

        for dir_name in directory_names:
            log_message += " '" + dir_name + "'"

        log_message += " name filter"

    directory_patterns = []
    if (options.directory_patterns is not None) and  (len(options.directory_patterns) > 0):
        directory_patterns = options.directory_patterns
        if options.timeout > 0 or with_directory_name:
            log_message += " and"
        else:
            log_message += " with"

        for pattern in directory_patterns:
            log_message += " '" + pattern + "'"

        log_message += " pattern filter"

    print (log_message)

    # Create NRT structure
    NRT = nrt.NonRegressionTest(timeout)

    # Test whether output repository exists, otherwise create it
    if os.path.isdir(nrt.output_dir_all_nrt) == False:
        os.mkdir(nrt.output_dir_all_nrt)

    # Outputs repository is deleted and then created
    if os.path.isdir(nrt.output_dir) == True:
        shutil.rmtree(nrt.output_dir)

    os.mkdir(nrt.output_dir)

    # Copy resources in outputs repository
    shutil.copytree(nrt.resources_dir, os.path.join(nrt.output_dir,"resources"))

    # Delete former results if they exists
    NRT.clear(nrt.html_output)

    # Loop on testcases
    numCase = 0
    for case_dir in os.listdir(nrt.data_dir):
        case_path = os.path.join(nrt.data_dir, case_dir)
        if os.path.isdir(case_path) == True and \
            case_dir != ".svn" : # In order to check that we are dealing with a repository and not a file, .svn repository is filtered

            # Get needed info to build object TestCase
            sys.path.append(case_path)
            try:
                import cases
                sys.path.remove(case_path) # Remove from path because all files share the same name

                for case_name, case_description, job_type, job_file, variation, estimated_computation_time, zip_inputs, return_code_type, expected_return_codes in cases.test_cases:
                    relative_job_dir = os.path.relpath (os.path.dirname (job_file), nrt.data_dir)
                    keep_job = True

                    if (len (directory_names) > 0):
                        for dir_name in directory_names:
                            if (dir_name not in relative_job_dir):
                                keep_job = False
                                break

                    if keep_job and (len (directory_patterns) > 0):
                        for pattern in directory_patterns:
                            if  (re.search(pattern, relative_job_dir) is None):
                                keep_job = False
                                break

                    if keep_job :
                        case = "case_" + str(numCase)
                        numCase += 1
                        current_test = TestCaseAlgo(case, case_name, case_description, job_type, job_file, variation, estimated_computation_time, zip_inputs, return_code_type, expected_return_codes)
                        NRT.addTestCase(current_test)

                del sys.modules['cases'] # Delete load module in order to load another module with the same name
            except:
                pass

    if len (NRT.test_cases_) == 0:
        print('No non-regression tests to run')
        sys.exit()

    try:
        # remove the diff notification file
        NRT_RESULT_FILE = os.path.join(nrt.output_dir, "nrt_nok.txt")
        if (os.path.isfile(NRT_RESULT_FILE)):
            try:
                os.remove (NRT_RESULT_FILE)
            except:
                print("Failed to remove notification file. Unable to conduct nrt")
                sys.exit(1)
        start_time = time.time()
        NRT.launchCases()
        end_time = time.time()
        NRT.setRealTime(end_time - start_time)

        for case in NRT.test_cases_:
            if (not case.ok_):
                case.diff_ = nrtDiff.UNABLE_TO_CHECK
            else:
                (status, messages) = nrtDiff.DirectoryDiffReferenceDataJob (case.jobs_file_)
                case.diff_ = status
                case.diff_messages_ = messages

        # Export results as html
        NRT.exportHTML(nrt.html_output)
        if( NRT.number_of_nok_cases_ > 0):
            logFile = open(NRT_RESULT_FILE, "w")
            logFile.write("NRT NOK\n")
            logFile.write("Run on " + datetime.datetime.now().strftime("%Y-%m-%d at %Hh%M") + "\n")
            logFile.write(" Nb nok case : " + str(NRT.number_of_nok_cases_) + "\n")
            logFile.close()
        sys.exit(NRT.number_of_nok_cases_)
    except KeyboardInterrupt:
        exit()

if __name__ == "__main__":
    main()
