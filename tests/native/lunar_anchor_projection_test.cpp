#include "../../libraries/render/native/lunar_surface_mesh.hpp"
#include "../../libraries/render/native/scene_mesh_contract.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>

namespace scene = sagan_render::scene;

auto anchor_item(const scene::render_item &moon) -> scene::render_item
{
  auto origin = moon;
  constexpr auto a = scene::lunar_mesh_anchor;
  // Match the live draw's precision-sensitive marker/datum arithmetic.
  origin.position.x_metres += a.x * (moon.radius_metres + 10.0);
  origin.position.y_metres += a.y * (moon.radius_metres + 10.0);
  origin.position.z_metres += a.z * (moon.radius_metres + 10.0);
  origin.position.x_metres -= a.x * 10.0;
  origin.position.y_metres -= a.y * 10.0;
  origin.position.z_metres -= a.z * 10.0;
  origin.position.x_metres -= a.x * 0.25;
  origin.position.y_metres -= a.y * 0.25;
  origin.position.z_metres -= a.z * 0.25;
  return origin;
}

auto mesh(const scene::render_item &origin, const scene::camera &view)
{
  return scene::prepare_oriented_mesh_draw(origin, {1.0, 1.0, 1.0},
    {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, view, {912.0, 424.0});
}

void strict_box(const scene::render_item &origin, const scene::camera &view)
{
  bool rejected = false;
  try {
    (void)scene::prepare_oriented_box_draw(origin, {1.0, 1.0, 1.0},
      {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, view, {912.0, 424.0});
  } catch (const std::invalid_argument &error) {
    rejected = std::string{error.what()} == "Mesh box center must be in front of the camera";
  }
  assert(rejected);
}

void earth_pixels(const std::string &path)
{
  std::ifstream input(path, std::ios::binary);
  assert(input);
  const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>{input}, {}};
  assert(bytes.size() >= 138 && bytes[0] == 'B' && bytes[1] == 'M');
  auto u32 = [&](std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
      (static_cast<std::uint32_t>(bytes[offset+1]) << 8) |
      (static_cast<std::uint32_t>(bytes[offset+2]) << 16) |
      (static_cast<std::uint32_t>(bytes[offset+3]) << 24);
  };
  const auto offset = u32(10);
  const auto width = u32(18);
  const auto raw_height = static_cast<std::int32_t>(u32(22));
  const auto height = raw_height < 0 ? -raw_height : raw_height;
  assert(width == 960 && height == 540 && bytes[28] == 32);
  assert(u32(54) == 0xff0000 && u32(58) == 0xff00 && u32(62) == 0xff);
  assert(bytes.size() >= offset + width * height * 4);
  std::size_t blue_pixels = 0;
  // Only the central Earth disk, excluding HUD text and the orbit-guide lines.
  // Global "Earth" color counts alone accepted a completely black viewport.
  for (int y = 188; y < 388; ++y) for (int x = 380; x < 580; ++x) {
    const auto row = raw_height > 0 ? height - 1 - y : y;
    const auto pixel = offset + (static_cast<std::size_t>(row) * width + x) * 4;
    const int blue = bytes[pixel], green = bytes[pixel+1], red = bytes[pixel+2];
    if (blue > red + 25 && blue > green + 20) ++blue_pixels;
  }
  assert(blue_pixels > 5000);
  std::cout << "Earth disk pixels passed: " << path << " blue=" << blue_pixels << '\n';
}

int main(int argc, char **argv)
{
  constexpr auto a = scene::lunar_mesh_anchor;
  constexpr auto radius = scene::lunar_mesh_radius_metres;
  const scene::render_item moon{3, {0.0, 0.0, 0.0}, radius, "Moon"};
  const auto origin = anchor_item(moon);
  scene::camera view;
  view.forward = scene::scale(a, -1.0);
  view.up = {0.0, 0.0, 1.0};
  view.vertical_field_of_view_radians = 2.8;
  view.near_metres = 1.0;
  view.far_metres = radius * 10.0;
  for (double depth : {-1.0, -0.01, 0.0, 0.01, 1.0}) {
    view.position = {origin.position.x_metres + a.x * depth,
      radius * 2.0, origin.position.z_metres + a.z * depth};
    const auto prepared = mesh(origin, view);
    for (float value : prepared.camera_uniform.model_view_projection) assert(std::isfinite(value));
    for (float value : prepared.camera_uniform.normal_matrix) assert(std::isfinite(value));
    const auto &m = prepared.camera_uniform.model_view_projection;
    assert(std::abs(m[15] - depth) < 0.000001);
    // An actual far-surface vertex has positive clip W even with a rear origin.
    const double vertex_w = m[3] * static_cast<float>(-2.0 * a.x * radius) +
      m[7] * static_cast<float>(-2.0 * a.y * radius) +
      m[11] * static_cast<float>(-2.0 * a.z * radius) + m[15];
    assert(vertex_w > view.near_metres);
    assert(std::abs(vertex_w - (2.0 * radius + depth)) < 1.0);
    if (depth <= 0.0) strict_box(origin, view);
    else {
      const auto box = scene::prepare_oriented_box_draw(origin, {1.0, 1.0, 1.0},
        {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, view, {912.0, 424.0});
      assert(box.camera_uniform.model_view_projection == prepared.camera_uniform.model_view_projection);
      assert(box.camera_uniform.normal_matrix == prepared.camera_uniform.normal_matrix);
    }
  }

  // The live Earth-focus fixture crosses this guard at 4450 ms after wheel zoom.
  constexpr double time = 4.45;
  constexpr double origin_metres = 1e15;
  constexpr double distance = 17024000.0; // 80,000 km * 0.28 * 0.76
  const double phase = time * 0.08 * 13.368746;
  const scene::render_item passing_moon{3,
    {origin_metres + 384400000.0 * std::cos(phase),
     origin_metres + 384400000.0 * std::sin(phase), origin_metres}, radius, "Moon"};
  view.position = {origin_metres + a.x * distance, origin_metres, origin_metres + a.z * distance};
  view.near_metres = 1000000.0;
  view.far_metres = 500000000000.0;
  view.vertical_field_of_view_radians = std::numbers::pi / 3.0;
  const auto sphere = scene::prepare_sphere_draw(passing_moon, view, {912.0, 424.0});
  const double center_depth = scene::dot(sphere.camera_relative_center, view.forward);
  const auto rear_origin = anchor_item(passing_moon);
  const double anchor_depth = scene::dot(scene::subtract(rear_origin.position, view.position), view.forward);
  assert(center_depth > view.near_metres);
  assert(anchor_depth < 0.0);
  strict_box(rear_origin, view);
  const auto lunar = mesh(rear_origin, view);
  assert(lunar.camera_uniform.model_view_projection[15] < 0.0F);
  assert(std::abs(lunar.camera_uniform.model_view_projection[15] - anchor_depth) < 1.0);
  const scene::render_item earth{2, {origin_metres, origin_metres, origin_metres}, 6371000.0, "Earth"};
  auto shallow = view;
  shallow.near_metres = 1.0;
  const auto collapsed = scene::prepare_sphere_draw(earth, shallow, {912.0, 424.0});
  const auto &bad_depth = collapsed.camera_uniform.model_view_projection;
  for (int column=0;column<4;++column) assert(bad_depth[column*4+2] == bad_depth[column*4+3]);
  const auto visible = scene::prepare_sphere_draw(earth, view, {912.0, 424.0});
  const auto &good_depth = visible.camera_uniform.model_view_projection;
  assert(good_depth[14] / good_depth[15] < 1.0F);
  view.position.x_metres = std::numeric_limits<double>::quiet_NaN();
  bool invalid_rejected = false;
  try { (void)mesh(rear_origin, view); }
  catch (const std::invalid_argument &) { invalid_rejected = true; }
  assert(invalid_rejected);
  std::cout << "Lunar anchor projection passed: rear/eye-plane origins, signed finite transforms, "
    "unchanged front transforms, strict box guard and Earth-zoom crossing (Moon center="
    << center_depth << ", anchor=" << anchor_depth << ").\n";
  if (argc == 2) for (const char *time_label : {"4400", "4450", "4500"})
    earth_pixels(std::string{argv[1]} + "/frame-" + time_label + ".bmp");
}
