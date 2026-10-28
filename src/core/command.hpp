#pragma once

#include "core.hpp"

#include <string_view>

namespace mikugl {

enum class CommandError : u8 {
  None = 0,
  ExecutionFailed,
  UndoFailed,
  RedoFailed,
  StackEmpty,
};

template<typename T>
using CommandExpect = Expected<T, CommandError>;

class ICommand {
public:
  virtual ~ICommand() = default;
  virtual CommandExpect<void> execute() = 0;
  virtual CommandExpect<void> undo() = 0;
  virtual std::string_view name() const noexcept = 0;
};

} // namespace mikugl
