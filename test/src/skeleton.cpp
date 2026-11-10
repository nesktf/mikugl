#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "animation/skeleton.hpp"

using namespace mikugl;

TEST_CASE("Skeleton rest pose creates identity skinning matrices", "[skeleton]") {
  ModelBone b0{};
  b0.name_en = "Root";
  b0.name_jp = "全ての親";
  b0.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  b0.parent_index = -1;

  ModelBone b1{};
  b1.name_en = "Center";
  b1.name_jp = "センター";
  b1.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  b1.parent_index = 0;

  ModelBone b2{};
  b2.name_en = "Head";
  b2.name_jp = "頭";
  b2.position = ran::Vec3f32{0.0f, 20.0f, 0.0f};
  b2.parent_index = 1;

  std::array<ModelBone, 3> bones = {b0, b1, b2};
  Skeleton skel = Skeleton::from_model_bones(bones).value();

  REQUIRE(skel.bone_count() == 3);
  REQUIRE(skel.find_bone_index("Center") == 1);
  REQUIRE(skel.find_bone_index("センター") == 1);
  REQUIRE(skel.find_bone_index("頭") == 2);
  REQUIRE(skel.find_bone_index("Nonexistent") == -1);

  const auto skin_mats = skel.skinning_matrices();
  for (u32 i = 0; i < 3; ++i) {
    const auto& m = skin_mats[i];
    const ran::Mat4f32 id = ran::Mat4f32::identity();
    for (size_t k = 0; k < 16; ++k) {
      REQUIRE_THAT(m[k], Catch::Matchers::WithinAbs(id[k], 0.0001f));
    }
  }
}

TEST_CASE("Skeleton FK translation propagation", "[skeleton]") {
  ModelBone b0{};
  b0.name_en = "Root";
  b0.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  b0.parent_index = -1;

  ModelBone b1{};
  b1.name_en = "Spine";
  b1.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  b1.parent_index = 0;

  std::array<ModelBone, 2> bones = {b0, b1};
  Skeleton skel = Skeleton::from_model_bones(bones).value();

  skel.set_bone_translation(0, ran::Vec3f32{2.0f, 3.0f, 4.0f});
  skel.update_fk();

  const auto& node0 = skel.bone(0);
  const auto& node1 = skel.bone(1);

  REQUIRE_THAT(node0.world_position.x, Catch::Matchers::WithinAbs(2.0f, 0.0001f));
  REQUIRE_THAT(node0.world_position.y, Catch::Matchers::WithinAbs(3.0f, 0.0001f));
  REQUIRE_THAT(node0.world_position.z, Catch::Matchers::WithinAbs(4.0f, 0.0001f));

  REQUIRE_THAT(node1.world_position.x, Catch::Matchers::WithinAbs(2.0f, 0.0001f));
  REQUIRE_THAT(node1.world_position.y, Catch::Matchers::WithinAbs(8.0f, 0.0001f));
  REQUIRE_THAT(node1.world_position.z, Catch::Matchers::WithinAbs(4.0f, 0.0001f));

  // vertex at spine rest position transformed by skinning matrix should move to world position
  const ran::Vec4f32 rest_vert{0.0f, 5.0f, 0.0f, 1.0f};
  const ran::Vec4f32 skinned_vert = node1.skinning_matrix * rest_vert;
  REQUIRE_THAT(skinned_vert.x, Catch::Matchers::WithinAbs(2.0f, 0.0001f));
  REQUIRE_THAT(skinned_vert.y, Catch::Matchers::WithinAbs(8.0f, 0.0001f));
  REQUIRE_THAT(skinned_vert.z, Catch::Matchers::WithinAbs(4.0f, 0.0001f));
}

TEST_CASE("Skeleton FK rotation around joint", "[skeleton]") {
  ModelBone b0{};
  b0.name_en = "Shoulder";
  b0.position = ran::Vec3f32{0.0f, 10.0f, 0.0f};
  b0.parent_index = -1;

  ModelBone b1{};
  b1.name_en = "Arm";
  b1.position = ran::Vec3f32{0.0f, 15.0f, 0.0f};
  b1.parent_index = 0;

  std::array<ModelBone, 2> bones = {b0, b1};
  Skeleton skel = Skeleton::from_model_bones(bones).value();

  // rotate shoulder 90 deg around Z axis
  const f32 angle = 3.1415926535f * 0.5f;
  const ran::Quatf32 rot_z = ran::to_quat(angle, ran::Vec3f32{0.0f, 0.0f, 1.0f});
  skel.set_bone_rotation(0, rot_z);
  skel.update_fk();

  const auto& arm = skel.bone(1);

  // relative off is (0, 5, 0), rotated 90 deg around Z (-5, 0, 0).
  // world pos = shoulder (0, 10, 0) + (-5, 0, 0) = (-5, 10, 0)
  REQUIRE_THAT(arm.world_position.x, Catch::Matchers::WithinAbs(-5.0f, 0.001f));
  REQUIRE_THAT(arm.world_position.y, Catch::Matchers::WithinAbs(10.0f, 0.001f));
  REQUIRE_THAT(arm.world_position.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));

  // reset restores rest position
  skel.reset_pose();
  const auto& arm_reset = skel.bone(1);
  REQUIRE_THAT(arm_reset.world_position.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
  REQUIRE_THAT(arm_reset.world_position.y, Catch::Matchers::WithinAbs(15.0f, 0.001f));
  REQUIRE_THAT(arm_reset.world_position.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));
}

TEST_CASE("Skeleton root armatures and hierarchical DFS ordering", "[skeleton]") {
  std::vector<ModelBone> input(13);
  input[0].name_en = "root";
  input[0].parent_index = -1;
  input[0].position = ran::Vec3f32{0.0f, 0.0f, 0.0f};

  input[1].name_en = "random_bone";
  input[1].parent_index = -1;
  input[1].position = ran::Vec3f32{10.0f, 0.0f, 0.0f};

  input[2].name_en = "leg_l_ik";
  input[2].parent_index = -1;
  input[2].position = ran::Vec3f32{-1.0f, 0.0f, 0.0f};

  input[3].name_en = "leg_r_ik";
  input[3].parent_index = -1;
  input[3].position = ran::Vec3f32{1.0f, 0.0f, 0.0f};

  input[4].name_en = "spine";
  input[4].parent_index = 11;
  input[4].position = ran::Vec3f32{0.0f, 6.0f, 0.0f};

  input[5].name_en = "foot_r_ik";
  input[5].parent_index = 3;
  input[5].position = ran::Vec3f32{1.0f, 0.5f, 0.0f};

  input[6].name_en = "head";
  input[6].parent_index = 4;
  input[6].position = ran::Vec3f32{0.0f, 10.0f, 0.0f};

  input[7].name_en = "foot_l_ik";
  input[7].parent_index = 2;
  input[7].position = ran::Vec3f32{-1.0f, 0.5f, 0.0f};

  input[8].name_en = "arm_l";
  input[8].parent_index = 4;
  input[8].position = ran::Vec3f32{-3.0f, 7.0f, 0.0f};

  input[9].name_en = "arm_r";
  input[9].parent_index = 4;
  input[9].position = ran::Vec3f32{3.0f, 7.0f, 0.0f};

  input[10].name_en = "leg_l";
  input[10].parent_index = 11;
  input[10].position = ran::Vec3f32{-1.0f, 2.0f, 0.0f};

  input[11].name_en = "hips";
  input[11].parent_index = 0;
  input[11].position = ran::Vec3f32{0.0f, 3.0f, 0.0f};

  input[12].name_en = "leg_r";
  input[12].parent_index = 11;
  input[12].position = ran::Vec3f32{1.0f, 2.0f, 0.0f};

  Skeleton skel = Skeleton::from_model_bones(input).value();
  REQUIRE(skel.bone_count() == 13);

  // check armatures
  const auto armatures = skel.armatures();
  REQUIRE(armatures.size() == 4);
  REQUIRE(armatures[0].start_index == 0);
  REQUIRE(armatures[0].count == 8);
  REQUIRE(armatures[1].start_index == 8);
  REQUIRE(armatures[1].count == 1);
  REQUIRE(armatures[2].start_index == 9);
  REQUIRE(armatures[2].count == 2);
  REQUIRE(armatures[3].start_index == 11);
  REQUIRE(armatures[3].count == 2);

  // verify bone indices and names in sorted order
  const auto sorted_bones = skel.bones();
  REQUIRE(sorted_bones[0].name_en == "root");
  REQUIRE(sorted_bones[0].parent_index == -1);

  REQUIRE(sorted_bones[1].name_en == "hips");
  REQUIRE(sorted_bones[1].parent_index == 0);

  REQUIRE(sorted_bones[2].name_en == "spine");
  REQUIRE(sorted_bones[2].parent_index == 1);

  REQUIRE(sorted_bones[3].name_en == "head");
  REQUIRE(sorted_bones[3].parent_index == 2);

  REQUIRE(sorted_bones[4].name_en == "arm_l");
  REQUIRE(sorted_bones[4].parent_index == 2);

  REQUIRE(sorted_bones[5].name_en == "arm_r");
  REQUIRE(sorted_bones[5].parent_index == 2);

  REQUIRE(sorted_bones[6].name_en == "leg_l");
  REQUIRE(sorted_bones[6].parent_index == 1);

  REQUIRE(sorted_bones[7].name_en == "leg_r");
  REQUIRE(sorted_bones[7].parent_index == 1);

  REQUIRE(sorted_bones[8].name_en == "random_bone");
  REQUIRE(sorted_bones[8].parent_index == -1);

  REQUIRE(sorted_bones[9].name_en == "leg_l_ik");
  REQUIRE(sorted_bones[9].parent_index == -1);

  REQUIRE(sorted_bones[10].name_en == "foot_l_ik");
  REQUIRE(sorted_bones[10].parent_index == 9);

  REQUIRE(sorted_bones[11].name_en == "leg_r_ik");
  REQUIRE(sorted_bones[11].parent_index == -1);

  REQUIRE(sorted_bones[12].name_en == "foot_r_ik");
  REQUIRE(sorted_bones[12].parent_index == 11);

  // check if lookup by name returns the original model index
  REQUIRE(skel.find_bone_index("root") == 0);
  REQUIRE(skel.find_bone_index("random_bone") == 1);
  REQUIRE(skel.find_bone_index("leg_l_ik") == 2);
  REQUIRE(skel.find_bone_index("leg_r_ik") == 3);
  REQUIRE(skel.find_bone_index("spine") == 4);
  REQUIRE(skel.find_bone_index("foot_r_ik") == 5);
  REQUIRE(skel.find_bone_index("head") == 6);
  REQUIRE(skel.find_bone_index("foot_l_ik") == 7);
  REQUIRE(skel.find_bone_index("arm_l") == 8);
  REQUIRE(skel.find_bone_index("arm_r") == 9);
  REQUIRE(skel.find_bone_index("leg_l") == 10);
  REQUIRE(skel.find_bone_index("hips") == 11);
  REQUIRE(skel.find_bone_index("leg_r") == 12);

  skel.set_bone_translation(0, ran::Vec3f32{1.0f, 2.0f, 3.0f}); // translate root armature
  skel.set_bone_translation(2, ran::Vec3f32{0.0f, 5.0f, 0.0f}); // translate leg_l_ik armature
  skel.update_fk();

  // root should be at (1, 2, 3)
  REQUIRE_THAT(skel.bone(0).world_position.x, Catch::Matchers::WithinAbs(1.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(0).world_position.y, Catch::Matchers::WithinAbs(2.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(0).world_position.z, Catch::Matchers::WithinAbs(3.0f, 0.0001f));

  // head rest pos is (0, 10, 0) -> (1, 12, 3)
  REQUIRE_THAT(skel.bone(6).world_position.x, Catch::Matchers::WithinAbs(1.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(6).world_position.y, Catch::Matchers::WithinAbs(12.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(6).world_position.z, Catch::Matchers::WithinAbs(3.0f, 0.0001f));

  // random_bone should be at rest position (10, 0, 0)
  REQUIRE_THAT(skel.bone(1).world_position.x, Catch::Matchers::WithinAbs(10.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(1).world_position.y, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(1).world_position.z, Catch::Matchers::WithinAbs(0.0f, 0.0001f));

  // leg_l_ik rest is (-1, 0, 0) -> (-1, 5, 0)
  REQUIRE_THAT(skel.bone(2).world_position.x, Catch::Matchers::WithinAbs(-1.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(2).world_position.y, Catch::Matchers::WithinAbs(5.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(2).world_position.z, Catch::Matchers::WithinAbs(0.0f, 0.0001f));

  // foot_L_ik rest is (-1, 0.5, 0), offset from leg_l_ik is (0, 0.5, 0).
  // leg_l_ik world is (-1, 5, 0), so foot_l_ik world is (-1, 5.5, 0)
  REQUIRE_THAT(skel.bone(7).world_position.x, Catch::Matchers::WithinAbs(-1.0f, 0.0001f));
  REQUIRE_THAT(skel.bone(7).world_position.y, Catch::Matchers::WithinAbs(5.5f, 0.0001f));
  REQUIRE_THAT(skel.bone(7).world_position.z, Catch::Matchers::WithinAbs(0.0f, 0.0001f));

  // skinning matrix of head at original index transforms rest vertex to world position
  const ran::Vec4f32 rest_head{0.0f, 10.0f, 0.0f, 1.0f};
  const ran::Vec4f32 skinned_head = skel.skinning_matrices()[6] * rest_head;
  REQUIRE_THAT(skinned_head.x, Catch::Matchers::WithinAbs(1.0f, 0.0001f));
  REQUIRE_THAT(skinned_head.y, Catch::Matchers::WithinAbs(12.0f, 0.0001f));
  REQUIRE_THAT(skinned_head.z, Catch::Matchers::WithinAbs(3.0f, 0.0001f));
}

TEST_CASE("Skeleton bone scale and world-space transform helpers", "[skeleton]") {
  ModelBone b0{};
  b0.name_en = "Root";
  b0.position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  b0.parent_index = -1;

  ModelBone b1{};
  b1.name_en = "Child";
  b1.position = ran::Vec3f32{0.0f, 5.0f, 0.0f};
  b1.parent_index = 0;

  std::array<ModelBone, 2> bones = {b0, b1};
  Skeleton skel = Skeleton::from_model_bones(bones).value();

  skel.set_bone_scale(0, ran::Vec3f32{1.0f, 2.0f, 1.0f});
  skel.update_pose();
  REQUIRE_THAT(skel.node(1).world_position.y, Catch::Matchers::WithinAbs(10.0f, 0.001f));

  skel.reset_node_pose(0);
  REQUIRE_THAT(skel.node(1).world_position.y, Catch::Matchers::WithinAbs(5.0f, 0.001f));

  skel.set_node_world_position(1, ran::Vec3f32{3.0f, 8.0f, -2.0f});
  skel.update_pose();
  REQUIRE_THAT(skel.node(1).world_position.x, Catch::Matchers::WithinAbs(3.0f, 0.001f));
  REQUIRE_THAT(skel.node(1).world_position.y, Catch::Matchers::WithinAbs(8.0f, 0.001f));
  REQUIRE_THAT(skel.node(1).world_position.z, Catch::Matchers::WithinAbs(-2.0f, 0.001f));
}
