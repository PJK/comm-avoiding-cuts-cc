#!/usr/bin/env bash

reps=$1

cores=( 24 48 96 192 384 768 1536 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/d1

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		process_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/d1 30:00 ${corec} ${out} ${seed}
	done
done
