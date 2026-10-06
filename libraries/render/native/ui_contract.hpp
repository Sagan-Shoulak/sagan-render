#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sagan_render::ui
{
  using scalar = double;

  struct point
  {
    scalar x{};
    scalar y{};
  };

  struct size
  {
    scalar width{};
    scalar height{};
  };

  struct rectangle
  {
    scalar x{};
    scalar y{};
    scalar width{};
    scalar height{};

    auto contains(const point value) const -> bool
    {
      return value.x >= x && value.y >= y && value.x < x + width &&
             value.y < y + height;
    }
  };

  struct insets
  {
    scalar left{};
    scalar top{};
    scalar right{};
    scalar bottom{};
  };

  inline auto validate_non_negative(const scalar value, const char *name) -> void
  {
    if (!std::isfinite(value) || value < 0.0)
      throw std::invalid_argument(std::string{name} + " must be finite and non-negative");
  }

  inline auto inset(const rectangle bounds, const insets padding) -> rectangle
  {
    validate_non_negative(bounds.width, "Rectangle width");
    validate_non_negative(bounds.height, "Rectangle height");
    validate_non_negative(padding.left, "Left inset");
    validate_non_negative(padding.top, "Top inset");
    validate_non_negative(padding.right, "Right inset");
    validate_non_negative(padding.bottom, "Bottom inset");
    return {
      bounds.x + padding.left,
      bounds.y + padding.top,
      std::max<scalar>(0.0, bounds.width - padding.left - padding.right),
      std::max<scalar>(0.0, bounds.height - padding.top - padding.bottom)
    };
  }

  inline auto intersect(const rectangle first, const rectangle second) -> rectangle
  {
    const scalar left = std::max(first.x, second.x);
    const scalar top = std::max(first.y, second.y);
    const scalar right = std::min(first.x + first.width, second.x + second.width);
    const scalar bottom = std::min(first.y + first.height, second.y + second.height);
    return {left, top, std::max<scalar>(0.0, right - left),
            std::max<scalar>(0.0, bottom - top)};
  }

  struct pixel_point
  {
    std::int64_t x{};
    std::int64_t y{};
  };

  struct pixel_rectangle
  {
    std::int64_t x{};
    std::int64_t y{};
    std::int64_t width{};
    std::int64_t height{};
  };

  // These wrappers make the boundary visible in native code. Sagan's public
  // API carries the stronger Float64<length-unit> subtype instead of a bare
  // scalar; drawable pixels must never be accepted as physical lengths.
  struct physical_point
  {
    scalar x_metres{};
    scalar y_metres{};
  };

  struct physical_size
  {
    scalar width_metres{};
    scalar height_metres{};
  };

  class orthographic_transform
  {
    rectangle viewport_value;
    physical_point center_value;
    physical_size span_value;

  public:
    orthographic_transform(const rectangle viewport,
                           const physical_point center,
                           const physical_size span)
      : viewport_value{viewport}, center_value{center}, span_value{span}
    {
      validate_non_negative(viewport.width, "Viewport width");
      validate_non_negative(viewport.height, "Viewport height");
      if (viewport.width == 0.0 || viewport.height == 0.0)
        throw std::invalid_argument("Viewport dimensions must be positive");
      if (!std::isfinite(center.x_metres) || !std::isfinite(center.y_metres) ||
          !std::isfinite(span.width_metres) || !std::isfinite(span.height_metres) ||
          span.width_metres <= 0.0 || span.height_metres <= 0.0)
        throw std::invalid_argument("Physical view center and span must be finite and span must be positive");
    }

    static auto with_horizontal_span(const rectangle viewport,
                                     const physical_point center,
                                     const scalar width_metres)
      -> orthographic_transform
    {
      validate_non_negative(width_metres, "Physical horizontal span");
      if (width_metres == 0.0 || viewport.width <= 0.0 || viewport.height <= 0.0)
        throw std::invalid_argument("Physical horizontal span and viewport dimensions must be positive");
      return orthographic_transform{
        viewport,
        center,
        {width_metres, width_metres * viewport.height / viewport.width}
      };
    }

    auto viewport() const -> rectangle { return viewport_value; }
    auto center() const -> physical_point { return center_value; }
    auto span() const -> physical_size { return span_value; }

    auto to_logical(const physical_point value) const -> point
    {
      const scalar left = center_value.x_metres - span_value.width_metres / 2.0;
      const scalar top = center_value.y_metres + span_value.height_metres / 2.0;
      return {
        viewport_value.x + (value.x_metres - left) * viewport_value.width /
          span_value.width_metres,
        viewport_value.y + (top - value.y_metres) * viewport_value.height /
          span_value.height_metres
      };
    }

    auto to_physical(const point value) const -> physical_point
    {
      const scalar left = center_value.x_metres - span_value.width_metres / 2.0;
      const scalar top = center_value.y_metres + span_value.height_metres / 2.0;
      return {
        left + (value.x - viewport_value.x) * span_value.width_metres /
          viewport_value.width,
        top - (value.y - viewport_value.y) * span_value.height_metres /
          viewport_value.height
      };
    }

    auto metres_per_logical_x() const -> scalar
    {
      return span_value.width_metres / viewport_value.width;
    }

    auto metres_per_logical_y() const -> scalar
    {
      return span_value.height_metres / viewport_value.height;
    }
  };

  class display_transform
  {
    scalar scale_value{};

  public:
    explicit display_transform(const scalar scale) : scale_value{scale}
    {
      if (!std::isfinite(scale) || scale <= 0.0)
        throw std::invalid_argument("Display scale must be finite and positive");
    }

    auto scale() const -> scalar { return scale_value; }

    auto to_logical(const pixel_point value) const -> point
    {
      return {static_cast<scalar>(value.x) / scale_value,
              static_cast<scalar>(value.y) / scale_value};
    }

    auto to_drawable(const rectangle value) const -> pixel_rectangle
    {
      return {
        std::lround(value.x * scale_value),
        std::lround(value.y * scale_value),
        std::lround(value.width * scale_value),
        std::lround(value.height * scale_value)
      };
    }
  };

  enum class axis { horizontal, vertical };
  enum class alignment { start, center, end, stretch };

  struct linear_item
  {
    scalar minimum{};
    scalar preferred{};
    scalar maximum{std::numeric_limits<scalar>::infinity()};
    scalar grow{};
    scalar cross_size{};
  };

  inline auto linear_layout(const rectangle bounds, const axis direction,
                            const std::vector<linear_item> &items,
                            const scalar gap = 0.0,
                            const insets padding = {},
                            const alignment cross_alignment = alignment::stretch)
    -> std::vector<rectangle>
  {
    validate_non_negative(gap, "Layout gap");
    const rectangle content = inset(bounds, padding);
    if (items.empty()) return {};

    const scalar available_main = (direction == axis::horizontal)
      ? content.width : content.height;
    const scalar available_cross = (direction == axis::horizontal)
      ? content.height : content.width;
    const scalar gaps = gap * static_cast<scalar>(items.size() - 1);

    std::vector<scalar> main_sizes;
    main_sizes.reserve(items.size());
    scalar used = gaps;
    scalar grow_total{};
    for (const auto &item : items)
    {
      validate_non_negative(item.minimum, "Minimum size");
      validate_non_negative(item.preferred, "Preferred size");
      validate_non_negative(item.grow, "Grow weight");
      validate_non_negative(item.cross_size, "Cross size");
      if (item.maximum < item.minimum)
        throw std::invalid_argument("Maximum size cannot be smaller than minimum size");
      const scalar value = std::clamp(item.preferred, item.minimum, item.maximum);
      main_sizes.push_back(value);
      used += value;
      grow_total += item.grow;
    }

    scalar remaining = available_main - used;
    while (remaining > 0.0 && grow_total > 0.0)
    {
      scalar distributed{};
      for (std::size_t index = 0; index < items.size(); ++index)
      {
        if (main_sizes[index] >= items[index].maximum) continue;
        const scalar share = remaining * items[index].grow / grow_total;
        const scalar addition = std::min(share, items[index].maximum - main_sizes[index]);
        main_sizes[index] += addition;
        distributed += addition;
      }
      if (distributed <= 0.0) break;
      remaining -= distributed;
      grow_total = 0.0;
      for (std::size_t index = 0; index < items.size(); ++index)
        if (main_sizes[index] < items[index].maximum)
          grow_total += items[index].grow;
    }
    if (remaining < 0.0)
    {
      scalar shrinkable{};
      for (std::size_t index = 0; index < items.size(); ++index)
        shrinkable += main_sizes[index] - items[index].minimum;
      if (shrinkable > 0.0)
        for (std::size_t index = 0; index < items.size(); ++index)
        {
          const scalar capacity = main_sizes[index] - items[index].minimum;
          const scalar reduction = std::min(capacity, -remaining * capacity / shrinkable);
          main_sizes[index] -= reduction;
        }
    }

    std::vector<rectangle> result;
    result.reserve(items.size());
    scalar cursor = (direction == axis::horizontal) ? content.x : content.y;
    for (std::size_t index = 0; index < items.size(); ++index)
    {
      scalar cross = cross_alignment == alignment::stretch || items[index].cross_size == 0.0
        ? available_cross : std::min(available_cross, items[index].cross_size);
      scalar cross_offset{};
      if (cross_alignment == alignment::center) cross_offset = (available_cross - cross) / 2.0;
      if (cross_alignment == alignment::end) cross_offset = available_cross - cross;
      if (direction == axis::horizontal)
        result.push_back({cursor, content.y + cross_offset, main_sizes[index], cross});
      else
        result.push_back({content.x + cross_offset, cursor, cross, main_sizes[index]});
      cursor += main_sizes[index] + gap;
    }
    return result;
  }

  enum class text_direction { automatic, left_to_right, right_to_left };

  struct text_request
  {
    std::string text;
    std::string font_family;
    scalar point_size{};
    scalar wrap_width{};
    text_direction direction{text_direction::automatic};
    std::string language;
  };

  struct glyph
  {
    std::uint32_t identifier{};
    scalar advance{};
    point offset{};
  };

  struct shaped_text
  {
    size measured{};
    std::vector<glyph> glyphs;
  };

  class text_shaper
  {
  public:
    virtual ~text_shaper() = default;
    virtual auto shape(const text_request &request) const -> shaped_text = 0;
  };

  struct control
  {
    std::string identifier;
    rectangle bounds;
    bool enabled{true};
    bool focusable{true};
  };

  class input_router
  {
    std::vector<control> controls;
    std::optional<std::size_t> focused;
    std::optional<std::size_t> captured;
    std::optional<std::string> activation;

    auto hittable_at(const point location) const -> std::optional<std::size_t>
    {
      for (std::size_t reverse = controls.size(); reverse > 0; --reverse)
      {
        const std::size_t index = reverse - 1;
        if (controls[index].enabled && controls[index].bounds.contains(location))
          return index;
      }
      return std::nullopt;
    }

  public:
    auto set_controls(std::vector<control> value) -> void
    {
      std::optional<std::string> previous;
      if (focused) previous = controls[*focused].identifier;
      controls = std::move(value);
      focused.reset();
      captured.reset();
      activation.reset();
      if (previous)
        for (std::size_t index = 0; index < controls.size(); ++index)
          if (controls[index].identifier == *previous && controls[index].enabled &&
              controls[index].focusable)
            focused = index;
    }

    auto pointer_down(const point location) -> void
    {
      captured = hittable_at(location);
      if (captured && controls[*captured].focusable) focused = captured;
    }

    auto pointer_up(const point location) -> void
    {
      if (captured && controls[*captured].bounds.contains(location) &&
          controls[*captured].enabled)
        activation = controls[*captured].identifier;
      captured.reset();
    }

    auto cancel_pointer() -> void { captured.reset(); }

    auto focus_next(const bool reverse = false) -> void
    {
      if (controls.empty()) { focused.reset(); return; }
      const std::size_t start = focused.value_or(reverse ? 0 : controls.size() - 1);
      for (std::size_t offset = 1; offset <= controls.size(); ++offset)
      {
        const std::size_t index = reverse
          ? (start + controls.size() - offset % controls.size()) % controls.size()
          : (start + offset) % controls.size();
        if (controls[index].enabled && controls[index].focusable)
        {
          focused = index;
          return;
        }
      }
      focused.reset();
    }

    auto activate_focused() -> void
    {
      if (focused && controls[*focused].enabled)
        activation = controls[*focused].identifier;
    }

    auto take_activation() -> std::optional<std::string>
    {
      auto result = activation;
      activation.reset();
      return result;
    }

    auto focused_identifier() const -> std::optional<std::string>
    {
      if (!focused) return std::nullopt;
      return controls[*focused].identifier;
    }

    auto captured_identifier() const -> std::optional<std::string>
    {
      if (!captured) return std::nullopt;
      return controls[*captured].identifier;
    }

    auto clear() -> void
    {
      controls.clear();
      focused.reset();
      captured.reset();
      activation.reset();
    }
  };
}
