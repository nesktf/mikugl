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
  const std::string candidate_paths[] = {
    "models/default/Miku_Hatsune_Ver2.pmd",
    "lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd",
    "../lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd",
  };

  std::string valid_path;
  for (const auto& path : candidate_paths) {
    if (std::filesystem::exists(path)) {
      valid_path = path;
      break;
    }
  }

  REQUIRE_FALSE(valid_path.empty());

  const auto result = ModelLoader::load_file(valid_path);
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
}

TEST_CASE("AsyncLoader asynchronous polling", "[async_loader]") {
  const std::string candidate_paths[] = {
    "models/default/Miku_Hatsune_Ver2.pmd",
    "lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd",
    "../lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd",
  };

  std::string valid_path;
  for (const auto& path : candidate_paths) {
    if (std::filesystem::exists(path)) {
      valid_path = path;
      break;
    }
  }

  REQUIRE_FALSE(valid_path.empty());

  AsyncLoader loader;
  loader.request_load(valid_path);

  Optional<ModelExpect<ModelData>> result = std::nullopt;
  for (int i = 0; i < 50; ++i) {
    result = loader.poll_result();
    if (result.has_value()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  REQUIRE(result.has_value());
  REQUIRE(result->has_value());
  REQUIRE_FALSE(result->value().vertices.empty());
}
