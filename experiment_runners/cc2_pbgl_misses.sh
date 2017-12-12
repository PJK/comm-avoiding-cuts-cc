#!/usr/bin/env bash

reps=$1

cores=( 1 9 18 36 72 108 144 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/cc2pbgl

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		run_pbglcc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc2/rmat_128k_1k.in ${corec} 60:00 ${out} 300
	done
done
