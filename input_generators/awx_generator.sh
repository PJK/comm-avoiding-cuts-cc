#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

parmat=$1
out=$2

inputs=( 256 512 768 1024 1280 1536 1792 2048 )

for degree in "${inputs[@]}"; do
	${DIR}/rmat_driver.sh ${parmat} 16000 $((16000 * $degree)) > $out/rmat_16k_${degree}.in
done
