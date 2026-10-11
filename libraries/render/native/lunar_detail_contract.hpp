#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace sagan_render::scene
{
  // The LROC fixture is an illuminated observation, not measured albedo.
  // Use only its fine spatial variation as a neutral demo-detail signal. A
  // separable 64-metre box average removes broad exposure/shadow regions; the
  // residual modulates the global map's anchor luminance. Source pixels and
  // provenance are never edited, and this does not generate terrain heights.
  inline auto neutral_lunar_detail(const std::span<const std::uint8_t> rgba,
                                   const std::uint32_t width,
                                   const std::uint32_t height,
                                   const std::uint8_t anchor_luminance)
    -> std::vector<std::uint8_t>
  {
    if (!width || !height || rgba.size() != static_cast<std::size_t>(width) * height * 4)
      throw std::invalid_argument("Invalid lunar detail image dimensions");
    constexpr int radius_pixels = 40; // 81 samples at 0.8 m/pixel = 64.8 m.
    const std::size_t count = static_cast<std::size_t>(width) * height;
    std::vector<double> luminance(count), horizontal(count);
    for (std::size_t p = 0; p < count; ++p)
      luminance[p] = (54.0 * rgba[p * 4] + 183.0 * rgba[p * 4 + 1] +
                      19.0 * rgba[p * 4 + 2]) / 256.0;
    for (std::uint32_t y = 0; y < height; ++y)
    {
      double sum{};
      int low{}, high{-1};
      for (int x = 0; x < static_cast<int>(width); ++x)
      {
        const int next_high = std::min(static_cast<int>(width) - 1, x + radius_pixels);
        while (high < next_high) sum += luminance[y * width + ++high];
        const int next_low = std::max(0, x - radius_pixels);
        while (low < next_low) sum -= luminance[y * width + low++];
        horizontal[y * width + x] = sum / (high - low + 1);
      }
    }
    std::vector<std::uint8_t> result(count * 4);
    for (std::uint32_t x = 0; x < width; ++x)
    {
      double sum{};
      int low{}, high{-1};
      for (int y = 0; y < static_cast<int>(height); ++y)
      {
        const int next_high = std::min(static_cast<int>(height) - 1, y + radius_pixels);
        while (high < next_high) sum += horizontal[++high * width + x];
        const int next_low = std::max(0, y - radius_pixels);
        while (low < next_low) sum -= horizontal[low++ * width + x];
        const double background = sum / (high - low + 1);
        const std::size_t p = static_cast<std::size_t>(y) * width + x;
        // Limit residual contrast and the denominator in fully shadowed areas.
        // This is a detail convention, not invented light in an albedo asset.
        const double residual = std::clamp((luminance[p] - background) /
                                          std::max(background, 16.0), -0.5, 0.5);
        const auto value = static_cast<std::uint8_t>(std::clamp(
          std::lround(anchor_luminance * (1.0 + residual)), 0L, 255L));
        result[p * 4] = result[p * 4 + 1] = result[p * 4 + 2] = value;
        result[p * 4 + 3] = 255;
      }
    }
    return result;
  }
}
