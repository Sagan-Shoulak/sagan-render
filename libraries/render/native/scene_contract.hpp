#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sagan_render::scene
{
  using scalar = double;

  struct length3
  {
    scalar x_metres{};
    scalar y_metres{};
    scalar z_metres{};
  };

  struct direction3
  {
    scalar x{};
    scalar y{};
    scalar z{};
  };

  struct render_item
  {
    std::uint64_t identifier{};
    length3 position{};
    scalar radius_metres{};
    std::string label;
  };

  // This is render input, not simulation state. It deliberately has no mass,
  // velocity, force, time-step, ownership pointer, or mutation callback.
  class snapshot
  {
    std::uint64_t revision_value{};
    std::string frame_value;
    scalar sample_time_seconds_value{};
    std::vector<render_item> items_value;

  public:
    snapshot(std::uint64_t revision, std::string frame,
             scalar sample_time_seconds, std::vector<render_item> items)
      : revision_value{revision}, frame_value{std::move(frame)},
        sample_time_seconds_value{sample_time_seconds},
        items_value{std::move(items)}
    {
      if (frame_value.empty()) throw std::invalid_argument("Snapshot frame must be named");
      if (!std::isfinite(sample_time_seconds_value))
        throw std::invalid_argument("Snapshot sample time must be finite");
      std::unordered_set<std::uint64_t> identifiers;
      for (const auto &item : items_value)
      {
        if (item.identifier == 0 || !identifiers.insert(item.identifier).second)
          throw std::invalid_argument("Render item identifiers must be unique and non-zero");
        if (!std::isfinite(item.position.x_metres) ||
            !std::isfinite(item.position.y_metres) ||
            !std::isfinite(item.position.z_metres) ||
            !std::isfinite(item.radius_metres) || item.radius_metres < 0.0)
          throw std::invalid_argument("Render item lengths must be finite and radius non-negative");
      }
    }

    auto revision() const -> std::uint64_t { return revision_value; }
    auto frame() const -> const std::string & { return frame_value; }
    auto sample_time_seconds() const -> scalar { return sample_time_seconds_value; }
    auto items() const -> const std::vector<render_item> & { return items_value; }
  };

  inline auto subtract(const length3 first, const length3 second) -> length3
  {
    return {first.x_metres - second.x_metres,
            first.y_metres - second.y_metres,
            first.z_metres - second.z_metres};
  }

  inline auto dot(const length3 value, const direction3 direction) -> scalar
  {
    return value.x_metres * direction.x + value.y_metres * direction.y +
           value.z_metres * direction.z;
  }

  inline auto dot(const direction3 first, const direction3 second) -> scalar
  {
    return first.x * second.x + first.y * second.y + first.z * second.z;
  }

  inline auto cross(const direction3 first, const direction3 second) -> direction3
  {
    return {first.y * second.z - first.z * second.y,
            first.z * second.x - first.x * second.z,
            first.x * second.y - first.y * second.x};
  }

  inline auto normalize(const direction3 value) -> direction3
  {
    const scalar magnitude = std::sqrt(dot(value, value));
    if (!std::isfinite(magnitude) || magnitude == 0.0)
      throw std::invalid_argument("Camera direction must be finite and non-zero");
    return {value.x / magnitude, value.y / magnitude, value.z / magnitude};
  }

  struct camera
  {
    length3 position{};
    direction3 forward{0.0, 0.0, -1.0};
    direction3 up{0.0, 1.0, 0.0};
    scalar vertical_field_of_view_radians{1.0471975511965976};
    scalar near_metres{1.0};
    scalar far_metres{1.0e16};
  };

  struct viewport
  {
    scalar width_logical{};
    scalar height_logical{};
  };

  struct projected_item
  {
    std::uint64_t identifier{};
    scalar logical_x{};
    scalar logical_y{};
    scalar linear_depth{};
    scalar radius_logical{};
    bool visible{};
    std::string label;
  };

  // Row-major matrices operating on column vectors. The view matrix contains
  // rotation only because positions are rebased around camera.position first.
  using matrix4 = std::array<scalar, 16>;

  class projector
  {
    camera camera_value;
    viewport viewport_value;
    direction3 forward_value;
    direction3 right_value;
    direction3 up_value;
    scalar tangent_half_field{};

  public:
    projector(camera view, const viewport output)
      : camera_value{view}, viewport_value{output},
        forward_value{normalize(view.forward)},
        right_value{normalize(cross(forward_value, view.up))},
        up_value{normalize(cross(right_value, forward_value))},
        tangent_half_field{std::tan(view.vertical_field_of_view_radians / 2.0)}
    {
      if (!std::isfinite(output.width_logical) ||
          !std::isfinite(output.height_logical) || output.width_logical <= 0.0 ||
          output.height_logical <= 0.0)
        throw std::invalid_argument("Viewport dimensions must be finite and positive");
      if (!std::isfinite(view.vertical_field_of_view_radians) ||
          view.vertical_field_of_view_radians <= 0.0 ||
          view.vertical_field_of_view_radians >= 3.14159265358979323846 ||
          !std::isfinite(view.near_metres) || !std::isfinite(view.far_metres) ||
          view.near_metres <= 0.0 || view.far_metres <= view.near_metres)
        throw std::invalid_argument("Camera field of view and depth range are invalid");
    }

    auto project(const render_item &item) const -> projected_item
    {
      // Subtraction happens while values still use 64-bit physical precision.
      // Only camera-relative results should later be narrowed for a GPU buffer.
      const length3 relative = subtract(item.position, camera_value.position);
      const scalar camera_x = dot(relative, right_value);
      const scalar camera_y = dot(relative, up_value);
      const scalar camera_z = dot(relative, forward_value);
      const scalar aspect = viewport_value.width_logical / viewport_value.height_logical;
      const scalar half_height = camera_z * tangent_half_field;
      const scalar half_width = half_height * aspect;
      const bool in_depth = camera_z + item.radius_metres >= camera_value.near_metres &&
                            camera_z - item.radius_metres <= camera_value.far_metres;
      const bool finite_projection = camera_z > 0.0 && half_width > 0.0 && half_height > 0.0;
      scalar normalized_x{};
      scalar normalized_y{};
      scalar radius_x{};
      scalar radius_y{};
      if (finite_projection)
      {
        normalized_x = camera_x / half_width;
        normalized_y = camera_y / half_height;
        radius_x = item.radius_metres / half_width;
        radius_y = item.radius_metres / half_height;
      }
      const bool in_frustum = finite_projection && in_depth &&
        std::abs(normalized_x) <= 1.0 + radius_x &&
        std::abs(normalized_y) <= 1.0 + radius_y;
      return {
        item.identifier,
        (normalized_x + 1.0) * viewport_value.width_logical / 2.0,
        (1.0 - normalized_y) * viewport_value.height_logical / 2.0,
        (camera_z - camera_value.near_metres) /
          (camera_value.far_metres - camera_value.near_metres),
        std::max(radius_x * viewport_value.width_logical / 2.0,
                 radius_y * viewport_value.height_logical / 2.0),
        in_frustum,
        item.label
      };
    }

    auto rebased_view_matrix() const -> matrix4
    {
      return {
        right_value.x, right_value.y, right_value.z, 0.0,
        up_value.x, up_value.y, up_value.z, 0.0,
        forward_value.x, forward_value.y, forward_value.z, 0.0,
        0.0, 0.0, 0.0, 1.0
      };
    }

    // Backend-neutral perspective convention: X and Y are normalized to
    // [-1, 1], forward camera depth maps to [0, 1]. A backend adapter may
    // transpose or reverse depth without changing the public camera contract.
    auto projection_matrix() const -> matrix4
    {
      const scalar aspect = viewport_value.width_logical / viewport_value.height_logical;
      const scalar depth = camera_value.far_metres - camera_value.near_metres;
      return {
        1.0 / (aspect * tangent_half_field), 0.0, 0.0, 0.0,
        0.0, 1.0 / tangent_half_field, 0.0, 0.0,
        0.0, 0.0, camera_value.far_metres / depth,
          -camera_value.near_metres * camera_value.far_metres / depth,
        0.0, 0.0, 1.0, 0.0
      };
    }

    auto prepare(const snapshot &source) const -> std::vector<projected_item>
    {
      std::vector<projected_item> result;
      result.reserve(source.items().size());
      for (const auto &item : source.items()) result.push_back(project(item));
      return result;
    }
  };
}
