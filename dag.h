#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>

// Forward declarations
template <typename T>
class Var;

template <typename... Ts>
class VarTuple;

template <typename T>
class Node;

// Graph representation
class Graph {
  struct NodeInfo {
    std::string op_name;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
  };
  std::vector<NodeInfo> nodes_;

public:
  void add_node(const std::string& op_name,
                const std::vector<std::string>& inputs,
                const std::vector<std::string>& outputs) {
    nodes_.push_back({op_name, inputs, outputs});
  }

  size_t node_count() const {
    return nodes_.size();
  }

  // Predicate for testing: does op_name exist?
  bool has_node(const std::string& op_name) const {
    return std::any_of(nodes_.begin(), nodes_.end(), [&](const NodeInfo& node) {
      return node.op_name == op_name;
    });
  }

  // Predicate for testing: does op_name consume input?
  bool has_edge(const std::string& from, const std::string& to) const {
    for (const auto& node : nodes_) {
      if (node.outputs.size() > 0 && node.outputs[0] == to) {
        return std::find(node.inputs.begin(), node.inputs.end(), from) !=
               node.inputs.end();
      }
    }
    return false;
  }

  // Predicate for testing: does op_name produce output?
  bool produces(const std::string& op_name, const std::string& output) const {
    for (const auto& node : nodes_) {
      if (node.op_name == op_name) {
        return std::find(node.outputs.begin(), node.outputs.end(), output) !=
               node.outputs.end();
      }
    }
    return false;
  }

  // Predicate for testing: does op_name consume input?
  bool consumes(const std::string& op_name, const std::string& input) const {
    for (const auto& node : nodes_) {
      if (node.op_name == op_name) {
        return std::find(node.inputs.begin(), node.inputs.end(), input) !=
               node.inputs.end();
      }
    }
    return false;
  }

  // Get all inputs for a specific operator
  std::vector<std::string> get_inputs(const std::string& op_name) const {
    for (const auto& node : nodes_) {
      if (node.op_name == op_name) {
        return node.inputs;
      }
    }
    return {};
  }

  // Get the output for a specific operator
  std::vector<std::string> get_outputs(const std::string& op_name) const {
    for (const auto& node : nodes_) {
      if (node.op_name == op_name) {
        return node.outputs;
      }
    }
    return {};
  }

  // Method to print the graph structure
  void print() const {
    std::cout << "Graph Structure:" << std::endl;
    for (const auto& node : nodes_) {
      std::cout << "Node: " << node.op_name << std::endl;
      std::cout << "  Inputs: ";
      for (const auto& input : node.inputs) {
        std::cout << input << " ";
      }
      std::cout << std::endl;
      std::cout << "  Outputs: ";
      for (const auto& output : node.outputs) {
        std::cout << output << " ";
      }
      std::cout << std::endl;
    }
  }
};

// Program to build the graph
class Program {
public:
  struct PendingNode {
    std::string op_name;
    std::vector<std::string> inputs;
    std::vector<std::string> output;
  };
  // pending nodes to be added to the graph.
  std::vector<PendingNode> pending_nodes_;

public:
  template <typename T>
  void add(Var<T>& var) {
    // The assignment operator in Var will have stored the node info
    if (var.has_pending_node()) {
      pending_nodes_.push_back(var.take_pending_node());
    }
  }

  template <typename... Ts>
  void add(VarTuple<Ts...>& tuple) {
    if (tuple.has_pending_nodes()) {
      pending_nodes_.push_back(tuple.take_pending_nodes());
    }
  }

  Graph graph() const {
    Graph g;
    for (const auto& node : pending_nodes_) {
      g.add_node(node.op_name, node.inputs, node.output);
    }
    return g;
  }
};


// Variable wrapper class
template <typename T>
class Var {
private:
  std::string name_;
  Program::PendingNode pending_node_;
  bool has_pending_ = false;

public:
  // variable assignment.
  // if a Var has pending node, it can not be assigned again.
  Var& operator=(const Var& other) {
    if (has_pending_) {
      throw std::runtime_error("Var already assigned");
    }
    pending_node_ = std::move(other.pending_node_);
    pending_node_.output = {name_};
    has_pending_ = other.has_pending_;
    return *this;
  }

  Var& operator=(Var&& other) {
    if (has_pending_) {
      throw std::runtime_error("Var already assigned");
    }
    pending_node_ = std::move(other.pending_node_);
    pending_node_.output = {name_};
    has_pending_ = other.has_pending_;
    return *this;
  }

  // copy constructor deleted. because a var is unique in the dag.
  Var(const Var& other) = delete;
  // move constructor ok
  Var(Var&& other) = default;

  explicit Var(const std::string& name) : name_(name) {}

  const std::string& name() const {
    return name_;
  }

  bool has_pending_node() const {
    return has_pending_;
  }

  Program::PendingNode take_pending_node() {
    has_pending_ = false;
    return std::move(pending_node_);
  }

  void set_pending_node(const std::string& op_name,
                        const std::vector<std::string>& inputs) {
    pending_node_ = {op_name, inputs, {}};
    has_pending_ = true;
  }
};

// Helper class for multiple outputs
template <typename... Ts>
class VarTuple {
  std::tuple<Var<Ts>&...> vars_;
  bool has_pending_ = false;
  Program::PendingNode pending_node_;

public:
  explicit VarTuple(Var<Ts>&... vars) : vars_(vars...) {}

  VarTuple<Ts...>& operator=(Var<std::tuple<Ts...>>&& result) {
    std::cout << "VarTuple operator=" << std::endl;

    if (has_pending_) {
      throw std::runtime_error("VarTuple already assigned");
    }

    // Move the pending node from the result
    pending_node_ = result.take_pending_node();

    // Reserve space for outputs
    pending_node_.output.reserve(sizeof...(Ts));

    // Traverse the tuple and get the name of each var
    std::apply(
        [&](auto&... vars) {
          ((pending_node_.output.push_back(vars.name())), ...);
        },
        vars_);

    has_pending_ = true;  // Mark this VarTuple as having a pending node
    return *this;
  }

  // debug string
  std::string to_string() const {
    std::stringstream ss;
    ss << "VarTuple(";
    std::apply([&](auto&... vars) { ((ss << vars.name() << ", "), ...); }, vars_);
    ss << ")";
    return ss.str();
  }

  bool has_pending_nodes() const {
    return has_pending_;
  }

  Program::PendingNode take_pending_nodes() {
    has_pending_ = false;
    return std::move(pending_node_);
  }

};

// Helper function to create VarTuple - binary version
template <typename T1, typename T2>
VarTuple<T1, T2> operator,(Var<T1>& var1, Var<T2>& var2) {
  return VarTuple<T1, T2>(var1, var2);
}

template <typename... Ts, typename T>
VarTuple<Ts..., T> operator,(VarTuple<Ts...> tuple, Var<T>& var) {
  return VarTuple<Ts..., T>(tuple, var);
}

// Operator wrapper class
template <typename Signature>
class Op;

template <typename R, typename... Args>
class Op<R(Args...)> {
  std::string op_name_;

public:
  // Constructor accepting a string for the operation name
  Op(const std::string& name) : op_name_(name) {}

  const std::string& name() const {
    return op_name_;
  }

  // For single output
  Var<R> operator()(const Var<Args>&... inputs) const {
    std::vector<std::string> input_names{inputs.name()...};
    Var<R> result("_result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

// Wrapper types for different argument patterns
template <typename T>
struct Variadic {
  using type = T;
};

// Add specialization for variadic inputs
template <typename R, typename ArgT>
class Op<R(Variadic<ArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}

  const std::string& name() const {
    return op_name_;
  }

  template <typename... Args>
  Var<R> operator()(const Args&... args) const {
    static_assert((std::is_same_v<Args, Var<ArgT>> && ...),
                  "All arguments must be of the same type Var<ArgT>");

    std::vector<std::string> input_names;
    input_names.reserve(sizeof...(args));
    (input_names.push_back(args.name()), ...);

    Var<R> result("result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

// Specialization for one fixed argument followed by variadic arguments
// We can omit the Fixed wrapper if there is only one fixed argument
template <typename R, typename FixedArgT, typename VarArgT>
class Op<R(FixedArgT, Variadic<VarArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}
  const std::string& name() const {
    return op_name_;
  }

  template <typename... Args>
  Var<R> operator()(const Var<FixedArgT>& fixed_arg,
                    const Args&... args) const {
    static_assert((std::is_same_v<Args, Var<VarArgT>> && ...),
                  "All variadic arguments must be of type Var<VarArgT>");

    std::vector<std::string> input_names;
    input_names.reserve(1 + sizeof...(args));
    input_names.push_back(fixed_arg.name());
    (input_names.push_back(args.name()), ...);

    Var<R> result("result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

// Specialization for 2 fixed arguments followed by variadic arguments
template <typename R, typename FixedArg1T, typename FixedArg2T,
          typename VarArgT>
class Op<R(FixedArg1T, FixedArg2T, Variadic<VarArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}
  const std::string& name() const {
    return op_name_;
  }

  template <typename... Args>
  Var<R> operator()(const Var<FixedArg1T>& fixed_arg1,
                    const Var<FixedArg2T>& fixed_arg2,
                    const Args&... args) const {
    static_assert((std::is_same_v<Args, Var<VarArgT>> && ...),
                  "All variadic arguments must be of type Var<VarArgT>");

    std::vector<std::string> input_names;
    input_names.reserve(2 + sizeof...(args));
    input_names.push_back(fixed_arg1.name());
    input_names.push_back(fixed_arg2.name());
    (input_names.push_back(args.name()), ...);

    Var<R> result("result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};
