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

TEST(DagTest, VaridicOp) {
  // merge n inputs into one output

  // Takes a vector as a single argument
  Op<std::string(std::vector<std::string>)> vector_op;

  // Takes variadic arguments
  Op<std::string(Variadic<std::string>)> variadic_op;

  Op<std::string(std::string, Variadic<int32_t>)> mixed_op;

  Op<std::string(std::string, bool, Variadic<int32_t>)> mixed_op2;

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
  EXPECT_EQ(g.node_count(), 4);
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
