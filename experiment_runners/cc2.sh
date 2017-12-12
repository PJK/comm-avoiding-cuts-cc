#!/usr/bin/env bash

reps=$1

galois_cores=( 1 2 3 6 9 12 15 18 21 24 27 30 33 36 )
cores=( 1 2 3 6 9 12 15 18 21 24 27 30 33 36 42 48 54 60 66 72 )

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh


out=${SCRATCH}/sc_evaluation_outputs/cc2mpitimes_fixed

mkdir -p ${out}

for _rep in $(seq 1 ${reps}); do
	seed=$RANDOM

	for corec in "${cores[@]}"; do
		process_dir_cc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc2 40:00 ${corec} ${out} ${seed} 300
	done

	for corec in "${cores[@]}"; do
		process_dir_pbglcc ${SCRATCH}/mincuts/sc_evaluation_inputs/cc2 35:00 ${corec} ${out} 30
	done

	for corec in "${galois_cores[@]}"; do
		process_dir_galois ${SCRATCH}/mincuts/sc_evaluation_inputs/cc2 40:00 ${corec} ${out} 100
	done
done
