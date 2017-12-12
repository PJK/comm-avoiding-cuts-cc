#!/usr/bin/env bash

# Runs a single execution with a fixed seed and a single input on an increasing number of cores.
# Usage: INPUT TIME_ESTIMATE_MINUTES OUTPUT_DIR

input=$1
time_estimate=$2
output_dir=$3


cores=( 16 64 128 256 384 512 768 1024 1280 )

for cores_count in "${cores[@]}"; do
	echo "#!/usr/bin/env bash
srun --cpu_bind=rank $SCRATCH/square_root 0.95 $input 0
	 " | sbatch -n ${cores_count} \
	 	--ntasks-per-node=36 \
		--time=00:$(printf %02d ${time_estimate}):00 \
		--output=${output_dir}/${cores_count}_%j.out

	# https://rc.fas.harvard.edu/resources/documentation/submitting-large-numbers-of-jobs-to-odyssey/
	sleep 1
done
