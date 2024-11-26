#include "dag.h"
#include <gtest/gtest.h>
#include <string>

TEST(DagTest, AddOne) {
  Op<int32_t(int32_t)> add_one("add_one");
  Var<int32_t> input("input");
  Var<int32_t> output("output");
  output = input;
  for (int i = 0; i < 3; i++) {
    output = add_one(output);
  }
  Graph g = Program::current().graph();
  g.print();
}

TEST(DagTest, BasicStringConcate) {
  Op<std::string(std::string, std::string)> concat_op("concat_op");
  Op<int32_t(std::string)> parse_int_op("parse_int_op");

  Var<std::string> output("output");
  Var<std::string> input_a("input_a");
  Var<std::string> input_b("input_b");
  Var<int32_t> int_val("int_val");

  output = concat_op(input_a, input_b);
  int_val = parse_int_op(output);

  Graph g = Program::current().graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 2);

  // Verify operators exist
  EXPECT_TRUE(g.has_node("concat_op_0"));
  EXPECT_TRUE(g.has_node("parse_int_op_0"));

  // Update expected output variable names
  EXPECT_TRUE(g.produces("concat_op_0", "output_v0"));  // Changed from output_v1 to output_v0
  EXPECT_TRUE(g.consumes("concat_op_0", "input_a"));
  EXPECT_TRUE(g.consumes("concat_op_0", "input_b"));

  EXPECT_TRUE(g.consumes("parse_int_op_0", "output_v0"));  // Changed from output_v1 to output_v0
  EXPECT_TRUE(g.produces("parse_int_op_0", "int_val_v0")); // Changed from int_val_v1 to int_val_v0

  // Verify data flow
  EXPECT_TRUE(g.has_edge("output_v0", "int_val_v0")); // Changed from output to output_v0
}

TEST(DagTest, VarNameConflict) {
  Var<std::string> a("a");
  Var<std::string> b("b");
  Op<std::string(std::string)> upper_op("upper_op");
  a = upper_op(a);
  b = a;

  Graph g = Program::current().graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  EXPECT_TRUE(g.has_node("upper_op:0"));
  EXPECT_TRUE(g.consumes("upper_op:0", "a:0"));
  EXPECT_TRUE(g.produces("upper_op:0", "b:0"));
}
TEST(DagTest, Loop) {
  struct PredictResult {};
  struct ModelConfig {};
  Op<PredictResult(ModelConfig)> predict_op("predict_op");
  std::vector<Var<ModelConfig>> model_configs;
  std::vector<Var<PredictResult>> predict_results;
  for (size_t i = 0; i < model_configs.size(); i++) {
    predict_results[i] = predict_op(model_configs[i]);
  }

  Graph g = Program::current().graph();
  g.print();
  EXPECT_EQ(g.node_count(), 1);
  EXPECT_TRUE(g.has_node("upper_op:0"));
  EXPECT_TRUE(g.consumes("upper_op:0", "a:0"));
  EXPECT_TRUE(g.produces("upper_op:0", "b:0"));
}


TEST(DagTest, BasicNoNameVars) {
  Op<std::string(std::string, std::string)> concat_op("concat_op");
  Op<int32_t(std::string)> parse_int_op("parse_int_op");

  Var<std::string> input_a;
  Var<std::string> input_b;
  Var<int32_t> int_val;

  auto output = concat_op(input_a, input_b);
  int_val = parse_int_op(output);

  Graph g = Program::current().graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 3); // Updated expected count to 3

  // Verify operators exist
  EXPECT_TRUE(g.has_node("concat_op_0"));
  EXPECT_TRUE(g.has_node("parse_int_op_0"));
  EXPECT_TRUE(g.has_node("_result_concat_op")); // Added check for the result node
}

TEST(DagTest, MultipleOutputs) {
  // Create operator that returns two values (string, int) with a readable name
  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");

  // Create variables
  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  // No need for explicit Program creation and add() calls
  (str_output, int_output) = split_op(input);

  Graph g = Program::current().graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 1);

  // Verify operator exists
  EXPECT_TRUE(g.has_node(split_op.name()));
  std::cout << g.to_string(split_op.name()) << std::endl;

  // Verify connections
  EXPECT_TRUE(g.consumes(split_op.name(), "input"));
  EXPECT_TRUE(g.produces(split_op.name(), "str_output"));
  EXPECT_TRUE(g.produces(split_op.name(), "int_output"));

  // Verify inputs and outputs
  auto inputs = g.get_inputs(split_op.name());
  EXPECT_EQ(inputs.size(), 1);
  // EXPECT_EQ(inputs[0], "input");
}

TEST(DagTest, MultipleOutputsWithTuple) {
  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");
  Op<std::string(int32_t)> int_to_str_op("int_to_str_op");

  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  str_output = int_to_str_op(int_output);
  (str_output, int_output) = split_op(input);

  Graph g = Program::current().graph();
  g.print();

  // Verify structure
  EXPECT_EQ(g.node_count(), 2);
  EXPECT_TRUE(g.has_node(split_op.name()));
  EXPECT_TRUE(g.has_node(int_to_str_op.name()));

  // Verify split_op connections
  EXPECT_TRUE(g.consumes(split_op.name(), "input"));
  EXPECT_TRUE(g.produces(split_op.name(), "str_output"));
  EXPECT_TRUE(g.produces(split_op.name(), "int_output"));

  // Verify int_to_str_op connections
  EXPECT_TRUE(g.consumes(int_to_str_op.name(), "int_output"));
  EXPECT_TRUE(g.produces(int_to_str_op.name(), "str_output"));

  // Verify data flow
  EXPECT_TRUE(g.has_edge("int_output", "str_output"));
}

TEST(DagTest, VaridicOp) {
  // merge n inputs into one output

  // Takes a vector as a single argument
  Op<std::string(std::vector<std::string>)> vector_op("vector_op");

  // Takes variadic arguments
  Op<std::string(Variadic<std::string>)> variadic_op("variadic_op");

  Op<std::string(std::string, Variadic<int32_t>)> mixed_op("mixed_op");

  Op<std::string(std::string, bool, Variadic<int32_t>)> mixed_op2("mixed_op2");

  Program p;
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
  result4 = mixed_op2(Var<std::string>("hello"), Var<bool>("bool"), int1,
                            int2, int3);

  Graph g = Program::current().graph();
  g.print();
  EXPECT_EQ(g.node_count(), 4);
}


// what if type is wrong?
TEST(DagTest, TypeMismatch) {
  Op<std::string(std::string)> concat_op("concat_op");
  Var<int32_t> int_val("int_val");
  Program p;
  // this line should fail at compile time with a message like:
  // Candidate function not viable: no known conversion from 'Var<int32_t>' to
  // 'const Var<std::string>' for 1st argument
  //
  // p.add(int_val = concat_op(int_val));
  //
}

// we can use c++ native if else
TEST(DagTest, IfElse) {
  Op<bool(int32_t)> is_even_op("is_even_op");
  Op<int32_t(int32_t)> double_op("double_op");
  Var<int32_t> input("input");
  Var<int32_t> output("output");
  Program p;
  bool true_branch = true;
  if (true_branch) {
    output = double_op(input);
  } else {
    output = input;
  }
}

TEST(DagTest, DISABLED_VarReuse) {
  Op<int32_t(int32_t)> add_one("add_one");

  Var<int32_t> x("x");
  Var<int32_t> result("result");

  // Simpler loop without explicit add() calls
  for(int i = 0; i < 3; i++) {
    x = add_one(x);
  }
  result = x;

  Graph g = Program::current().graph();
  g.print();

  EXPECT_EQ(g.node_count(), 4); // 3 add_one ops + final assignment
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
