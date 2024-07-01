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

error_exit() {
  echo "${1:-"Unknown Error"}" 1>&2
  exit 1
}

error() {
  echo "${1:-"Unknown Error"}" 1>&2
}

usage="Usage: `basename $0` [option] -- program to deal with dynawo-algorithms environnement

where [option] can be:

    =========== Building dynawo-algorithms project
    config                        configure dynawo-algorithms's compiling environnement using CMake
    config-3rd-Party              configure dynawo-algorithms 3rd parties compiling environnement using CMake
    build                         build dynawo-algorithms and install
    build-3rd-party               build dynawo-algorithms 3rd parties and install
    build-doc                     build dynawo-algorithms's documentation

    clean-build-all               call in this order clean, config, build, build-doc

    distrib                       create distribution of dynawo-algorithms
    distrib-headers               create distribution of dynawo-algorithms with headers
    distrib-omc                   create distribution of dynawo-algorithms with OpenModelica
    deploy                        deploy the current version of dynawo-algorithms binaries/libraries/includes to be used as a release by an another project

    clean-3rd-party               remove all 3rd party softwares objects
    clean-dynawo-algorithms       remove dynawo-algorithms objects
    clean-all                     call clean-3rd-party, clean-dynawo-algorithms
    uninstall-3rd-party           uninstall all 3rd party softwares
    uninstall-dynawo-algorithms   uninstall dynawo-algorithms
    uninstall-all                 call uninstall-3rd-party, uninstall-dynawo-algorithms
    clean-build                   clean, then configure and build dynawo-algorithms

    =========== dynawo-algorithms main launching options
    SA ([args])                   launch a systematic analysis
    MC ([args])                   launch a margin calculation
    CS ([args])                   launch a simple Dynawo simulation

    =========== Tests
    nrt ([-p regex] [-n name_filter])     run (filtered) non-regression tests and open the result in chosen browser
    nrt-clean                     clean non-regression tests
    version-validation            clean all built items, then build them all
    SA-gdb ([args])               launch a systematic analysis with gdb
    MC-gdb ([args])               launch a margin calculation with gdb
    CS-gdb ([args])               launch a simple Dynawo simulationwith gdb
    build-tests                   build and launch dynawo-algorithms's unittest
    build-tests-coverage          build/launch dynawo-algorithms's unittest and generate code coverage report
    unittest-gdb [arg]            call unittest in gdb

    =========== dynawo-algorithms documentation
    doc                                   open Dynawo-algorithms's documentation
    doxygen-doc                           open Dynawo-algorithms's Doxygen documentation into chosen browser

    build-doc                             build documentation
    build-doxygen-doc                     build all doxygen documentation

    clean-doc                             clean documentation

    =========== dynawo-algorithms
    version-validation            clean all built items, then build them all

    =========== Others
    display-environment           display all environment variables set by this scripts
    clean-old-branches            remove old build/install/nrt results from merge branches
    doc                           open dynawo-algorithms documentation into chosen browser
    version                       show dynawo-algorithms version
    help                          show this message"

SCRIPT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && echo "$(pwd)"/"$(basename ${BASH_SOURCE[0]})")
TOTAL_CPU=$(grep -c ^processor /proc/cpuinfo)
NBPROCS=1
MPIRUN_PATH=$(which mpirun 2> /dev/null)

export_var_env_force() {
  local var=$@
  local name=${var%%=*}
  local value=${var#*=}

  if ! `expr $name : "DYNAWO_.*" > /dev/null`; then
    error_exit "You must export variables with DYNAWO prefix for $name."
  fi

  if eval "[ \$$name ]"; then
    unset $name
    export $name="$value"
    return
  fi

  if [ "$value" = UNDEFINED ]; then
    error_exit "You must define the value of $name"
  fi
  export $name="$value"
}

export_var_env() {
  local var=$@
  local name=${var%%=*}
  local value=${var#*=}

  if ! `expr $name : "DYNAWO_.*" > /dev/null`; then
    error_exit "You must export variables with DYNAWO prefix for $name."
  fi

  if eval "[ \$$name ]"; then
    eval "value=\${$name}"
    ##echo "Environment variable for $name already set : $value"
    return
  fi

  if [ "$value" = UNDEFINED ]; then
    error_exit "You must define the value of $name"
  fi
  export $name="$value"
}

export_var_env_default() {
  local var=$@
  local name=${var%%=*}
  local value=${var#*=}

  if ! `expr $name : "DYNAWO_.*" > /dev/null`; then
    error_exit "You must export variables with DYNAWO prefix for $name."
  fi

  if [  "$value" = UNDEFINED ]; then
    if eval "[ \$$name ]"; then
      eval "value=\${$name}"
      export_var_env ${name}_DEFAULT=false
    else
      export_var_env ${name}_DEFAULT=true
      return
    fi
  fi

  export $name="$value"
  export_var_env ${name}_DEFAULT=false
}

export_git_branch() {
  current_dir=$PWD
  pushd $DYNAWO_ALGORITHMS_HOME> /dev/null
  branch_name=$(git rev-parse --abbrev-ref HEAD 2> /dev/null)
  if [[ "${branch_name}" == "" ]]; then
    branch_ref=$(git rev-parse --short HEAD)
    branch_name="detached_"${branch_ref}
  fi
  export_var_env_force DYNAWO_BRANCH_NAME=${branch_name}
  popd > /dev/null
}

export_preload() {
  lib="tcmalloc"
  # uncomment to activate tcmalloc in debug when build is in debug
  # if [ $DYNAWO_BUILD_TYPE == "Debug" ]; then
  #   lib=$lib"_debug"
  # fi
  lib=$lib".so"

  if [ -d $DYNAWO_TCMALLOC_INSTALL_DIR/lib ]; then
    externalTcMallocLib=$(find $DYNAWO_TCMALLOC_INSTALL_DIR/lib -iname *$lib)
    if [ -n "$externalTcMallocLib" ]; then
      echo "Use downloaded tcmalloc library $externalTcMallocLib"
      export LD_PRELOAD=$externalTcMallocLib
      return
    fi
  fi

  nativeTcMallocLib=$(ldconfig -p | grep -e $lib$ | cut -d ' ' -f4)
  if [ -n "$nativeTcMallocLib" ]; then
    echo "Use native tcmalloc library $nativeTcMallocLib"
    export LD_PRELOAD=$nativeTcMallocLib
    return
  fi
}

# Export variables needed for dynawo-algorithms
set_environnement() {
  # Force build type when building tests (or tests coverage)
  case $1 in
    build-tests-coverage)
      export_var_env_force DYNAWO_BUILD_TYPE=Debug
      export_var_env_force DYNAWO_BUILD_TESTS=OFF
      export_var_env_force DYNAWO_BUILD_TESTS_COVERAGE=ON
      export_var_env_force DYNAWO_USE_XSD_VALIDATION=true
      ;;
    build-tests)
      export_var_env_force DYNAWO_BUILD_TYPE=Debug
      export_var_env_force DYNAWO_BUILD_TESTS=ON
      export_var_env_force DYNAWO_BUILD_TESTS_COVERAGE=OFF
      export_var_env_force DYNAWO_USE_XSD_VALIDATION=true
      ;;
    unittest-gdb)
      export_var_env_force DYNAWO_BUILD_TYPE=Debug
      export_var_env_force DYNAWO_BUILD_TESTS=ON
      export_var_env_force DYNAWO_BUILD_TESTS_COVERAGE=OFF
      export_var_env_force DYNAWO_USE_XSD_VALIDATION=true
      ;;
    *)
      ;;
  esac

  # Compiler, to have default with gcc
  export_var_env DYNAWO_COMPILER=GCC

  # Set path to compilers
  set_compiler

  # Build_config
  export_var_env DYNAWO_BUILD_TESTS=OFF
  export_var_env DYNAWO_BUILD_TESTS_COVERAGE=OFF
  export_var_env DYNAWO_BUILD_TYPE=UNDEFINED
  export_var_env DYNAWO_CMAKE_GENERATOR="Unix Makefiles"

  export_var_env DYNAWO_COMPILER_VERSION=$($DYNAWO_C_COMPILER -dumpversion)

  # dynawo-algorithms
  export_var_env DYNAWO_ALGORITHMS_HOME=UNDEFINED
  export_git_branch
  export_var_env_force DYNAWO_ALGORITHMS_SRC_DIR=$DYNAWO_ALGORITHMS_HOME
  export_var_env DYNAWO_ALGORITHMS_DEPLOY_DIR=$DYNAWO_ALGORITHMS_HOME/deploy/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/dynawo-algorithms

  export_var_env DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR=$DYNAWO_ALGORITHMS_HOME/build/3rdParty/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/shared/$DYNAWO_BUILD_TYPE$SUFFIX_CX11
  export_var_env DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR=$DYNAWO_ALGORITHMS_HOME/install/3rdParty/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/shared/$DYNAWO_BUILD_TYPE$SUFFIX_CX11
  export_var_env_force DYNAWO_ALGORITHMS_THIRD_PARTY_SRC_DIR=$DYNAWO_ALGORITHMS_HOME/3rdParty

  export_var_env DYNAWO_ALGORITHMS_BUILD_DIR=$DYNAWO_ALGORITHMS_HOME/build/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/$DYNAWO_BRANCH_NAME/$DYNAWO_BUILD_TYPE/dynawo-algorithms
  export_var_env DYNAWO_ALGORITHMS_INSTALL_DIR=$DYNAWO_ALGORITHMS_HOME/install/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/$DYNAWO_BRANCH_NAME/$DYNAWO_BUILD_TYPE/dynawo-algorithms
  export_var_env DYNAWO_FORCE_CXX11_ABI=false

  export_var_env_force DYNAWO_TCMALLOC_INSTALL_DIR=$DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/gperftools

  # External libs
  export_var_env DYNAWO_HOME=UNDEFINED
  export_var_env_force DYNAWO_LIBARCHIVE_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_BOOST_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_LIBZIP_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_LIBXML_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_LIBIIDM_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_LIBIIDM_INSTALL_DIR=$DYNAWO_LIBIIDM_HOME
  export_var_env_force DYNAWO_XERCESC_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_ADEPT_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_SUNDIALS_HOME=$DYNAWO_HOME
  export_var_env_force DYNAWO_SUITESPARSE_HOME=$DYNAWO_HOME
  export_var_env DYNAWO_GTEST_HOME=$DYNAWO_HOME
  export_var_env DYNAWO_GMOCK_HOME=$DYNAWO_HOME

  export_var_env_force DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_LIBIIDM_INSTALL_DIR/lib

  # For Modelica models compilation
  export_var_env_force DYNAWO_ADEPT_INSTALL_DIR=$DYNAWO_ADEPT_HOME
  export_var_env_force DYNAWO_SUITESPARSE_INSTALL_DIR=$DYNAWO_SUITESPARSE_HOME
  export_var_env_force DYNAWO_SUNDIALS_INSTALL_DIR=$DYNAWO_SUNDIALS_HOME
  export_var_env_force DYNAWO_XERCESC_INSTALL_DIR=$DYNAWO_XERCESC_HOME

  export_var_env DYNAWO_DDB_DIR=$DYNAWO_HOME/ddb
  export_var_env DYNAWO_RESOURCES_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR/share:$DYNAWO_ALGORITHMS_INSTALL_DIR/share/xsd:$DYNAWO_HOME/share:$DYNAWO_HOME/share/xsd

  # Miscellaneous
  export_var_env DYNAWO_USE_XSD_VALIDATION=false
  export_var_env DYNAWO_XSD_DIR=$DYNAWO_HOME/share/xsd/
  export_var_env DYNAWO_ALGORITHMS_XSD_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR/share/xsd
  export_var_env DYNAWO_ALGORITHMS_LOCALE=en_GB # or fr_FR
  export_var_env DYNAWO_BROWSER=firefox
  export_var_env DYNAWO_PDFVIEWER=xdg-open
  export_var_env DYNAWO_SCRIPTS_DIR=$DYNAWO_HOME/sbin/
  export_var_env_force DYNAWO_ALGORITHMS_NRT_DIR=$DYNAWO_ALGORITHMS_HOME/nrt
  export_var_env_force DYNAWO_NRT_DIR=$DYNAWO_ALGORITHMS_NRT_DIR
  export_var_env_force DYNAWO_ALGORITHMS_ENV_DYNAWO_ALGORITHMS=$SCRIPT
  export_var_env_force DYNAWO_ENV_DYNAWO=$SCRIPT
  export_var_env DYNAWO_PYTHON_COMMAND="python"
  if [ ! -x "$(command -v ${DYNAWO_PYTHON_COMMAND})" ]; then
    error_exit "Your python interpreter \"${DYNAWO_PYTHON_COMMAND}\" does not work. Use export DYNAWO_PYTHON_COMMAND=<Python Interpreter> in your myEnvDynawoAlgorithms.sh."
  fi
  export_var_env_force DYNAWO_CURVES_TO_HTML_DIR=$DYNAWO_HOME/sbin/curvesToHtml
  export_var_env_force DYNAWO_INSTALL_DIR=$DYNAWO_HOME
  export_var_env DYNAWO_INSTALL_OPENMODELICA=$DYNAWO_HOME/OpenModelica
  export_var_env DYNAWO_DICTIONARIES=dictionaries_mapping

  export_var_env DYNAWO_CMAKE_BUILD_OPTION=""
  if [ -x "$(command -v cmake)" ]; then
    CMAKE_VERSION=$(cmake --version | head -1 | awk '{print $(NF)}')
    CMAKE_BUILD_OPTION=""
    if [ $(echo $CMAKE_VERSION | cut -d '.' -f 1) -ge 3 -a $(echo $CMAKE_VERSION | cut -d '.' -f 2) -ge 12 ]; then
      CMAKE_BUILD_OPTION="-j $DYNAWO_NB_PROCESSORS_USED"
    fi
    export_var_env_force DYNAWO_CMAKE_BUILD_OPTION="$CMAKE_BUILD_OPTION"
  fi

  export IIDM_XML_XSD_PATH=${DYNAWO_LIBIIDM_INSTALL_DIR}/share/iidm/xsd/

  # MPI
  export_var_env DYNAWO_USE_MPI=YES
  if [ "${DYNAWO_USE_MPI}" == "YES" -a -z "$MPIRUN_PATH" ]; then
    MPIRUN_PATH="$DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich/bin/mpirun"
  fi

  # Only used until now by nrt
  export_var_env DYNAWO_NB_PROCESSORS_USED=1
  if [ $DYNAWO_NB_PROCESSORS_USED -gt $TOTAL_CPU ]; then
    error_exit "PROCESSORS_USED ($DYNAWO_NB_PROCESSORS_USED) is higher than the number of cpu of the system ($TOTAL_CPU)"
  fi

  # Export library path, path and other standard environment variables
  set_standardEnvironmentVariables

  set_commit_hook
}

ld_library_path_remove() {
  export LD_LIBRARY_PATH=`echo -n $LD_LIBRARY_PATH | awk -v RS=: -v ORS=: '$0 != "'$1'"' | sed 's/:$//'`;
}

ld_library_path_prepend() {
  if [ ! -z "$LD_LIBRARY_PATH" ]; then
    ld_library_path_remove $1
    export LD_LIBRARY_PATH="$1:$LD_LIBRARY_PATH"
  else
    export LD_LIBRARY_PATH="$1"
  fi
}

python_path_remove() {
  export PYTHONPATH=`echo -n $PYTHONPATH | awk -v RS=: -v ORS=: '$0 != "'$1'"' | sed 's/:$//'`;
}

python_path_append() {
  if [ ! -z "$PYTHONPATH" ]; then
    python_path_remove $1
    export PYTHONPATH="$PYTHONPATH:$1"
  else
    export PYTHONPATH="$1"
  fi
}

path_remove() {
  export PATH=`echo -n $PATH | awk -v RS=: -v ORS=: '$0 != "'$1'"' | sed 's/:$//'`;
}

path_prepend() {
  if [ ! -z "$PATH" ]; then
    path_remove $1
    export PATH="$1:$PATH"
  else
    export PATH="$1"
  fi
}

set_standardEnvironmentVariables() {
  ld_library_path_prepend $DYNAWO_HOME/lib
  ld_library_path_prepend $DYNAWO_ALGORITHMS_INSTALL_DIR/lib

  path_prepend $DYNAWO_INSTALL_OPENMODELICA/bin
  python_path_append $SCRIPTS_DIR

  if [ "$DYNAWO_BUILD_TYPE" = "Debug" ]; then
    if [ -d "$DYNAWO_GTEST_HOME/lib64" ]; then
      ld_library_path_prepend $DYNAWO_GTEST_HOME/lib64
    elif [ -d "$DYNAWO_GTEST_HOME/lib" ]; then
      ld_library_path_prepend $DYNAWO_GTEST_HOME/lib
    else
      error_exit "Not enable to find GoogleTest library directory for runtime."
    fi
  fi
}

reset_environment_variables() {
  ld_library_path_remove $DYNAWO_HOME/lib
  ld_library_path_remove $DYNAWO_ALGORITHMS_INSTALL_DIR/lib
  path_remove $DYNAWO_INSTALL_OPENMODELICA/bin
  python_path_remove $SCRIPTS_DIR
}

set_compiler() {
  if [ "$DYNAWO_COMPILER" = "GCC" ]; then
    export_var_env_force DYNAWO_COMPILER_NAME=$(echo $DYNAWO_COMPILER | tr "[A-Z]" "[a-z]")
  elif [ "$DYNAWO_COMPILER" = "CLANG" ]; then
    export_var_env_force DYNAWO_COMPILER_NAME=$(echo $DYNAWO_COMPILER | tr "[A-Z]" "[a-z]")
  else
    error_exit "DYNAWO_COMPILER environment variable needs to be GCC or CLANG."
  fi
  export_var_env_force DYNAWO_C_COMPILER=$(which $DYNAWO_COMPILER_NAME)
  export_var_env_force DYNAWO_CXX_COMPILER=$(which ${DYNAWO_COMPILER_NAME%cc}++) # Trick to remove cc from gcc and leave clang alone, because we want fo find g++ and clang++
}

set_commit_hook() {
  hook_file_msg='#!'"/bin/bash
$DYNAWO_ALGORITHMS_HOME/util/hooks/commit_hook.sh"' $1'
  if [ -f "$DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg" ]; then
    current_file=$(cat $DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg)
    if [ "$hook_file_msg" != "$current_file" ]; then
      echo "$hook_file_msg" > $DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg
    fi
    if [ ! -x "$DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg" ]; then
      chmod +x $DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg
    fi
  else
    if [ -d ".git" ]; then
      echo "$hook_file_msg" > $DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg
      chmod +x $DYNAWO_ALGORITHMS_HOME/.git/hooks/commit-msg
    fi
  fi

  hook_file_master='#!'"/bin/bash
# Avoid committing in master
branch=\"\$(git rev-parse --abbrev-ref HEAD)\"
if [ \"\$branch\" = \"master\" ]; then
  echo \"You can't commit directly to master branch.\"
  exit 1
fi"
  if [ -f "$DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit" ]; then
    current_file=$(cat $DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit)
    if [ "$hook_file_master" != "$current_file" ]; then
      echo "$hook_file_master" > $DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit
    fi
    if [ ! -x "$DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit" ]; then
      chmod +x $DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit
    fi
  else
    if [ -d "$DYNAWO_ALGORITHMS_HOME/.git" ]; then
      echo "$hook_file_master" > $DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit
      chmod +x $DYNAWO_ALGORITHMS_HOME/.git/hooks/pre-commit
    fi
  fi

  if [ -e "$DYNAWO_ALGORITHMS_HOME/.git" ]; then
    if [ -z "$(git --git-dir $DYNAWO_ALGORITHMS_HOME/.git config --get core.commentchar 2> /dev/null)" ] || [ $(git --git-dir $DYNAWO_ALGORITHMS_HOME/.git config --get core.commentchar 2> /dev/null) = "#" ]; then
      git --git-dir $DYNAWO_ALGORITHMS_HOME/.git config core.commentchar % || error_exit "You need to change git config commentchar from # to %."
    fi
  fi
}

display_environmentVariables() {
  set -x
  set_standardEnvironmentVariables
  set +x
}

config_dynawo_algorithms_3rdParties() {
  if [ ! -d "$DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR" ]; then
    mkdir -p $DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR
  fi
  cd $DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR
  cmake -DCMAKE_C_COMPILER:PATH=$DYNAWO_C_COMPILER \
    -DCMAKE_CXX_COMPILER:PATH=$DYNAWO_CXX_COMPILER \
    -DCMAKE_BUILD_TYPE:STRING=$DYNAWO_BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR \
    -DDOWNLOAD_DIR=$DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR/src \
    -DTMP_DIR=$DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR/tmp \
    -DDYNAWO_HOME=$DYNAWO_HOME \
    -DMPI_HOME=$DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich \
    -DUSE_MPI=$DYNAWO_USE_MPI \
    $DYNAWO_ALGORITHMS_THIRD_PARTY_SRC_DIR
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

build_dynawo_algorithms_3rdParties() {
  pushd $DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR > /dev/null
  cmake --build . $DYNAWO_CMAKE_BUILD_OPTION
  RETURN_CODE=$?
  popd > /dev/null
  return ${RETURN_CODE}
}

# Configure dynawo-algorithms
config_dynawo_algorithms() {
  if [ ! -d "$DYNAWO_ALGORITHMS_BUILD_DIR" ]; then
    mkdir -p $DYNAWO_ALGORITHMS_BUILD_DIR
  fi

  CMAKE_OPTIONAL=""
  if [ $DYNAWO_FORCE_CXX11_ABI = true ]; then
    CMAKE_OPTIONAL="$CMAKE_OPTIONAL -DFORCE_CXX11_ABI=$DYNAWO_FORCE_CXX11_ABI"
  fi
  if [ $DYNAWO_BUILD_TESTS = "ON" -o $DYNAWO_BUILD_TESTS_COVERAGE = "ON" ]; then
    CMAKE_OPTIONAL="$CMAKE_OPTIONAL -DGTEST_ROOT=$DYNAWO_GTEST_HOME"
    CMAKE_OPTIONAL="$CMAKE_OPTIONAL -DGMOCK_HOME=$DYNAWO_GMOCK_HOME"
  fi

  pushd $DYNAWO_ALGORITHMS_BUILD_DIR > /dev/null
  cmake -DCMAKE_C_COMPILER:PATH=$DYNAWO_C_COMPILER \
    -DCMAKE_CXX_COMPILER:PATH=$DYNAWO_CXX_COMPILER \
    -DCMAKE_BUILD_TYPE:STRING=$DYNAWO_BUILD_TYPE \
    -DDYNAWO_ALGORITHMS_HOME:PATH=$DYNAWO_ALGORITHMS_HOME \
    -DDYNAWO_HOME:PATH=$DYNAWO_HOME \
    -DDYNAWO_ALGORITHMS_THIRD_PARTY_DIR=$DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR \
    -DBUILD_TESTS=$DYNAWO_BUILD_TESTS \
    -DBUILD_TESTS_COVERAGE=$DYNAWO_BUILD_TESTS_COVERAGE \
    -DCMAKE_INSTALL_PREFIX:PATH=$DYNAWO_ALGORITHMS_INSTALL_DIR \
    -DXERCESC_HOME=$DYNAWO_XERCESC_HOME \
    -DBOOST_ROOT=$DYNAWO_BOOST_HOME/ \
    -DBOOST_ROOT_DEFAULT:STRING=FALSE \
    -DLIBZIP_HOME=$DYNAWO_LIBZIP_HOME \
    -DDYNAWO_PYTHON_COMMAND="$DYNAWO_PYTHON_COMMAND" \
    -DUSE_MPI=$DYNAWO_USE_MPI \
    $CMAKE_OPTIONAL \
    -G "$DYNAWO_CMAKE_GENERATOR" \
    $DYNAWO_ALGORITHMS_SRC_DIR
  RETURN_CODE=$?
  popd > /dev/null
  return ${RETURN_CODE}
}

# Compile dynawo-algorithms
build_dynawo_algorithms() {
  cd $DYNAWO_ALGORITHMS_BUILD_DIR
  cmake --build . --target install $DYNAWO_CMAKE_BUILD_OPTION
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

build_doc() {
  if [ ! -d "$DYNAWO_ALGORITHMS_HOME/documentation" ]; then
    error_exit "$DYNAWO_ALGORITHMS_HOME/documentation does not exist."
  fi
  cd $DYNAWO_ALGORITHMS_HOME/documentation
  bash dynawo_algorithms_documentation.sh
}

clean_doc() {
  if [ ! -d "$DYNAWO_ALGORITHMS_HOME/documentation" ]; then
    error_exit "$DYNAWO_ALGORITHMS_HOME/documentation does not exist."
  fi
  cd $DYNAWO_ALGORITHMS_HOME/documentation
  bash clean.sh
}

open_pdf() {
  if [ -z "$1" ]; then
    error_exit "You need to specify a pdf file to open."
  fi
  reset_environment_variables #zlib conflict
  if [ ! -z "$DYNAWO_PDFVIEWER" ]; then
    if [ -x "$(command -v $DYNAWO_PDFVIEWER)" ]; then
      if [ -f "$1" ]; then
        $DYNAWO_PDFVIEWER $1
      else
        error_exit "Pdf file $1 you try to open does not exist."
      fi
    else
      error_exit "pdfviewer $DYNAWO_PDFVIEWER seems not to be executable."
    fi
  elif [ -x "$(command -v xdg-open)" ]; then
      xdg-open $1
  else
    error_exit "Cannot determine how to open pdf document from command line. Use DYNAWO_PDFVIEWER environment variable."
  fi
}

open_doc() {
  open_pdf $DYNAWO_ALGORITHMS_HOME/documentation/dynawoAlgorithmsDocumentation/DynawoAlgorithmsDocumentation.pdf
}

#build all doc
build_doxygen_doc() {
  build_doxygen_doc_dynawo_algorithms || error_exit
  test_doxygen_doc_dynawo_algorithms || error_exit
}

build_doxygen_doc_dynawo_algorithms() {
  cd $DYNAWO_ALGORITHMS_BUILD_DIR
  mkdir -p $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/
  cmake --build . --target doc $DYNAWO_CMAKE_BUILD_OPTION
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

test_doxygen_doc_dynawo_algorithms() {
  if [ -f $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings.txt  ] ; then
    rm -f $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings_filtered.txt
    # need to filter "return type of member (*) is not documented" as it is a doxygen bug detected on 1.8.17 that will be solved in 1.8.18
    grep -Fvf $DYNAWO_ALGORITHMS_HOME/util/warnings_to_filter.txt $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings.txt > $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings_filtered.txt
    nb_warnings=$(wc -l $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings_filtered.txt | awk '{print $1}')
    if [ ${nb_warnings} -ne 0 ]; then
      echo "===================================="
      echo "| Result of doxygen doc generation |"
      echo "===================================="
      echo " nbWarnings = ${nb_warnings} > 0 => doc is incomplete"
      echo " edit ${DYNAWO_ALGORITHMS_INSTALL_DIR}/doxygen/warnings_filtered.txt  to have more details"
      error_exit "Doxygen doc is not complete"
    fi
  fi
}

build_tests() {
  clean_dynawo_algorithms || error_exit
  config_dynawo_algorithms_3rdParties || error_exit
  build_dynawo_algorithms_3rdParties || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
  tests=$@
  if [ -z "$tests" ]; then
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target tests --config Debug $DYNAWO_CMAKE_BUILD_OPTION
  else
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target ${tests} --config Debug $DYNAWO_CMAKE_BUILD_OPTION
  fi
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

build_tests_coverage() {
  clean_dynawo_algorithms || error_exit
  config_dynawo_algorithms_3rdParties || error_exit
  build_dynawo_algorithms_3rdParties || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
  tests=$@
  cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target reset-coverage --config Debug $DYNAWO_CMAKE_BUILD_OPTION || error_exit "Error during reset-coverage."
  if [ -z "$tests" ]; then
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target tests-coverage --config Debug $DYNAWO_CMAKE_BUILD_OPTION || error_exit "Error during tests-coverage."
  else
    for test in ${tests}; do
      cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target ${test}-coverage --config Debug $DYNAWO_CMAKE_BUILD_OPTION || error_exit "Error during ${test}-coverage."
    done
  fi
  cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target export-coverage --config Debug $DYNAWO_CMAKE_BUILD_OPTION

  RETURN_CODE=$?
  if [ ${RETURN_CODE} -ne 0 ]; then
    exit ${RETURN_CODE}
  fi
  if [ "$DYNAWO_RESULTS_SHOW" = true ] ; then
    $DYNAWO_BROWSER $DYNAWO_ALGORITHMS_BUILD_DIR/coverage/index.html
  fi
  cp $DYNAWO_ALGORITHMS_BUILD_DIR/coverage/coverage.info $DYNAWO_ALGORITHMS_HOME/build
  if [ -d "$DYNAWO_ALGORITHMS_HOME/build/coverage-sonar" ]; then
    rm -rf "$DYNAWO_ALGORITHMS_HOME/build/coverage-sonar"
  fi
  mkdir -p $DYNAWO_ALGORITHMS_HOME/build/coverage-sonar || error_exit "Impossible to create $DYNAWO_ALGORITHMS_HOME/build/coverage-sonar."
  cd $DYNAWO_ALGORITHMS_HOME/build/coverage-sonar
  for file in $(find $DYNAWO_ALGORITHMS_BUILD_DIR -name "*.gcno" | grep -v "/test/"); do
    cpp_file_name=$(basename $file .gcno)
    cpp_file=$(find $DYNAWO_ALGORITHMS_HOME/sources -name "$cpp_file_name" 2> /dev/null)
    gcov -pb $cpp_file -o $file > /dev/null
  done
  find $DYNAWO_ALGORITHMS_HOME/build/coverage-sonar -type f -not -name "*dynawo-algorithms*" -exec rm -f {} \;
}

unittest_gdb() {
  list_of_tests=($(find $DYNAWO_ALGORITHMS_BUILD_DIR/sources -executable -type f -exec basename {} \; | grep test))
  if [[ ${#list_of_tests[@]} == 0 ]]; then
    echo "The list of tests is empty. This should not happen."
    exit 1
  fi
  if [ -z "$1" ]; then
    echo "You need to give the name of unittest to run."
    echo "List of available unittests:"
    for name in ${list_of_tests[@]}; do
      echo "  $name"
    done
    exit 1
  fi
  unittest_exe=$(find $DYNAWO_ALGORITHMS_BUILD_DIR/sources -name "$1")
  if [ -z "$unittest_exe" ]; then
    echo "The unittest you gave is not available."
    echo "List of available unittests:"
    for name in ${list_of_tests[@]}; do
      echo "  $name"
    done
    exit 1
  fi
  if [ ! -d "$(dirname $unittest_exe)" ]; then
    error_exit "$(dirname $unittest_exe) does not exist."
  fi
  cd $(dirname $unittest_exe)
  gdb -q --args $unittest_exe
}

clean_third_parties() {
  rm -rf $DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR
}

#clean dynawo-algorithms
clean_dynawo_algorithms() {
  rm -rf $DYNAWO_ALGORITHMS_BUILD_DIR
}

clean_all() {
  clean_third_parties
  clean_dynawo_algorithms
}

uninstall_third_parties() {
  rm -rf $DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR
}

# uninstall dynawo-algorithms
uninstall_dynawo_algorithms() {
  rm -rf $DYNAWO_ALGORITHMS_INSTALL_DIR
}

uninstall_all() {
  uninstall_third_parties
  uninstall_dynawo_algorithms
}

# Clean, then configure and build dynawo-algorithms
clean_build_dynawo_algorithms() {
  clean_dynawo_algorithms || error_exit
  uninstall_dynawo_algorithms || error_exit
  config_dynawo_algorithms_3rdParties || error_exit
  build_dynawo_algorithms_3rdParties || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
}

clean_build_all_dynawo_algorithms() {
  clean_build_dynawo_algorithms || error_exit
  build_doxygen_doc || error_exit
}

version_validation() {
  clean_build_all_dynawo_algorithms || error_exit
  create_distrib || error_exit
  nrt || error_exit
}

clean_old_dir() {
  directory=$1
  cd ${directory}
  shift 1
  branches=$@

  nbdir=$(find . -maxdepth 1 -type d | wc -l) # count . as a dir
  if [ $nbdir -eq 1 ]; then
    return
  fi

  directories=$(ls -d */ | cut -f1 -d'/')
  for d in $directories; do
    branch_exist="false"
    for b in ${branches}; do
      if [[ "${b}" = "${d}" ]]; then
        branch_exist="true"
        break
      fi
    done
    if [[ "${branch_exist}" = "false" ]]; then
      rm -rvf ${directory}/${d}
    fi
  done
}

# Clean build/install/nrt of old branches
clean_old_branches() {
  current_dir=$PWD
  cd ${DYNAWO_ALGORITHMS_HOME}
  branches=$(git for-each-ref --format='%(refname:short)' refs/heads)
  # clean old directories in build_dir, nrt and install
  clean_old_dir ${DYNAWO_ALGORITHMS_HOME}/build/ ${branches}
  clean_old_dir ${DYNAWO_ALGORITHMS_HOME}/install/ ${branches}

  cd ${current_dir}
}

open_doxygen_doc() {
  if [ ! -f $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/html/index.html ]; then
    echo "Doxygen documentation not yet generated"
    echo "Generating ..."
    build_doxygen_doc
    RETURN_CODE=$?
    if [ ${RETURN_CODE} -ne 0 ]; then
      exit ${RETURN_CODE}
    fi
    echo "... end of doc generation"
  fi
  $DYNAWO_BROWSER $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/html/index.html
}

version() {
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --version
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

export_var_env_to_file() {
  export | grep DYNAWO_ | sed -e 's/declare -x //g' > "$1"
}

deploy_dynawo_algorithms() {
  rm -rf $DYNAWO_ALGORITHMS_DEPLOY_DIR

  mkdir -p $DYNAWO_ALGORITHMS_DEPLOY_DIR
  pushd $DYNAWO_ALGORITHMS_DEPLOY_DIR > /dev/null

  export_var_env_to_file "dynawoAlgorithmsEnv.txt"

  mkdir -p bin
  mkdir -p lib
  mkdir -p include
  mkdir -p share

  echo "deploying gperftools libraries"
  if [ -d $DYNAWO_TCMALLOC_INSTALL_DIR/lib ]; then
    cp -r $DYNAWO_TCMALLOC_INSTALL_DIR/lib/libtcmalloc.so* lib/.
  else
    nativeTcMallocLib=$(ldconfig -p | grep -e libtcmalloc.so | cut -d ' ' -f4)
    cp $nativeTcMallocLib lib/.
  fi

  if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
    echo "deploying mpich libraries and binaries"
    cp $MPIRUN_PATH bin/.
    if [ -d $DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich ]; then
      cp $DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich/lib/libmpi.so* lib/.
      cp $DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich/lib/libmpicxx.so* lib/.
      cp $DYNAWO_ALGORITHMS_THIRD_PARTY_INSTALL_DIR/mpich/bin/hydra_pmi_proxy bin/.
    else
      if [ ! -f "$DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR/CMakeCache.txt" ]; then
        error_exit "$DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR should not be deleted before deploy to be able to determine lib system paths used during compilation."
      fi
      mpi_lib=$(cat $DYNAWO_ALGORITHMS_THIRD_PARTY_BUILD_DIR/CMakeCache.txt | grep "MPI_.*_LIBRARY:FILEPATH" | cut -d '=' -f 2)
      for lib in ${mpi_lib}; do
        cp ${lib}* lib/.
      done
      cp $(dirname $MPIRUN_PATH)/hydra_pmi_proxy bin/.
    fi
  fi

  echo "deploying Dynawo-algorithms libraries"
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/* bin/.
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/lib/* lib/.
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/include/* include/.
  cp -r $DYNAWO_ALGORITHMS_INSTALL_DIR/share/* share/.
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/dynawo-algorithms.sh .

  if [ -d "$DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen" ]; then
    mkdir -p doxygen
    echo "deploying doxygen"
    cp -r $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/html doxygen/.
  fi

  rm -f lib/*.la
  rm -f lib/*.a

  popd > /dev/null
}

create_distrib() {
  DYNAWO_ALGORITHMS_VERSION=$(version)
  version=$(echo $DYNAWO_ALGORITHMS_VERSION | cut -f1 -d' ')

  ZIP_FILE=DynawoAlgorithms_v$version.zip

  deploy_dynawo_algorithms

  # create distribution
  if [ ! -d "$DYNAWO_ALGORITHMS_DEPLOY_DIR" ]; then
    error_exit "$DYNAWO_ALGORITHMS_DEPLOY_DIR does not exist."
  fi
  pushd $DYNAWO_ALGORITHMS_DEPLOY_DIR > /dev/null

  mkdir tmp/
  cd tmp/
  mkdir -p dynawo-algorithms
  cp -r $DYNAWO_HOME/bin dynawo-algorithms/
  cp -r $DYNAWO_HOME/lib dynawo-algorithms/
  cp -r $DYNAWO_HOME/ddb dynawo-algorithms/
  cp -r $DYNAWO_HOME/share dynawo-algorithms/
  cp -r $DYNAWO_HOME/sbin dynawo-algorithms/
  cp -r $DYNAWO_HOME/dynawo.sh dynawo-algorithms/

  cp ../bin/* dynawo-algorithms/bin/
  cp ../lib/* dynawo-algorithms/lib/.
  cp -r ../share/* dynawo-algorithms/share/
  cp ../dynawo-algorithms.sh dynawo-algorithms/

  mkdir -p dynawo-algorithms/examples/CS
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/IEEE14_BlackBoxModels/* dynawo-algorithms/examples/CS
  mkdir -p dynawo-algorithms/examples/SA
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/SA/files/* dynawo-algorithms/examples/SA
  mkdir -p dynawo-algorithms/examples/MC
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/MC/files/* dynawo-algorithms/examples/MC

  # combines dictionaries mapping
  cat $DYNAWO_HOME/share/dictionaries_mapping.dic | grep -v -F // | grep -v -e '^$' >> dynawo-algorithms/share/dictionaries_mapping.dic
  zip -r -y ../$ZIP_FILE dynawo-algorithms/
  cd $DYNAWO_ALGORITHMS_DEPLOY_DIR

  # remove temp directory
  rm -rf tmp

  # move distribution in distribution directory
  DISTRIB_DIR=$DYNAWO_ALGORITHMS_HOME/distributions
  mkdir -p $DISTRIB_DIR
  mv $ZIP_FILE $DISTRIB_DIR

  popd > /dev/null
}

create_distrib_with_headers() {
  if [ ! -z "$1" ]; then
    if [ "$1" = "with_omc" ]; then
      with_omc=yes
    fi
  fi
  DYNAWO_ALGORITHMS_VERSION=$(version)
  version=$(echo $DYNAWO_ALGORITHMS_VERSION | cut -f1 -d' ')

  if [ "$with_omc" = "yes" ]; then
    ZIP_FILE=DynawoAlgorithms_omc_v$version.zip
  else
    ZIP_FILE=DynawoAlgorithms_headers_v$version.zip
  fi

  deploy_dynawo_algorithms

  # create distribution
  if [ ! -d "$DYNAWO_ALGORITHMS_DEPLOY_DIR" ]; then
    error_exit "$DYNAWO_ALGORITHMS_DEPLOY_DIR does not exist."
  fi
  pushd $DYNAWO_ALGORITHMS_DEPLOY_DIR > /dev/null

  mkdir tmp/
  cd tmp/
  mkdir -p dynawo-algorithms
  cp -r $DYNAWO_HOME/bin dynawo-algorithms/
  cp -r $DYNAWO_HOME/include dynawo-algorithms/
  cp -r $DYNAWO_HOME/lib dynawo-algorithms/
  cp -r $DYNAWO_HOME/ddb dynawo-algorithms/
  cp -r $DYNAWO_HOME/share dynawo-algorithms/
  cp -r $DYNAWO_HOME/sbin dynawo-algorithms/
  cp -r $DYNAWO_HOME/dynawo.sh dynawo-algorithms/
  cp -r $DYNAWO_HOME/dynawoEnv.txt dynawo-algorithms/
  if [ "$with_omc" = "yes" ]; then
    cp -r $DYNAWO_HOME/OpenModelica dynawo-algorithms/
  fi

  cp ../bin/* dynawo-algorithms/bin/
  cp ../include/* dynawo-algorithms/include/
  cp ../lib/* dynawo-algorithms/lib/.
  cp -r ../share/* dynawo-algorithms/share/
  cp ../dynawo-algorithms.sh dynawo-algorithms/

  mkdir -p dynawo-algorithms/examples/CS
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/IEEE14_BlackBoxModels/* dynawo-algorithms/examples/CS
  mkdir -p dynawo-algorithms/examples/SA
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/SA/files/* dynawo-algorithms/examples/SA
  mkdir -p dynawo-algorithms/examples/MC
  cp $DYNAWO_ALGORITHMS_HOME/nrt/data/IEEE14/MC/files/* dynawo-algorithms/examples/MC

  # combines dictionaries mapping
  cat $DYNAWO_HOME/share/dictionaries_mapping.dic | grep -v -F // | grep -v -e '^$' >> dynawo-algorithms/share/dictionaries_mapping.dic
  zip -r -y ../$ZIP_FILE dynawo-algorithms/
  zip -r -y ../$ZIP_FILE dynawo-algorithms/dynawo-algorithms.sh
  cd $DYNAWO_ALGORITHMS_DEPLOY_DIR

  # remove temp directory
  rm -rf tmp

  # move distribution in distribution directory
  DISTRIB_DIR=$DYNAWO_ALGORITHMS_HOME/distributions
  mkdir -p $DISTRIB_DIR
  mv $ZIP_FILE $DISTRIB_DIR

  popd > /dev/null
}

verify_browser() {
  if [ ! -x "$(command -v $DYNAWO_BROWSER)" ]; then
    error_exit "Specified browser DYNAWO_BROWSER=$DYNAWO_BROWSER not found."
  fi
}

nrt() {
  export_var_env_force DYNAWO_NRT_DIFF_DIR=$DYNAWO_HOME/sbin/nrt/nrt_diff
  $DYNAWO_PYTHON_COMMAND -u $DYNAWO_ALGORITHMS_NRT_DIR/nrtAlgo.py $@
  FAILED_CASES_NUM=$?

  if [ ! -f "$DYNAWO_ALGORITHMS_NRT_DIR/output/$DYNAWO_BRANCH_NAME/report.html" ]; then
    error_exit "No report was generated by the non regression test script"
  fi
  if [ "$DYNAWO_RESULTS_SHOW" = true ] ; then
    verify_browser
    $DYNAWO_BROWSER $DYNAWO_ALGORITHMS_NRT_DIR/output/$DYNAWO_BRANCH_NAME/report.html &
  fi

  if [ ${FAILED_CASES_NUM} -ne 0 ]; then
    error_exit "${FAILED_CASES_NUM} non regression tests failed"
  fi
  if [ "$DYNAWO_BUILD_TYPE" = "Debug" ]; then
    echo "Warning: Debug mode is activated, references comparison was not done"
  fi
}

nrt_clean() {
  $DYNAWO_PYTHON_COMMAND $DYNAWO_ALGORITHMS_HOME/util/nrt-clean.py
}

launch_CS() {
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType CS $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_SA() {
  export_preload
  if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
    "$MPIRUN_PATH" -np $NBPROCS $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  else
    $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  fi
  RETURN_CODE=$?
  unset LD_PRELOAD
  return ${RETURN_CODE}
}

launch_MC() {
  export_preload
  if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
    "$MPIRUN_PATH" -np $NBPROCS $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  else
    $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  fi
  RETURN_CODE=$?
  unset LD_PRELOAD
  return ${RETURN_CODE}
}

launch_CS_gdb() {
  gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType CS $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_SA_gdb() {
  export_preload
  if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
    "$MPIRUN_PATH" -np $NBPROCS xterm -e gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  else
    gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  fi
  RETURN_CODE=$?
  unset LD_PRELOAD
  return ${RETURN_CODE}
}

launch_MC_gdb() {
  export_preload
  if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
    "$MPIRUN_PATH" -np $NBPROCS xterm -e gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  else
    gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  fi
  RETURN_CODE=$?
  unset LD_PRELOAD
  return ${RETURN_CODE}
}

#################################
########### Main script #########
if [ -n "$BASH_VERSION" ]; then
  SCRIPT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && echo "$(pwd)"/"$(basename ${BASH_SOURCE[0]})")
elif [ -n "$ZSH_VERSION" ]; then
  SCRIPT=$(cd "$(dirname "$0")" && echo "$(pwd)"/"$(basename $0)")
fi

## force build_type for specific cases
MODE=$1
ARGS=""
while (($#)); do
  key="$1"
  case $key in
  --nbThreads|-np)
    NBPROCS=$2
    shift 2 # past argument and value
    ;;
  --nbThreads=*)
    NBPROCS="${1#*=}"
    shift # past value
    ;;
  *)
    ARGS="$ARGS $key"
    shift # past argument
    ;;
  esac
done

## set environnement variables
set_environnement $MODE

ARGS=`echo ${ARGS} | cut -d' ' -f2-`
## launch command
case $MODE in
  config)
    config_dynawo_algorithms || error_exit "Error while configuring dynawo-algorithms"
    ;;

  config-3rd-Party)
    config_dynawo_algorithms_3rdParties || error_exit "Error while configuring dynawo-algorithms 3rd parties"
    ;;

  build)
    config_dynawo_algorithms_3rdParties || error_exit "Error while configuring dynawo-algorithms 3rd parties"
    build_dynawo_algorithms_3rdParties || error_exit "Error while building dynawo-algorithms 3rd parties"
    config_dynawo_algorithms ||  error_exit "Error while configuring dynawo-algorithms"
    build_dynawo_algorithms || error_exit "Error while building dynawo-algorithms"
    ;;

  build-3rd-party)
    config_dynawo_algorithms_3rdParties || error_exit "Error while configuring dynawo-algorithms 3rd parties"
    build_dynawo_algorithms_3rdParties || error_exit "Error while building dynawo-algorithms 3rd parties"
    ;;

  build-doc)
    build_doc || error_exit "Error during the build of dynawo documentation"
    ;;

  build-doxygen-doc)
    build_doxygen_doc || error_exit "Error while building doxygen documentation"
    ;;

  build-tests)
    build_tests || error_exit
    ;;

  build-tests-coverage)
    build_tests_coverage || error_exit
    ;;

  unittest-gdb)
    unittest_gdb ${ARGS} || error_exit "Error during the run unittest in gdb"
    ;;

  clean-build-all)
    clean_build_all_dynawo_algorithms || error_exit
    ;;

  version-validation)
    version_validation || error_exit "The current version does not fulfill the standard quality check"
    ;;

  distrib)
    create_distrib || error_exit "Error while building dynawo-algorithms distribution"
    ;;

  distrib-headers)
    create_distrib_with_headers || error_exit "Error while building dynawo-algorithms distribution with headers"
    ;;

  distrib-omc)
    create_distrib_with_headers "with_omc" || error_exit "Error while building dynawo-algorithms distribution with omc"
    ;;

  deploy)
    deploy_dynawo_algorithms || error_exit "Error during the deployment of dynawo-algorithms"
    ;;

  clean-3rd-party)
    clean_third_parties || error_exit "Error while cleaning 3rd parties"
    ;;

  clean-dynawo-algorithms)
    clean_dynawo_algorithms || error_exit "Error while cleaning dynawo-algorithms"
    ;;

  clean-all)
    clean_all || error_exit "Error while cleaning all"
    ;;

  uninstall-3rd-party)
    uninstall_third_parties || error_exit "Error while uninstalling 3rd parties"
    ;;

  uninstall-dynawo-algorithms)
    uninstall_dynawo_algorithms || error_exit "Error while uninstalling dynawo-algorithms"
    ;;

  uninstall-all)
    uninstall_all || error_exit "Error while uninstalling all"
    ;;

  clean-build)
    clean_build_dynawo_algorithms || error_exit
    ;;

  SA)
    launch_SA ${ARGS} || error_exit "Systematic analysis failed"
    ;;

  MC)
    launch_MC ${ARGS} || error_exit "Margin calculation failed"
    ;;
  CS)
    launch_CS ${ARGS} || error_exit "Dynawo simulation failed"
    ;;
  SA-gdb)
    launch_SA_gdb ${ARGS} || error_exit "Systematic analysis failed"
    ;;

  MC-gdb)
    launch_MC_gdb ${ARGS} || error_exit "Margin calculation failed"
    ;;

  CS-gdb)
    launch_CS_gdb ${ARGS} || error_exit "Dynawo simulation failed"
    ;;

  nrt)
    nrt ${ARGS} || error_exit "Error during Dynawo's non regression tests execution"
    ;;
  
  nrt-clean)
    nrt_clean || error_exit "Error during Dynawo's non-regression tests clean"
    ;;

  display-environment)
    display_environmentVariables || error_exit "Failed to display environment variables"
    ;;

  clean-old-branches)
    clean_old_branches || error_exit "Error during the cleaning of old branches build/install/nrt"
    ;;

  clean-doc)
    clean_doc || error_exit "Error during the clean of Dynawo documentation"
    ;;

  doc)
    open_doc || error_exit "Error during the opening of Dynawo documentation"
    ;;

  doxygen-doc)
    open_doxygen_doc || error_exit "Error during Dynawo Doxygen doc visualisation"
    ;;

  version)
    version || error_exit "Error during version visualisation"
    ;;

  help)
    echo "$usage"
    ;;

  *)
    echo "$1 is an invalid option"
    echo "$usage"
    exit 1
    ;;
esac
