#!/usr/bin/env bash

input=$1
time=$2
cores=$3

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

out=${SCRATCH}/real_world
mkdir -p ${out}

seed=$RANDOM


run ${input} ${seed} ${cores} ${time} ${out}/$(basename ${input})_${cores}
