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

## Design

* Program level:
  * Op: a operator class, with a name, works like a function.
  * Var: a variable
  * Free Vars: variables that are not the output of any operator.

  Example: concat three strings

  ``` c++
  Op<std::string(std::string,std::string)> concat_op("concat_op");
  VarStr output = concat_op(VarStr("a"), VarStr("b"));
  output = concat_op(output, VarStr("c"));
  output.set_name("output");
  ```

Note: variable name for input and output is significant and wil be kepted in
generated DAG. they act as the interface to the outside world. Intermediate
variables' names are generated automatically.

  In this example, `a`, `b`, `c` are input variables. `output` is the output
  variable.

We generate variable names and unique operator node names automatically, when
translating program to DAG.

* DAG level:
  * Node: a node in the DAG, binds to an operator class
    * node name: unique name in DAG
    * op_class: the class of the operator
    * inputs: the input variables, unique names in DAG
    * outputs: the output variables, unique names in DAG


example DAG generated from previous example:


  ``` plaintext
  var_0 = concat_op_0(a, b)
  var_1 = concat_op_1(var_0, c)
  ```

##  More Examples

loop over a list of `Var<string>` and concat them into a single string

``` c++
Op<Res(Res, std::string)> concat_op("concat_op");
std::list<VarStr> items = {VarStr("a"), VarStr("b"), VarStr("c")};
VarStr output("output");
for (int i = 1; i < items.size(); ++i) {
  output = concat_op(output, items[i]);
}

```

generated DAG:

``` plaintext
var_0 = concat_op(a, b)
output = concat_op(var_0, c)
```


``` c++
Op<CrossFeaturePtr(std::vector<Gid>, BfsServiceParams, CtxInfo, std::vector<ModelConfig>)> cross_feature_op("cross_feature_op");


Op<std::tuple<std::vector<std::string>, std::vector<std::string>>, std::unordered_set<Tag>>(ABParam*, std::vector<std::string>, std::vector<std::string>, LabelMap) TagUsageParser("tag_usage_parser");


Op<std::vector<Tags>(Tagger, std::vector<std::string>, std::vector<Gid>)> TaggerOp("tagger_op");

auto gids = VarVec<Gid>("gids");

auto cross_feature = cross_feature_op(gids, Var<BfsServiceParams>("bfs_service_params"), Var<CtxInfo>("ctx_info"), VarVec<ModelConfig>("model_configs"));

(rough_tags, predict_tags, tags_id) = TagUsageParser(Var<ABParam>("ab_param"), VarVec<std::string>("input_tags"), VarVec<std::string>("output_tags"), Var<LabelMap>("label_map"));

auto tagger = Var<Tagger>("tagger");
auto tags_for_rough = TaggerOp(tagger, rough_tags, gids);
auto tags_for_predict = TaggerOp(tagger, predict_tags, gids);
```

if we declare two var with the same name, will it be a problem?
it is not ok.



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


## Debug

add the following to `.bazelrc` to enable source file map for debugging

```
build:debug --strip=never
build:debug --copt=-g
```
