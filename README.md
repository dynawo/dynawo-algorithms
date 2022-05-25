<!--
    Copyright (c) 2015-2021, RTE (http://www.rte-france.com)
    See AUTHORS.txt
    All rights reserved.
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, you can obtain one at http://mozilla.org/MPL/2.0/.
    SPDX-License-Identifier: MPL-2.0

    This file is part of Dynawo, an hybrid C++/Modelica open source suite
    of simulation tools for power systems.
-->

# Dyna&omega;o algorithms

[![Build Status](https://github.com/dynawo/dynawo-algorithms/workflows/CI/badge.svg)](https://github.com/dynawo/dynawo-algorithms/actions)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynawo-algorithms&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=dynawo_dynawo-algorithms)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynawo-algorithms&metric=coverage)](https://sonarcloud.io/summary/new_code?id=dynawo_dynawo-algorithms)
[![MPL-2.0 License](https://img.shields.io/badge/license-MPL_2.0-blue.svg)](https://www.mozilla.org/en-US/MPL/2.0/)

Dyna&omega;o algorithms aims to contain the complex algorithms used in analysis over multiple simulation such as:

- systematic analysis
- margin calulation

It depends on the [core dynawo libraries of Dyna&omega;o](https://github.com/dynawo/dynawo)

## Dyna&omega;o algorithms Distribution

You can download a pre-built Dyna&omega;o algorithms release to start testing it. Pre-built releases are available for **Linux**:
- [Linux](https://github.com/dynawo/dynawo-algorithms/releases/download/v1.3.0/DynawoAlgorithms_Linux_v1.3.0.zip)

### Linux Requirements for Distribution

- Binary utilities: [curl](https://curl.haxx.se/) and unzip

``` bash
$> apt install -y unzip curl
$> dnf install -y unzip curl
```

### Using a distribution

#### Linux

You can launch the following commands to download and test the latest distribution:

``` bash
$> curl -L $(curl -s -L -X GET https://api.github.com/repos/dynawo/dynawo-algorithms/releases/latest | grep "DynawoAlgorithms_Linux_v" | grep url | cut -d '"' -f 4) -o DynawoAlgorithms_Linux_latest.zip
$> unzip DynawoAlgorithms_Linux_latest.zip
$> cd dynawo-algorithms
$> ./dynawo-algorithms.sh --help
$> ./dynawo-algorithms.sh CS --input examples/CS/IEEE14.jobs
$> ./dynawo-algorithms.sh SA --input fic_MULTIPLE.xml --output testSA.zip --directory examples/SA
$> ./dynawo-algorithms.sh MC --input fic_MULTIPLE.xml --output testMC.zip --directory examples/MC
```

## Building Dyna&omega;o algorithms

### Optionnal requirements

You can install the following 3rd parties with system packages for Ubuntu 20.04:

``` bash
$> apt install -y libgoogle-perftools-dev mpich
```

For other Linux distributions similar packages should exist. If not they will be downloaded and installed during the next steps.

### Configure

To build Dyna&omega;o algorithms, you must first deploy the Dyna&omega;o library.

``` bash
$> ./myEnvDynawo.sh build-all
$> ./myEnvDynawo.sh deploy
```

This command creates a deploy folder in ${DYNAWO_HOME}.
The path to dynawo deploy is then the path to the subdirectory `dynawo` in the deploy folder. It is generally similar to:

``` bash
PATH_TO_DYNAWO_DEPLOY=${DYNAWO_HOME}/deploy/gcc8/shared/dynawo/
```

To build Dyna&omega;o algorithms you need to clone the repository launch the following commands in the source code directory. it will create a `myEnvDynawoAlgorithms.sh` script file that will be your personal entrypoint to launch algorithms and configure some options.

``` bash
$> echo '#!/bin/bash
export DYNAWO_ALGORITHMS_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=PATH_TO_DYNAWO_DEPLOY

export DYNAWO_LOCALE=en_GB
export DYNAWO_RESULTS_SHOW=true
export DYNAWO_BROWSER=firefox

export DYNAWO_NB_PROCESSORS_USED=1

export DYNAWO_BUILD_TYPE=Release

$DYNAWO_ALGORITHMS_HOME/util/envDynawoAlgorithms.sh $@' > myEnvDynawoAlgorithms.sh
$> chmod +x myEnvDynawoAlgorithms.sh
```

Then update the path "PATH_TO_DYNAWO_DEPLOY" in the file to your deployed installation of Dyna&omega;o

### Build

launch the following command:

``` bash
$> ./myEnvDynawoAlgorithms.sh build
```

All commands described in the rest of this README are accessible throught this script. To access all options of the script myEnvDynawoAlgorithms.sh, type:

``` bash
$> ./myEnvDynawoAlgorithms.sh help
```

To deploy Dyna&omega;o algorithms, launch the following command after build:

``` bash
$> ./myEnvDynawoAlgorithms.sh deploy
```

## Tests

To launch Dyna&omega;o algorithms unit tests, launch the following command:

``` bash
$> ./myEnvDynawoAlgorithms.sh build-tests
```

To launch Dyna&omega;o algorithms nrt, launch the following command after build:

``` bash
$> ./myEnvDynawoAlgorithms.sh nrt
```

## Launching a simulation

For more details on the fic_MULTIPLE.xml and aggregatedResults.xml formats please refer to the documentation.

``` bash
$> ./myEnvDynawoAlgorithms.sh CS --input PATH_TO_JOBS
```

## Launching a systematic analysis

``` bash
$> ./myEnvDynawoAlgorithms.sh SA --directory PATH_TO_SA_FOLDER --input fic_MULTIPLE.xml --output aggregatedResults.xml --nbThreads NB_THREADS_TO_USE
```

fic_MULTIPLE.xml can be replaced by an archive file containing all the required input files

## Launching a margin calculation

``` bash
$> ./myEnvDynawoAlgorithms.sh MC --directory PATH_TO_MC_FOLDER --input fic_MULTIPLE.xml --output aggregatedResults.xml --nbThreads NB_THREADS_TO_USE
```

fic_MULTIPLE.xml can be replaced by an archive file containing all the required input files

## Creating the distribution

``` bash
$> ./myEnvDynawoAlgorithms.sh distrib
```
