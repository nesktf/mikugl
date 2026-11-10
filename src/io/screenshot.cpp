#include "io/screenshot.hpp"

#include <glad/glad.h>
#include <stb_image_write.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>

namespace mikugl {

namespace {

namespace stdfs = std::filesystem;

std::string make_timestamp_filename() {
  const auto now = std::chrono::system_clock::now();
  const auto ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf{};
#if defined(_WIN32)
  localtime_s(&tm_buf, &now_c);
#else
  localtime_r(&now_c, &tm_buf);
#endif
  char buf[64];
  std::snprintf(buf, sizeof(buf), "screenshot_%04d%02d%02d_%02d%02d%02d_%03d.png",
                tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday, tm_buf.tm_hour,
                tm_buf.tm_min, tm_buf.tm_sec, static_cast<int>(ms.count()));
  return std::string(buf);
}

} // namespace

ScreenshotExpect<void> Screenshot::write_rgba_png(std::string_view path, i32 width, i32 height,
                                                  Span<const u8> pixels, bool flip_vertically) {
  if (width <= 0 || height <= 0) {
    return Unexpected(ScreenshotError::InvalidDimensions);
  }
  const size_t row_bytes = static_cast<size_t>(width) * 4;
  const size_t total_bytes = row_bytes * static_cast<size_t>(height);
  if (pixels.size() < total_bytes) {
    return Unexpected(ScreenshotError::InvalidDimensions);
  }

  const std::string path_str(path);
  if (!flip_vertically) {
    const int ok = stbi_write_png(path_str.c_str(), width, height, 4, pixels.data(),
                                  static_cast<int>(row_bytes));
    if (!ok) {
      return Unexpected(ScreenshotError::WriteFailed);
    }
    return {};
  }

  auto flipped = make_unique_array<u8>(uninitialized, total_bytes);
  for (i32 y = 0; y < height; ++y) {
    const size_t src_off = static_cast<size_t>(height - 1 - y) * row_bytes;
    const size_t dst_off = static_cast<size_t>(y) * row_bytes;
    std::memcpy(flipped.data() + dst_off, pixels.data() + src_off, row_bytes);
  }

  const int ok = stbi_write_png(path_str.c_str(), width, height, 4, flipped.data(),
                                static_cast<int>(row_bytes));
  if (!ok) {
    return Unexpected(ScreenshotError::WriteFailed);
  }
  return {};
}

ScreenshotExpect<void> Screenshot::capture_viewport_to_file(std::string_view path, i32 width,
                                                            i32 height) {
  if (width <= 0 || height <= 0) {
    return Unexpected(ScreenshotError::InvalidDimensions);
  }
  const size_t total_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
  auto pixels = make_unique_array<u8>(uninitialized, total_bytes);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  for (size_t i = 3; i < total_bytes; i += 4) {
    pixels[i] = 255;
  }

  return write_rgba_png(path, width, height, Span<const u8>(pixels.data(), total_bytes), true);
}

ScreenshotExpect<std::string> Screenshot::capture_to_screenshots_dir(const stdfs::path& exe_dir,
                                                                     i32 width, i32 height) {
  std::error_code ec;
  const auto dir = exe_dir / "screenshots";
  stdfs::create_directories(dir, ec);
  if (ec || !stdfs::exists(dir)) {
    return Unexpected(ScreenshotError::DirectoryCreateFailed);
  }

  const auto file_path = dir / make_timestamp_filename();
  const auto path_str = file_path.string();
  const auto res = capture_viewport_to_file(path_str, width, height);
  if (!res) {
    return Unexpected(res.error());
  }
  return path_str;
}

} // namespace mikugl
