#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace sagan::render {

struct alignas(16) Float4 {
  float x;
  float y;
  float z;
  float w;
};

struct alignas(16) CameraUniform {
  std::array<float, 16> model_view_projection;
  std::array<float, 16> normal_matrix;
};

struct alignas(16) MaterialUniform {
  Float4 base_color_linear;
  Float4 emissive_linear_and_roughness;
};

struct alignas(16) LightingUniform {
  Float4 ambient_linear;
  Float4 direction_to_light_and_intensity;
  Float4 light_color_linear;
};

constexpr float srgb_channel_to_linear(float value) {
  if (value <= 0.04045F) {
    return value / 12.92F;
  }
  return std::pow((value + 0.055F) / 1.055F, 2.4F);
}

constexpr Float4 srgb_to_linear(Float4 value) {
  return {
    srgb_channel_to_linear(value.x),
    srgb_channel_to_linear(value.y),
    srgb_channel_to_linear(value.z),
    value.w,
  };
}

inline Float4 lambert_reference(
  Float4 unit_normal,
  const MaterialUniform &material,
  const LightingUniform &lighting) {
  const float cosine = std::max(
    0.0F,
    unit_normal.x * lighting.direction_to_light_and_intensity.x +
    unit_normal.y * lighting.direction_to_light_and_intensity.y +
    unit_normal.z * lighting.direction_to_light_and_intensity.z);
  const float diffuse = cosine * lighting.direction_to_light_and_intensity.w;
  return {
    material.base_color_linear.x *
        (lighting.ambient_linear.x + lighting.light_color_linear.x * diffuse) +
      material.emissive_linear_and_roughness.x,
    material.base_color_linear.y *
        (lighting.ambient_linear.y + lighting.light_color_linear.y * diffuse) +
      material.emissive_linear_and_roughness.y,
    material.base_color_linear.z *
        (lighting.ambient_linear.z + lighting.light_color_linear.z * diffuse) +
      material.emissive_linear_and_roughness.z,
    material.base_color_linear.w,
  };
}

static_assert(sizeof(Float4) == 16);
static_assert(alignof(Float4) == 16);
static_assert(sizeof(CameraUniform) == 128);
static_assert(sizeof(MaterialUniform) == 32);
static_assert(sizeof(LightingUniform) == 48);
static_assert(offsetof(MaterialUniform, emissive_linear_and_roughness) == 16);
static_assert(offsetof(LightingUniform, direction_to_light_and_intensity) == 16);
static_assert(offsetof(LightingUniform, light_color_linear) == 32);

} // namespace sagan::render
