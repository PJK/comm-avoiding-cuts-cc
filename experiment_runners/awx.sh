#!/usr/bin/env bash

reps=$1

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

out=${SCRATCH}/sc_evaluation_outputs/awxfixed
mkdir -p ${out}

inputs=( 256 512 768 1024 1280 1536 1792 2048 )
cores=( 72 144 216 288 360 432 504 576 )

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for id in $(seq 0 5); do
		input="${SCRATCH}/mincuts/sc_evaluation_inputs/awx/rmat_16k_${inputs[$id]}.in"
		corecount="${cores[$id]}"

		run_approx ${input} ${seed} ${corecount} 5:00 ${out}/$(basename ${input})_${corecount}
	done
done
