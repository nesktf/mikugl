#include "animation/morph.hpp"
#include "model/model_loader.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <filesystem>

using namespace mikugl;
using Catch::Matchers::WithinAbs;

TEST_CASE("MorphManager synthetic vertex morph delta accumulation", "[morph]") {
  Vec<ModelVertex> base_vertices(3);
  base_vertices[0].position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  base_vertices[1].position = ran::Vec3f32{1.0f, 0.0f, 0.0f};
  base_vertices[2].position = ran::Vec3f32{0.0f, 1.0f, 0.0f};

  Vec<ModelMorph> morphs;

  ModelMorph blink{};
  blink.name_en = "Blink";
  blink.name_jp = "まばたき";
  blink.category = MorphCategory::Eye;
  blink.type = MorphType::Vertex;
  blink.vertex_offsets.emplace_back(0, ran::Vec3f32{0.0f, -0.5f, 0.1f});
  morphs.push_back(blink);

  ModelMorph smile{};
  smile.name_en = "Smile";
  smile.name_jp = "笑い";
  smile.category = MorphCategory::Mouth;
  smile.type = MorphType::Vertex;
  smile.vertex_offsets.emplace_back(1, ran::Vec3f32{0.2f, 0.3f, 0.0f});
  morphs.push_back(smile);

  MorphManager manager(base_vertices, std::move(morphs));
  REQUIRE(manager.morph_count() == 2);
  REQUIRE(manager.is_dirty());

  REQUIRE(manager.evaluate());
  REQUIRE_FALSE(manager.is_dirty());
  REQUIRE_FALSE(manager.evaluate());

  auto deformed = manager.deformed_vertices();
  REQUIRE_THAT(deformed[0].position.x, WithinAbs(0.0f, 1e-5f));
  REQUIRE_THAT(deformed[0].position.y, WithinAbs(0.0f, 1e-5f));
  REQUIRE_THAT(deformed[0].position.z, WithinAbs(0.0f, 1e-5f));

  // set blink to 1.0
  manager.set_weight(0, 1.0f);
  REQUIRE(manager.is_dirty());
  REQUIRE(manager.evaluate());

  deformed = manager.deformed_vertices();
  REQUIRE_THAT(deformed[0].position.x, WithinAbs(0.0f, 1e-5f));
  REQUIRE_THAT(deformed[0].position.y, WithinAbs(-0.5f, 1e-5f));
  REQUIRE_THAT(deformed[0].position.z, WithinAbs(0.1f, 1e-5f));
  REQUIRE_THAT(deformed[1].position.x, WithinAbs(1.0f, 1e-5f));

  // set blink to 0.5, smile to 0.5
  manager.set_weight(0, 0.5f);
  manager.set_weight(1, 0.5f);
  REQUIRE(manager.evaluate());

  deformed = manager.deformed_vertices();
  REQUIRE_THAT(deformed[0].position.y, WithinAbs(-0.25f, 1e-5f));
  REQUIRE_THAT(deformed[0].position.z, WithinAbs(0.05f, 1e-5f));
  REQUIRE_THAT(deformed[1].position.x, WithinAbs(1.1f, 1e-5f));
  REQUIRE_THAT(deformed[1].position.y, WithinAbs(0.15f, 1e-5f));
}

TEST_CASE("MorphManager group morph expansion", "[morph]") {
  Vec<ModelVertex> base_vertices(2);
  base_vertices[0].position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  base_vertices[1].position = ran::Vec3f32{2.0f, 0.0f, 0.0f};

  Vec<ModelMorph> morphs;

  ModelMorph m0{};
  m0.name_en = "Target0";
  m0.type = MorphType::Vertex;
  m0.category = MorphCategory::Eye;
  m0.vertex_offsets.emplace_back(0, ran::Vec3f32{0.0f, 1.0f, 0.0f});
  morphs.push_back(m0);

  ModelMorph m1{};
  m1.name_en = "Target1";
  m1.type = MorphType::Vertex;
  m1.category = MorphCategory::Mouth;
  m1.vertex_offsets.emplace_back(1, ran::Vec3f32{0.0f, 0.0f, 2.0f});
  morphs.push_back(m1);

  ModelMorph group{};
  group.name_en = "GroupCombo";
  group.type = MorphType::Group;
  group.category = MorphCategory::Other;
  group.group_offsets.emplace_back(0, 1.0f);
  group.group_offsets.emplace_back(1, 0.5f);
  morphs.push_back(group);

  MorphManager manager(base_vertices, std::move(morphs));
  REQUIRE(manager.morph_count() == 3);

  manager.set_weight(2, 1.0f);
  REQUIRE(manager.evaluate());

  const auto deformed = manager.deformed_vertices();
  REQUIRE_THAT(deformed[0].position.y, WithinAbs(1.0f, 1e-5f));
  REQUIRE_THAT(deformed[1].position.z, WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("MorphManager category reset and find_morph_index", "[morph]") {
  Vec<ModelVertex> base_vertices(2);
  Vec<ModelMorph> morphs;

  ModelMorph brow{};
  brow.name_en = "BrowUp";
  brow.name_jp = "眉上";
  brow.category = MorphCategory::Eyebrow;
  brow.type = MorphType::Vertex;
  morphs.push_back(brow);

  ModelMorph eye{};
  eye.name_en = "EyeWink";
  eye.name_jp = "ウィンク";
  eye.category = MorphCategory::Eye;
  eye.type = MorphType::Vertex;
  morphs.push_back(eye);

  MorphManager manager(base_vertices, std::move(morphs));

  REQUIRE(manager.find_morph_index("BrowUp") == 0);
  REQUIRE(manager.find_morph_index("眉上") == 0);
  REQUIRE(manager.find_morph_index("ウィンク") == 1);
  REQUIRE(manager.find_morph_index("NonExistent") == -1);

  manager.set_weight(0, 0.8f);
  manager.set_weight(1, 0.6f);
  REQUIRE(manager.weight(0) == 0.8f);
  REQUIRE(manager.weight(1) == 0.6f);

  manager.reset_category(MorphCategory::Eyebrow);
  REQUIRE(manager.weight(0) == 0.0f);
  REQUIRE(manager.weight(1) == 0.6f);

  manager.reset_all();
  REQUIRE(manager.weight(0) == 0.0f);
  REQUIRE(manager.weight(1) == 0.0f);
}

TEST_CASE("MorphManager real PMD model morph loading and evaluation", "[morph]") {
  const std::string model_path = MIKUGL_TEST_MODEL_PATH;
  REQUIRE(std::filesystem::exists(model_path));

  const auto model_res = ModelLoader::load_file(model_path);
  REQUIRE(model_res.has_value());
  const auto& model = *model_res;

  REQUIRE(model.morphs.size() > 0);

  bool has_vertex_morph = false;
  for (const auto& m : model.morphs) {
    if (m.type == MorphType::Vertex && !m.vertex_offsets.empty()) {
      has_vertex_morph = true;
      break;
    }
  }
  REQUIRE(has_vertex_morph);

  MorphManager manager(model.vertices, model.morphs);
  REQUIRE(manager.morph_count() == model.morphs.size());

  // find a known PMD morph or test first non-empty vertex morph
  i32 test_morph_idx = manager.find_morph_index("まばたき");
  if (test_morph_idx < 0) {
    for (size_t i = 0; i < model.morphs.size(); ++i) {
      if (model.morphs[i].type == MorphType::Vertex && !model.morphs[i].vertex_offsets.empty()) {
        test_morph_idx = static_cast<i32>(i);
        break;
      }
    }
  }
  REQUIRE(test_morph_idx >= 0);

  manager.set_weight(static_cast<u32>(test_morph_idx), 1.0f);
  REQUIRE(manager.evaluate());

  const auto deformed = manager.deformed_vertices();
  REQUIRE(deformed.size() == model.vertices.size());

  const auto& target_morph = model.morphs[static_cast<size_t>(test_morph_idx)];
  REQUIRE_FALSE(target_morph.vertex_offsets.empty());
  const u32 sample_vidx = target_morph.vertex_offsets[0].vertex_index;
  REQUIRE(sample_vidx < deformed.size());

  const auto diff = deformed[sample_vidx].position - model.vertices[sample_vidx].position;
  const f32 expected_len = ran::length(target_morph.vertex_offsets[0].position_offset);
  const f32 actual_len = ran::length(diff);
  REQUIRE_THAT(actual_len, WithinAbs(expected_len, 0.001f));

  REQUIRE_FALSE(std::isnan(deformed[sample_vidx].position.x));
  REQUIRE_FALSE(std::isnan(deformed[sample_vidx].position.y));
  REQUIRE_FALSE(std::isnan(deformed[sample_vidx].position.z));
}
