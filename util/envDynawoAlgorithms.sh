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
    build                         build dynawo-algorithms and install
    build-doc                     build dynawo-algorithms's documentation

    clean-build-all               call in this order clean, config, build, build-doc

    distrib                       create distribution of dynawo-algorithms
    distrib-headers               create distribution of dynawo-algorithms with headers
    deploy                        deploy the current version of dynawo-algorithms binaries/libraries/includes to be used as a release by an another project

    clean                         remove dynawo-algorithms objects
    uninstall                     uninstall dynawo-algorithms
    clean-build                   clean, then configure and build dynawo-algorithms

    =========== dynawo-algorithms main launching options
    SA ([args])                   launch a systematic analysis
    MC ([args])                   launch a margin calculation
    CS ([args])                   launch a simple Dynawo simulation

    =========== Tests
    nrt ([-p regex] [-n name_filter])     run (filtered) non-regression tests and open the result in chosen browser
    version-validation            clean all built items, then build them all
    SA-gdb ([args])               launch a systematic analysis with gdb
    MC-gdb ([args])               launch a margin calculation with gdb
    CS-gdb ([args])               launch a simple Dynawo simulationwith gdb
    build-tests                   build and launch dynawo-algorithms's unittest
    build-tests-coverage          build/launch dynawo-algorithms's unittest and generate code coverage report
    unittest-gdb [arg]            call unittest in gdb

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

env_var_sanity_check() {
  value_dynawo=`grep "DYNAWO_CXX11_ENABLED=\"" ${DYNAWO_HOME}/dynawoEnv.txt | sed -e 's/DYNAWO_CXX11_ENABLED="//g' | sed -e 's/"//g'`
  if [[ "${DYNAWO_CXX11_ENABLED}" != "${value_dynawo}" ]]; then
  	error_exit "Cannot build dynawo-algorithms with DYNAWO_CXX11_ENABLED=\"${DYNAWO_CXX11_ENABLED}\" as dynawo core was built with DYNAWO_CXX11_ENABLED=\"${value_dynawo}\""
  fi
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

# Export variables needed for dynawo-algorithms
set_environnement() {
  env_var_sanity_check
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
  export_var_env DYNAWO_CXX11_ENABLED=UNDEFINED
  export_var_env DYNAWO_CMAKE_GENERATOR="Unix Makefiles"

  export_var_env DYNAWO_COMPILER_VERSION=$($DYNAWO_C_COMPILER -dumpversion)

  # dynawo-algorithms
  export_var_env DYNAWO_ALGORITHMS_HOME=UNDEFINED
  export_git_branch
  export_var_env_force DYNAWO_ALGORITHMS_SRC_DIR=$DYNAWO_ALGORITHMS_HOME
  export_var_env DYNAWO_ALGORITHMS_DEPLOY_DIR=$DYNAWO_ALGORITHMS_HOME/deploy/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/dynawo-algorithms

  export_var_env DYNAWO_ALGORITHMS_BUILD_DIR=$DYNAWO_ALGORITHMS_HOME/build/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/$DYNAWO_BRANCH_NAME/$DYNAWO_BUILD_TYPE/dynawo-algorithms
  export_var_env DYNAWO_ALGORITHMS_INSTALL_DIR=$DYNAWO_ALGORITHMS_HOME/install/$DYNAWO_COMPILER_NAME$DYNAWO_COMPILER_VERSION/$DYNAWO_BRANCH_NAME/$DYNAWO_BUILD_TYPE/dynawo-algorithms
  export_var_env DYNAWO_FORCE_CXX11_ABI=false

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
  export_var_env_force DYNAWO_NICSLU_HOME=$DYNAWO_HOME

  export_var_env_force DYNAWO_IIDM_EXTENSION=$DYNAWO_LIBIIDM_INSTALL_DIR/lib/libdynawo_DataInterfaceIIDMExtension.so
  export_var_env_force DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_LIBIIDM_INSTALL_DIR/lib

  # For Modelica models compilation
  export_var_env_force DYNAWO_ADEPT_INSTALL_DIR=$DYNAWO_ADEPT_HOME
  export_var_env_force DYNAWO_SUITESPARSE_INSTALL_DIR=$DYNAWO_SUITESPARSE_HOME
  export_var_env_force DYNAWO_SUNDIALS_INSTALL_DIR=$DYNAWO_SUNDIALS_HOME
  export_var_env_force DYNAWO_NICSLU_INSTALL_DIR=$DYNAWO_NICSLU_HOME
  export_var_env_force DYNAWO_XERCESC_INSTALL_DIR=$DYNAWO_XERCESC_HOME

  export_var_env DYNAWO_DDB_DIR=$DYNAWO_HOME/ddb
  export_var_env DYNAWO_RESOURCES_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR/share:$DYNAWO_ALGORITHMS_INSTALL_DIR/share/xsd:$DYNAWO_HOME/share:$DYNAWO_HOME/share/xsd

  # Miscellaneous
  export_var_env DYNAWO_USE_XSD_VALIDATION=false
  export_var_env DYNAWO_XSD_DIR=$DYNAWO_HOME/share/xsd/
  export_var_env DYNAWO_ALGORITHMS_XSD_DIR=$DYNAWO_ALGORITHMS_INSTALL_DIR/share/xsd
  export_var_env DYNAWO_ALGORITHMS_LOCALE=en_GB # or fr_FR
  export_var_env DYNAWO_BROWSER=firefox
  export_var_env DYNAWO_SCRIPTS_DIR=$DYNAWO_HOME/sbin/
  export_var_env_force DYNAWO_ALGORITHMS_NRT_DIR=$DYNAWO_ALGORITHMS_HOME/nrt
  export_var_env_force DYNAWO_NRT_DIR=$DYNAWO_ALGORITHMS_NRT_DIR
  export_var_env_force DYNAWO_ALGORITHMS_ENV_DYNAWO_ALGORITHMS=$SCRIPT
  export_var_env_force DYNAWO_ENV_DYNAWO=$SCRIPT
  export_var_env DYNAWO_ALGORITHMS_PYTHON_COMMAND="python"
  export_var_env_force DYNAWO_CURVES_TO_HTML_DIR=$DYNAWO_HOME/sbin/curvesToHtml
  export_var_env_force DYNAWO_INSTALL_DIR=$DYNAWO_HOME
  export_var_env DYNAWO_INSTALL_OPENMODELICA=$DYNAWO_HOME/OpenModelica
  export_var_env DYNAWO_DICTIONARIES=dictionaries_mapping

  export IIDM_XML_XSD_PATH=${DYNAWO_LIBIIDM_INSTALL_DIR}/share/iidm/xsd/

  # Only used until now by nrt
  export_var_env DYNAWO_NB_PROCESSORS_USED=1
  if [ $DYNAWO_NB_PROCESSORS_USED -gt $TOTAL_CPU ]; then
    error_exit "PROCESSORS_USED ($DYNAWO_NB_PROCESSORS_USED) is higher than the number of cpu of the system ($TOTAL_CPU)"
  fi

  # Export library path, path and other standard environment variables
  set_standardEnvironmentVariables

  set_commit_hook
}

set_standardEnvironmentVariables() {
  export LD_LIBRARY_PATH=$DYNAWO_HOME/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH=$DYNAWO_ALGORITHMS_INSTALL_DIR/lib:$LD_LIBRARY_PATH

  export PATH=$DYNAWO_INSTALL_OPENMODELICA/bin:$PATH
  export PYTHONPATH=$PYTHONPATH:$SCRIPTS_DIR
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

# Configure dynawo-algorithms
config_dynawo_algorithms() {
  if [ ! -d "$DYNAWO_ALGORITHMS_BUILD_DIR" ]; then
    mkdir -p $DYNAWO_ALGORITHMS_BUILD_DIR
  fi
  CMAKE_OPTIONAL=""
  if [ $DYNAWO_FORCE_CXX11_ABI = true ]; then
    CMAKE_OPTIONAL="$CMAKE_OPTIONAL -DFORCE_CXX11_ABI=$DYNAWO_FORCE_CXX11_ABI"
  fi
  cd $DYNAWO_ALGORITHMS_BUILD_DIR
  cmake -DCMAKE_C_COMPILER:PATH=$DYNAWO_C_COMPILER \
    -DCMAKE_CXX_COMPILER:PATH=$DYNAWO_CXX_COMPILER \
    -DCMAKE_BUILD_TYPE:STRING=$DYNAWO_BUILD_TYPE \
    -DDYNAWO_ALGORITHMS_HOME:PATH=$DYNAWO_ALGORITHMS_HOME \
    -DDYNAWO_HOME:PATH=$DYNAWO_HOME \
    -DBUILD_TESTS=$DYNAWO_BUILD_TESTS \
    -DBUILD_TESTS_COVERAGE=$DYNAWO_BUILD_TESTS_COVERAGE \
    -DCMAKE_INSTALL_PREFIX:PATH=$DYNAWO_ALGORITHMS_INSTALL_DIR \
    -DCXX11_ENABLED:BOOL=$DYNAWO_CXX11_ENABLED \
    -DXERCESC_HOME=$DYNAWO_XERCESC_HOME \
    -DBOOST_ROOT=$DYNAWO_BOOST_HOME/ \
    -DBOOST_ROOT_DEFAULT:STRING=FALSE \
    -DLIBZIP_HOME=$DYNAWO_LIBZIP_HOME \
    -DCMAKE_MODULE_PATH=$DYNAWO_HOME/share/cmake \
    -DDYNAWO_PYTHON_COMMAND="$DYNAWO_ALGORITHMS_PYTHON_COMMAND" \
    $CMAKE_OPTIONAL \
    -G "$DYNAWO_CMAKE_GENERATOR" \
    "-DCMAKE_PREFIX_PATH=$DYNAWO_LIBXML_HOME;$DYNAWO_HOME/share" \
    $DYNAWO_ALGORITHMS_SRC_DIR
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

# Compile dynawo-algorithms
build_dynawo_algorithms() {
  cd $DYNAWO_ALGORITHMS_BUILD_DIR
  cmake --build . &&
  cmake --build . --target install
  RETURN_CODE=$?
  return ${RETURN_CODE}
}


#build all doc
build_test_doc() {
  build_doc_dynawo_algorithms || error_exit
  test_doxygen_doc_dynawo_algorithms || error_exit
}

build_doc_dynawo_algorithms() {
  cd $DYNAWO_ALGORITHMS_BUILD_DIR
  mkdir -p $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/
  cmake --build . --target doc
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

test_doxygen_doc_dynawo_algorithms() {
  if [ -f $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings.txt  ] ; then
    nb_warnings=$(wc -l $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/warnings.txt | cut -f1 -d' ')
    if [ ${nb_warnings} -ne 0 ]; then
      echo "===================================="
      echo "| Result of doxygen doc generation |"
      echo "===================================="
      echo " nbWarnings = ${nb_warnings} > 0 => doc is incomplete"
      echo " edit ${DYNAWO_ALGORITHMS_INSTALL_DIR}/doxygen/warnings.txt  to have more details"
      error_exit "Doxygen doc is not complete"
    fi
  fi
}


build_tests() {
  clean_dynawo_algorithms || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
  tests=$@
  if [ -z "$tests" ]; then
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target tests --config Debug
  else
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target ${tests} --config Debug
  fi
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

build_tests_coverage() {
  clean_dynawo_algorithms || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
  tests=$@
  cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target reset-coverage --config Debug || error_exit "Error during reset-coverage."
  if [ -z "$tests" ]; then
    cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target tests-coverage --config Debug || error_exit "Error during tests-coverage."
  else
    for test in ${tests}; do
      cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target ${test}-coverage --config Debug || error_exit "Error during ${test}-coverage."
    done
  fi
  cmake --build $DYNAWO_ALGORITHMS_BUILD_DIR --target export-coverage --config Debug

  RETURN_CODE=$?
  if [ ${RETURN_CODE} -ne 0 ]; then
    exit ${RETURN_CODE}
  fi
  if [ "$DYNAWO_RESULTS_SHOW" = true ] ; then
    $DYNAWO_BROWSER $DYNAWO_ALGORITHMS_BUILD_DIR/coverage/index.html
  fi
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

#clean dynawo-algorithms
clean_dynawo_algorithms() {
  rm -rf $DYNAWO_ALGORITHMS_BUILD_DIR
}

# uninstall dynawo-algorithms
uninstall_dynawo_algorithms() {
  rm -rf $DYNAWO_ALGORITHMS_INSTALL_DIR
}

# Clean, then configure and build dynawo-algorithms
clean_build_dynawo_algorithms() {
  clean_dynawo_algorithms || error_exit
  uninstall_dynawo_algorithms || error_exit
  config_dynawo_algorithms || error_exit
  build_dynawo_algorithms || error_exit
}

clean_build_all_dynawo_algorithms() {
  clean_build_dynawo_algorithms || error_exit
  build_test_doc || error_exit
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

doc_dynawo_algorithms() {
  if [ ! -f $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/html/index.html ]; then
    echo "Doxygen documentation not yet generated"
    echo "Generating ..."
    build_test_doc
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
  echo "deploying Dynawo-algorithms libraries"
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/* bin/.
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/lib/* lib/.
  cp $DYNAWO_ALGORITHMS_INSTALL_DIR/include/* include/.
  cp -r $DYNAWO_ALGORITHMS_INSTALL_DIR/share/* share/.

  if [ -d "$DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen" ]; then
    mkdir -p doxygen
    echo "deploying doxygen"
    cp -r $DYNAWO_ALGORITHMS_INSTALL_DIR/doxygen/html doxygen/.
  fi

  popd > /dev/null
}

create_distrib() {
  DYNAWO_ALGORITHMS_VERSION=$(version)
  version=$(echo $DYNAWO_ALGORITHMS_VERSION | cut -f1 -d' ')

  ZIP_FILE=DynawoAlgorithms_V$version.zip

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
  cp -r $DYNAWO_HOME/dynawo.sh dynawo-algorithms/

  cp ../bin/* dynawo-algorithms/bin/
  cp ../lib/* dynawo-algorithms/lib/.
  cp -r ../share/* dynawo-algorithms/share/
  # combines dictionaries mapping
  cat $DYNAWO_HOME/share/dictionaries_mapping.dic | grep -v -F // | grep -v -e '^$' >> dynawo-algorithms/share/dictionaries_mapping.dic
  cp $DYNAWO_HOME/sbin/timeline_filter/timelineFilter.py dynawo-algorithms/bin/.
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
  DYNAWO_ALGORITHMS_VERSION=$(version)
  version=$(echo $DYNAWO_ALGORITHMS_VERSION | cut -f1 -d' ')

  ZIP_FILE=DynawoAlgorithms_headers_V$version.zip

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
  cp -r $DYNAWO_HOME/dynawo.sh dynawo-algorithms/
  cp -r $DYNAWO_HOME/dynawoEnv.txt dynawo-algorithms/

  cp ../bin/* dynawo-algorithms/bin/
  cp ../include/* dynawo-algorithms/include/
  cp ../lib/* dynawo-algorithms/lib/.
  cp -r ../share/* dynawo-algorithms/share/
  # combines dictionaries mapping
  cat $DYNAWO_HOME/share/dictionaries_mapping.dic | grep -v -F // | grep -v -e '^$' >> dynawo-algorithms/share/dictionaries_mapping.dic
  cp $DYNAWO_HOME/sbin/timeline_filter/timelineFilter.py dynawo-algorithms/bin/.
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

verify_browser() {
  if [ ! -x "$(command -v $DYNAWO_BROWSER)" ]; then
    error_exit "Specified browser DYNAWO_BROWSER=$DYNAWO_BROWSER not found."
  fi
}

nrt() {
  export_var_env_force DYNAWO_NRT_DIFF_DIR=$DYNAWO_HOME/sbin/nrt/nrt_diff
  $DYNAWO_ALGORITHMS_PYTHON_COMMAND -u $DYNAWO_ALGORITHMS_NRT_DIR/nrtAlgo.py $@
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

launch_CS() {
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType CS $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_SA() {
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_MC() {
  $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_CS_gdb() {
  gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType CS $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_SA_gdb() {
  gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType SA $@
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

launch_MC_gdb() {
  gdb -q --args $DYNAWO_ALGORITHMS_INSTALL_DIR/bin/dynawoAlgorithms --simulationType MC $@
  RETURN_CODE=$?
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
LAUNCH_COMMAND=$*
MODE=$1

## set environnement variables
set_environnement $MODE

ARGS=`echo ${LAUNCH_COMMAND} | cut -d' ' -f2-`
## launch command
case $MODE in
  config)
    config_dynawo_algorithms || error_exit "Error while configuring dynawo-algorithms"
    ;;

  build)
    config_dynawo_algorithms ||  error_exit "Error while configuring dynawo-algorithms"
    build_dynawo_algorithms || error_exit "Error while building dynawo-algorithms"
    ;;

  build-doc)
    build_test_doc || error_exit "Error while building doxygen documentation"
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

  deploy)
    deploy_dynawo_algorithms || error_exit "Error during the deployment of dynawo-algorithms"
    ;;

  clean)
    clean_dynawo_algorithms || error_exit "Error while cleaning dynawo-algorithms"
    ;;

  uninstall)
    uninstall_dynawo_algorithms || error_exit "Error while uninstalling dynawo-algorithms"
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

  display-environment)
    display_environmentVariables || error_exit "Failed to display environment variables"
    ;;

  clean-old-branches)
    clean_old_branches || error_exit "Error during the cleaning of old branches build/install/nrt"
    ;;

  doc)
    doc_dynawo_algorithms || error_exit "Error during dynawo-algorithms doc visualisation"
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
