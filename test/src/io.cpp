#include <catch2/catch_test_macros.hpp>

#include "io/screenshot.hpp"

#include <filesystem>
#include <fstream>

namespace stdfs = std::filesystem;
using namespace mikugl;

TEST_CASE("Screenshot PNG export writes valid PNG file", "[phase8][screenshot]") {
  const auto tmp_path = stdfs::temp_directory_path() / "mikugl_test_shot.png";
  std::error_code ec;
  stdfs::remove(tmp_path, ec);

  static constexpr i32 w = 4;
  static constexpr i32 h = 4;
  Vec<u8> pixels(w * h * 4, 200);
  const auto res = Screenshot::write_rgba_png(tmp_path.string(), w, h, pixels, true);
  REQUIRE(res.has_value());
  REQUIRE(stdfs::exists(tmp_path));

  std::ifstream ifs(tmp_path, std::ios::binary);
  char sig[8] = {};
  ifs.read(sig, 8);
  REQUIRE(static_cast<u8>(sig[0]) == 0x89);
  REQUIRE(sig[1] == 'P');
  REQUIRE(sig[2] == 'N');
  REQUIRE(sig[3] == 'G');

  stdfs::remove(tmp_path, ec);
}
