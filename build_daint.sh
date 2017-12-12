#!/usr/bin/env bash


module load daint-mc
module switch PrgEnv-cray PrgEnv-gnu
module load CMake Boost papi

buildir=$(mktemp -d ~/tmp_build.XXXX)

pushd $buildir
cmake -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/project/g34/kalvodap/boost_1_63_0 -DPAPI_PROFILE=ON -DWITH_METIS=ON -DWITH_GALOIS=ON ~/parallel-mincut-ipdps
make -j 8
BINDIR=${SCRATCH}/mincuts/executables
mkdir -p ${BINDIR}
cp src/executables/{square_root,seq_square_root,boost_stoer_wagner,karger_stein,parallel_cc,pbgl_cc,approx_cut,metis_cut,bgl_cc,galois_cc} ${BINDIR}
popd
echo "Output ready in ${buildir}. Executables copied to ${BINDIR}"
