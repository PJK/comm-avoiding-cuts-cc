#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


cc1=${SCRATCH}/sc_evaluation_outputs/baselines/cc1fixed
mkdir -p ${cc1}
process_dir_bglcc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc1 120:00 ${cc1} 100

cc2=${SCRATCH}/sc_evaluation_outputs/baselines/cc2fixed
mkdir -p ${cc2}
process_dir_bglcc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc2 120:00 ${cc2} 100


