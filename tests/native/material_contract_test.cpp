#include "../../libraries/render/native/material_contract.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool near(float left, float right, float tolerance = 0.0001F) {
  return std::fabs(left - right) <= tolerance;
}

} // namespace

int main() {
  using sagan::render::Float4;
  using sagan::render::LightingUniform;
  using sagan::render::MaterialUniform;
  using sagan::render::lambert_reference;
  using sagan::render::srgb_to_linear;

  const Float4 converted = srgb_to_linear({0.0F, 0.5F, 1.0F, 0.75F});
  assert(near(converted.x, 0.0F));
  assert(near(converted.y, 0.214041F));
  assert(near(converted.z, 1.0F));
  assert(near(converted.w, 0.75F));

  const MaterialUniform material{
    .base_color_linear = {0.8F, 0.4F, 0.2F, 1.0F},
    .emissive_linear_and_roughness = {0.01F, 0.02F, 0.03F, 0.6F},
  };
  const LightingUniform lighting{
    .ambient_linear = {0.1F, 0.1F, 0.1F, 0.0F},
    .direction_to_light_and_intensity = {0.0F, 0.0F, 1.0F, 0.75F},
    .light_color_linear = {1.0F, 0.8F, 0.6F, 0.0F},
  };

  const Float4 front = lambert_reference({0.0F, 0.0F, 1.0F, 0.0F}, material, lighting);
  assert(near(front.x, 0.69F));
  assert(near(front.y, 0.30F));
  assert(near(front.z, 0.14F));
  assert(near(front.w, 1.0F));

  const Float4 back = lambert_reference({0.0F, 0.0F, -1.0F, 0.0F}, material, lighting);
  assert(near(back.x, 0.09F));
  assert(near(back.y, 0.06F));
  assert(near(back.z, 0.05F));

  std::cout << "Material contract passed: std140 layout, linear color, "
               "Lambert clamp, ambient, and emission verified.\n";
}
