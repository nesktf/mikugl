#pragma once

#include "core/command.hpp"

#include <string_view>

namespace mikugl {

class CommandStack final {
public:
  static constexpr size_t default_max_history = 128;

public:
  explicit CommandStack(size_t max_history = default_max_history);
  ~CommandStack() = default;

  NTF_NO_COPY(CommandStack);
  CommandStack(CommandStack&&) noexcept = default;
  CommandStack& operator=(CommandStack&&) noexcept = default;

public:
  CommandExpect<void> execute(UniquePtr<ICommand> command);
  CommandExpect<void> undo();
  CommandExpect<void> redo();
  void clear() noexcept;

public:
  bool can_undo() const noexcept;
  bool can_redo() const noexcept;
  size_t undo_count() const noexcept;
  size_t redo_count() const noexcept;
  std::string_view undo_name() const noexcept;
  std::string_view redo_name() const noexcept;

private:
  Vec<UniquePtr<ICommand>> _undo_stack;
  Vec<UniquePtr<ICommand>> _redo_stack;
  size_t _max_history;
};

} // namespace mikugl
