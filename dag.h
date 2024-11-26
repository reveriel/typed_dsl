#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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
    std::string name;
    std::string op_class;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
  };
  std::vector<NodeInfo> nodes_;

public:
  void add_node(const std::string& op_class,
                const std::vector<std::string>& inputs,
                const std::vector<std::string>& outputs) {
    // generate a unique name for the node based on op_class
    std::string op_name = op_class + "_" + std::to_string(nodes_.size());
    nodes_.push_back({op_name, op_class, inputs, outputs});
  }

  size_t node_count() const {
    return nodes_.size();
  }

  // debug string for a node,  {inputs} -> {node_name} -> {outputs}
  std::string to_string(const std::string& node_name) const;

  // Predicate for testing: does op_name exist?
  bool has_node(const std::string& node_name) const {
    return std::any_of(nodes_.begin(), nodes_.end(), [&](const NodeInfo& node) {
      return node.name == node_name;
    });
  }

  // Predicate for testing: does node_name consume input?
  bool has_edge(const std::string& from, const std::string& to) const {
    for (const auto& node : nodes_) {
      if (node.outputs.size() > 0 && node.outputs[0] == to) {
        return std::find(node.inputs.begin(), node.inputs.end(), from) !=
               node.inputs.end();
      }
    }
    return false;
  }

  // Predicate for testing: does node_name produce output?
  bool produces(const std::string& node_name, const std::string& output) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return std::find(node.outputs.begin(), node.outputs.end(), output) !=
               node.outputs.end();
      }
    }
    return false;
  }

  // Predicate for testing: does node_name consume input?
  bool consumes(const std::string& node_name, const std::string& input) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return std::find(node.inputs.begin(), node.inputs.end(), input) !=
               node.inputs.end();
      }
    }
    return false;
  }

  // Get all inputs for a specific operator
  std::vector<std::string> get_inputs(const std::string& node_name) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return node.inputs;
      }
    }
    return {};
  }

  // Get the output for a specific operator
  std::vector<std::string> get_outputs(const std::string& node_name) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return node.outputs;
      }
    }
    return {};
  }

  // Method to print the graph structure
  void print() const {
    std::cout << "Graph Structure (node_count=" << node_count() << "):" << std::endl;
    for (const auto& node : nodes_) {
      std::cout << " + Node: " << node.name << std::endl;
      std::cout << "   - Inputs: ";
      for (const auto& input : node.inputs) {
        std::cout << input << " ";
      }
      std::cout << std::endl;
      std::cout << "   - Outputs: ";
      for (const auto& output : node.outputs) {
        std::cout << output << " ";
      }
      std::cout << std::endl;
    }
  }
};


// Program to build the graph
class Program {
private:
  // set of variable names to avoid name conflict
  // in a program, input and output variables must have unique names.
  std::unordered_set<std::string> var_names;
public:
  static Program& current() {
    static Program instance;
    return instance;
  }
  template <typename T>
  friend class Var;

public:
  struct PendingNode {
    std::string op_name;
    std::vector<std::string> inputs;
    std::vector<std::string> output;
  };
  std::vector<PendingNode> pending_nodes_;

  template <typename T>
  void add(Var<T>& var) {
    pending_nodes_.push_back(var.take_pending_node());
  }

  template <typename... Ts>
  void add(VarTuple<Ts...>& tuple) {
    pending_nodes_.push_back(tuple.take_pending_node());
  }

  Graph graph() const {
    // Create a new graph from the pending nodes
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
  // Constructor, if no name is provided, it will be "__var".
  // name is required and unique for graph's input and output variables.
  // throw error if the name is already used in a Program.
  explicit Var(const std::string& name = "__var") : name_(name) {
    // "__var" is a default name for unnamed Var. These are usually internal
    // vars. We will rename them to avoid conflict when generating DAG.
    if (name == "__var") {
      return;
    }
    // check name conflict
    std::unordered_set<std::string>& var_names = Program::current().var_names;
    if (var_names.find(name) != var_names.end()) {
      throw std::runtime_error("Var name already exists");
    }
    var_names.insert(name);
  }

  // Make copy constructor not available, because each named Var must be unique.
  Var(const Var& other) = delete;

  // Copy assignment operator, used in every assignment statement.
  // It will make the new Var has a pending node.
  Var& operator=(const Var& other) {
    if (has_pending_) {
      throw std::runtime_error("Var already assigned");
    }

    pending_node_ = other.pending_node_;

    // The output should be this var's name plus a unique suffix
    pending_node_.output = {name_};

    has_pending_ = other.has_pending_;
    return *this;
  }

  // Move assignment operator
  Var& operator=(Var&& other) {
    if (has_pending_) {
      throw std::runtime_error("Var already assigned");
    }

    pending_node_ = std::move(other.pending_node_);

    pending_node_.output = {name_};

    has_pending_ = other.has_pending_;

    // Automatically add to current program
    Program::current().add(*this);

    return *this;
  }

  // Move constructor
  Var(Var&& other) = default;


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

    pending_node_.output = {name_};
  }

  // Add a new operator= for Op results
  template <typename OpType>
  Var& operator=(OpType&& op) {
    // First assign the operation result to this var
    *this = std::forward<OpType>(op);
    // Then automatically add it to the current program
    Program::current().add(*this);
    return *this;
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

  Program::PendingNode take_pending_node() {
    has_pending_ = false;
    return std::move(pending_node_);
  }

  // Add a new operator= for Op results
  template <typename OpType>
  VarTuple& operator=(OpType&& op) {
    // First assign the operation result
    *this = std::forward<OpType>(op);
    // Then automatically add it to the current program
    Program::current().add(*this);
    return *this;
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

  // Add this method to support chaining operations
  Var<R> operator()(Var<R>&& input) const {
    std::vector<std::string> input_names{input.name()};
    Var<R> result("_result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    
    // If the input has a pending node, we need to add it to the program first
    if (input.has_pending_node()) {
      Program::current().add(input);
    }
    
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
