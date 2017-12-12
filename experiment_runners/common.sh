#!/usr/bin/env bash

function run() {
	local input=$1
	local seed=$2
	local cores=$3
	local time=$4
	local output=$5

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/square_root 0.90 $input $seed
	 " | sbatch -n ${cores} \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_click() {
	local size=$1
	local seed=$2
	local cores=$3
	local time=$4
	local output=$5

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/square_root 0.90 CLICK $size $seed
	 " | sbatch -n ${cores} \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_extra() {
	local input=$1
	local time=$2
	local cores=$3
	local output=$4
	local seed=$5

	run ${input} ${seed} ${cores} ${time} ${output}/$(basename ${input})_${cores}
}


function run_approx() {
	local input=$1
	local seed=$2
	local cores=$3
	local time=$4
	local output=$5

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/approx_cut 0.90 $input $seed
	 " | sbatch -n ${cores} \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_cc() {
	local input=$1
	local seed=$2
	local cores=$3
	local time=$4
	local output=$5
	local reps=$6

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/parallel_cc $input $seed $reps
	 " | sbatch -n ${cores} \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_pbglcc() {
	local input=$1
	local cores=$2
	local time=$3
	local output=$4
	local reps=$5

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/pbgl_cc $input $reps
	 " | sbatch -n ${cores} \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_bglcc() {
	local input=$1
	local time=$2
	local output=$3
	local reps=$4

	echo "#!/usr/bin/env bash
srun --cpu_bind=rank ${SCRATCH}/mincuts/executables/bgl_cc $input $reps
	 " | sbatch -n 1 \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

function run_galois() {
	local input=$1
	local cores=$2
	local time=$3
	local output=$4
	local reps=$5

	echo "#!/usr/bin/env bash
srun ${SCRATCH}/mincuts/executables/galois_cc $input $cores $reps
	 " | sbatch \
		-C mc \
		--time=${time} \
		--output=${output}_%j.out
}

# TODO node tiling for lwo concurrency

function dir_contents() {
	echo $(ls $1/*.in)
}

function galois_dir_contents() {
	echo $(ls $1/*.gl)
}

function process_dir() {
	local dir=$1
	local time=$2
	local cores=$3
	local output_dir=$4
	local seed=$5

	for input in $(dir_contents ${dir}); do
		run ${input} ${seed} ${cores} ${time} ${output_dir}/$(basename ${input})_${cores}
	done
}

function process_dir_approx() {
	local dir=$1
	local time=$2
	local cores=$3
	local output_dir=$4
	local seed=$5

	for input in $(dir_contents ${dir}); do
		run_approx ${input} ${seed} ${cores} ${time} ${output_dir}/$(basename ${input})_${cores}
	done
}

function process_dir_cc() {
	local dir=$1
	local time=$2
	local cores=$3
	local output_dir=$4
	local seed=$5
	local reps=$6

	for input in $(dir_contents ${dir}); do
		run_cc ${input} ${seed} ${cores} ${time} ${output_dir}/$(basename ${input})_${cores} ${reps}
	done
}

function process_dir_pbglcc() {
	local dir=$1
	local time=$2
	local cores=$3
	local output_dir=$4
	local reps=$5

	for input in $(dir_contents ${dir}); do
		run_pbglcc ${input} ${cores} ${time} ${output_dir}/pbgl_$(basename ${input})_${cores} ${reps}
	done
}

function process_dir_bglcc() {
	local dir=$1
	local time=$2
	local output_dir=$3
	local reps=$4

	for input in $(dir_contents ${dir}); do
		run_bglcc ${input} ${time} ${output_dir}/bgl_$(basename ${input}) ${reps}
	done
}

function process_dir_galois() {
	local dir=$1
	local time=$2
	local cores=$3
	local output_dir=$4
	local reps=$5

	for input in $(galois_dir_contents ${dir}); do
		run_galois ${input} ${cores} ${time} ${output_dir}/galois_$(basename ${input}) ${reps}
	done
}

function run_two() {
	local input1=$1
	local input2=$2
	local time=$3
	local output=$4


	echo "#!/usr/bin/env bash
$(for i in $(seq 1 18); do
	echo "$SCRATCH/karger_stein $input1 $(($seed + $i)) &"
done)

$(for i in $(seq 1 18); do
	echo "$SCRATCH/karger_stein $input2 $(($seed + 0)) &"
done)

wait
		 " | sbatch --time=${time} \
			-N 1 \
			--output=${output}/basline_%j.out
}

# hi memory
function run_one() {
	local input=$1
	local seed=$2
	local time=$3
	local output=$4
	local multiplex=$5


	echo "#!/usr/bin/env bash
$(for i in $(seq 1 ${multiplex}); do
	echo "$SCRATCH/mincuts/executables/karger_stein $input $(($seed + $i)) &"
done)
wait
		 " | sbatch -C mc \
			-N 1 \
			--output=${output}_baseline_%j.out \
			--time=${time}
}

function run_one_seq() {
	local input=$1
	local seed=$2
	local time=$3
	local output=$4
	local multiplex=$5


	echo "#!/usr/bin/env bash
$(for i in $(seq 1 ${multiplex}); do
	echo "$SCRATCH/mincuts/executables/seq_square_root 0.9 $input $(($seed + $i)) &"
done)
wait
		 " | sbatch -C mc \
			-N 1 \
			--partition=debug \
			--output=${output}_%j.out \
			--time=${time}
}


function run_one_sw() {
	local input=$1
	local time=$2
	local output=$3
	local multiplex=$4


	echo "#!/usr/bin/env bash
$(for i in $(seq 1 ${multiplex}); do
	echo "$SCRATCH/mincuts/executables/boost_stoer_wagner $input &"
done)
wait
		 " | sbatch -C mc \
			-N 1 \
			--output=${output}_%j.out \
			--time=${time}
}

function baseline_dir() {
	local dir=$1
	local time=$2
	local output=$3
	local multiplex=$4
	seed=$RANDOM

	for input in $(dir_contents ${dir}); do
		run_one ${input} ${seed} ${time} ${output}/$(basename ${input}) ${multiplex}
	done
}


function multiplex_dir() {
	local dir=$1
	local time=$2
	local output=$3
	local multiplex=$4
	seed=$RANDOM

	for input in $(dir_contents ${dir}); do
		run_one_seq ${input} ${seed} ${time} ${output}/$(basename ${input}) ${multiplex}
	done
}

function baseline_dir_sw() {
	local dir=$1
	local time=$2
	local output=$3
	local multiplex=$4

	for input in $(dir_contents ${dir}); do
		run_one_sw ${input} ${time} ${output}/sw_$(basename ${input}) ${multiplex}
	done
}
