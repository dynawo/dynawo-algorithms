#!/bin/bash

# Except where otherwise noted, content in this documentation is Copyright (c)
# 2022, RTE (http://www.rte-france.com) and licensed under a
# CC-BY-4.0 (https://creativecommons.org/licenses/by/4.0/)
# license. All rights reserved.

folders=(introduction installation configuringDynawoAlgorithms advancedDoc
licenses/dynawo licenses/dynawo-algorithms licenses/dynawo-algorithms-documentation licenses/gperftools licenses/mpich licenses/cpplint)

for folder in ${folders[*]}; do
  if [ -d "$folder" ]; then
    (cd $folder; rm -f *.toc *.aux *.bbl *.blg *.log *.out *.pdf *.gz *.mtc* *.maf *.lof)
  fi
done

rm -rf dynawoAlgorithmsDocumentation
