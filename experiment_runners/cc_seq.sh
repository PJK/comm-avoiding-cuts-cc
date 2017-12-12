#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


cc=${SCRATCH}/sc_evaluation_outputs/baselines/cc_seq_er
mkdir -p ${cc}

baseline_dir_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_seq 190:00 ${cc} 36
multiplex_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_seq 20:00 ${cc} 36
run_one_seq ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_seq/er_8k_16.in 0 20:00 ${cc}/$(basename ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_seq/er_8k_16.in) 36
baseline_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_seq 10:00 ${cc} 36
