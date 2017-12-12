#!/usr/bin/env bash

reps=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


cc=${SCRATCH}/sc_evaluation_outputs/baselines/cc_cc_seq
mkdir -p ${cc}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	process_dir_bglcc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_cc_seq 60:00 ${cc} 100
	process_dir_cc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_cc_seq 60:00 1 ${cc} ${seed} 100
	process_dir_galois ${SCRATCH}/mincuts/sc_evaluation_inputs/cc_cc_seq 60:00 1 ${cc} 100
done
