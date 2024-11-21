#include "dag.h"
#include <gtest/gtest.h>
#include <string>

TEST(DagTest, BasicStringConcate) {
  // Create operators with readable names
  Op<std::string(std::string, std::string)> concat_op("concat_op");
  Op<int32_t(std::string)> parse_int_op("parse_int_op");

  // Create variables
  Var<std::string> output("output");
  Var<std::string> input_a("input_a");
  Var<std::string> input_b("input_b");
  Var<int32_t> int_val("int_val");

  Program p;

  p.add(output = concat_op(input_a, input_b));
  p.add(int_val = parse_int_op(output));

  Graph g = p.graph();
  g.print();

  // Print the graph structure
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 2);

  // Verify operators exist
  EXPECT_TRUE(g.has_node(concat_op.name()));
  EXPECT_TRUE(g.has_node(parse_int_op.name()));

  // Verify connections
  EXPECT_TRUE(g.consumes(concat_op.name(), "input_a"));
  EXPECT_TRUE(g.consumes(concat_op.name(), "input_b"));
  EXPECT_TRUE(g.produces(concat_op.name(), "output"));

  EXPECT_TRUE(g.consumes(parse_int_op.name(), "output"));
  EXPECT_TRUE(g.produces(parse_int_op.name(), "int_val"));

  // Verify data flow
  EXPECT_TRUE(g.has_edge("output", "int_val"));
}

TEST(DagTest, MultipleOutputs) {
  // Create operator that returns two values (string, int) with a readable name
  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");

  // Create variables
  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  Program p;

  p.add((str_output, int_output) = split_op(input));

  Graph g = p.graph();
  g.print();

  // Verify node count
  EXPECT_EQ(g.node_count(), 1);

  // Verify operator exists
  EXPECT_TRUE(g.has_node(split_op.name()));

  // Verify connections
  EXPECT_TRUE(g.consumes(split_op.name(), "input"));
  EXPECT_TRUE(g.produces(split_op.name(), "str_output"));
  EXPECT_TRUE(g.produces(split_op.name(), "int_output"));

  // Verify inputs and outputs
  auto inputs = g.get_inputs(split_op.name());
  EXPECT_EQ(inputs.size(), 1);
  EXPECT_EQ(inputs[0], "input");
}

TEST(DagTest, MultipleOutputsWithTuple) {
  Op<std::tuple<std::string, int32_t>(std::string)> split_op("split_op");
  Op<std::string(int32_t)> int_to_str_op("int_to_str_op");

  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  Program p;
  p.add(str_output = int_to_str_op(int_output));
  p.add((str_output, int_output) = split_op(input));

  Graph g = p.graph();
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
  p.add(result1 = vector_op(vec));  // Takes vector as single argument

  Var<std::string> result2("result2");
  p.add(result2 = variadic_op(Var<std::string>("a"), Var<std::string>("b"),
                              Var<std::string>("c")));

  Var<int32_t> int1("int1");
  Var<int32_t> int2("int2");
  Var<int32_t> int3("int3");
  Var<std::string> result3("result3");
  p.add(result3 = mixed_op(Var<std::string>("hello"), int1, int2, int3));

  Var<std::string> result4("result4");
  p.add(result4 = mixed_op2(Var<std::string>("hello"), Var<bool>("bool"), int1,
                            int2, int3));

  Graph g = p.graph();
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
    p.add(output = double_op(input));
  } else {
    p.add(output = input);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
