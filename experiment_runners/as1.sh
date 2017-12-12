#!/usr/bin/env bash

reps=$1

cores=( 144 288 432 576 720 864 1008 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/as1fixed

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		process_dir_approx ${SCRATCH}/mincuts/sc_evaluation_inputs/s1 5:00 ${corec} ${out} ${seed}
	done
done
