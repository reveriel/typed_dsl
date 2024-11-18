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

