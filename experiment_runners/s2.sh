#!/usr/bin/env bash

reps=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

out=${SCRATCH}/sc_evaluation_outputs/s2
mkdir -p ${out}

inputs=( 16 32 48 64 80 96 )
cores=( 144 288 432 576 720 864 )

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for id in $(seq 0 1); do
		input="${SCRATCH}/mincuts/sc_evaluation_inputs/s2/ws_${inputs[$id]}k_16.in"
		corecount="${cores[$id]}"

		run ${input} ${seed} ${corecount} 10:00 ${out}/$(basename ${input})_${corecount}
	done
done
