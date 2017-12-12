#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."

degrees=( 16 32 64 128 256 512 1024 2048)

for degree in "${degrees[@]}"; do
	${DIR}/utils/generate.py 'random_regular_graph(2 * K, N)' 8000 -k ${degree} > $1/regular_8k_${degree}.in
done
