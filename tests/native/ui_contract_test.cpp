#include "../../libraries/render/native/ui_contract.hpp"

#include <cassert>
#include <cmath>
#include <fstream>
#include <string>

namespace
{
  auto near(const double first, const double second) -> bool
  {
    return std::abs(first - second) < 0.000001;
  }

  class deterministic_shaper final : public sagan_render::ui::text_shaper
  {
  public:
    auto shape(const sagan_render::ui::text_request &request) const
      -> sagan_render::ui::shaped_text override
    {
      assert(request.font_family == "Sagan Sans");
      assert(request.language == "en");
      assert(request.direction == sagan_render::ui::text_direction::left_to_right);
      sagan_render::ui::shaped_text result;
      result.measured = {
        std::min(request.wrap_width,
                 request.point_size * 0.6 * static_cast<double>(request.text.size())),
        request.point_size * 1.2
      };
      for (const unsigned char character : request.text)
        result.glyphs.push_back({character, request.point_size * 0.6, {}});
      return result;
    }
  };
}

int main(int argc, char **argv)
{
  using namespace sagan_render::ui;

  const rectangle logical_window{0.0, 0.0, 800.0, 450.0};
  const display_transform scale_one{1.0};
  const display_transform scale_two{2.0};
  assert(scale_one.to_drawable(logical_window).width == 800);
  assert(scale_two.to_drawable(logical_window).width == 1600);
  assert(near(scale_two.to_logical({400, 200}).x, 200.0));
  assert(near(scale_two.to_logical({400, 200}).y, 100.0));

  const auto columns = linear_layout(
    logical_window, axis::horizontal,
    {{180.0, 200.0, 240.0, 0.0, 0.0},
     {320.0, 500.0, 1000.0, 1.0, 0.0}},
    12.0, {16.0, 16.0, 16.0, 16.0});
  assert(columns.size() == 2);
  assert(near(columns[0].width, 200.0));
  assert(near(columns[1].width, 556.0));
  assert(near(columns[1].x, 228.0));

  const auto constrained = linear_layout(
    {0.0, 0.0, 600.0, 100.0}, axis::horizontal,
    {{100.0, 100.0, 120.0, 1.0, 0.0},
     {100.0, 100.0, 1000.0, 1.0, 0.0}});
  assert(near(constrained[0].width, 120.0));
  assert(near(constrained[1].width, 480.0));
  const auto compact = linear_layout(
    {0.0, 0.0, 180.0, 100.0}, axis::horizontal,
    {{60.0, 100.0, 200.0, 0.0, 0.0},
     {60.0, 100.0, 200.0, 0.0, 0.0}});
  assert(near(compact[0].width, 90.0));
  assert(near(compact[1].width, 90.0));

  const auto rows = linear_layout(
    columns[1], axis::vertical,
    {{40.0, 52.0, 52.0, 0.0, 0.0},
     {100.0, 200.0, 1000.0, 1.0, 0.0},
     {48.0, 64.0, 64.0, 0.0, 0.0}}, 8.0);
  assert(rows.size() == 3);
  assert(near(rows[0].height, 52.0));
  assert(near(rows[2].height, 64.0));
  assert(near(intersect({760.0, 420.0, 100.0, 100.0}, logical_window).width, 40.0));

  // A view that is exactly 400,000 km wide remains exactly that wide at
  // either display scale. Only the final logical-to-drawable conversion changes.
  const auto space_view = orthographic_transform::with_horizontal_span(
    rows[1], {0.0, 0.0}, 400000000.0);
  assert(near(space_view.span().width_metres, 400000000.0));

  const auto wider_view = orthographic_transform::with_horizontal_span(
    {0.0, 0.0, 1024.0, 768.0}, {0.0, 0.0}, 400000000.0);
  assert(near(wider_view.span().width_metres, 400000000.0));
  assert(near(wider_view.span().height_metres, 300000000.0));
  assert(near(space_view.metres_per_logical_x(),
              space_view.metres_per_logical_y()));
  const point earth_logical = space_view.to_logical({149600000000.0, 0.0});
  const physical_point earth_round_trip = space_view.to_physical(earth_logical);
  assert(near(earth_round_trip.x_metres, 149600000000.0));
  assert(scale_two.to_drawable({earth_logical.x, earth_logical.y, 1.0, 1.0}).x ==
         2 * scale_one.to_drawable({earth_logical.x, earth_logical.y, 1.0, 1.0}).x);
  assert(near(space_view.span().width_metres, 400000000.0));

  deterministic_shaper shaper;
  const auto shaped = shaper.shape({"Pause", "Sagan Sans", 20.0, 120.0,
                                    text_direction::left_to_right, "en"});
  assert(shaped.glyphs.size() == 5);
  assert(near(shaped.measured.width, 60.0));

  input_router input;
  input.set_controls({
    {"scene", rows[1], true, true},
    {"pause", {680.0, 24.0, 96.0, 40.0}, true, true},
    {"modal", {280.0, 120.0, 300.0, 210.0}, true, true}
  });
  input.pointer_down({300.0, 150.0});
  assert(input.captured_identifier() == "modal");
  input.pointer_up({700.0, 400.0});
  assert(!input.take_activation());
  input.pointer_down({300.0, 150.0});
  input.pointer_up({300.0, 150.0});
  assert(input.take_activation() == "modal");
  input.focus_next();
  assert(input.focused_identifier() == "scene");
  input.focus_next(true);
  assert(input.focused_identifier() == "modal");
  input.activate_focused();
  assert(input.take_activation() == "modal");

  input.set_controls({
    {"scene", {228.0, 76.0, 556.0, 298.0}, true, true},
    {"pause", {680.0, 24.0, 96.0, 40.0}, true, true},
    {"modal", {270.0, 110.0, 320.0, 230.0}, true, true}
  });
  assert(input.focused_identifier() == "modal");
  assert(!input.captured_identifier());
  input.clear();
  assert(!input.focused_identifier());

  if (argc == 2)
  {
    std::ofstream svg{argv[1]};
    assert(svg);
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"450\" "
           "viewBox=\"0 0 800 450\" data-contract=\"sagan-ui-logical-v1\">\n"
           "<rect width=\"800\" height=\"450\" fill=\"#07111f\"/>\n"
           "<rect x=\"16\" y=\"16\" width=\"200\" height=\"418\" rx=\"8\" fill=\"#102b46\"/>\n"
           "<rect x=\"228\" y=\"16\" width=\"556\" height=\"52\" rx=\"8\" fill=\"#173b5e\"/>\n"
           "<rect x=\"228\" y=\"76\" width=\"556\" height=\"298\" fill=\"#02060b\"/>\n"
           "<circle cx=\"506\" cy=\"225\" r=\"28\" fill=\"#ffd45b\"/>\n"
           "<rect x=\"228\" y=\"382\" width=\"556\" height=\"52\" rx=\"8\" fill=\"#173b5e\"/>\n"
           "<rect x=\"270\" y=\"110\" width=\"320\" height=\"230\" rx=\"12\" fill=\"#243752\" stroke=\"#8ecbff\"/>\n"
           "<text x=\"300\" y=\"155\" fill=\"white\" font-family=\"sans-serif\" font-size=\"20\">Pause</text>\n"
           "</svg>\n";
  }
}
