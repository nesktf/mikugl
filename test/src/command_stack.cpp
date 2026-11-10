#include <catch2/catch_test_macros.hpp>

#include "core/command.hpp"
#include "core/command_stack.hpp"

#include <memory>

namespace {

using namespace mikugl;

class IncrementCommand final : public ICommand {
public:
  IncrementCommand(int* target, int delta) : _target(target), _delta(delta) {}

  CommandExpect<void> execute() override {
    if (!_target) {
      return Unexpected<CommandError>(CommandError::ExecutionFailed);
    }
    *_target += _delta;
    return CommandExpect<void>();
  }

  CommandExpect<void> undo() override {
    if (!_target) {
      return Unexpected<CommandError>(CommandError::UndoFailed);
    }
    *_target -= _delta;
    return CommandExpect<void>();
  }

  std::string_view name() const noexcept override { return "IncrementCommand"; }

private:
  int* _target;
  int _delta;
};

} // namespace

TEST_CASE("CommandStack basic execution and undo/redo", "[command_stack]") {
  CommandStack stack(10);
  int counter = 0;

  REQUIRE_FALSE(stack.can_undo());
  REQUIRE_FALSE(stack.can_redo());
  REQUIRE(stack.undo_count() == 0);
  REQUIRE(stack.redo_count() == 0);

  SECTION("execute updates state and can be undone") {
    const auto res1 = stack.execute(std::make_unique<IncrementCommand>(&counter, 5));
    REQUIRE(res1.has_value());
    REQUIRE(counter == 5);
    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo_count() == 1);
    REQUIRE(stack.undo_name() == "IncrementCommand");

    const auto res_undo = stack.undo();
    REQUIRE(res_undo.has_value());
    REQUIRE(counter == 0);
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE(stack.can_redo());
    REQUIRE(stack.redo_count() == 1);
    REQUIRE(stack.redo_name() == "IncrementCommand");

    const auto res_redo = stack.redo();
    REQUIRE(res_redo.has_value());
    REQUIRE(counter == 5);
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
  }

  SECTION("new command clears redo stack") {
    const auto res1 = stack.execute(std::make_unique<IncrementCommand>(&counter, 2));
    REQUIRE(res1.has_value());
    const auto res2 = stack.execute(std::make_unique<IncrementCommand>(&counter, 3));
    REQUIRE(res2.has_value());
    REQUIRE(counter == 5);

    const auto res_undo = stack.undo();
    REQUIRE(res_undo.has_value());
    REQUIRE(counter == 2);
    REQUIRE(stack.can_redo());

    const auto res3 = stack.execute(std::make_unique<IncrementCommand>(&counter, 10));
    REQUIRE(res3.has_value());
    REQUIRE(counter == 12);
    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.redo_count() == 0);
  }

  SECTION("max history limits undo stack depth") {
    CommandStack small_stack(3);
    for (int i = 0; i < 5; ++i) {
      const auto res = small_stack.execute(std::make_unique<IncrementCommand>(&counter, 1));
      REQUIRE(res.has_value());
    }
    REQUIRE(counter == 5);
    REQUIRE(small_stack.undo_count() == 3);

    REQUIRE(small_stack.undo().has_value());
    REQUIRE(small_stack.undo().has_value());
    REQUIRE(small_stack.undo().has_value());
    REQUIRE_FALSE(small_stack.can_undo());
    REQUIRE(counter == 2);
  }

  SECTION("undo on empty stack returns error") {
    const auto err = stack.undo();
    REQUIRE(err.has_error());
    REQUIRE(err.error() == CommandError::StackEmpty);
  }
}
