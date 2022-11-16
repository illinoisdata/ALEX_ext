# Benchmark Instruction

This is an instruction to benchmark ALEX for experiments in AirIndex: Versatile Index Tuning Through Data and Storage.

Please follow [dataset](https://github.com/illinoisdata/airindex-public/blob/main/dataset_setup.md) and [query key set](https://github.com/illinoisdata/airindex-public/blob/main/keyset_setup.md) instructions to setup the benchmarking environment. These are examples of environment [reset scripts](https://github.com/illinoisdata/airindex-public/blob/main/reload_examples.md). The following assumes that the datasets are under `/path/to/data/`, key sets are under `/path/to/keyset/` , indexes output are under `/path/to/build_output` and benchmark measurements output are under `/path/to/benchmark_output`

## Build

To build the binaries

```
git submodule init
git submodule update --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make kv_build kv_benchmark
cd ..
```

To build indexes for all datasets

```
bash alex-build.sh /path/to/data /path/to/build_output 
```

Built indexes will be stored in `/path/to/build_output` folder

## Benchmark (6.2 & 6.4)

Benchmark over 40 key set of 1M keys

```
bash alex-benchmark.sh /path/to/build_output /path/to/keyset /path/to/benchmark_output ~/reload_local.sh
```

The measurements will be recorded in `/path/to/output` folder.
