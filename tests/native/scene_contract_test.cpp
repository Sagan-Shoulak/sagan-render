#include "../../libraries/render/native/scene_contract.hpp"
#include "../../libraries/render/native/scene_mesh_contract.hpp"

#include <cassert>
#include <cmath>
#include <fstream>

namespace
{
  auto near(const double first, const double second, const double tolerance = 0.000001) -> bool
  {
    return std::abs(first - second) <= tolerance;
  }
}

int main(int argc, char **argv)
{
  using namespace sagan_render::scene;

  // These are deliberately generic scene objects at difficult coordinates,
  // not celestial bodies and not the result of an orbit calculation.
  constexpr double anchor = 1.0e15;
  const snapshot source{42, "example-right-handed-frame", 1234.5, {
    {1, {anchor, anchor, anchor - 1000000.0}, 50000.0, "center"},
    {2, {anchor + 250000.0, anchor, anchor - 1000000.0}, 25000.0, "right"},
    {3, {anchor, anchor + 200000.0, anchor - 1500000.0}, 40000.0, "high"},
    {4, {anchor + 9000000.0, anchor, anchor - 1000000.0}, 1000.0, "culled"}
  }};
  const camera view{{anchor, anchor, anchor}, {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
                    1.0471975511965976, 100.0, 10000000.0};
  const render_item mesh_earth{
    11, {anchor + 12000.0, anchor - 4000.0, anchor - 2000000.0},
    6371000.0, "mesh-earth"};
  const auto mesh_draw = prepare_sphere_draw(
    mesh_earth, view, {960.0, 540.0});
  assert(mesh_center_is_projectable(
    {0.0, 0.0, -2000000.0}, {0.0, 0.0, -1.0}, 1000.0));
  assert(!mesh_center_is_projectable(
    {0.0, 0.0, 2000000.0}, {0.0, 0.0, -1.0}, 1000.0));
  // Radius is deliberately irrelevant: a huge sphere centered behind the
  // camera is culled before the center-based perspective transform.
  assert(!mesh_center_is_projectable(
    {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0}, 0.5));
  assert(mesh_draw.camera_relative_center.x_metres == 12000.0);
  assert(mesh_draw.camera_relative_center.y_metres == -4000.0);
  assert(mesh_draw.camera_relative_center.z_metres == -2000000.0);
  assert(mesh_draw.physical_radius_metres == 6371000.0);
  assert(near(mesh_draw.camera_uniform.model_view_projection[0],
              6371000.0 / ((960.0 / 540.0) *
                std::tan(view.vertical_field_of_view_radians / 2.0)), 1.0));
  assert(mesh_draw.camera_uniform.model_view_projection[15] == 2000000.0F);
  assert(mesh_draw.camera_uniform.normal_matrix[0] == 1.0F);
  assert(mesh_draw.camera_uniform.normal_matrix[5] == 1.0F);
  assert(mesh_draw.camera_uniform.normal_matrix[10] == 1.0F);

  const render_item local_structure{
    21, {anchor + 32.0, anchor - 16.0, anchor - 500.0}, 30.0,
    "local-structure"};
  const auto box_draw = prepare_box_draw(
    local_structure, {10.0, 20.0, 30.0}, 1.5707963267948966,
    view, {960.0, 540.0});
  assert(box_draw.camera_relative_center.x_metres == 32.0);
  assert(box_draw.camera_relative_center.y_metres == -16.0);
  assert(box_draw.camera_relative_center.z_metres == -500.0);
  assert(box_draw.half_extents_metres.x_metres == 10.0);
  assert(box_draw.half_extents_metres.y_metres == 20.0);
  assert(box_draw.half_extents_metres.z_metres == 30.0);
  assert(box_draw.camera_uniform.model_view_projection[15] == 500.0F);
  assert(near(box_draw.camera_uniform.normal_matrix[0], 0.0));
  assert(near(box_draw.camera_uniform.normal_matrix[1], 1.0));
  assert(near(box_draw.camera_uniform.normal_matrix[4], -1.0));
  assert(near(box_draw.camera_uniform.normal_matrix[5], 0.0));
  assert(box_draw.camera_uniform.normal_matrix[10] == 1.0F);

  const auto oriented_box_draw = prepare_oriented_box_draw(
    local_structure, {10.0, 20.0, 30.0},
    {0.0, 1.0, 0.0}, {-0.5, 0.0, 0.8660254037844386},
    {0.8660254037844386, 0.0, 0.5}, view, {960.0, 540.0});
  assert(oriented_box_draw.camera_relative_center.x_metres == 32.0);
  assert(oriented_box_draw.camera_relative_center.y_metres == -16.0);
  assert(oriented_box_draw.camera_relative_center.z_metres == -500.0);
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[0], 0.0));
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[1], 1.0));
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[4], -0.5));
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[6],
              0.8660254037844386));
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[8],
              0.8660254037844386));
  assert(near(oriented_box_draw.camera_uniform.normal_matrix[10], 0.5));

  bool non_orthogonal_box_rejected = false;
  try
  {
    prepare_oriented_box_draw(
      local_structure, {10.0, 20.0, 30.0},
      {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
      view, {960.0, 540.0});
  }
  catch (const std::invalid_argument &)
  {
    non_orthogonal_box_rejected = true;
  }
  assert(non_orthogonal_box_rejected);

  const render_item tiny_far_body{
    12, {anchor, anchor, anchor - 8000000.0}, 10.0, "tiny"};
  const auto tiny_draw = prepare_sphere_draw(
    tiny_far_body, view, {960.0, 540.0});
  const double projected_radius =
    (tiny_draw.physical_radius_metres / 8000000.0) /
    std::tan(view.vertical_field_of_view_radians / 2.0) * 540.0 / 2.0;
  assert(projected_radius < 0.001);
  assert(tiny_draw.physical_radius_metres == 10.0);

  bool behind_rejected = false;
  try
  {
    prepare_sphere_draw(
      {13, {anchor, anchor, anchor + 1.0}, 1.0, "behind"},
      view, {960.0, 540.0});
  }
  catch (const std::invalid_argument &)
  {
    behind_rejected = true;
  }
  assert(behind_rejected);
  const projector project{view, {960.0, 540.0}};
  const auto frame = project.prepare(source);
  const auto view_matrix = project.rebased_view_matrix();
  const auto projection_matrix = project.projection_matrix();

  assert(source.revision() == 42);
  assert(source.frame() == "example-right-handed-frame");
  assert(source.items()[0].position.x_metres == anchor);
  assert(frame.size() == 4);
  assert(frame[0].visible);
  assert(near(frame[0].logical_x, 480.0));
  assert(near(frame[0].logical_y, 270.0));
  assert(frame[1].logical_x > frame[0].logical_x);
  assert(frame[2].logical_y < frame[0].logical_y);
  assert(!frame[3].visible);
  assert(near(view_matrix[0], 1.0));
  assert(near(view_matrix[5], 1.0));
  assert(near(view_matrix[10], -1.0));
  assert(near(view_matrix[3], 0.0));
  assert(projection_matrix[0] > 0.0);
  assert(projection_matrix[5] > projection_matrix[0]);
  assert(near(projection_matrix[14], 1.0));

  horizon_locked_camera controlled{{anchor, anchor, anchor}};
  assert(near(controlled.forward().z, -1.0));
  assert(near(controlled.right().x, 1.0));
  assert(near(dot(controlled.right(), controlled.world_up()), 0.0));
  controlled.rotate(1.5707963267948966, 0.0);
  assert(near(controlled.forward().x, 1.0));
  assert(near(controlled.forward().y, 0.0));
  assert(near(controlled.forward().z, 0.0));
  controlled.rotate(0.0, 100.0);
  assert(controlled.pitch_radians() < 1.5707963267948966);
  assert(dot(controlled.up(), controlled.world_up()) > 0.0);
  assert(near(dot(controlled.right(), controlled.world_up()), 0.0));

  horizon_locked_camera moving{{0.0, 0.0, 0.0}};
  moving.translate(10.0, 20.0, 30.0);
  assert(near(moving.position().x_metres, 10.0));
  assert(near(moving.position().y_metres, 20.0));
  assert(near(moving.position().z_metres, -30.0));
  moving.orbit({100.0, 200.0, 300.0}, 50.0);
  assert(near(moving.position().x_metres, 100.0));
  assert(near(moving.position().y_metres, 200.0));
  assert(near(moving.position().z_metres, 350.0));
  const horizon_locked_camera alternate_up{
    {0.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
  assert(near(dot(alternate_up.right(), alternate_up.world_up()), 0.0));
  assert(dot(alternate_up.up(), alternate_up.world_up()) > 0.0);
  bool rejected_pole_forward = false;
  try
  {
    const horizon_locked_camera invalid{
      {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}};
    (void)invalid;
  }
  catch (const std::invalid_argument &) { rejected_pole_forward = true; }
  assert(rejected_pole_forward);

  // Reframing creates another projection; the immutable source is unchanged.
  camera moved = view;
  moved.position.x_metres += 250000.0;
  const auto reframed = projector{moved, {960.0, 540.0}}.prepare(source);
  assert(near(reframed[1].logical_x, 480.0));
  assert(source.items()[1].position.x_metres == anchor + 250000.0);

  // Subtracting before narrowing preserves a 32 m separation around 1e15 m.
  const render_item precise{5, {anchor + 32.0, anchor, anchor - 1000000.0}, 1.0, "precise"};
  const auto precise_projection = project.project(precise);
  assert(precise_projection.logical_x > frame[0].logical_x);

  selection selected;
  for (std::size_t index = 0; index < 3; ++index)
  {
    assert(selected.select_at(frame, {frame[index].logical_x, frame[index].logical_y}));
    assert(selected.identifier() == frame[index].identifier);
  }
  assert(!selected.select_at(frame, {0.0, 0.0}));
  assert(!selected.identifier());
  selected.select(3);
  assert(selected.identifier() == 3);
  selected.clear();
  assert(!selected.identifier());

  const length3 focused = focus_camera_position(
    source.items()[1], view.forward, 500000.0);
  assert(near(focused.x_metres, source.items()[1].position.x_metres));
  assert(near(focused.z_metres, source.items()[1].position.z_metres + 500000.0));
  const focus_transition transition{view.position, focused, 2.0};
  const length3 focus_start = transition.sample(0.0);
  const length3 focus_middle = transition.sample(1.0);
  const length3 focus_end = transition.sample(2.0);
  assert(near(focus_start.x_metres, view.position.x_metres));
  assert(near(focus_middle.x_metres,
              view.position.x_metres + (focused.x_metres - view.position.x_metres) / 2.0));
  assert(near(focus_end.x_metres, focused.x_metres));
  assert(!transition.complete(1.999));
  assert(transition.complete(2.0));
  assert(source.items()[1].position.x_metres == anchor + 250000.0);
  const focus_transition scale_transition{
    {-1.0e15, 0.0, 0.0}, {1.0e15, 1.0e12, -1.0e12}, 4.0};
  const length3 scale_middle = scale_transition.sample(2.0);
  assert(near(scale_middle.x_metres, 0.0));
  assert(near(scale_middle.y_metres, 5.0e11));
  assert(near(scale_middle.z_metres, -5.0e11));

  const auto labels = place_labels({
    {1, {480.0, 270.0}, 80.0, 18.0, 0.2},
    {2, {482.0, 271.0}, 80.0, 18.0, 0.3},
    {3, {950.0, 535.0}, 80.0, 18.0, 0.4}
  }, {960.0, 540.0});
  assert(labels.size() == 3);
  assert(!labels_overlap(labels[0], labels[1]));
  assert(labels[2].x + labels[2].width <= 960.0);
  assert(labels[2].y + labels[2].height <= 540.0);

  bool rejected_duplicate = false;
  try
  {
    const snapshot invalid{1, "frame", 0.0,
      {{1, {}, 1.0, "a"}, {1, {}, 1.0, "b"}}};
    (void)invalid;
  }
  catch (const std::invalid_argument &) { rejected_duplicate = true; }
  assert(rejected_duplicate);

  if (argc == 2)
  {
    std::ofstream svg{argv[1]};
    assert(svg);
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"960\" height=\"540\" "
           "viewBox=\"0 0 960 540\" data-contract=\"sagan-scene-camera-v1\">\n"
           "<rect width=\"960\" height=\"540\" fill=\"#02060b\"/>\n";
    for (const auto &item : frame)
      if (item.visible)
        svg << "<circle cx=\"" << item.logical_x << "\" cy=\"" << item.logical_y
            << "\" r=\"" << std::max(3.0, item.radius_logical)
            << "\" fill=\"#8ecbff\"/>\n"
            << "<text x=\"" << item.logical_x + std::max(3.0, item.radius_logical) + 6.0
            << "\" y=\"" << item.logical_y
            << "\" fill=\"white\" font-family=\"sans-serif\" font-size=\"14\">"
            << item.label << "</text>\n";
    for (const auto &label : labels)
      svg << "<rect x=\"" << label.x << "\" y=\"" << label.y
          << "\" width=\"" << label.width << "\" height=\"" << label.height
          << "\" fill=\"none\" stroke=\"#ff8bd1\"/>\n";
    svg << "<text x=\"24\" y=\"32\" fill=\"#8ecbff\" font-family=\"sans-serif\" "
           "font-size=\"16\">camera-relative projection near 1e15 metres</text>\n</svg>\n";
  }
}
