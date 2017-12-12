#!/usr/bin/env bash
set -e
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function fetch() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

function fetch_cc() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/cc_header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

function fetch_seq() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/seq_header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

function fetch_cc_seq() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/cc_seq_header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

function fetch_ks() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/ks_header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

function fetch_baseline() {
	echo ${1}
	cat ${DIR}/../sc_evaluation/data/baseline_header.csv > ${DIR}/../sc_evaluation/data/$1
	cat sc_outputs/sc_evaluation_outputs/${2} | ${DIR}/parallel_papi_transform.rb | gsort -V >> ${DIR}/../sc_evaluation/data/$1
}

rsync --delete -av -e "ssh -A kalvodap@ela.cscs.ch ssh" kalvodap@daint.cscs.ch:/scratch/snx3000/kalvodap/sc_evaluation_outputs sc_outputs

fetch s1/ws.csv 's1/ws_96k*'
fetch s1/ba.csv 's1/ba_96k*'
fetch s1/er.csv 's1/er_96k*'
fetch_baseline s1/baseline_ba_sw.csv 'baselines/s1/sw_ba_*'
fetch_baseline s1/baseline_ws_sw.csv 'baselines/s1/sw_ws_*'
fetch_baseline s1/baseline_er_sw.csv 'baselines/s1/sw_er_*'
fetch_baseline s1/baseline_ks_ws.csv 'baselines/s1/ws_*_baseline_*'
fetch_baseline s1/baseline_ks_er.csv 'baselines/s1/er_*_baseline_*'
fetch_baseline s1/baseline_ks_ba.csv 'baselines/s1/ba_*_baseline_*'


fetch s2/ws.csv 's2/*'
fetch s2/baseline.csv 'baselines/s2/*'

fetch d1x/rmat.csv 'd1x/parmat_16k*'

fetch d1/rmat.csv 'd1/parmat_16k*'

fetch d2/parmat.csv 'd2/rmat_*'

fetch fp/16.csv 'fp/16/*'
fetch fp/128.csv 'fp/128/*'
fetch fp/512.csv 'fp/512/*'


fetch_cc cc1/ws.csv 'cc1fixed/ws_*'
fetch_cc cc1/ba.csv 'cc1fixed/ba_*'
fetch_cc cc1/er.csv 'cc1fixed/er_*'
fetch_cc_seq cc1/bgl_ws.csv 'baselines/cc1fixed/bgl_ws_*'
fetch_cc_seq cc1/bgl_ba.csv 'baselines/cc1fixed/bgl_ba_*'
fetch_cc_seq cc1/bgl_er.csv 'baselines/cc1fixed/bgl_er_*'


fetch_cc cc1/pbgl_ws.csv 'cc1/pbgl_ws_*'
fetch_cc cc1/pbgl_ba.csv 'cc1fixed/pbgl_ba_*'
fetch_cc cc1/pbgl_er.csv 'cc1/pbgl_er_*'

fetch_cc cc1/ws.csv 'cc1fixed/ws_*'
fetch_cc cc1/pbgl_ba.csv 'cc1fixed/pbgl_ba_*'
fetch_cc cc1/galois_ba.csv 'cc1fixed/galois_ba_*'


fetch_cc cc2/rmat.csv 'cc2fixed/rmat_*'
fetch_cc cc2/er.csv 'cc2fixed/er_*'

fetch_cc_seq cc2/bgl_rmat.csv 'baselines/cc2fixed/bgl_rmat_*'
fetch_cc_seq cc2/bgl_er.csv 'baselines/cc2fixed/bgl_er_*'


fetch_cc cc2/pbgl_rmat.csv 'cc2/pbgl_rmat_*'
fetch_cc cc2/pbgl_er.csv 'cc2/pbgl_er_*'
fetch_cc cc2/pbgl_rmat_papi.csv 'cc2pbgl/pbgl_*'

fetch_cc cc2/rmat.csv 'cc2mpitimes_fixed/rmat_*'
fetch_cc cc2/pbgl_rmat.csv 'cc2mpitimes_fixed/pbgl_rmat_*'
fetch_cc cc2/galois_rmat.csv 'cc2mpitimes_fixed/galois_rmat_*'

fetch as1/ws.csv 'as1/ws_96k*'
fetch as1/ba.csv 'as1/ba_96k*'
fetch as1/er.csv 'as1/er_96k*'

fetch as1/wsf.csv 'as1fixed/ws_96k*'
fetch as1/baf.csv 'as1fixed/ba_96k*'
fetch as1/erf.csv 'as1fixed/er_96k*'

fetch as2/ws.csv 'as2/*'

fetch as2/wsf.csv 'as2f/*'

fetch awx/rmat.csv 'awxfixed/*'

fetch ad2/parmat.csv 'ad2/rmat_*'

fetch ads/rmat.csv 'adsfixed/rmat_*'

fetch_cc_seq cc_cc_seq/rmat.csv 'baselines/cc_cc_seq/rmat_*'
fetch_cc_seq cc_cc_seq/bgl_rmat.csv 'baselines/cc_cc_seq/bgl_rmat_*'
fetch_cc_seq cc_cc_seq/galois_rmat.csv 'baselines/cc_cc_seq/galois_rmat_*'

fetch_seq cc_seq/er.csv 'baselines/cc_seq_er/er_*k_??.in_???????.out*'
fetch_ks cc_seq/ks_er.csv 'baselines/cc_seq_er/*_baseline_*'
fetch_seq cc_seq/sw_er.csv 'baselines/cc_seq_er/sw_*'
