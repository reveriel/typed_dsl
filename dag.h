#pragma once
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <stack>

// Forward declarations
template <typename T>
class Var;

template <typename... Ts>
class VarTuple;

class Graph;
class IR;
class Program;

// Graph representation
class Graph {
  struct NodeInfo {
    std::string name;
    std::string op_class;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
  };
  std::vector<NodeInfo> nodes_;
  std::unordered_set<std::string> placeholders_;

public:
  void add_node(const std::string& op_class,
                const std::vector<std::string>& inputs,
                const std::vector<std::string>& outputs,
                const std::vector<std::string>& placeholder_inputs = {}) {
    std::string op_name = op_class + "_" + std::to_string(nodes_.size());
    nodes_.push_back({op_name, op_class, inputs, outputs});
    for (const auto& input : placeholder_inputs) {
      placeholders_.insert(input);
    }
  }

  bool is_placeholder(const std::string& var_name) const {
    return placeholders_.find(var_name) != placeholders_.end();
  }

  size_t node_count() const {
    return nodes_.size();
  }

  bool has_node(const std::string& node_name) const {
    return std::any_of(nodes_.begin(), nodes_.end(), [&](const NodeInfo& node) {
      return node.name == node_name;
    });
  }

  bool consumes(const std::string& node_name, const std::string& input) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return std::find(node.inputs.begin(), node.inputs.end(), input) !=
               node.inputs.end();
      }
    }
    return false;
  }

  bool consumes(const std::string& node_name,
                const std::vector<std::string>& inputs) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return std::all_of(
            inputs.begin(), inputs.end(), [&](const std::string& input) {
              return std::find(node.inputs.begin(), node.inputs.end(), input) !=
                     node.inputs.end();
            });
      }
    }
    return false;
  }

  bool produces(const std::string& node_name, const std::string& output) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name) {
        return std::find(node.outputs.begin(), node.outputs.end(), output) !=
               node.outputs.end();
      }
    }
    return false;
  }

  void print() const {
    std::cout << "Graph Structure (node_count=" << node_count()
              << "):" << std::endl;
    for (const auto& node : nodes_) {
      std::cout << " + Node: " << node.name << std::endl;
      std::cout << "   - Inputs: ";
      for (const auto& input : node.inputs)
        std::cout << input << " ";
      std::cout << std::endl << "   - Outputs: ";
      for (const auto& output : node.outputs)
        std::cout << output << " ";
      std::cout << std::endl;
    }
  }
};

// IR Node Types
enum class IRNodeType { PLACEHOLDER, OPERATION, VARIABLE };

// IR Node representing operations before they're compiled into the final graph
struct IRNode {
  IRNodeType type;
  std::string name;
  std::string op_class;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  bool is_dead = false;

  IRNode(const std::string& op, const std::vector<std::string>& ins,
         const std::vector<std::string>& outs)
      : type(IRNodeType::OPERATION), op_class(op), inputs(ins), outputs(outs) {
    name = op + "_" + std::to_string(get_next_id());
  }

  static IRNode create_placeholder(const std::string& name) {
    IRNode node("", {}, {name});
    node.type = IRNodeType::PLACEHOLDER;
    node.name = name;
    return node;
  }

private:
  static size_t get_next_id() {
    static size_t next_id = 0;
    return next_id++;
  }
};

// Intermediate Representation
class IR {
private:
  std::vector<IRNode> nodes_;
  std::unordered_map<std::string, size_t> var_to_last_def_;
  std::unordered_set<std::string> placeholders_;

public:
  void add_node(IRNode node) {
    for (const auto& output : node.outputs) {
      var_to_last_def_[output] = nodes_.size();
    }
    nodes_.push_back(std::move(node));
  }

  void add_placeholder(const std::string& name) {
    placeholders_.insert(name);
    add_node(IRNode::create_placeholder(name));
  }

  void optimize() {
    dead_store_elimination();
  }

  void dead_store_elimination() {
    std::unordered_set<size_t> live_nodes;
    std::unordered_set<std::string> live_vars;

    for (const auto& [var, idx] : var_to_last_def_) {
      if (placeholders_.count(var) > 0 ||
          var.find("__var") == std::string::npos) {
        live_vars.insert(var);
        live_nodes.insert(idx);
      }
    }

    bool changed;
    do {
      changed = false;
      for (size_t i = nodes_.size(); i-- > 0;) {
        if (live_nodes.count(i) > 0) {
          for (const auto& input : nodes_[i].inputs) {
            if (live_vars.insert(input).second) {
              auto it = var_to_last_def_.find(input);
              if (it != var_to_last_def_.end()) {
                live_nodes.insert(it->second);
                changed = true;
              }
            }
          }
        }
      }
    } while (changed);

    for (size_t i = 0; i < nodes_.size(); i++) {
      nodes_[i].is_dead = (live_nodes.count(i) == 0);
    }
  }

  Graph to_graph() const {
    Graph g;
    for (const auto& node : nodes_) {
      if (!node.is_dead && node.type == IRNodeType::OPERATION) {
        g.add_node(node.op_class, node.inputs, node.outputs,
                   get_placeholder_inputs(node.inputs));
      }
    }
    return g;
  }

private:
  std::vector<std::string>
  get_placeholder_inputs(const std::vector<std::string>& inputs) const {
    std::vector<std::string> result;
    for (const auto& input : inputs) {
      if (placeholders_.count(input) > 0) {
        result.push_back(input);
      }
    }
    return result;
  }
};

// Context for managing program scopes
class Context {
private:
  std::stack<Program*> program_stack_;

  static Context& instance() {
    static Context ctx;
    return ctx;
  }

public:
  static void push_program(Program* prog) {
    if (!prog) {
      throw std::runtime_error("Cannot push null program to context");
    }
    instance().program_stack_.push(prog);
  }

  static void pop_program() {
    auto& ctx = instance();
    if (ctx.program_stack_.empty()) {
      throw std::runtime_error("Cannot pop from empty program stack");
    }
    ctx.program_stack_.pop();
  }

  static Program& current_program() {
    auto& ctx = instance();
    if (ctx.program_stack_.empty()) {
      throw std::runtime_error("No active program context");
    }
    return *ctx.program_stack_.top();
  }

  // RAII helper for program scope
  class Scope {
    private:
      bool active_ = false;
    
    public:
      explicit Scope(Program* prog) {
        if (!prog) {
          throw std::runtime_error("Cannot create scope with null program");
        }
        push_program(prog);
        active_ = true;
      }

      ~Scope() {
        if (active_) {
          pop_program();
        }
      }

      // Prevent copying
      Scope(const Scope&) = delete;
      Scope& operator=(const Scope&) = delete;

      // Allow moving
      Scope(Scope&& other) noexcept : active_(other.active_) {
        other.active_ = false;
      }

      Scope& operator=(Scope&& other) noexcept {
        if (this != &other) {
          if (active_) {
            pop_program();
          }
          active_ = other.active_;
          other.active_ = false;
        }
        return *this;
      }
  };
};

// Program to build the graph
class Program {
private:
  IR ir_;
  std::unordered_set<std::string> var_names_;

public:
  Program() = default;

  struct PendingNode {
    std::string op_name;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
  };

  template <typename T>
  void add(Var<T>& var) {
    auto pending = var.take_pending_node();
    ir_.add_node(IRNode(pending.op_name, pending.inputs, pending.outputs));
  }

  template <typename... Ts>
  void add(VarTuple<Ts...>& tuple) {
    auto pending = tuple.take_pending_node();
    ir_.add_node(IRNode(pending.op_name, pending.inputs, pending.outputs));
  }

  Graph graph() const {
    auto ir_copy = ir_;
    ir_copy.optimize();
    return ir_copy.to_graph();
  }

  void register_var_name(const std::string& name) {
    if (name != "__var" && var_names_.find(name) != var_names_.end()) {
      throw std::runtime_error("Var name already exists: " + name);
    }
    var_names_.insert(name);
  }

  void register_placeholder(const std::string& name) {
    register_var_name(name);
    ir_.add_placeholder(name);
  }
};

// Tag for placeholder
struct placeholder_t {};
[[maybe_unused]] static constexpr placeholder_t placeholder{};

// Variable wrapper class
template <typename T>
class Var {
private:
  std::string name_;
  Program::PendingNode pending_node_;
  bool has_pending_ = false;

public:
  explicit Var(const std::string& name = "__var") : name_(name) {
    if (name != "__var") {
      Context::current_program().register_var_name(name);
    }
  }

  Var(placeholder_t, const std::string& name) : name_(name) {
    Context::current_program().register_placeholder(name);
  }

  // Make copy constructor not available
  Var(const Var& other) = delete;

  // Move constructor
  Var(Var&& other) = default;

  Var& operator=(const Var& other) {
    pending_node_ = other.pending_node_;
    if (pending_node_.outputs.empty()) {
      pending_node_ = {"copy", {other.name()}, {name_}};
    } else {
      pending_node_.outputs = {name_};
    }
    has_pending_ = true;
    Context::current_program().add(*this);
    return *this;
  }

  Var& operator=(Var&& other) {
    pending_node_ = std::move(other.pending_node_);
    pending_node_.outputs = {name_};
    has_pending_ = other.has_pending_;
    if (has_pending_) {
      Context::current_program().add(*this);
    }
    return *this;
  }

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
    pending_node_ = {op_name, inputs, {name_}};
    has_pending_ = true;
  }

  template <typename OpType>
  std::enable_if_t<!std::is_same_v<std::decay_t<OpType>, Var>, Var&>
  operator=(OpType&& op) {
    *this = std::forward<OpType>(op);
    Context::current_program().add(*this);
    return *this;
  }
};

// Helper class for multiple outputs
template <typename... Ts>
class VarTuple {
private:
  std::tuple<Var<Ts>&...> vars_;
  Program::PendingNode pending_node_;
  bool has_pending_ = false;

public:
  explicit VarTuple(Var<Ts>&... vars) : vars_(vars...) {}

  VarTuple& operator=(Var<std::tuple<Ts...>>&& result) {
    if (has_pending_) {
      throw std::runtime_error("VarTuple already assigned");
    }
    pending_node_ = result.take_pending_node();
    pending_node_.outputs.clear();
    std::apply(
        [&](auto&... vars) {
          (pending_node_.outputs.push_back(vars.name()), ...);
        },
        vars_);
    has_pending_ = true;
    return *this;
  }

  Program::PendingNode take_pending_node() {
    has_pending_ = false;
    return std::move(pending_node_);
  }

  template <typename OpType>
  VarTuple& operator=(OpType&& op) {
    *this = std::forward<OpType>(op);
    Context::current_program().add(*this);
    return *this;
  }
};

// Wrapper types for different argument patterns
template <typename T>
struct Variadic {
  using type = T;
};

// Operator wrapper class
template <typename Signature>
class Op;

template <typename R, typename... Args>
class Op<R(Args...)> {
  std::string op_name_;

public:
  explicit Op(const std::string& name) : op_name_(name) {}

  const std::string& name() const {
    return op_name_;
  }

  static std::string get_next_var_name(const std::string& op_name) {
    static std::unordered_map<std::string, size_t> op_counters;
    return op_name + "_" + std::to_string(op_counters[op_name]++) + "/output";
  }

  Var<R> operator()(const Var<Args>&... inputs) const {
    std::vector<std::string> input_names{inputs.name()...};
    Var<R> result(get_next_var_name(op_name_));
    result.set_pending_node(op_name_, input_names);
    return result;
  }

  Var<R> operator()(Var<R>&& input) const {
    std::vector<std::string> input_names{input.name()};
    Var<R> result(get_next_var_name(op_name_));
    result.set_pending_node(op_name_, input_names);
    if (input.has_pending_node()) {
      Context::current_program().add(input);
    }
    return result;
  }
};

template <typename R, typename ArgT>
class Op<R(Variadic<ArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}

  const std::string& name() const {
    return op_name_;
  }

  static std::string get_next_var_name(const std::string& op_name) {
    static std::unordered_map<std::string, size_t> op_counters;
    return op_name + "_" + std::to_string(op_counters[op_name]++) + "/output";
  }

  template <typename... Args>
  Var<R> operator()(const Args&... args) const {
    static_assert((std::is_same_v<Args, Var<ArgT>> && ...),
                  "All arguments must be of the same type Var<ArgT>");

    std::vector<std::string> input_names;
    input_names.reserve(sizeof...(args));
    (input_names.push_back(args.name()), ...);

    Var<R> result(get_next_var_name(op_name_));
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

template <typename R, typename FixedArgT, typename VarArgT>
class Op<R(FixedArgT, Variadic<VarArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}
  
  const std::string& name() const {
    return op_name_;
  }

  static std::string get_next_var_name(const std::string& op_name) {
    static std::unordered_map<std::string, size_t> op_counters;
    return op_name + "_" + std::to_string(op_counters[op_name]++) + "/output";
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

    Var<R> result(get_next_var_name(op_name_));
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

template <typename R, typename FixedArg1T, typename FixedArg2T,
          typename VarArgT>
class Op<R(FixedArg1T, FixedArg2T, Variadic<VarArgT>)> {
  std::string op_name_;

public:
  Op(const std::string& name) : op_name_(name) {}
  
  const std::string& name() const {
    return op_name_;
  }

  static std::string get_next_var_name(const std::string& op_name) {
    static std::unordered_map<std::string, size_t> op_counters;
    return op_name + "_" + std::to_string(op_counters[op_name]++) + "/output";
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

    Var<R> result(get_next_var_name(op_name_));
    result.set_pending_node(op_name_, input_names);
    return result;
  }
};

// Helper functions for VarTuple creation
template <typename T1, typename T2>
VarTuple<T1, T2> operator,(Var<T1>& var1, Var<T2>& var2) {
  return VarTuple<T1, T2>(var1, var2);
}

template <typename... Ts, typename T>
VarTuple<Ts..., T> operator,(VarTuple<Ts...> tuple, Var<T>& var) {
  return std::apply(
      [&](auto&... vars) { return VarTuple<Ts..., T>(vars..., var); },
      tuple.vars_);
}
