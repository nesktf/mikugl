#pragma once

#define MIKU_UNUSED(thing) (void)thing
#define miku_assert        assert

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <ntf/core.hpp>
#include <ntf/expected.hpp>
#include <ntf/unique.hpp>

namespace mikugl {

using size_t = ::size_t;
using ptrdiff_t = ::ptrdiff_t;
using uintptr_t = ::uintptr_t;

using u8 = ::uint8_t;
using u16 = ::uint16_t;
using u32 = ::uint32_t;
using u64 = ::uint64_t;
using i8 = ::int8_t;
using i16 = ::int16_t;
using i32 = ::int32_t;
using i64 = ::int64_t;

using f32 = float;
static_assert(sizeof(f32) == 4);

using f64 = double;
static_assert(sizeof(f64) == 8);

template<typename T>
using Vec = std::vector<T>;

template<typename T, typename E>
using Expected = ::ntf::Expected<T, E>;

template<typename E>
using Unexpected = ::ntf::unexpected<E>;

template<typename T>
using UniqueArray = ::ntf::UniqueArray<T>;

template<typename T>
using Optional = std::optional<T>;

using ::ntf::make_unique_array;
using ::ntf::uninitialized;

template<typename T>
using Span = std::span<T>;

template<typename T, typename Deleter = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Deleter>;

// TODO: update ntfstl DeferFn with this
template<typename Fn>
class DeferFn {
public:
  template<typename Func>
  constexpr DeferFn(Func&& func) : _func(std::forward<Func>(func)), _engaged(true) {}

  constexpr ~DeferFn() noexcept { invoke(); }

  DeferFn(DeferFn&&) = delete;
  DeferFn(const DeferFn&) = delete;
  DeferFn& operator=(DeferFn&&) = delete;
  DeferFn& operator=(const DeferFn&) = delete;

public:
  constexpr void invoke() noexcept {
    if (_engaged) {
      _func();
    }
    _engaged = false;
  }

  constexpr void operator()() noexcept { invoke(); }

  constexpr void disengage() noexcept { _engaged = false; }

private:
  Fn _func;
  bool _engaged;
};

template<typename Fn>
DeferFn(Fn) -> DeferFn<Fn>;

template<typename... Fs>
struct OverloadFn : Fs... {
  using Fs::operator()...;
};

template<typename... Fs>
OverloadFn(Fs...) -> OverloadFn<Fs...>;

constexpr auto missing_texture = []() {
  std::array<u8, 4 * 16 * 16> pixels;
  for (u32 x = 0; x < 16; x++) {
    for (u32 y = 0; y < 16; y++) {
      const u8 a = 0xFF;
      const u8 g = 0x00;
      u8 r = 0x00;
      u8 b = 0x00;
      if ((x % 2) ^ (y % 2)) {
        // make magenta
        r = 0xFF;
        b = 0xFF;
      }
      pixels[(y * 16 + x) * 4 + 0] = r;
      pixels[(y * 16 + x) * 4 + 1] = g;
      pixels[(y * 16 + x) * 4 + 2] = b;
      pixels[(y * 16 + x) * 4 + 3] = a;
    }
  }
  return pixels;
}();

} // namespace mikugl
