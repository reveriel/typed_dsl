# DAG Project, a statically type checked DSL embedded in c++ for building DAGs/dataflow graphs

[![Tests](https://github.com/reveriel/typed_dsl/actions/workflows/tests.yml/badge.svg)](https://github.com/reveriel/typed_dsl/actions/workflows/tests.yml)

a DSL embedded in c++ that is statically type checked.
the DSL is used to express a computational DAG (or a data flow graph).
The actual data should not be stored in the DAG.

Should I distinguish the type of the variable from the type of operators ?

1. style 1, DAG style

``` c++
Op<std::string(std::string,std::string)> concat_op;

Var<std::string>("output") = concat_op(Var<std::string>("input_a"), Var<std::string>("input_b"));
```

1. style 2, functional style

``` c++
T<std::string(std::string,std::string)> op;

T<std::string>("out") = op(T<std::string>("a"), T<std::string>("b"))
```

Which one is better?

The first one is more clear. When reading the code, it is immediately clear
which elements are variables and which are operators. It has a direct
correspondence to the actual DAG.

It is tempting to use the second one, which is more concise, and resembles the
functional programming style. (we may have currying and uncurrying in the
future). It add a layer of indirection.

We can implement both at the same time. provide two levels of API.

## Usage

run test with sanitizers:

``` bash
bazel run :dag_test --config=asan
```

generate `compile_commands.json`

``` bash
bazel run :refresh_compile_commands
```

Format code

``` bash
clang-format -i *.{h,cc}
```

## Benchmarking

To run the benchmarks:

```bash
bazel run -c opt :dag_benchmark
```

The benchmarks measure the performance of:

- Linear DAG creation (simple chain of operations)
- Multiple outputs DAG creation
- Wide DAG creation (many parallel operations)
- Deep DAG creation (long chain of operations)

Results on my Macbook Pro M3 Pro

``` plaintext
Run on (12 X 2400 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x12)
Load Average: 8.52, 5.40, 5.12
----------------------------------------------------------------
Benchmark                      Time             CPU   Iterations
----------------------------------------------------------------
BM_LinearDAG                1619 ns         1610 ns       435754
BM_MultipleOutputsDAG       1939 ns         1904 ns       363283
BM_WideDAG/8                4146 ns         4146 ns       169924
BM_WideDAG/64              27027 ns        27027 ns        25506
BM_WideDAG/512            197070 ns       196336 ns         3233
BM_WideDAG/1024           400324 ns       400320 ns         1817
BM_DeepDAG/8                4069 ns         3947 ns       177042
BM_DeepDAG/64              25438 ns        25438 ns        27878
BM_DeepDAG/512            204398 ns       200028 ns         3599
BM_DeepDAG/1024           387667 ns       387449 ns         1729
```
