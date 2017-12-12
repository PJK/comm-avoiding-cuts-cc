#!/usr/bin/env bash

reps=$1

cores=( 128 256 512 1024 2048 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/d1x

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		process_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/d1 10:00 ${corec} ${out} ${seed}
	done
done
