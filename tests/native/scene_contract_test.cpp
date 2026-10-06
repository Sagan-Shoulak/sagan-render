#include "../../libraries/render/native/scene_contract.hpp"

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
    svg << "<text x=\"24\" y=\"32\" fill=\"#8ecbff\" font-family=\"sans-serif\" "
           "font-size=\"16\">camera-relative projection near 1e15 metres</text>\n</svg>\n";
  }
}
