#pragma once

#include "ui_contract.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sagan_render::ui
{
  struct color
  {
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
    std::uint8_t alpha{255};

    auto operator==(const color &) const -> bool = default;
  };

  struct fill_rectangle
  {
    rectangle bounds;
    color paint;
  };

  class draw_list
  {
    rectangle viewport_value;
    std::vector<rectangle> clips;
    std::vector<fill_rectangle> fills_value;

  public:
    explicit draw_list(const rectangle viewport) : viewport_value{viewport}
    {
      clips.push_back(viewport);
    }

    auto push_clip(const rectangle value) -> void
    {
      clips.push_back(intersect(clips.back(), value));
    }

    auto pop_clip() -> void
    {
      if (clips.size() <= 1)
        throw std::logic_error("Cannot pop the draw-list viewport clip");
      clips.pop_back();
    }

    auto fill(const rectangle value, const color paint) -> void
    {
      const rectangle clipped = intersect(value, clips.back());
      if (clipped.width > 0.0 && clipped.height > 0.0)
        fills_value.push_back({clipped, paint});
    }

    auto viewport() const -> rectangle { return viewport_value; }
    auto fills() const -> const std::vector<fill_rectangle> & { return fills_value; }
    auto clip_depth() const -> std::size_t { return clips.size(); }
  };
}
