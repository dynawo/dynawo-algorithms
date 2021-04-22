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

## Building Dyna&omega;o algorithms

You need to deploy Dyna&omega;o first.  
Then you need to launch the following commands:

``` bash
$> echo '#!/bin/bash
export DYNAWO_ALGORITHMS_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=PATH_TO_DYNAWO_DEPLOY

export DYNAWO_LOCALE=en_GB
export DYNAWO_RESULTS_SHOW=true
export DYNAWO_BROWSER=firefox

export DYNAWO_NB_PROCESSORS_USED=1

export DYNAWO_BUILD_TYPE=Release
export DYNAWO_CXX11_ENABLED=YES

$DYNAWO_ALGORITHMS_HOME/util/envDynawoAlgorithms.sh $@' > myEnvDynawoAlgorithms.sh
$> chmod +x myEnvDynawoAlgorithms.sh
$> ./myEnvDynawoAlgorithms.sh build
```

For more details on the fic_MULTIPLE.xml and aggregatedResults.xml formats please refer to the documentation.
## Launching a simulation

``` bash
$> ./myEnvDynawoAlgorithms.sh CS --input PATH_TO_JOBS

```

## Launching a systematic analysis

``` bash
$> ./myEnvDynawoAlgorithms.sh SA --directory PATH_TO_SA_FOLDER --input fic_MULTIPLE.xml --output aggregatedResults.xml --nbThreads NB_THREADS_TO_USE

```

## Launching a margin calculation
``` bash
$> ./myEnvDynawoAlgorithms.sh MC --directory PATH_TO_MC_FOLDER --input fic_MULTIPLE.xml --output aggregatedResults.xml --nbThreads NB_THREADS_TO_USE

```

## Creating the distribution

``` bash
$> ./myEnvDynawoAlgorithms.sh distrib

```


