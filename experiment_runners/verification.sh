#!/usr/bin/env bash

# Runs a single execution with a fixed seed and a single input on an increasing number of cores.
# Usage: INPUT_DIR TIME_ESTIMATE_MINUTES OUTPUT_DIR

input_dir=$1
time_estimate=$2
output_dir=$3

inputs=$(ls ${input_dir}/*.in | xargs -n1 basename)

for input in ${inputs}; do
	echo "#!/usr/bin/env bash
srun --cpu_bind=rank $SCRATCH/square_root 0.95 $input_dir/$input 0
	 " | sbatch -n 16 \
		--time=00:$(printf %02d ${time_estimate}):00 \
		--output=${output_dir}/sqrt_${input}_%j.out

	serial_bound=$(($time_estimate * 5))
	echo "#!/usr/bin/env bash
srun --cpu_bind=rank $SCRATCH/karger_stein $input_dir/$input 0
	 " | sbatch -n 1 \
		--time=00:$(printf %02d ${serial_bound}):00 \
		--output=${output_dir}/ks_${input}_%j.out

	# https://rc.fas.harvard.edu/resources/documentation/submitting-large-numbers-of-jobs-to-odyssey/
	sleep 1
done
