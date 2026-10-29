#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "model/model_data.hpp"
#include "render/shadow_map.hpp"
#include "render/toon_manager.hpp"

#include <filesystem>

TEST_CASE("ShadowMap light matrix calculations", "[shadow_map]") {
  const mikugl::ShadowMap shadow_map{};
  const ran::Vec3f32 target{0.0f, 10.0f, 0.0f};
  const ran::Vec3f32 light_dir{0.5f, 1.0f, 0.7f};

  const auto [light_view, light_proj] =
    shadow_map.compute_light_matrices(target, light_dir, 30.0f);

  // target in world space should have x=0, y=0 in light eye space
  const ran::Vec4f32 target_eye = light_view * ran::Vec4f32{target.x, target.y, target.z, 1.0f};
  REQUIRE_THAT(target_eye.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
  REQUIRE_THAT(target_eye.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));

  // light is placed along +Z of light view, looking towards target at -Z
  REQUIRE(target_eye.z < 0.0f);

  // proj should preserve x/y extent mapping
  const ran::Vec4f32 extent_corner = light_proj * ran::Vec4f32{30.0f, 30.0f, target_eye.z, 1.0f};
  REQUIRE_THAT(extent_corner.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
  REQUIRE_THAT(extent_corner.y, Catch::Matchers::WithinAbs(1.0f, 0.001f));
}

TEST_CASE("ToonManager assets existence", "[toon_manager]") {
  REQUIRE(mikugl::ToonManager::shared_toon_count == 10);

  const std::string candidate_dirs[] = {
    "data",
    "../data",
    "build/data",
  };

  std::string valid_dir;
  for (const auto& dir : candidate_dirs) {
    if (std::filesystem::exists(dir + "/toon01.bmp")) {
      valid_dir = dir;
      break;
    }
  }

  REQUIRE_FALSE(valid_dir.empty());

  for (size_t i = 1; i <= 10; ++i) {
    const std::string filename =
      (i < 10) ? ("toon0" + std::to_string(i) + ".bmp") : ("toon" + std::to_string(i) + ".bmp");
    REQUIRE(std::filesystem::exists(valid_dir + "/" + filename));
  }
}

TEST_CASE("ModelMaterial sphere and toon properties", "[model_material]") {
  mikugl::ModelMaterial mat{};
  mat.sphere_mode = 1; // multiplicative .sph
  mat.toon_sharing_flag = 1;
  mat.toon_index = 0;
  mat.has_edge = true;
  mat.edge_size = 1.0f;
  mat.edge_color = ran::Vec4f32{0.0f, 0.0f, 0.0f, 1.0f};

  REQUIRE(mat.sphere_mode == 1);
  REQUIRE(mat.toon_sharing_flag == 1);
  REQUIRE(mat.toon_index == 0);
  REQUIRE(mat.has_edge);
  REQUIRE(mat.edge_size == 1.0f);
}
