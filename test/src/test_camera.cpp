#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "scene/camera.hpp"

TEST_CASE("MmdCamera initialization and defaults", "[camera]") {
  const mikugl::MmdCamera camera{};

  REQUIRE_THAT(camera.target().x, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
  REQUIRE_THAT(camera.target().y, Catch::Matchers::WithinAbs(5.0f, 0.0001f));
  REQUIRE_THAT(camera.target().z, Catch::Matchers::WithinAbs(0.0f, 0.0001f));

  REQUIRE_THAT(camera.distance(), Catch::Matchers::WithinAbs(20.0f, 0.0001f));
  REQUIRE_THAT(camera.near_z(), Catch::Matchers::WithinAbs(0.5f, 0.0001f));
  REQUIRE_THAT(camera.far_z(), Catch::Matchers::WithinAbs(2000.0f, 0.0001f));
}

TEST_CASE("MmdCamera orbit and clamping", "[camera]") {
  mikugl::MmdCamera camera{};

  SECTION("orbit updates yaw and pitch") {
    camera.set_pitch(0.0f);
    camera.set_yaw(0.0f);

    camera.orbit(10.0f, 5.0f);
    REQUIRE(camera.yaw() > 0.0f);
    REQUIRE(camera.pitch() > 0.0f);
  }

  SECTION("pitch is clamped to avoid gimbal flip") {
    camera.orbit(0.0f, 10000.0f);
    REQUIRE(camera.pitch() <= 1.55f);

    camera.orbit(0.0f, -20000.0f);
    REQUIRE(camera.pitch() >= -1.55f);
  }
}

TEST_CASE("MmdCamera pan and zoom", "[camera]") {
  mikugl::MmdCamera camera{};

  SECTION("pan updates target position") {
    const ran::Vec3f32 initial_target = camera.target();
    camera.pan(20.0f, 10.0f);
    REQUIRE(camera.target().x != initial_target.x);
    REQUIRE(camera.target().y != initial_target.y);
  }

  SECTION("zoom scales distance and clamps to bounds") {
    camera.zoom(2.0f);
    REQUIRE(camera.distance() < 40.0f);

    camera.zoom(100.0f);
    REQUIRE(camera.distance() >= 0.5f);

    camera.zoom(-200.0f);
    REQUIRE(camera.distance() <= 2000.0f);
  }

  SECTION("reset restores default framing") {
    camera.orbit(50.0f, 50.0f);
    camera.pan(10.0f, 20.0f);
    camera.zoom(5.0f);

    camera.reset();
    REQUIRE_THAT(camera.target().x, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(camera.target().y, Catch::Matchers::WithinAbs(5.0f, 0.0001f));
    REQUIRE_THAT(camera.target().z, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(camera.distance(), Catch::Matchers::WithinAbs(20.0f, 0.0001f));
  }
}

TEST_CASE("MmdCamera matrices computation", "[camera]") {
  mikugl::MmdCamera camera{};

  SECTION("position is offset from target by distance") {
    camera.set_pitch(0.0f);
    camera.set_yaw(0.0f);
    camera.set_distance(20.0f);

    const ran::Vec3f32 pos = camera.compute_position();
    REQUIRE_THAT(pos.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
    REQUIRE_THAT(pos.y, Catch::Matchers::WithinAbs(5.0f, 0.001f));
    REQUIRE_THAT(pos.z, Catch::Matchers::WithinAbs(20.0f, 0.001f));
  }

  SECTION("projection matrix is valid perspective") {
    const ran::Mat4f32 proj = camera.compute_projection_matrix(16.0f / 9.0f);
    REQUIRE(proj.x1 != 0.0f);
    REQUIRE(proj.y2 != 0.0f);
    REQUIRE_THAT(proj.w3, Catch::Matchers::WithinAbs(-1.0f, 0.0001f));
  }

  SECTION("view matrix is valid transform") {
    const ran::Mat4f32 view = camera.compute_view_matrix();
    REQUIRE_THAT(view.w4, Catch::Matchers::WithinAbs(1.0f, 0.0001f));
  }
}
