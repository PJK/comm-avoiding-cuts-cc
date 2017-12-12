#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

reps=$1

cores=288
out_base=${SCRATCH}/sc_evaluation_outputs/fp

densities=( 16 128 512 )


for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for density in "${densities[@]}"; do
		out=${out_base}/${density}
		mkdir -p ${out}

		for corec in "${cores[@]}"; do
			process_dir ${SCRATCH}/mincuts/sc_evaluation_inputs/fp/${density} 10:00 ${corec} ${out} ${seed}
		done
	done
done
