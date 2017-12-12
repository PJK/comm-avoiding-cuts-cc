#!/usr/bin/env bash

module load daint-mc
module switch PrgEnv-cray PrgEnv-gnu
module load Score-P/3.0-CrayGNU-2016.11

buildir=$(mktemp -d ~/tmp_build.XXXX)

pushd $buildir
cmake -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=${PROJECT}/boost_1_63_0 -DCMAKE_CXX_FLAGS=-g -DCMAKE_CXX_COMPILER='scorep-CC' -DCMAKE_C_COMPILER='scorep-cc' -DCMAKE_LINKER='scorep-CC' ~/parallel-mincut-ipdps
make -j 8 SCOREP_WRAPPER_INSTRUMENTER_FLAGS=--mpp=mpi
