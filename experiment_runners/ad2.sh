#!/usr/bin/env bash

reps=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

out=${SCRATCH}/sc_evaluation_outputs/ad2f
mkdir -p ${out}

inputs=( 4 8 12 16 20 24 )
cores=( 144 288 432 576 720 864 )

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for id in $(seq 0 5); do
		input="${SCRATCH}/mincuts/sc_evaluation_inputs/d2/rmat_${inputs[$id]}k_1k.in"
		corecount="${cores[$id]}"

		run_approx ${input} ${seed} ${corecount} 10:00 ${out}/$(basename ${input})_${corecount}
	done
done
