#pragma once

#include <cstdint>
#include <stdexcept>

namespace sagan_render::native
{
  struct window_metrics
  {
    std::int64_t logical_width{};
    std::int64_t logical_height{};
    std::int64_t drawable_width{};
    std::int64_t drawable_height{};
    double display_scale{1.0};
    bool focused{};
    bool resized{};
    bool close_requested{};
    bool open{};
  };

  class window_contract
  {
    window_metrics value{};

    static auto validate_dimensions(const std::int64_t logical_width,
                                    const std::int64_t logical_height,
                                    const std::int64_t drawable_width,
                                    const std::int64_t drawable_height,
                                    const double display_scale) -> void
    {
      if (logical_width < 0 || logical_height < 0 || drawable_width < 0 ||
          drawable_height < 0 || !(display_scale > 0.0))
        throw std::invalid_argument(
          "Window dimensions must be non-negative and display scale must be positive");
    }

  public:
    auto open(const std::int64_t logical_width, const std::int64_t logical_height,
              const std::int64_t drawable_width, const std::int64_t drawable_height,
              const double display_scale, const bool focused) -> void
    {
      if (logical_width < 1 || logical_height < 1 || drawable_width < 1 ||
          drawable_height < 1)
        throw std::invalid_argument("An open window requires positive dimensions");
      validate_dimensions(logical_width, logical_height, drawable_width, drawable_height,
                          display_scale);
      value = {logical_width, logical_height, drawable_width, drawable_height,
               display_scale, focused, true, false, true};
    }

    auto update_dimensions(const std::int64_t logical_width,
                           const std::int64_t logical_height,
                           const std::int64_t drawable_width,
                           const std::int64_t drawable_height,
                           const double display_scale) -> void
    {
      validate_dimensions(logical_width, logical_height, drawable_width, drawable_height,
                          display_scale);
      if (!value.open) return;
      const bool changed = value.logical_width != logical_width ||
                           value.logical_height != logical_height ||
                           value.drawable_width != drawable_width ||
                           value.drawable_height != drawable_height ||
                           value.display_scale != display_scale;
      value.logical_width = logical_width;
      value.logical_height = logical_height;
      value.drawable_width = drawable_width;
      value.drawable_height = drawable_height;
      value.display_scale = display_scale;
      value.resized = value.resized || changed;
    }

    auto set_focused(const bool focused) -> void
    {
      if (value.open) value.focused = focused;
    }

    auto request_close() -> void { value.close_requested = true; }

    auto close() -> void
    {
      value.logical_width = 0;
      value.logical_height = 0;
      value.drawable_width = 0;
      value.drawable_height = 0;
      value.display_scale = 1.0;
      value.focused = false;
      value.resized = false;
      value.close_requested = true;
      value.open = false;
    }

    auto consume_resized() -> bool
    {
      const bool result = value.resized;
      value.resized = false;
      return result;
    }

    auto metrics() const -> const window_metrics & { return value; }
  };
}
