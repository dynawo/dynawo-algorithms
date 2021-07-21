#!/bin/bash
#
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
#

############################################################################################################################
#
# This script is meant to be used with a Dynawo distribution.
# It assumes Dynawo algorithms will be installed in the same directory of Dynawo.
# The following environnement variables can be set outside this script:
#
#    - DYNAWO_ALGORITHMS_LOCALE: the locale you want to use (fr_FR or en_GB)
#
###########################################################################################################################

error_exit() {
  RETURN_CODE=$?
  echo "${1:-"Unknown Error"}" 1>&2
  exit ${RETURN_CODE}
}

export_var_env() {
  local var=$@
  local name=${var%%=*}
  local value=${var#*=}

  if eval "[ \$$name ]"; then
    eval "value=\${$name}"
    return
  fi
  export $name="$value"
}

usage="Usage: `basename $0` [option] -- program to launch Dynawo simulation

where [option] can be:
    simulationType             could be:
                                        CS ([args])  call Dynawo's launcher with given arguments setting LD_LIBRARY_PATH correctly
                                        SA ([args])  call a dynamic systematic analysis
                                        MC ([args])  call a margin calculation
    --version                  show Dynawo version
    --help                     show this message"

setEnv() {
  export_var_env DYNAWO_ALGORITHMS_INSTALL_DIR=$(dirname $(dirname $(readlink -f $0)))
  export_var_env DYNAWO_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR

  export_var_env DYNAWO_ADEPT_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR
  export_var_env DYNAWO_SUNDIALS_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR
  export_var_env DYNAWO_SUITESPARSE_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR
  export_var_env DYNAWO_NICSLU_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR
  export_var_env DYNAWO_LIBIIDM_INSTALL_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR

  export_var_env DYNAWO_IIDM_EXTENSION=$DYNAWO_LIBIIDM_INSTALL_DIR/lib/libdynawo_DataInterfaceIIDMExtension.so
  export_var_env DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_LIBIIDM_INSTALL_DIR/lib

  export_var_env DYNAWO_ALGORITHMS_LOCALE=en_GB # or fr_FR

  export_var_env DYNAWO_LOCALE=$DYNAWO_ALGORITHMS_LOCALE
  export_var_env DYNAWO_USE_XSD_VALIDATION=false
  export_var_env DYNAWO_DDB_DIR=$DYNAWO_INSTALL_DIR/ddb
  export_var_env DYNAWO_RESOURCES_DIR=$DYNAWO_INSTALL_DIR/share:$DYNAWO_INSTALL_DIR/share/xsd
  export_var_env DYNAWO_DICTIONARIES=dictionaries_mapping

  export IIDM_XML_XSD_PATH=${DYNAWO_LIBIIDM_INSTALL_DIR}/share/iidm/xsd/

  # set LD_LIBRARY_PATH
  export LD_LIBRARY_PATH=$DYNAWO_ALGORITHMS_INSTALL_DIR/lib:$LD_LIBRARY_PATH
}

find_and_call_timeline() {
  if [ ! -d "$1" ]; then
    return 1
  fi
find $1 -name $2 | while read filename; do
    echo "Processing file '$filename'"
    python $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/timelineFilter.py --timelineFile $filename
    RESULT_FILE=`dirname $filename`
    RESULT_FILE=$RESULT_FILE/$3
    mv $RESULT_FILE $filename
  done
}

filter_timeline() {
  find_and_call_timeline $1 "timeline.log" "filtered_timeline.log"
  find_and_call_timeline $1 "timeline.xml" "filtered_timeline.xml"
}

algo_CS() {
  setEnv

  # launch dynawo-algorithms
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType=CS $@
  RETURN_CODE=$?
  
  while (($#)); do
  case $1 in
    --input)
      if [ ! -z "$2" ]; then
  	    if [ -f "$2" ]; then
          filter_timeline `dirname $2`
        fi
      fi
      break
      ;;
    *)
      shift
      break
      ;;
    esac
  done
  
  return ${RETURN_CODE}
}

algo_MC() {
  setEnv

  # launch margin calculation
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType=MC $@
  RETURN_CODE=$?

  while (($#)); do
  case $1 in
    --directory)
      if [ ! -z "$2" ]; then
  	    if [ -d "$2" ]; then
          filter_timeline $2
        fi
      fi
      break
      ;;
    *)
      shift
      break
      ;;
    esac
  done

  return ${RETURN_CODE}
}

algo_SA() {
  setEnv

  # launch dynamic systematic analysis
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType=SA $@
  RETURN_CODE=$?

  while (($#)); do
  case $1 in
    --directory)
      if [ ! -z "$2" ]; then
  	    if [ -d "$2" ]; then
          filter_timeline $2
        fi
      fi
      break
      ;;
    *)
      shift
      break
      ;;
    esac
  done

  return ${RETURN_CODE}
}

if [ $# -eq 0 ]; then
  echo "$usage"
  exit 1
fi

while (($#)); do
  case $1 in
    CS)
      shift
      algo_CS $@ || error_exit "Dynawo execution failed"
      break
      ;;
    MC)
      shift
      algo_MC $@ || error_exit "Dynawo execution failed"
      break
      ;;
    SA)
      shift
      algo_SA $@ || error_exit "Dynawo execution failed"
      break
      ;;
    --version)
      setEnv
      $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --version
      break
      ;;
    --help)
      echo "$usage"
      break
      ;;
    *)
      echo "$1 is an invalid option"
      echo "$usage"
      break
      ;;
    esac
done
