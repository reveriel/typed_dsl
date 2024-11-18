#include "dag.h"
#include <gtest/gtest.h>
#include <string>

TEST(DagTest, BasicStringConcate) {
  // Create operators
  Op<std::string(std::string, std::string)> concat_op;
  Op<int32_t(std::string)> parse_int_op;

  // Create variables
  Var<std::string> output("output");
  Var<std::string> input_a("input_a");
  Var<std::string> input_b("input_b");
  Var<int32_t> int_val("int_val");

  Program p;

  p.add(output = concat_op(input_a, input_b));
  p.add(int_val = parse_int_op(output));

  Graph g = p.graph();
  EXPECT_EQ(g.node_count(), 2);  // Expect 2 operator nodes
}

TEST(DagTest, MultipleOutputs) {
  // Create operator that returns two values (string, int)
  Op<std::tuple<std::string, int32_t>(std::string)> split_op;

  // Create variables
  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  Program p;

  p.add((str_output, int_output) = split_op(input));

  Graph g = p.graph();
  EXPECT_EQ(g.node_count(),
            1);  // expect one node for the split_op
}

TEST(DagTest, MultipleOutputsWithTuple) {
  // Create operator that returns two values (string, int)
  Op<std::tuple<std::string, int32_t>(std::string)> split_op;
  Op<std::string(int32_t)> int_to_str_op;

  // Create variables
  Var<std::string> input("input");
  Var<std::string> str_output("str_output");
  Var<int32_t> int_output("int_output");

  Program p;
  p.add(str_output = int_to_str_op(int_output));
  p.add((str_output, int_output) = split_op(input));

  Graph g = p.graph();
  EXPECT_EQ(g.node_count(), 2);
}

// what if type is wrong?
TEST(DagTest, TypeMismatch) {
  Op<std::string(std::string)> concat_op;
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
  Op<bool(int32_t)> is_even_op;
  Op<int32_t(int32_t)> double_op;
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
