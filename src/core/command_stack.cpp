#include "core/command_stack.hpp"

#include <utility>

namespace mikugl {

CommandStack::CommandStack(size_t max_history) :
    _undo_stack(), _redo_stack(), _max_history(max_history) {}

Expected<void, CommandError> CommandStack::execute(UniquePtr<ICommand> command) {
  miku_assert(command);
  return command->execute().transform([&]() {
    _redo_stack.clear();

    if (_undo_stack.size() >= _max_history && !_undo_stack.empty()) {
      _undo_stack.erase(_undo_stack.begin());
    }

    _undo_stack.push_back(std::move(command));
  });
}

Expected<void, CommandError> CommandStack::undo() {
  if (_undo_stack.empty()) {
    return Unexpected<CommandError>(CommandError::StackEmpty);
  }

  UniquePtr<ICommand> cmd = std::move(_undo_stack.back());
  _undo_stack.pop_back();
  return cmd->undo().transform([&]() { _redo_stack.push_back(std::move(cmd)); });
}

Expected<void, CommandError> CommandStack::redo() {
  if (_redo_stack.empty()) {
    return Unexpected<CommandError>(CommandError::StackEmpty);
  }

  UniquePtr<ICommand> cmd = std::move(_redo_stack.back());
  _redo_stack.pop_back();
  return cmd->execute().transform([&]() { _undo_stack.push_back(std::move(cmd)); });
}

bool CommandStack::can_undo() const noexcept {
  return !_undo_stack.empty();
}

bool CommandStack::can_redo() const noexcept {
  return !_redo_stack.empty();
}

size_t CommandStack::undo_count() const noexcept {
  return _undo_stack.size();
}

size_t CommandStack::redo_count() const noexcept {
  return _redo_stack.size();
}

std::string_view CommandStack::undo_name() const noexcept {
  if (_undo_stack.empty()) {
    return {};
  }
  return _undo_stack.back()->name();
}

std::string_view CommandStack::redo_name() const noexcept {
  if (_redo_stack.empty()) {
    return {};
  }
  return _redo_stack.back()->name();
}

void CommandStack::clear() noexcept {
  _undo_stack.clear();
  _redo_stack.clear();
}

} // namespace mikugl
