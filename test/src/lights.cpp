#include <catch2/catch_test_macros.hpp>

#include "scene/light.hpp"

using namespace mikugl;

TEST_CASE("LightManager manages directional and point lights", "[lights]") {
  LightManager lights{};
  REQUIRE(lights.directional_light().enabled);
  REQUIRE(lights.point_light_count() == 0);

  const u32 idx0 = lights.add_default_point_light();
  REQUIRE(idx0 == 0);
  REQUIRE(lights.point_light_count() == 1);
  REQUIRE(lights.point_lights()[0].name == "Point Light 1");

  LightManager::PointLight custom{};
  custom.name = "Custom Rim Light";
  custom.position = ran::Vec3f32{5.0f, 10.0f, -5.0f};
  custom.radius = 40.0f;
  custom.intensity = 2.5f;
  const u32 idx1 = lights.add_point_light(custom);
  REQUIRE(idx1 == 1);
  REQUIRE(lights.point_light_count() == 2);
  REQUIRE(lights.point_lights()[1].name == "Custom Rim Light");

  lights.remove_point_light(0);
  REQUIRE(lights.point_light_count() == 1);
  REQUIRE(lights.point_lights()[0].name == "Custom Rim Light");
}
