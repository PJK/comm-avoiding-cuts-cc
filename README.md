# Parallel Communication-Avoiding Minimum Cuts and Connected Components

This is an implementation of the PPoPP 2018 "Parallel Communication-Avoiding Minimum Cuts and Connected Components" paper by

- [Pavel Kalvoda](https://github.com/PJK)
- [Lukas Gianinazzi](https://github.com/glukas)
- [Alessandro de Palma](https://github.com/AleDepo93)

## Overview
The main C++ MPI application is located in `src`. It implements both the sparse and the dense algorithm, as well as the sequential base cases.

We also provide our experimental setup. The inputs were (partly) generated by the scripts in `input_generators`, which use the following software

 - [NetworkX](https://networkx.github.io/) (see below)
 - [PaRMAT](https://github.com/farkhor/PaRMAT)

We have also used the generators provided by Levine along with "Experimental Study of Minimum Cut Algorithms", and the real-world inputs as described in the paper. The large dense graph is generated in memory.
 
The experiments were executed on the [CSCS Piz Daint](https://www.cscs.ch/computers/piz-daint/) Cray XC50. We provide the automation scripts in `experiment_runner`. Please note that these are specific for the system and our FS layout and are only included for completeness. Before executing, be sure to adapt them to your setup.
 
## Building and running
The following dependencies are required:

 - Boost 1.64
 - C++11 compiler
 - MPI3 libraries and runtime
 - CMake 3.4+

The specific versions of software used in our experiments are detailed in the paper. Our build for Piz Daint can be reproduced by running `build_daint.sh` 

Configure and execute the build:
```
buildir=$(mktemp -d ~/tmp_build.XXXX)
pushd $buildir
cmake -DCMAKE_BUILD_TYPE=Release <PATH-TO-SOURCES>
make -j 8
popd
```

The executables will be ready in `$buildir/src/executables`. Of particular interest are the following:

- `square_root` -- our mincut implementation
- `parallel_cc` -- our CC implementation
- `approx_cut` -- our approximate mincut implementation

All of the executables are documented with the input parameters and accept inputs generated by our generators or transformed using the provided utilities.

Finally, execute the code using e.g.
```
mpiexec -n 48 <PATH>/src/executables/square_root 0.95 input.in 0
```
or equivalent for your platform. This will print the value of the resulting cut, timing information, and possibly more fine-grained profiling data.

## Experimental workflows


In our particular setup, we uploaded the sources to the cluster and performed the process described in the previous section using the `build_daint.sh` script.

Then, evaluation inputs were generated. Every input graph is contained in a single file, stored as a list of edges together with associated metadata.

For smaller experiments, this was done manually by invoking the generators, as described in the README. For the bigger experiments, we use scripts located in `input_generators` that often generate the complete set of inputs.

For example, in the *AppMC* weak scaling experiment (Figure 6 in the paper), codenamed AWX, the inputs were first generated by running
```
input_generators/awx_generator.sh
```

which outputs the graphs in the corresponding folder.

In order to execute the experiments, we run the scripts located in `experiment_runners`. Each script describes one self-contained experiment. Following our earlier example, we would run the 

```
experiment_runners/awx.sh
```


script to execute the experiment. This submits a number of jobs corresponding to the different datapoints to the scheduling system.

Every job outputs a comma-separated list of values (CSV) describing properties of execution, similar to the one shown below
```
PAPI,0,39125749,627998425,1184539166,1012658737,35015970,5382439,0.0119047
/scratch/inputs/cc1/ba_1M_16.in,5226,1,1024000,16383744,0.428972,0.011905,cc,1
```

Once all the jobs finish, we filter, merge, and copy relevant data from the cluster to a local computer using 

```
experiment_runners/pull_fresh_data.sh
```


which results in one CSV file per experiment or part of experiment. The output mirrors the input folder structure and is located in `evaluation/data`. For reference, we have included the measurements we used for the figures in this paper. These are located in `evaluation/data`.

The data is then loaded into a suite of R scripts located in `evaluation/R`. The `evaluation/R/common.R` file is perhaps of most interest, as it contains the routines that aggregate the data and verify the variance. These routines are used to build a separate analysis for every experiment. Referring back to our example experiment, the `evaluation/R/awx.R` is the script that was used to produce Figure 6.

In case the statistical significance of results is found to be unsatisfactory during this step (verified by the `verify_ci` routine found in `evaluation/R/common.R`), we repeat the experiment execution and the following steps. One presented datapoint is typically based on 20 to 100 individual measurements.


### Generators and utilities:

Interesting, albeit small graphs can be generated using the tools in `utilities`. The `cut` utility can also independently verify correctness (keep in mind that our algorithm is Monte-Carlo and all randomness is controlled by the seed).

#### Setup

Get Python 3, do `pip3 install networkx`

#### Usage

 - `cut`: Cuts a graphs from stdin
 - `generate`: Generates graphs

##### Examples

Complete graph on 100 vertices with weights uniformly distributed between 1 and 42
```
./utils/generate.py 'complete_graph(N)' 100 --weight 42 --randomize
```

Erdos-Renyi graph on 100 vertices with 0.2 edge creation probability
```
./utils/generate.py 'fast_gnp_random_graph(N, P)' 100 --prob 0.2
```

Watts–Strogatz small-world graph with `k = 6, p = 0.4` (short parameters possible)
```
./utils/generate.py 'connected_watts_strogatz_graph(N, K, P)' 100 -p 0.4 -k 6
```

Help
```
./utils/generate.py -h
```

Checking the cut value
```
./utils/generate.py 'cycle_graph(N)' 100 | ./utils/cut.py
```

Also see [the list of available generators](https://networkx.github.io/documentation/networkx-1.10/reference/generators.html]).

## License

Communication-Efficient Randomized Minimum Cuts    
Copyright (C) 2015, 2016, 2017  Pavel Kalvoda, Lukas Gianinazzi, Alessandro de Palma

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

### 3rd party license

This repository contains third-party dependencies licensed under other
licenses that remain subject to those licenses under their respective
terms and conditions. These dependencies are provided solely for 
research and experimentation purposes. The third-party dependencies are not 
a part of the "Communication-Efficient Randomized Minimum Cuts" software
and thus not subject to the license terms above.

 - [Galois](http://iss.ices.utexas.edu/?p=projects/galois)
 - [ParMETIS](http://glaros.dtc.umn.edu/gkhome/metis/parmetis/overview)
 - [Simto PRNG](http://sitmo.com/)

