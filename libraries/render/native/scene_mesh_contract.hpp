#pragma once

#include "material_contract.hpp"
#include "scene_contract.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sagan_render::scene
{
  inline auto mesh_center_is_projectable(const length3 relative_center,
                                         const direction3 forward,
                                         const scalar near_metres) -> bool
  {
    if (!std::isfinite(near_metres) || near_metres <= 0.0) return false;
    return dot(relative_center, normalize(forward)) > near_metres;
  }

  struct prepared_sphere_draw
  {
    sagan::render::CameraUniform camera_uniform{};
    length3 camera_relative_center{};
    scalar physical_radius_metres{};
  };

  inline auto prepare_sphere_draw(const render_item &item,
                                  const camera &view,
                                  const viewport output)
    -> prepared_sphere_draw
  {
    if (output.width_logical <= 0.0 || output.height_logical <= 0.0 ||
        !std::isfinite(output.width_logical) ||
        !std::isfinite(output.height_logical))
      throw std::invalid_argument("Sphere viewport must be valid");
    if (item.radius_metres <= 0.0 || !std::isfinite(item.radius_metres))
      throw std::invalid_argument("Mesh sphere radius must be finite and positive");

    const direction3 forward = normalize(view.forward);
    const direction3 right = normalize(cross(forward, view.up));
    const direction3 camera_up = normalize(cross(right, forward));
    const length3 relative = subtract(item.position, view.position);
    const scalar camera_x = dot(relative, right);
    const scalar camera_y = dot(relative, camera_up);
    const scalar camera_z = dot(relative, forward);
    if (camera_z <= 0.0 || !std::isfinite(camera_z))
      throw std::invalid_argument("Mesh sphere center must be in front of the camera");
    if (view.near_metres <= 0.0 || view.far_metres <= view.near_metres ||
        !std::isfinite(view.near_metres) || !std::isfinite(view.far_metres) ||
        view.vertical_field_of_view_radians <= 0.0 ||
        view.vertical_field_of_view_radians >= 3.14159265358979323846 ||
        !std::isfinite(view.vertical_field_of_view_radians))
      throw std::invalid_argument("Mesh camera projection must be valid");

    const scalar tangent_half_field =
      std::tan(view.vertical_field_of_view_radians / 2.0);
    const scalar aspect = output.width_logical / output.height_logical;
    const scalar depth_scale =
      view.far_metres / (view.far_metres - view.near_metres);
    const scalar x_scale = 1.0 / (aspect * tangent_half_field);
    const scalar y_scale = 1.0 / tangent_half_field;

    sagan::render::CameraUniform uniform{};
    auto &matrix = uniform.model_view_projection;

    // HLSL consumes row vectors. Each matrix column therefore describes one
    // clip-space component. Translation is written only after the 64-bit
    // camera-origin subtraction above has preserved nearby differences.
    matrix[0] = static_cast<float>(item.radius_metres * right.x * x_scale);
    matrix[4] = static_cast<float>(item.radius_metres * right.y * x_scale);
    matrix[8] = static_cast<float>(item.radius_metres * right.z * x_scale);
    matrix[12] = static_cast<float>(camera_x * x_scale);

    matrix[1] = static_cast<float>(item.radius_metres * camera_up.x * y_scale);
    matrix[5] = static_cast<float>(item.radius_metres * camera_up.y * y_scale);
    matrix[9] = static_cast<float>(item.radius_metres * camera_up.z * y_scale);
    matrix[13] = static_cast<float>(camera_y * y_scale);

    matrix[2] = static_cast<float>(item.radius_metres * forward.x * depth_scale);
    matrix[6] = static_cast<float>(item.radius_metres * forward.y * depth_scale);
    matrix[10] = static_cast<float>(item.radius_metres * forward.z * depth_scale);
    matrix[14] = static_cast<float>(
      camera_z * depth_scale - view.near_metres * depth_scale);

    matrix[3] = static_cast<float>(item.radius_metres * forward.x);
    matrix[7] = static_cast<float>(item.radius_metres * forward.y);
    matrix[11] = static_cast<float>(item.radius_metres * forward.z);
    matrix[15] = static_cast<float>(camera_z);

    // Sphere normals remain in the shared world/light frame. Camera rotation
    // affects projection, not the renderer-independent lighting direction.
    uniform.normal_matrix[0] = 1.0F;
    uniform.normal_matrix[5] = 1.0F;
    uniform.normal_matrix[10] = 1.0F;
    uniform.normal_matrix[15] = 1.0F;

    return {uniform, relative, item.radius_metres};
  }

  struct prepared_box_draw
  {
    sagan::render::CameraUniform camera_uniform{};
    length3 camera_relative_center{};
    length3 half_extents_metres{};
  };

  inline auto prepare_box_draw(const render_item &item,
                               const length3 half_extents,
                               const scalar yaw_radians,
                               const camera &view,
                               const viewport output)
    -> prepared_box_draw
  {
    if (output.width_logical <= 0.0 || output.height_logical <= 0.0 ||
        !std::isfinite(output.width_logical) ||
        !std::isfinite(output.height_logical))
      throw std::invalid_argument("Box viewport must be valid");
    if (half_extents.x_metres <= 0.0 || half_extents.y_metres <= 0.0 ||
        half_extents.z_metres <= 0.0 || !std::isfinite(half_extents.x_metres) ||
        !std::isfinite(half_extents.y_metres) ||
        !std::isfinite(half_extents.z_metres) ||
        !std::isfinite(yaw_radians))
      throw std::invalid_argument("Box half extents and yaw must be valid");
    if (view.near_metres <= 0.0 || view.far_metres <= view.near_metres ||
        !std::isfinite(view.near_metres) || !std::isfinite(view.far_metres) ||
        view.vertical_field_of_view_radians <= 0.0 ||
        view.vertical_field_of_view_radians >= 3.14159265358979323846 ||
        !std::isfinite(view.vertical_field_of_view_radians))
      throw std::invalid_argument("Mesh camera projection must be valid");

    const direction3 forward = normalize(view.forward);
    const direction3 right = normalize(cross(forward, view.up));
    const direction3 camera_up = normalize(cross(right, forward));
    const length3 relative = subtract(item.position, view.position);
    const scalar camera_x = dot(relative, right);
    const scalar camera_y = dot(relative, camera_up);
    const scalar camera_z = dot(relative, forward);
    if (camera_z <= 0.0 || !std::isfinite(camera_z))
      throw std::invalid_argument("Mesh box center must be in front of the camera");

    const scalar tangent_half_field =
      std::tan(view.vertical_field_of_view_radians / 2.0);
    const scalar aspect = output.width_logical / output.height_logical;
    const scalar depth_scale =
      view.far_metres / (view.far_metres - view.near_metres);
    const scalar x_scale = 1.0 / (aspect * tangent_half_field);
    const scalar y_scale = 1.0 / tangent_half_field;
    const scalar yaw_cosine = std::cos(yaw_radians);
    const scalar yaw_sine = std::sin(yaw_radians);
    const direction3 axes[3] = {
      {yaw_cosine, yaw_sine, 0.0},
      {-yaw_sine, yaw_cosine, 0.0},
      {0.0, 0.0, 1.0}};
    const scalar scales[3] = {
      half_extents.x_metres, half_extents.y_metres, half_extents.z_metres};

    sagan::render::CameraUniform uniform{};
    auto &matrix = uniform.model_view_projection;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
      matrix[axis * 4] = static_cast<float>(
        scales[axis] * dot(axes[axis], right) * x_scale);
      matrix[axis * 4 + 1] = static_cast<float>(
        scales[axis] * dot(axes[axis], camera_up) * y_scale);
      matrix[axis * 4 + 2] = static_cast<float>(
        scales[axis] * dot(axes[axis], forward) * depth_scale);
      matrix[axis * 4 + 3] = static_cast<float>(
        scales[axis] * dot(axes[axis], forward));
    }
    matrix[12] = static_cast<float>(camera_x * x_scale);
    matrix[13] = static_cast<float>(camera_y * y_scale);
    matrix[14] = static_cast<float>(
      camera_z * depth_scale - view.near_metres * depth_scale);
    matrix[15] = static_cast<float>(camera_z);

    auto &normal = uniform.normal_matrix;
    normal[0] = static_cast<float>(axes[0].x);
    normal[1] = static_cast<float>(axes[0].y);
    normal[2] = static_cast<float>(axes[0].z);
    normal[4] = static_cast<float>(axes[1].x);
    normal[5] = static_cast<float>(axes[1].y);
    normal[6] = static_cast<float>(axes[1].z);
    normal[8] = static_cast<float>(axes[2].x);
    normal[9] = static_cast<float>(axes[2].y);
    normal[10] = static_cast<float>(axes[2].z);
    normal[15] = 1.0F;

    return {uniform, relative, half_extents};
  }
}
