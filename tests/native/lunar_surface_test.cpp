#include "../../libraries/render/native/lunar_surface_mesh.hpp"
#include "../../libraries/render/native/lunar_detail_contract.hpp"

#include <cassert>
#include <iostream>
#include <map>

int main()
{
  using namespace sagan_render::scene;
  const auto mesh = build_lunar_surface_mesh();
  assert(mesh.indices.size() == 6 * lunar_face_index_count);
  assert(mesh.vertices.size() > 65536); // The GPU must use 32-bit lunar indices.
  std::map<std::array<float, 3>, std::uint32_t> unique;
  std::vector<std::uint32_t> vertex_ids;
  bool north{}, south{}, local{};
  for (const auto &v : mesh.vertices)
  {
    const double x = v.position[0] + lunar_mesh_anchor.x * lunar_mesh_radius_metres;
    const double y = v.position[1] + lunar_mesh_anchor.y * lunar_mesh_radius_metres;
    const double z = v.position[2] + lunar_mesh_anchor.z * lunar_mesh_radius_metres;
    assert(std::abs(std::sqrt(x * x + y * y + z * z) - lunar_mesh_radius_metres) < 0.5);
    const auto [entry, inserted] = unique.emplace(
      std::array{v.normal[0], v.normal[1], v.normal[2]}, static_cast<std::uint32_t>(unique.size()));
    (void)inserted;
    vertex_ids.push_back(entry->second);
    if (v.normal[2] > 0.9999F) { north = true; assert(v.uv[1] < 0.01F); }
    if (v.normal[2] < -0.9999F) { south = true; assert(v.uv[1] > 0.99F); }
    const double distance = std::sqrt(v.position[0] * v.position[0] +
      v.position[1] * v.position[1] + v.position[2] * v.position[2]);
    if (distance < 100.0)
    {
      local = true;
      const double height = v.position[0] * lunar_mesh_anchor.x +
                            v.position[2] * lunar_mesh_anchor.z;
      assert(height <= 0.00001 && height > -0.004);
    }
  }
  assert(north && south && local);
  std::map<std::pair<std::uint32_t, std::uint32_t>, unsigned> edges;
  for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    for (unsigned e = 0; e < 3; ++e)
    {
      const auto a = mesh.indices[t + e], b = mesh.indices[t + (e + 1) % 3];
      assert(a < mesh.vertices.size() && b < mesh.vertices.size());
      const auto ids = std::minmax(vertex_ids[a], vertex_ids[b]);
      assert(ids.first != ids.second);
      ++edges[{ids.first, ids.second}];
    }
  for (const auto &[edge, count] : edges) { (void)edge; assert(count == 2); }
  assert(lunar_detail_width_metres == 1638.4);
  assert(lunar_detail_weight(0.0) == 1.0F);
  assert(lunar_detail_weight(20000.0) == 0.0F);
  assert(std::abs(lunar_detail_weight(4999.0) - lunar_detail_weight(5001.0)) < 0.001F);
  assert(lunar_detail_weight(10000.0) > lunar_detail_weight(10001.0));
  std::vector<std::uint8_t> rgba(256 * 256 * 4, 100);
  auto neutral = neutral_lunar_detail(rgba, 256, 256, 169);
  for (std::size_t p = 0; p < 256 * 256; ++p)
  { assert(neutral[p * 4] == 169); assert(neutral[p * 4 + 3] == 255); }
  const auto centre = (128 * 256 + 128) * 4;
  rgba[centre] = rgba[centre + 1] = rgba[centre + 2] = 255;
  neutral = neutral_lunar_detail(rgba, 256, 256, 169);
  assert(neutral[centre] > 169); // Fine detail is retained, not blanked out.
  assert(neutral[0] == 169);     // No broad illuminated tile footprint.
  for (unsigned y = 0; y < 256; ++y)
    for (unsigned x = 0; x < 256; ++x)
    {
      const auto p = (y * 256 + x) * 4;
      rgba[p] = rgba[p + 1] = rgba[p + 2] = static_cast<std::uint8_t>(x);
    }
  neutral = neutral_lunar_detail(rgba, 256, 256, 169);
  for (unsigned x = 50; x < 206; ++x)
    assert(neutral[(128 * 256 + x) * 4] == 169); // Broad illumination ramp is removed.
  std::cout << "Lunar surface passed: closed sphere, 32-bit indices, grounded fine cells, +Z north, continuous detail, neutral observation.\n";
}
