#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
template <typename T>
class Var;

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

// Operator wrapper class
template <typename Signature>
class Op;

template <typename R, typename... Args>
class Op<R(Args...)> {
  std::string op_name_;

public:
  Op() : op_name_(typeid(Op).name()) {}  // Use RTTI name as default op name

  Var<R> operator()(const Var<Args>&... inputs) const {
    std::vector<std::string> input_names{inputs.name()...};
    Var<R> result("result_" + op_name_);
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};
