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
  std::array<Float4, 4> direction_to_light_and_intensity;
  std::array<Float4, 4> light_color_linear;
  Float4 light_count_and_padding;
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
  Float4 illumination = lighting.ambient_linear;
  const auto light_count = std::min<std::size_t>(
    4, static_cast<std::size_t>(std::max(0.0F, lighting.light_count_and_padding.x)));
  for (std::size_t index = 0; index < light_count; ++index) {
    const auto &direction = lighting.direction_to_light_and_intensity[index];
    const auto &color = lighting.light_color_linear[index];
    const float cosine = std::max(
      0.0F,
      unit_normal.x * direction.x +
      unit_normal.y * direction.y +
      unit_normal.z * direction.z);
    const float diffuse = cosine * direction.w;
    illumination.x += color.x * diffuse;
    illumination.y += color.y * diffuse;
    illumination.z += color.z * diffuse;
  }
  return {
    material.base_color_linear.x *
        illumination.x +
      material.emissive_linear_and_roughness.x,
    material.base_color_linear.y *
        illumination.y +
      material.emissive_linear_and_roughness.y,
    material.base_color_linear.z *
        illumination.z +
      material.emissive_linear_and_roughness.z,
    material.base_color_linear.w,
  };
}

static_assert(sizeof(Float4) == 16);
static_assert(alignof(Float4) == 16);
static_assert(sizeof(CameraUniform) == 128);
static_assert(sizeof(MaterialUniform) == 32);
static_assert(sizeof(LightingUniform) == 160);
static_assert(offsetof(MaterialUniform, emissive_linear_and_roughness) == 16);
static_assert(offsetof(LightingUniform, direction_to_light_and_intensity) == 16);
static_assert(offsetof(LightingUniform, light_color_linear) == 80);
static_assert(offsetof(LightingUniform, light_count_and_padding) == 144);

} // namespace sagan::render
