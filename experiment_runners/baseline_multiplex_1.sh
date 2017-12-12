#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh



fp_16=${SCRATCH}/sc_evaluation_outputs/baselines/fp/16
mkdir -p ${fp_16}
baseline_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/fp/16 10:00 ${fp_16} 36

s1=${SCRATCH}/sc_evaluation_outputs/baselines/s1
mkdir -p ${s1}
baseline_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/s1 40:00 ${s1} 20
run_one_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/s1/er_96k_16.in 400:00 ${s1} 20
run_one_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/s1/er_96k_16.in 400:00 ${s1} 20
run_one_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/s1/er_96k_16.in 400:00 ${s1} 20

s2=${SCRATCH}/sc_evaluation_outputs/baselines/s2
mkdir -p ${s2}
baseline_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/s2 30:00 ${s2} 20
baseline_dir_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/s2 360:00 ${s2} 20


d1=${SCRATCH}/sc_evaluation_outputs/baselines/d1
mkdir -p ${d1}
baseline_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/d1 360:00 ${d1} 9
baseline_dir_sw ${SCRATCH}/mincuts/sc_evaluation_inputs/s2 360:00 ${s2} 9

