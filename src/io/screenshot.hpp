#pragma once

#include "core.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace mikugl {

enum class ScreenshotError : u8 {
  InvalidDimensions,
  DirectoryCreateFailed,
  WriteFailed,
};

template<typename T>
using ScreenshotExpect = Expected<T, ScreenshotError>;

class Screenshot final {
public:
  static ScreenshotExpect<void> write_rgba_png(std::string_view path, i32 width, i32 height,
                                               Span<const u8> pixels, bool flip_vertically = true);

  static ScreenshotExpect<void> capture_viewport_to_file(std::string_view path, i32 width,
                                                         i32 height);

  static ScreenshotExpect<std::string>
  capture_to_screenshots_dir(const std::filesystem::path& exe_dir, i32 width, i32 height);
};

} // namespace mikugl
