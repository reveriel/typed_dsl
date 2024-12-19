#include "dag.h"
#include <benchmark/benchmark.h>
#include <string>

// Benchmark creating a simple linear DAG
static void BM_LinearDAG(benchmark::State& state) {
  for (auto _ : state) {
    Op<int32_t(int32_t)> op1("op1"), op2("op2"), op3("op3");
    Program p;
    Context::Scope scope(&p);

    Var<int32_t> input("input");
    Var<int32_t> v1("v1"), v2("v2"), output("output");

    v1 = op1(input);
    v2 = op2(v1);
    output = op3(v2);

    Graph g = p.graph();
  }
}
BENCHMARK(BM_LinearDAG);

// Benchmark creating a DAG with multiple outputs
static void BM_MultipleOutputsDAG(benchmark::State& state) {
  for (auto _ : state) {
    Op<std::tuple<int32_t, int32_t>(int32_t)> split_op("split_op");
    Op<int32_t(int32_t)> process_op("process_op");

    Program p;
    Context::Scope scope(&p);

    Var<int32_t> input("input");
    Var<int32_t> out1("out1"), out2("out2");
    Var<int32_t> final_out("final_out");

    (out1, out2) = split_op(input);
    final_out = process_op(out1);

    Graph g = p.graph();
  }
}
BENCHMARK(BM_MultipleOutputsDAG);

// Benchmark creating a wide DAG (many parallel operations)
static void BM_WideDAG(benchmark::State& state) {
  const int width = state.range(0);

  for (auto _ : state) {
    Op<int32_t(int32_t)> op("op");
    Program p;
    Context::Scope scope(&p);

    Var<int32_t> input("input");
    std::vector<Var<int32_t>> outputs;
    outputs.reserve(width);

    for (int i = 0; i < width; ++i) {
      outputs.emplace_back("output_" + std::to_string(i));
      outputs.back() = op(input);
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
    Context::Scope scope(&p);

    Var<int32_t> input("input");
    Var<int32_t> current("current");
    current = input;  // Use operator= instead of copy constructor

    for (int i = 0; i < depth; ++i) {
      Var<int32_t> next("node_" + std::to_string(i));
      next = op(current);
      current = next;  // This is fine because we're using operator=
    }

    Graph g = p.graph();
  }
}
BENCHMARK(BM_DeepDAG)->Range(8, 1024);

BENCHMARK_MAIN();