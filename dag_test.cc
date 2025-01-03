#include "dag.h"
#include <gtest/gtest.h>
#include <string>

TEST(DagTest, AddOne1) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> output("output");
  output = add_one(input);
  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  EXPECT_TRUE(g.is_placeholder("input"));
  EXPECT_TRUE(g.consumes("add_one_0", "input"));
  EXPECT_TRUE(g.produces("add_one_0", "output"));
}

TEST(DagTest, Copy) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> output("output");
  output = input;
  output = add_one(output);
  Graph g = prog.graph();
  g.print();
  // expect
  // input -> [add_one_0] -> output
  EXPECT_EQ(g.node_count(), 1);
  EXPECT_TRUE(g.is_placeholder("input"));
  EXPECT_TRUE(g.consumes("add_one_0", "input"));
  EXPECT_TRUE(g.produces("add_one_0", "output"));
}

TEST(DagTest, AddOne) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> output("output");
  output = input;
  for (int i = 0; i < 3; i++) {
    output = add_one(output);
  }
  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 3);
  // expect
  // intermediate vars are automatically generated
  // input -> [add_one_0] -> add_one_0:0 -> [add_one_1] -> add_one_1:0 ->
  // [add_one_2] -> output
  EXPECT_TRUE(g.consumes("add_one_0", "input"));
  EXPECT_TRUE(g.produces("add_one_0", "add_one_0:0"));
  EXPECT_TRUE(g.consumes("add_one_1", "add_one_0:0"));
  EXPECT_TRUE(g.produces("add_one_1", "add_one_1:0"));
  EXPECT_TRUE(g.consumes("add_one_2", "add_one_1:0"));
  EXPECT_TRUE(g.produces("add_one_2", "output"));
}

TEST(DagTest, Overwrite) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> input2(placeholder, "input2");
  Var<int32_t> output("output");
  output = input;
  output = input2;
  output = add_one(output);
  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  // expect
  // input2 -> [add_one] -> output
  EXPECT_TRUE(g.consumes("add_one:0", "input2"));
  EXPECT_TRUE(g.produces("add_one:0", "output"));
}

TEST(DagTest, TwoInput) {
  Program prog;
  Context::Scope scope(&prog);

  Op<std::string(std::string, std::string)> concat_op("concat_op");
  Op<int32_t(std::string)> parse_int_op("parse_int_op");

  Var<std::string> output("output");
  Var<std::string> input_a(placeholder, "input_a");
  Var<std::string> input_b(placeholder, "input_b");
  Var<int32_t> int_val("int_val");

  output = concat_op(input_a, input_b);
  int_val = parse_int_op(output);

  Graph g = prog.graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 2);
  // expect
  // input_a, input_b -> [concat_op] -> output
  // output -> [parse_int_op] -> int_val
  EXPECT_TRUE(g.consumes("concat_op:0",
                         std::vector<std::string>{"input_a", "input_b"}));
  EXPECT_TRUE(g.produces("concat_op:0", "concat_op_0/output"));
  EXPECT_TRUE(g.consumes("parse_int_op:0", "concat_op_0/output"));
  EXPECT_TRUE(g.produces("parse_int_op:0", "int_val"));
}

TEST(DagTest, VarNameConflict) {
  Program prog;
  Context::Scope scope(&prog);

  Var<std::string> a("a");
  Var<std::string> b("b");
  Op<std::string(std::string)> upper_op("upper_op");
  a = upper_op(a);
  b = a;

  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  // expect
  // a -> [upper_op] -> b
  EXPECT_TRUE(g.has_node("upper_op:0"));
  EXPECT_TRUE(g.consumes("upper_op:0", "a"));
  EXPECT_TRUE(g.produces("upper_op:0", "b"));
}

TEST(DagTest, LoopParallel) {
  Program prog;
  Context::Scope scope(&prog);

  struct PredictResult {};
  struct ModelConfig {};
  Op<PredictResult(ModelConfig)> predict_op("predict_op");

  std::vector<Var<ModelConfig>> model_configs;
  model_configs.emplace_back(placeholder, "model_configs_0");
  model_configs.emplace_back(placeholder, "model_configs_1");
  model_configs.emplace_back(placeholder, "model_configs_2");

  std::vector<Var<PredictResult>> predict_results;
  predict_results.emplace_back(placeholder, "predict_results_0");
  predict_results.emplace_back(placeholder, "predict_results_1");
  predict_results.emplace_back(placeholder, "predict_results_2");

  for (size_t i = 0; i < model_configs.size(); i++) {
    predict_results[i] = predict_op(model_configs[i]);
  }

  Graph g = prog.graph();
  g.print();
  // expect
  // model_configs_0 -> [predict_op] -> predict_results_0
  // model_configs_1 -> [predict_op] -> predict_results_1
  // model_configs_2 -> [predict_op] -> predict_results_2
  EXPECT_EQ(g.node_count(), 3);

  EXPECT_TRUE(g.has_node("predict_op:0"));
  EXPECT_TRUE(g.consumes("predict_op:0", "model_configs_0"));
  EXPECT_TRUE(g.produces("predict_op:0", "predict_results_0"));
  EXPECT_TRUE(g.consumes("predict_op:1", "model_configs_1"));
  EXPECT_TRUE(g.produces("predict_op:1", "predict_results_1"));
  EXPECT_TRUE(g.consumes("predict_op:2", "model_configs_2"));
  EXPECT_TRUE(g.produces("predict_op:2", "predict_results_2"));
}

TEST(DagTest, propagate) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> output("output");
  Var<int32_t> a, b, c;
  a = input;
  b = a;
  c = b;
  output = add_one(c);

  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  // expect
  // input -> [add_one] -> output
  EXPECT_TRUE(g.consumes("add_one:0", "input"));
  EXPECT_TRUE(g.produces("add_one:0", "output"));
}

TEST(DagTest, DeadCodeElimination) {
  Program prog;
  Context::Scope scope(&prog);

  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input(placeholder, "input");
  Var<int32_t> output("output");
  Var<int32_t> a, b, c;
  // we cannot get unnamed var's result. thus these are dead code.
  c = input;
  c = add_one(c);
  c = add_one(c);
  b = c;
  output = add_one(input);
  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  // expect only one node
  // input -> [add_one] -> output
  EXPECT_TRUE(g.consumes("add_one:0", "input"));
  EXPECT_TRUE(g.produces("add_one:0", "output"));
}

TEST(DagTest, MultipleOutputs) {
  Program prog;
  Context::Scope scope(&prog);

  // Create operator that returns two values (string, int) with a readable name
  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");

  // Create variables
  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  (str_output, int_output) = split_op(input);

  Graph g = prog.graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 1);
  // expect
  // input -> [split_op] -> str_output, int_output
  EXPECT_TRUE(g.consumes("split_op:0", "input"));
  EXPECT_TRUE(g.produces("split_op:0", "str_output"));
  EXPECT_TRUE(g.produces("split_op:0", "int_output"));
}

TEST(DagTest, MultipleOutputsWithTuple) {
  Program prog;
  Context::Scope scope(&prog);

  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");
  Op<std::string(int32_t)> int_to_str_op("int_to_str_op");

  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  str_output = int_to_str_op(int_output);
  (str_output, int_output) = split_op(input);

  Graph g = prog.graph();
  g.print();
  // Verify node count
  EXPECT_EQ(g.node_count(), 2);
  // expect
  // input -> [split_op] -> str_output, int_output
  // int_output -> [int_to_str_op] -> str_output
  EXPECT_TRUE(g.consumes("split_op:0", "input"));
  EXPECT_TRUE(g.produces("split_op:0", "split_op_0/output"));
  EXPECT_TRUE(g.produces("split_op:0", "split_op_0/output"));
  EXPECT_TRUE(g.consumes("int_to_str_op:0", "split_op_0/output"));
  EXPECT_TRUE(g.produces("int_to_str_op:0", "str_output"));
}

TEST(DagTest, VaridicOp) {
  Program prog;
  Context::Scope scope(&prog);

  // merge n inputs into one output

  // Takes a vector as a single argument
  Op<std::string(std::vector<std::string>)> vector_op("vector_op");

  // Takes variadic arguments
  Op<std::string(Variadic<std::string>)> variadic_op("variadic_op");

  Op<std::string(std::string, Variadic<int32_t>)> mixed_op("mixed_op");

  Op<std::string(std::string, bool, Variadic<int32_t>)> mixed_op2("mixed_op2");

  // Usage:
  Var<std::vector<std::string>> vec("vec");
  Var<std::string> result1("result1");
  result1 = vector_op(vec);  // Takes vector as single argument

  Var<std::string> result2("result2");
  result2 = variadic_op(Var<std::string>("a"), Var<std::string>("b"),
                        Var<std::string>("c"));

  Var<int32_t> int1("int1");
  Var<int32_t> int2("int2");
  Var<int32_t> int3("int3");
  Var<std::string> result3("result3");
  result3 = mixed_op(Var<std::string>("hello"), int1, int2, int3);

  Var<std::string> result4("result4");
  result4 =
      mixed_op2(Var<std::string>("hello"), Var<bool>("bool"), int1, int2, int3);

  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 4);
  // expect
  // vec -> [vector_op] -> result1
  // a, b, c -> [variadic_op] -> result2
  // hello, int1, int2, int3 -> [mixed_op] -> result3
  // hello, bool, int1, int2, int3 -> [mixed_op2] -> result4
  EXPECT_TRUE(g.consumes("vector_op:0", "vec"));
  EXPECT_TRUE(g.produces("vector_op:0", "result1"));
  EXPECT_TRUE(g.consumes("variadic_op:0", {"a", "b", "c"}));
  EXPECT_TRUE(g.produces("variadic_op:0", "result2"));
  EXPECT_TRUE(g.consumes("mixed_op:0", {"hello", "int1", "int2", "int3"}));
  EXPECT_TRUE(g.produces("mixed_op:0", "result3"));
  EXPECT_TRUE(
      g.consumes("mixed_op2:0", {"hello", "bool", "int1", "int2", "int3"}));
  EXPECT_TRUE(g.produces("mixed_op2:0", "result4"));
}

// what if type is wrong?
TEST(DagTest, TypeMismatch) {
  Program prog;
  Context::Scope scope(&prog);

  Op<std::string(std::string)> concat_op("concat_op");
  Var<int32_t> int_val("int_val");
  // this line should fail at compile time with a message like:
  // Candidate function not viable: no known conversion from 'Var<int32_t>' to
  // 'const Var<std::string>' for 1st argument
  //
  // prog.add(int_val = concat_op(int_val));
  //
}

// we can use c++ native if else
TEST(DagTest, IfElse) {
  Program prog;
  Context::Scope scope(&prog);

  Op<bool(int32_t)> is_even_op("is_even_op");
  Op<int32_t(int32_t)> double_op("double_op");
  Var<int32_t> input("input");
  Var<int32_t> output("output");
  bool true_branch = true;
  if (true_branch) {
    output = double_op(input);
  } else {
    output = input;
  }

  Graph g = prog.graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  // expect
  // input -> [double_op] -> output
  EXPECT_TRUE(g.consumes("double_op:0", "input"));
  EXPECT_TRUE(g.produces("double_op:0", "output"));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
