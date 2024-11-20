#pragma once
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

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
    std::string output;
  };
  std::vector<NodeInfo> nodes_;

public:
  void add_node(const std::string& op_name,
                const std::vector<std::string>& inputs,
                const std::string& output) {
    nodes_.push_back({op_name, inputs, output});
  }

  size_t node_count() const {
    return nodes_.size();
  }
};

// Program to build the graph
class Program {
public:
  struct PendingNode {
    std::string op_name;
    std::vector<std::string> inputs;
    std::string output;
  };
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
      // You might want to add additional nodes for each output variable
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

// Base Node class (simplified - no evaluation)
template <typename T>
class Node {
protected:
  std::string name_;

public:
  explicit Node(const std::string& name) : name_(name) {}
  virtual ~Node() = default;
  const std::string& name() const {
    return name_;
  }
};

// Variable node in the DAG
template <typename T>
class VarNode : public Node<T> {
public:
  explicit VarNode(const std::string& name) : Node<T>(name) {}
};

// Operator node in the DAG
template <typename R, typename... Args>
class OpNode : public Node<R> {
  std::vector<std::string> input_names_;

public:
  OpNode(const std::string& name, const std::vector<std::string>& input_names)
      : Node<R>(name), input_names_(input_names) {}
};

// Variable wrapper class
template <typename T>
class Var {
  std::shared_ptr<Node<T>> node_;
  Program::PendingNode pending_node_;
  bool has_pending_ = false;

public:
  explicit Var(const std::string& name)
      : node_(std::make_shared<VarNode<T>>(name)) {}

  const std::string& name() const {
    return node_->name();
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
    pending_node_ = {op_name, inputs, this->name()};
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

  template <typename OpResult>
  VarTuple& operator=(OpResult&& result) {
    if constexpr (sizeof...(Ts) > 0) {
      set_pending_nodes(std::forward<OpResult>(result),
                        std::make_index_sequence<sizeof...(Ts)>{});
    }
    return *this;
  }

  bool has_pending_nodes() const {
    return has_pending_;
  }

  Program::PendingNode take_pending_nodes() {
    has_pending_ = false;
    return std::move(pending_node_);
  }

private:
  template <typename OpResult, size_t... Is>
  void set_pending_nodes(OpResult&& result, std::index_sequence<Is...>) {
    auto op_name = result.op_name();
    auto input_names = result.input_names();

    // Store the pending node info in the tuple
    pending_node_ = {op_name, input_names,
                     "tuple_output"};  // You might want a better name
    has_pending_ = true;

    // Set pending node for each output variable
    (std::get<Is>(vars_).set_pending_node(op_name + "_out" + std::to_string(Is),
                                          input_names),
     ...);
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
  Op() : op_name_(typeid(Op).name()) {}

  // For single output
  Var<R> operator()(const Var<Args>&... inputs) const {
    std::vector<std::string> input_names{inputs.name()...};
    Var<R> result("result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

// Specialization for tuple return types
template <typename... Rs, typename... Args>
class Op<std::tuple<Rs...>(Args...)> {
  std::string op_name_;

public:
  Op() : op_name_(typeid(Op).name()) {}

  class OpResult {
    std::string op_name_;
    std::vector<std::string> input_names_;

  public:
    OpResult(std::string op_name, std::vector<std::string> input_names)
        : op_name_(std::move(op_name)), input_names_(std::move(input_names)) {}

    const std::string& op_name() const {
      return op_name_;
    }
    const std::vector<std::string>& input_names() const {
      return input_names_;
    }
  };

  OpResult operator()(const Var<Args>&... inputs) const {
    std::vector<std::string> input_names{inputs.name()...};
    return OpResult(op_name_, input_names);
  }
};

// Wrapper types for different argument patterns
template<typename T>
struct Variadic {
    using type = T;
};

// Add specialization for variadic inputs
template <typename R, typename ArgT>
class Op<R(Variadic<ArgT>)> {
  std::string op_name_;

public:
  Op() : op_name_(typeid(Op).name()) {}

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
  Op() : op_name_(typeid(Op).name()) {}

  template<typename... Args>
  Var<R> operator()(const Var<FixedArgT>& fixed_arg, const Args&... args) const {
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
template <typename R, typename FixedArg1T, typename FixedArg2T, typename VarArgT>
class Op<R(FixedArg1T, FixedArg2T, Variadic<VarArgT>)> {
  std::string op_name_;

public:
  Op() : op_name_(typeid(Op).name()) {}

  template<typename... Args>
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
