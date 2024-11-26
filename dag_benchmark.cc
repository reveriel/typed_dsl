#include "dag.h"
#include <benchmark/benchmark.h>
#include <string>

// Benchmark creating a simple linear DAG
static void BM_LinearDAG(benchmark::State& state) {
  for (auto _ : state) {
    Op<int32_t(int32_t)> op1("op1"), op2("op2"), op3("op3");
    Var<int32_t> input("input");
    Var<int32_t> v1("v1"), v2("v2"), output("output");

    Program p;
    p.add(v1 = op1(input));
    p.add(v2 = op2(v1));
    p.add(output = op3(v2));

    Graph g = p.graph();
  }
}
BENCHMARK(BM_LinearDAG);

// Benchmark creating a DAG with multiple outputs
static void BM_MultipleOutputsDAG(benchmark::State& state) {
  for (auto _ : state) {
    Op<std::tuple<int32_t, int32_t>(int32_t)> split_op("split_op");
    Op<int32_t(int32_t)> process_op("process_op");

    Var<int32_t> input("input");
    Var<int32_t> out1("out1"), out2("out2");
    Var<int32_t> final_out("final_out");

    Program p;
    p.add((out1, out2) = split_op(input));
    p.add(final_out = process_op(out1));

    Graph g = p.graph();
  }
}
BENCHMARK(BM_MultipleOutputsDAG);

// Benchmark creating a wide DAG (many parallel operations)
static void BM_WideDAG(benchmark::State& state) {
  const int width = state.range(0);

  for (auto _ : state) {
    Op<int32_t(int32_t)> op("op");
    Var<int32_t> input("input");
    Program p;

    std::vector<Var<int32_t>> outputs;
    for (int i = 0; i < width; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
      p.add(outputs.back() = op(input));
    }

    Graph g = p.graph();
  }
}
BENCHMARK(BM_WideDAG)->Range(8, 1024);

// Benchmark creating a deep DAG (long chain of operations)
static void BM_DeepDAG(benchmark::State& state) {
  const int depth = state.range(0);

  for (auto _ : state) {
    Op<int32_t(int32_t)> op("op");
    Program p;

    Var<int32_t> input("input");
    Var<int32_t> current = input;

    for (int i = 0; i < depth; ++i) {
      Var<int32_t> next("v_" + std::to_string(i));
      p.add(next = op(current));
      current = next;
    }

    Graph g = p.graph();
  }
}
BENCHMARK(BM_DeepDAG)->Range(8, 1024);

BENCHMARK_MAIN();