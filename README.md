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

