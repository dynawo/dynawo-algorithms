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
# Dyna&omega;o-algorithms

[![Build Status](https://github.com/dynawo/dynawo-algorithms/workflows/CI/badge.svg)](https://github.com/dynawo/dynawo-algorithms/actions)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynawo-algorithms&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=dynawo_dynawo-algorithms)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynawo-algorithms&metric=coverage)](https://sonarcloud.io/summary/new_code?id=dynawo_dynawo-algorithms)
[![MPL-2.0 License](https://img.shields.io/badge/license-MPL_2.0-blue.svg)](https://www.mozilla.org/en-US/MPL/2.0/)

Dyna&omega;o-algorithms is a wrapper around [Dyna&omega;o](https://dynawo.github.io) that provides utility algorithms to calculate complex key values of a power system.

It can be used with any tool of the Dyna&omega;o toolsuite (DynaFlow, DynaWaltz, DynaSwing, DySym, DynaWave).

It provides the following possibilities:
  - **Unitary simulations**: simulations of one or several power systems test cases described in a .jobs file 
  (see [Dyna&omega;o documentation](https://github.com/dynawo/dynawo/releases/download/v1.3.1/DynawoDocumentation.pdf));
  - **Systematic analysis**: simulations of a same base power system test case subject to different events to assess the global stability;
  - **Margin calculation**: simulations of a same base power system test case with a load variation and subject to different events to compute the maximum load increase in a specific region before the voltage collapses;
  - **Load increase**: simulation of a single load variation.


## Get involved!

Dyna&omega;o-algorithms is an open-source project and as such, questions, discussions, feedbacks and more generally any form of contribution are very welcome and greatly appreciated!

For further informations about contributing guidelines, please refers to the [contributing documentation](https://github.com/dynawo/.github/blob/master/CONTRIBUTING.md).
## Dyna&omega;o algorithms Distribution

You can download a pre-built Dyna&omega;o-algorithms release to start testing it. Pre-built releases are available for **Linux**:
- [Linux](https://github.com/dynawo/dynawo-algorithms/releases/download/v1.3.1/DynawoAlgorithms_Linux_v1.3.1.zip)

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

## Building Dyna&omega;o-algorithms

### Optional requirements

You can install the following 3rd parties with system packages for Ubuntu 20.04:

``` bash
$> apt install -y libgoogle-perftools-dev mpich
```

For other Linux distributions similar packages should exist. If not they will be downloaded and installed during the next steps.

### Configure

You can either choose to build Dyna&omega;o-algorithms with a pre-built nightly version of Dyna&omega;o or by building Dyna&omega;o from scratch. Choose one of the two options below.

#### With pre-build dynawo nightly

``` bash
$> cd dynawo-algorithms
$> curl -LO https://github.com/dynawo/dynawo/releases/download/nightly/Dynawo_headers_V1.4.0.zip
$> unzip Dynawo_headers_V1.4.0.zip
```

``` bash
PATH_TO_DYNAWO_DEPLOY=$(pwd)/dynawo
```

#### By building dynawo

To build Dyna&omega;o and deploy, you can do:

``` bash
$> ./myEnvDynawo.sh build-all
$> ./myEnvDynawo.sh deploy
```

This command creates a deploy folder in ${DYNAWO_HOME}.
The path to dynawo deploy is then the path to the subdirectory `dynawo` in the deploy folder. It is generally similar to:

``` bash
PATH_TO_DYNAWO_DEPLOY=${DYNAWO_HOME}/deploy/gcc8/shared/dynawo/
```

#### Environment creation

To build Dyna&omega;o-algorithms you need to clone the repository and launch the following commands in the source code directory. it will create a `myEnvDynawoAlgorithms.sh` script file that will be your personal entrypoint to launch algorithms and configure some options.

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

To deploy Dyna&omega;o-algorithms, launch the following command after build:

``` bash
$> ./myEnvDynawoAlgorithms.sh deploy
```

## Tests

To launch Dyna&omega;o-algorithms unit tests, launch the following command:

``` bash
$> ./myEnvDynawoAlgorithms.sh build-tests
```

To launch Dyna&omega;o-algorithms nrt, launch the following command after build:

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

## Dyna&omega;o-algorithms Documentation
You can download Dyna&omega;o-algorithms documentation [here](https://github.com/dynawo/dynawo-algorithms/releases/download/v1.3.1/DynawoAlgorithmsDocumentation.pdf).

## Quoting Dyna&omega;o

If you use Dyna&omega;o or Dyna&omega;o-algorithms in your work or research, it is not mandatory but we kindly ask you to quote the following paper in your publications or presentations:

A. Guironnet, M. Saugier, S. Petitrenaud, F. Xavier, and P. Panciatici, “Towards an Open-Source Solution using Modelica for Time-Domain Simulation of Power Systems,” 2018 IEEE PES Innovative Smart Grid Technologies Conference Europe (ISGT-Europe), Oct. 2018.

## License

Dyna&omega;o-algorithms is licensed under the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0. You can also see the [LICENSE](LICENSE.txt) file for more information.

Dyna&omega;o-algorithms is using some external libraries to run simulations:
* [gperftools](https://github.com/gperftools/gperftools), a collection of a high-performance multi-threaded malloc implementations distributed under the BSD license. Dyna&omega;o-algorithms is currently using the version 2.6.1.
* [MPICH](https://www.mpich.org/), an implementation of the Message Passing Interface (MPI) standard distributed under a BSD-like license. Dyna&omega;o-algorithms currently using the version 3.4.2.

## Maintainers

Dyna&omega;o-algorithms is currently maintained by the following people in RTE:

* Mathilde Bongrain, [mathilde.bongrain@rte-france.com](mailto:mathilde.bongrain@rte-france.com)
* Gautier Bureau, [gautier.bureau@rte-france.com](mailto:gautier.bureau@rte-france.com)
* Marco Chiaramello, [marco.chiaramello@rte-france.com](mailto:marco.chiaramello@rte-france.com)
* Quentin Cossart, [quentin.cossart@rte-france.com](mailto:quentin.cossart@rte-france.com)
* Adrien Guironnet, [adrien.guironnet@rte-france.com](mailto:adrien.guironnet@rte-france.com)
* Florentine Rosiere, [florentine.rosiere@rte-france.com](mailto:florentine.rosiere@rte-france.com)

In case of questions or issues, you can also send an e-mail to [rte-dynawo@rte-france.com](mailto:rte-dynawo@rte-france.com).

## Links

For more information about Dyna&omega;o and Dyna&omega;o-algorithms:

* Consult [Dyna&omega;o website](http://dynawo.org)
* Contact us at [rte-dynawo@rte-france.com](mailto:rte-dynawo@rte-france.com)
