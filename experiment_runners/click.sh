#!/usr/bin/env bash

size=$1
time=$2
cores=$3

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
. ${DIR}/common.sh

out=${SCRATCH}/real_world
mkdir -p ${out}

seed=$RANDOM


run_click ${size} ${seed} ${cores} ${time} ${out}/CLICK_${size}_${cores}
