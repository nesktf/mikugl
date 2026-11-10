#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "scene/scene.hpp"

using namespace mikugl;

TEST_CASE("Scene supports multiple models, mesh visibility, and bone picking", "[scene]") {
  Scene scene{};
  REQUIRE(scene.models().empty());
  REQUIRE(scene.selected_model() == nullptr);

  ModelData m1{};
  m1.name = "Miku";
  ModelBone b0{};
  b0.name_en = "Root";
  b0.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  b0.parent_index = -1;
  m1.bones.push_back(b0);
  ModelSubmesh sm0{};
  sm0.name = "Hair";
  sm0.index_count = 30;
  m1.submeshes.push_back(sm0);

  ModelData m2{};
  m2.name = "Rin";
  m2.bones.push_back(b0);

  const u32 id1 = scene.add_model(std::move(m1), "miku.pmx", false);
  REQUIRE(id1 == 0);
  REQUIRE(scene.models().size() == 1);
  REQUIRE(scene.selected_model() != nullptr);
  REQUIRE(scene.selected_model()->name == "Miku");

  const u32 id2 = scene.add_model(std::move(m2), "rin.pmx", false);
  REQUIRE(id2 == 1);
  REQUIRE(scene.models().size() == 2);
  REQUIRE(scene.selected_model() != nullptr);
  REQUIRE(scene.selected_model()->name == "Rin");

  scene.select_model(0);
  REQUIRE(scene.selected_model()->name == "Miku");
  scene.selected_model()->set_all_submeshes_visible(false);
  REQUIRE(!scene.selected_model()->submeshes[0].visible);
  scene.selected_model()->set_all_submeshes_visible(true);
  REQUIRE(scene.selected_model()->submeshes[0].visible);

  scene.camera().reset();
  scene.camera().set_pitch(0.0f);
  scene.camera().set_yaw(0.0f);
  REQUIRE(scene.pick_bone_screen_space(640.0f, 360.0f, 1280.0f, 720.0f) == -1);

  scene.set_posing_mode(true);
  const i32 picked = scene.pick_bone_screen_space(640.0f, 360.0f, 1280.0f, 720.0f, 20.0f);
  REQUIRE(picked == 0);

  scene.selected_model()->skeleton->set_all_bones_visible(false);
  REQUIRE(scene.pick_bone_screen_space(640.0f, 360.0f, 1280.0f, 720.0f, 20.0f) == -1);

  scene.remove_model(0);
  REQUIRE(scene.models().size() == 1);
  REQUIRE(scene.selected_model()->name == "Rin");
}
