#include <catch2/catch_test_macros.hpp>

#include "model/async_loader.hpp"
#include "model/model_loader.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

using namespace mikugl;

TEST_CASE("ModelLoader error handling", "[model_loader]") {
  SECTION("loading non-existent file returns FileNotFound") {
    const auto result = ModelLoader::load_file("non_existent_file_path.pmd");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == ModelLoadError::FileNotFound);
  }
}

TEST_CASE("ModelLoader default PMD model loading", "[model_loader]") {
  const std::string_view model_path = MIKUGL_TEST_MODEL_PATH;
  REQUIRE_FALSE(model_path.empty());
  REQUIRE(std::filesystem::exists(model_path));

  const auto result = ModelLoader::load_file(model_path);
  REQUIRE(result.has_value());

  const auto& model = *result;
  REQUIRE_FALSE(model.vertices.empty());
  REQUIRE_FALSE(model.indices.empty());
  REQUIRE_FALSE(model.submeshes.empty());
  REQUIRE_FALSE(model.materials.empty());

  size_t total_submesh_indices = 0;
  for (const auto& submesh : model.submeshes) {
    total_submesh_indices += submesh.index_count;
    REQUIRE(submesh.material_index < model.materials.size());
  }

  REQUIRE(total_submesh_indices == model.indices.size());

  REQUIRE(model.bones.size() == 140);
  REQUIRE(model.bones[40].name_jp == "左足");
  REQUIRE(model.bones[41].name_jp == "左ひざ");
  REQUIRE(model.bones[42].name_jp == "左足首");
  REQUIRE(model.bones[84].name_jp == "左つま先");
  REQUIRE(model.bones[89].name_jp == "左足ＩＫ");
  REQUIRE(model.bones[91].name_jp == "左つま先ＩＫ");
  REQUIRE(model.bones[89].name_en == "leg IK_L");
  REQUIRE(model.bones[91].name_en == "toe IK_L");
}

TEST_CASE("AsyncLoader asynchronous polling", "[async_loader]") {
  const std::string_view model_path = MIKUGL_TEST_MODEL_PATH;
  REQUIRE_FALSE(model_path.empty());
  REQUIRE(std::filesystem::exists(model_path));

  AsyncLoader loader;
  loader.request_load(model_path);

  ModelExpect<ModelData> result{Unexpected(ModelLoadError::NotYetLoaded)};
  for (int i = 0; i < 50; ++i) {
    result = loader.poll_result();
    if (result.has_value()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  REQUIRE(result.has_value());
  REQUIRE_FALSE(result->vertices.empty());
}
