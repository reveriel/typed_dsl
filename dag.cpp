#include "dag.h"

std::string Graph::to_string(const std::string& node_name) const {
  auto join = [](const std::vector<std::string>& vec,
                 const std::string& delimiter) {
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
      ss << vec[i];
      if (i < vec.size() - 1) {
        ss << delimiter;
      }
    }
    return ss.str();
  };
  std::stringstream ss;
  for (const auto& node : nodes_) {
    if (node.name == node_name) {
      ss << "{" << join(node.inputs, ", ") << "} -> {" << node.name << "} -> {"
         << join(node.outputs, ", ") << "}";
      return ss.str();
    }
  }
  return "";
}
