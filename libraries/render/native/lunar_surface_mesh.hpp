#pragma once

#include "scene_contract.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace sagan_render::scene
{
  struct surface_vertex
  {
    float position[3];
    float normal[3];
    float uv[2];
  };

  inline constexpr double lunar_mesh_radius_metres = 1737400.0;
  inline constexpr direction3 lunar_mesh_anchor{
    0.8660254037835535, 0.0, 0.49999999999996425};
  inline constexpr std::uint32_t lunar_face_index_count = 128 * 128 * 6;
  // Provenance: the existing 2048-pixel LROC crop is 0.8 metres per pixel.
  inline constexpr double lunar_detail_width_metres = 2048.0 * 0.8;

  inline auto lunar_detail_weight(const double altitude_metres) -> float
  {
    const double t = std::clamp((altitude_metres - 1000.0) / 19000.0, 0.0, 1.0);
    return static_cast<float>(1.0 - t * t * (3.0 - 2.0 * t));
  }

  struct lunar_surface_mesh
  {
    std::vector<surface_vertex> vertices;
    std::vector<std::uint32_t> indices;
  };

  // One closed sphere, not a cap layered over a second sphere. A cubed sphere
  // with cubic spacing gives metre-scale cells at the fixture anchor and broad
  // cells far away. Every distance uses these same vertices and spherical UVs;
  // there is no distance threshold, finite patch edge, or depth-order handoff.
  inline auto build_lunar_surface_mesh() -> lunar_surface_mesh
  {
    constexpr std::uint32_t segments = 128;
    constexpr double pi = 3.14159265358979323846;
    constexpr direction3 east{0.0, 1.0, 0.0};
    constexpr direction3 north{-lunar_mesh_anchor.z, 0.0, lunar_mesh_anchor.x};
    struct face { direction3 normal; direction3 u; direction3 v; };
    const std::array<face, 6> faces{{
      {lunar_mesh_anchor, east, north},
      {scale(lunar_mesh_anchor, -1.0), scale(east, -1.0), north},
      {east, scale(lunar_mesh_anchor, -1.0), north},
      {scale(east, -1.0), lunar_mesh_anchor, north},
      {north, east, scale(lunar_mesh_anchor, -1.0)},
      {scale(north, -1.0), east, lunar_mesh_anchor}
    }};
    lunar_surface_mesh mesh;
    mesh.vertices.reserve(6 * (segments + 1) * (segments + 1));
    mesh.indices.reserve(6 * segments * segments * 6);
    for (const auto &face_value : faces)
    {
      const auto base = static_cast<std::uint32_t>(mesh.vertices.size());
      for (std::uint32_t row = 0; row <= segments; ++row)
      {
        const double t = 2.0 * row / segments - 1.0;
        for (std::uint32_t column = 0; column <= segments; ++column)
        {
          const double s = 2.0 * column / segments - 1.0;
          const auto normal = normalize(add(face_value.normal,
            add(scale(face_value.u, s * s * s), scale(face_value.v, t * t * t))));
          const double u = std::atan2(normal.y, normal.x) / (2.0 * pi) + 0.5;
          const double v = std::acos(std::clamp(normal.z, -1.0, 1.0)) / pi;
          mesh.vertices.push_back({{
            static_cast<float>((normal.x - lunar_mesh_anchor.x) * lunar_mesh_radius_metres),
            static_cast<float>((normal.y - lunar_mesh_anchor.y) * lunar_mesh_radius_metres),
            static_cast<float>((normal.z - lunar_mesh_anchor.z) * lunar_mesh_radius_metres)},
            {static_cast<float>(normal.x), static_cast<float>(normal.y), static_cast<float>(normal.z)},
            {static_cast<float>(u), static_cast<float>(v)}});
        }
      }
      for (std::uint32_t row = 0; row < segments; ++row)
        for (std::uint32_t column = 0; column < segments; ++column)
        {
          const auto first = base + row * (segments + 1) + column;
          const auto second = first + segments + 1;
          for (auto triangle : {std::array{first, second, first + 1},
                                std::array{first + 1, second, second + 1}})
          {
            // Unwrap each antimeridian triangle before interpolation. Sampler
            // repeat then preserves the global map without a longitude stripe.
            const auto [low, high] = std::minmax({mesh.vertices[triangle[0]].uv[0],
              mesh.vertices[triangle[1]].uv[0], mesh.vertices[triangle[2]].uv[0]});
            if (high - low > 0.5F)
              for (auto &index : triangle)
                if (mesh.vertices[index].uv[0] < 0.5F)
                {
                  auto duplicate = mesh.vertices[index];
                  duplicate.uv[0] += 1.0F;
                  index = static_cast<std::uint32_t>(mesh.vertices.size());
                  mesh.vertices.push_back(duplicate);
                }
            mesh.indices.insert(mesh.indices.end(), triangle.begin(), triangle.end());
          }
        }
    }
    return mesh;
  }
}
