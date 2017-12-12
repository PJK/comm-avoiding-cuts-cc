#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

parmat=$1
out=$2

sizes=( 128 256 384 512 640 768 896 1024 )

for size in "${sizes[@]}"; do
	${DIR}/rmat_driver.sh ${parmat} $((size * 1000)) $((size * 1000 * 128)) > $out/rmat_${size}k_128.in
done
