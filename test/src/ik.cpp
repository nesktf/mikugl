#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "animation/ik_solver.hpp"
#include "animation/skeleton.hpp"
#include "model/model_loader.hpp"

#include <cmath>
#include <filesystem>
#include <string>

using namespace mikugl;
using Catch::Matchers::WithinAbs;

TEST_CASE("CCD-IK synthetic 2-link chain converges to reachable target", "[ik]") {
  Vec<ModelBone> bones;

  ModelBone hip{};
  hip.name_en = "Hip";
  hip.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  hip.parent_index = -1;
  hip.connection_bone_index = 1;
  hip.ik_target_index = -1;
  bones.push_back(hip);

  ModelBone thigh{};
  thigh.name_en = "Thigh";
  thigh.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  thigh.parent_index = 0;
  thigh.connection_bone_index = 2;
  thigh.ik_target_index = -1;
  bones.push_back(thigh);

  ModelBone knee{};
  knee.name_en = "Knee";
  knee.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  knee.parent_index = 1;
  knee.connection_bone_index = 3;
  knee.ik_target_index = -1;
  bones.push_back(knee);

  ModelBone ankle{};
  ankle.name_en = "Ankle";
  ankle.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  ankle.parent_index = 2;
  ankle.connection_bone_index = -1;
  ankle.ik_target_index = -1;
  bones.push_back(ankle);

  ModelBone foot_ik{};
  foot_ik.name_en = "Foot_IK";
  foot_ik.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  foot_ik.parent_index = -1;
  foot_ik.connection_bone_index = -1;
  foot_ik.ik_target_index = 3;
  foot_ik.ik_loop_count = 50;
  foot_ik.ik_limit_angle = ran::pi<f32>;

  ModelIkLink knee_link{};
  knee_link.bone_index = 2;
  knee_link.has_limits = false;
  foot_ik.ik_links.push_back(knee_link);

  ModelIkLink thigh_link{};
  thigh_link.bone_index = 1;
  thigh_link.has_limits = false;
  foot_ik.ik_links.push_back(thigh_link);

  bones.push_back(foot_ik);

  auto skel_res = Skeleton::from_model_bones(bones);
  REQUIRE(skel_res.has_value());
  auto& skel = *skel_res;

  const i32 ik_idx = skel.find_bone_index("Foot_IK");
  REQUIRE(ik_idx >= 0);

  skel.set_bone_translation(static_cast<u32>(ik_idx), ran::Vec3f32{0.0f, 2.0f, 2.0f});
  skel.update_pose();

  const auto& ankle_node = skel.bone(3);
  const auto& ik_node = skel.bone(static_cast<u32>(ik_idx));

  const f32 dist = ran::length(ankle_node.world_position - ik_node.world_position);
  REQUIRE(dist < 0.1f);
}

TEST_CASE("CCD-IK handles unreachable target gracefully without NaN", "[ik]") {
  Vec<ModelBone> bones;

  ModelBone hip{};
  hip.name_en = "Hip";
  hip.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  hip.parent_index = -1;
  hip.connection_bone_index = 1;
  hip.ik_target_index = -1;
  bones.push_back(hip);

  ModelBone thigh{};
  thigh.name_en = "Thigh";
  thigh.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  thigh.parent_index = 0;
  thigh.connection_bone_index = 2;
  thigh.ik_target_index = -1;
  bones.push_back(thigh);

  ModelBone knee{};
  knee.name_en = "Knee";
  knee.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  knee.parent_index = 1;
  knee.connection_bone_index = 3;
  knee.ik_target_index = -1;
  bones.push_back(knee);

  ModelBone ankle{};
  ankle.name_en = "Ankle";
  ankle.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  ankle.parent_index = 2;
  ankle.connection_bone_index = -1;
  ankle.ik_target_index = -1;
  bones.push_back(ankle);

  ModelBone foot_ik{};
  foot_ik.name_en = "Foot_IK";
  foot_ik.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  foot_ik.parent_index = -1;
  foot_ik.connection_bone_index = -1;
  foot_ik.ik_target_index = 3;
  foot_ik.ik_loop_count = 30;
  foot_ik.ik_limit_angle = ran::pi<f32>;

  ModelIkLink knee_link{};
  knee_link.bone_index = 2;
  knee_link.has_limits = false;
  foot_ik.ik_links.push_back(knee_link);

  ModelIkLink thigh_link{};
  thigh_link.bone_index = 1;
  thigh_link.has_limits = false;
  foot_ik.ik_links.push_back(thigh_link);

  bones.push_back(foot_ik);

  auto skel_res = Skeleton::from_model_bones(bones);
  REQUIRE(skel_res.has_value());
  auto& skel = *skel_res;

  const i32 ik_idx = skel.find_bone_index("Foot_IK");
  REQUIRE(ik_idx >= 0);

  skel.set_bone_translation(static_cast<u32>(ik_idx), ran::Vec3f32{0.0f, -30.0f, 0.0f});
  skel.update_pose();

  const auto& ankle_node = skel.bone(3);
  REQUIRE_FALSE(std::isnan(ankle_node.world_position.x));
  REQUIRE_FALSE(std::isnan(ankle_node.world_position.y));
  REQUIRE_FALSE(std::isnan(ankle_node.world_position.z));
  REQUIRE_FALSE(std::isinf(ankle_node.world_position.y));
}

TEST_CASE("CCD-IK knee angle limits enforce 1-DOF constraint", "[ik]") {
  Vec<ModelBone> bones;

  ModelBone hip{};
  hip.name_en = "Hip";
  hip.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  hip.parent_index = -1;
  hip.connection_bone_index = 1;
  hip.ik_target_index = -1;
  bones.push_back(hip);

  ModelBone thigh{};
  thigh.name_en = "Thigh";
  thigh.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  thigh.parent_index = 0;
  thigh.connection_bone_index = 2;
  thigh.ik_target_index = -1;
  bones.push_back(thigh);

  ModelBone knee{};
  knee.name_en = "Knee";
  knee.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  knee.parent_index = 1;
  knee.connection_bone_index = 3;
  knee.ik_target_index = -1;
  bones.push_back(knee);

  ModelBone ankle{};
  ankle.name_en = "Ankle";
  ankle.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  ankle.parent_index = 2;
  ankle.connection_bone_index = -1;
  ankle.ik_target_index = -1;
  bones.push_back(ankle);

  ModelBone foot_ik{};
  foot_ik.name_en = "Foot_IK";
  foot_ik.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  foot_ik.parent_index = -1;
  foot_ik.connection_bone_index = -1;
  foot_ik.ik_target_index = 3;
  foot_ik.ik_loop_count = 50;
  foot_ik.ik_limit_angle = ran::pi<f32>;

  ModelIkLink knee_link{};
  knee_link.bone_index = 2;
  knee_link.has_limits = true;
  knee_link.min_angle = ran::Vec3f32{-ran::pi<f32>, 0.0f, 0.0f};
  knee_link.max_angle = ran::Vec3f32{-0.0087f, 0.0f, 0.0f};
  foot_ik.ik_links.push_back(knee_link);

  ModelIkLink thigh_link{};
  thigh_link.bone_index = 1;
  thigh_link.has_limits = false;
  foot_ik.ik_links.push_back(thigh_link);

  bones.push_back(foot_ik);

  auto skel_res = Skeleton::from_model_bones(bones);
  REQUIRE(skel_res.has_value());
  auto& skel = *skel_res;

  const i32 ik_idx = skel.find_bone_index("Foot_IK");
  REQUIRE(ik_idx >= 0);

  skel.set_bone_translation(static_cast<u32>(ik_idx), ran::Vec3f32{0.0f, 2.0f, -1.0f});
  skel.update_pose();

  const auto& knee_node = skel.bone(2);
  const f32 knee_pitch = ran::pitch(knee_node.local_rotation);
  REQUIRE(knee_pitch >= 0.008f);
  REQUIRE(knee_pitch <= 3.15f);
  REQUIRE(knee_node.world_position.z > 0.0f);
  REQUIRE_THAT(knee_node.local_rotation.y, WithinAbs(0.0f, 0.001f));
  REQUIRE_THAT(knee_node.local_rotation.z, WithinAbs(0.0f, 0.001f));
}

TEST_CASE("CCD-IK runtime toggle on/off", "[ik]") {
  Vec<ModelBone> bones;

  ModelBone hip{};
  hip.name_en = "Hip";
  hip.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  hip.parent_index = -1;
  hip.connection_bone_index = 1;
  hip.ik_target_index = -1;
  bones.push_back(hip);

  ModelBone thigh{};
  thigh.name_en = "Thigh";
  thigh.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  thigh.parent_index = 0;
  thigh.connection_bone_index = 2;
  thigh.ik_target_index = -1;
  bones.push_back(thigh);

  ModelBone knee{};
  knee.name_en = "Knee";
  knee.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  knee.parent_index = 1;
  knee.connection_bone_index = 3;
  knee.ik_target_index = -1;
  bones.push_back(knee);

  ModelBone ankle{};
  ankle.name_en = "Ankle";
  ankle.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  ankle.parent_index = 2;
  ankle.connection_bone_index = -1;
  ankle.ik_target_index = -1;
  bones.push_back(ankle);

  ModelBone foot_ik{};
  foot_ik.name_en = "Foot_IK";
  foot_ik.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  foot_ik.parent_index = -1;
  foot_ik.connection_bone_index = -1;
  foot_ik.ik_target_index = 3;
  foot_ik.ik_loop_count = 40;
  foot_ik.ik_limit_angle = ran::pi<f32>;

  ModelIkLink knee_link{};
  knee_link.bone_index = 2;
  knee_link.has_limits = false;
  foot_ik.ik_links.push_back(knee_link);

  ModelIkLink thigh_link{};
  thigh_link.bone_index = 1;
  thigh_link.has_limits = false;
  foot_ik.ik_links.push_back(thigh_link);

  bones.push_back(foot_ik);

  auto skel_res = Skeleton::from_model_bones(bones);
  REQUIRE(skel_res.has_value());
  auto& skel = *skel_res;

  const i32 ik_idx = skel.find_bone_index("Foot_IK");
  REQUIRE(ik_idx >= 0);

  skel.set_bone_translation(static_cast<u32>(ik_idx), ran::Vec3f32{0.0f, 3.0f, 2.0f});

  skel.set_ik_enabled(false);
  skel.update_pose();

  const auto& knee_node_off = skel.bone(2);
  REQUIRE_THAT(knee_node_off.local_rotation.w, WithinAbs(1.0f, 0.0001f));

  skel.set_ik_enabled(true);
  skel.update_pose();

  const auto& knee_node_on = skel.bone(2);
  REQUIRE_FALSE(std::abs(knee_node_on.local_rotation.w - 1.0f) < 0.0001f);
}

TEST_CASE("CCD-IK real PMD model leg IK solving", "[ik]") {
  const std::string_view model_path = MIKUGL_TEST_MODEL_PATH;
  REQUIRE_FALSE(model_path.empty());
  REQUIRE(std::filesystem::exists(model_path));

  const auto model_res = ModelLoader::load_file(model_path);
  REQUIRE(model_res.has_value());
  const auto& model = *model_res;

  auto skel_res = Skeleton::from_model_bones(model.bones);
  REQUIRE(skel_res.has_value());
  auto& skel = *skel_res;

  REQUIRE(skel.ik_chains().size() > 0);

  const i32 left_foot_ik = skel.find_bone_index("左足ＩＫ");
  REQUIRE(left_foot_ik >= 0);

  const i32 left_ankle = skel.find_bone_index("左足首");
  REQUIRE(left_ankle >= 0);

  skel.reset_pose();
  skel.set_bone_translation(static_cast<u32>(left_foot_ik), ran::Vec3f32{0.0f, 2.0f, 0.0f});
  skel.update_pose();

  const auto& knee_bone = skel.bone(static_cast<u32>(skel.find_bone_index("左ひざ")));
  const f32 knee_pitch = ran::pitch(knee_bone.local_rotation);
  REQUIRE(knee_pitch >= 0.008f);
  REQUIRE(knee_bone.world_position.z >= 0.05f);

  skel.set_bone_translation(static_cast<u32>(left_foot_ik), ran::Vec3f32{0.0f, 1.5f, 0.0f});
  skel.update_pose();

  const auto& ik_bone = skel.bone(static_cast<u32>(left_foot_ik));
  const auto& ankle_bone = skel.bone(static_cast<u32>(left_ankle));

  const f32 dist = ran::length(ankle_bone.world_position - ik_bone.world_position);
  REQUIRE(dist < 0.35f);
}
