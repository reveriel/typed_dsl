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

template <typename T>
class Value;

template <typename... Ts>
class VarTuple;

class Graph;
class Program;

// Graph representation
class Graph {
private:
  struct Node {
    std::string name;
    std::string op_class;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    bool is_dead = false;
  };

  std::vector<Node> nodes_;
  std::unordered_set<std::string> placeholders_;
  std::unordered_map<std::string, std::string> var_to_producer_;

public:
  void add_node(const std::string& op_class,
                const std::vector<std::string>& inputs,
                const std::vector<std::string>& outputs) {
    std::string op_name = op_class + ":" + std::to_string(nodes_.size());
    nodes_.push_back({op_name, op_class, inputs, outputs});

    // Update variable dependencies
    for (const auto& output : outputs) {
      var_to_producer_[output] = op_name;
    }
  }

  void add_placeholder(const std::string& name) {
    placeholders_.insert(name);
  }

  void optimize() {
    // Mark dead nodes
    std::unordered_set<std::string> live_vars;
    std::unordered_set<std::string> live_nodes;

    // First pass: collect root variables (named outputs and placeholders)
    for (const auto& node : nodes_) {
      for (const auto& output : node.outputs) {
        if (output.find("__var") == std::string::npos) {
          live_vars.insert(output);
        }
      }
    }
    for (const auto& placeholder : placeholders_) {
      live_vars.insert(placeholder);
    }

    // Second pass: propagate liveness backwards
    bool changed;
    do {
      changed = false;
      for (size_t i = nodes_.size(); i-- > 0;) {
        const auto& node = nodes_[i];
        bool any_output_live = false;
        for (const auto& output : node.outputs) {
          if (live_vars.count(output) > 0) {
            any_output_live = true;
            break;
          }
        }
        if (any_output_live) {
          live_nodes.insert(node.name);
          for (const auto& input : node.inputs) {
            if (live_vars.insert(input).second) {
              changed = true;
            }
          }
        }
      }
    } while (changed);

    // Mark dead nodes
    for (auto& node : nodes_) {
      node.is_dead = (live_nodes.count(node.name) == 0);
    }
  }

  bool is_placeholder(const std::string& var_name) const {
    return placeholders_.find(var_name) != placeholders_.end();
  }

  size_t node_count() const {
    return std::count_if(nodes_.begin(), nodes_.end(),
                        [](const Node& node) { return !node.is_dead; });
  }

  bool consumes(const std::string& node_name, const std::string& input) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name && !node.is_dead) {
        return std::find(node.inputs.begin(), node.inputs.end(), input) !=
               node.inputs.end();
      }
    }
    return false;
  }

  bool produces(const std::string& node_name, const std::string& output) const {
    for (const auto& node : nodes_) {
      if (node.name == node_name && !node.is_dead) {
        return std::find(node.outputs.begin(), node.outputs.end(), output) !=
               node.outputs.end();
      }
    }
    return false;
  }

  void print() const {
    std::cout << "Graph Structure (node_count=" << node_count() << "):" << std::endl;
    for (const auto& node : nodes_) {
      if (!node.is_dead) {
        std::cout << " + Node: " << node.name << std::endl;
        std::cout << "   - Inputs: ";
        for (const auto& input : node.inputs) {
          std::cout << input << " ";
        }
        std::cout << std::endl << "   - Outputs: ";
        for (const auto& output : node.outputs) {
          std::cout << output << " ";
        }
        std::cout << std::endl;
      }
    }
  }
};

// Program to build the graph
class Program {
private:
  Graph graph_;
  std::unordered_set<std::string> var_names_;

public:
  Program() = default;

  void register_var_name(const std::string& name) {
    if (name != "__var" && var_names_.find(name) != var_names_.end()) {
      throw std::runtime_error("Var name already exists: " + name);
    }
    var_names_.insert(name);
  }

  void register_placeholder(const std::string& name) {
    register_var_name(name);
    graph_.add_placeholder(name);
  }

  void add_node(const std::string& op_name,
                const std::vector<std::string>& inputs,
                const std::vector<std::string>& outputs) {
    graph_.add_node(op_name, inputs, outputs);
  }

  Graph graph() const {
    Graph g = graph_;
    g.optimize();
    return g;
  }

  size_t node_count() const {
    return graph_.node_count();
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

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

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

// Tag for placeholder
struct placeholder_t {};
[[maybe_unused]] static constexpr placeholder_t placeholder{};

// Value class to represent computation results
template <typename T>
class Value {
private:
  std::string name_;
  std::string op_name_;
  std::vector<std::string> input_names_;

public:
  Value(const std::string& name, const std::string& op = "",
        const std::vector<std::string>& inputs = {})
      : name_(name), op_name_(op), input_names_(inputs) {}

  const std::string& name() const { return name_; }
  const std::string& op_name() const { return op_name_; }
  const std::vector<std::string>& input_names() const { return input_names_; }
};

// Variable wrapper class
template <typename T>
class Var {
private:
  std::string name_;
  Value<T>* current_value_;

public:
  explicit Var(const std::string& name = "__var") : name_(name) {
    if (name != "__var") {
      Context::current_program().register_var_name(name);
    }
  }

  Var(placeholder_t, const std::string& name) : name_(name) {
    Context::current_program().register_placeholder(name);
    current_value_ = new Value<T>(name);
  }

  template <typename OpType>
  std::enable_if_t<!std::is_same_v<std::decay_t<OpType>, Var>, Var&>
  operator=(OpType&& op) {
    // For named variables (non-__var), use their name as output
    std::string output_name = name_;
    current_value_ = new Value<T>(output_name, op.op_name(), op.input_names());
    Context::current_program().add_node(op.op_name(), op.input_names(), {output_name});
    return *this;
  }

  const Value<T>* current_value() const { return current_value_; }
};

// Helper class for multiple outputs
template <typename... Ts>
class Values {
private:
  std::tuple<Value<Ts>...> values_;

public:
  Values(const std::tuple<Value<Ts>...>& values) : values_(values) {}

  template <size_t I>
  auto& get() {
    return std::get<I>(values_);
  }
};

// Wrapper for variadic arguments
template <typename T>
struct Variadic {
  using type = T;
};

// Base Op class for all operations
template <typename Signature>
class Op;

// Regular function signature specialization
template <typename R, typename... Args>
class Op<R(Args...)> {
private:
  std::string op_name_;

public:
  explicit Op(const std::string& op_name) : op_name_(op_name) {}

  template <typename... ActualArgs,
            typename = std::enable_if_t<sizeof...(ActualArgs) == sizeof...(Args)>>
  Value<R> operator()(const Value<ActualArgs>&... args) const {
    std::vector<std::string> input_names = {args.name()...};
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  template <typename... ActualArgs,
            typename = std::enable_if_t<sizeof...(ActualArgs) == sizeof...(Args)>>
  Value<R> operator()(const Var<ActualArgs>&... args) const {
    std::vector<std::string> input_names;
    (input_names.push_back(args.current_value()->name()), ...);
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  const std::string& op_name() const { return op_name_; }
};

// Vector argument specialization (for real vectors)
template <typename R, typename ArgT>
class Op<R(std::vector<ArgT>)> {
private:
  std::string op_name_;

public:
  explicit Op(const std::string& op_name) : op_name_(op_name) {}

  Value<R> operator()(const std::vector<Value<ArgT>>& args) const {
    std::vector<std::string> input_names;
    for (const auto& arg : args) {
      input_names.push_back(arg.name());
    }
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  Value<R> operator()(const std::vector<Var<ArgT>>& args) const {
    std::vector<std::string> input_names;
    for (const auto& arg : args) {
      input_names.push_back(arg.current_value()->name());
    }
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  const std::string& op_name() const { return op_name_; }
};

// Variadic argument specialization
template <typename R, typename ArgT>
class Op<R(Variadic<ArgT>)> {
private:
  std::string op_name_;

public:
  explicit Op(const std::string& op_name) : op_name_(op_name) {}

  template <typename... Args,
            typename = std::enable_if_t<(std::is_convertible_v<Args, ArgT> && ...)>>
  Value<R> operator()(const Var<Args>&... args) const {
    std::vector<std::string> input_names;
    (input_names.push_back(args.current_value()->name()), ...);
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  const std::string& op_name() const { return op_name_; }
};

// Mixed arguments (fixed + variadic)
template <typename R, typename FixedArg, typename VarArgT>
class Op<R(FixedArg, Variadic<VarArgT>)> {
private:
  std::string op_name_;

public:
  explicit Op(const std::string& op_name) : op_name_(op_name) {}

  template <typename... Args,
            typename = std::enable_if_t<(std::is_convertible_v<Args, VarArgT> && ...)>>
  Value<R> operator()(const Var<FixedArg>& fixed, const Var<Args>&... args) const {
    std::vector<std::string> input_names;
    input_names.push_back(fixed.current_value()->name());
    (input_names.push_back(args.current_value()->name()), ...);
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  const std::string& op_name() const { return op_name_; }
};

// Mixed arguments (fixed + fixed + variadic)
template <typename R, typename FixedArg1, typename FixedArg2, typename VarArgT>
class Op<R(FixedArg1, FixedArg2, Variadic<VarArgT>)> {
private:
  std::string op_name_;

public:
  explicit Op(const std::string& op_name) : op_name_(op_name) {}

  template <typename... Args,
            typename = std::enable_if_t<(std::is_convertible_v<Args, VarArgT> && ...)>>
  Value<R> operator()(const Var<FixedArg1>& fixed1, const Var<FixedArg2>& fixed2, const Var<Args>&... args) const {
    std::vector<std::string> input_names;
    input_names.push_back(fixed1.current_value()->name());
    input_names.push_back(fixed2.current_value()->name());
    (input_names.push_back(args.current_value()->name()), ...);
    auto& prog = Context::current_program();
    std::string result_name = op_name_ + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(op_name_, input_names, {result_name});
    return Value<R>(result_name, op_name_, input_names);
  }

  const std::string& op_name() const { return op_name_; }
};

// Helper for tuple destructuring
template <typename... Types>
class VarTuple {
  std::tuple<Var<Types>&...> vars_;

public:
  VarTuple(Var<Types>&... vars) : vars_(vars...) {}

  template <typename T>
  VarTuple& operator=(T&& value) {
    std::vector<std::string> outputs;
    std::apply([&](auto&... vars) {
      (outputs.push_back(vars.current_value()->name()), ...);
    }, vars_);

    auto& prog = Context::current_program();
    std::string result_name = value.op_name() + ":" + std::to_string(prog.node_count()) + ":0";
    prog.add_node(value.op_name(), {value.name()}, outputs);
    return *this;
  }
};

// Helper functions for VarTuple creation
template <typename T1, typename T2>
VarTuple<T1, T2> operator,(Var<T1>& var1, Var<T2>& var2) {
  return VarTuple<T1, T2>(var1, var2);
}

template <typename... Ts, typename T>
VarTuple<Ts..., T> operator,(VarTuple<Ts...>& tuple, Var<T>& var) {
  return std::apply([&var](auto&... vars) {
    return VarTuple<Ts..., T>(vars..., var);
  }, tuple.vars_);
}
