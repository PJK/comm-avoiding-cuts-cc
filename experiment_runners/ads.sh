#!/usr/bin/env bash

reps=$1

cores=( 36 72 108 144 180 216 252 288 324 360 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/adsfixed

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		process_dir_approx ${SCRATCH}/mincuts/sc_evaluation_inputs/ads 20:00 ${corec} ${out} ${seed}
	done
done
