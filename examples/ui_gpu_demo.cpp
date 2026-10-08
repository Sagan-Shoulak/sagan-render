#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "../libraries/render/native/ui_contract.hpp"
#include "../libraries/render/native/ui_draw_list.hpp"
#include "../libraries/render/native/ui_gpu_bridge.hpp"
#include "../libraries/render/native/scene_gpu_pass.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  using namespace sagan_render::ui;
  constexpr std::uint32_t initial_canvas_width = 960;
  constexpr std::uint32_t initial_canvas_height = 540;
  constexpr std::uint32_t bytes_per_pixel = 4;

  constexpr color background{7, 17, 31, 255};
  constexpr color panel{16, 43, 70, 255};
  constexpr color panel_light{23, 59, 94, 255};
  constexpr color scene{2, 6, 11, 255};
  constexpr color accent{71, 174, 255, 255};
  constexpr color focus_color{142, 203, 255, 255};
  constexpr color sun{255, 212, 91, 255};
  constexpr color earth{70, 133, 255, 255};
  constexpr color moon{205, 215, 225, 255};
  constexpr color sun_shadow{151, 91, 24, 255};
  constexpr color earth_shadow{20, 48, 112, 255};
  constexpr color moon_shadow{82, 91, 105, 255};
  constexpr color white{245, 249, 255, 255};
  constexpr color muted{142, 164, 188, 255};
  constexpr color modal{36, 55, 82, 255};
  constexpr color button{38, 78, 116, 255};
  constexpr color button_focus{55, 125, 181, 255};
  constexpr color system_orbit{47, 75, 104, 255};
  constexpr color lunar_orbit{70, 92, 118, 255};
  constexpr std::array palette{
    background, panel, panel_light, scene, accent, focus_color, sun, earth,
    moon, sun_shadow, earth_shadow, moon_shadow, white, muted, modal, button,
    button_focus, system_orbit, lunar_orbit
  };
  static_assert(sizeof(color) == bytes_per_pixel);

  auto fail(const std::string &operation) -> void
  {
    throw std::runtime_error(operation + ": " + SDL_GetError());
  }

  auto platform_driver() -> const char *
  {
#if defined(_WIN32)
    return "direct3d12";
#elif defined(__APPLE__)
    return "metal";
#else
    return "vulkan";
#endif
  }

  auto glyph_rows(const char raw) -> std::array<std::uint8_t, 7>
  {
    const char value = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
    switch (value)
    {
      case 'A': return {14, 17, 17, 31, 17, 17, 17};
      case 'B': return {30, 17, 17, 30, 17, 17, 30};
      case 'C': return {14, 17, 16, 16, 16, 17, 14};
      case 'D': return {30, 17, 17, 17, 17, 17, 30};
      case 'E': return {31, 16, 16, 30, 16, 16, 31};
      case 'F': return {31, 16, 16, 30, 16, 16, 16};
      case 'G': return {14, 17, 16, 23, 17, 17, 14};
      case 'H': return {17, 17, 17, 31, 17, 17, 17};
      case 'I': return {14, 4, 4, 4, 4, 4, 14};
      case 'J': return {7, 2, 2, 2, 18, 18, 12};
      case 'K': return {17, 18, 20, 24, 20, 18, 17};
      case 'L': return {16, 16, 16, 16, 16, 16, 31};
      case 'M': return {17, 27, 21, 21, 17, 17, 17};
      case 'N': return {17, 25, 21, 19, 17, 17, 17};
      case 'O': return {14, 17, 17, 17, 17, 17, 14};
      case 'P': return {30, 17, 17, 30, 16, 16, 16};
      case 'Q': return {14, 17, 17, 17, 21, 18, 13};
      case 'R': return {30, 17, 17, 30, 20, 18, 17};
      case 'S': return {15, 16, 16, 14, 1, 1, 30};
      case 'T': return {31, 4, 4, 4, 4, 4, 4};
      case 'U': return {17, 17, 17, 17, 17, 17, 14};
      case 'V': return {17, 17, 17, 17, 17, 10, 4};
      case 'W': return {17, 17, 17, 21, 21, 21, 10};
      case 'X': return {17, 17, 10, 4, 10, 17, 17};
      case 'Y': return {17, 17, 10, 4, 4, 4, 4};
      case 'Z': return {31, 1, 2, 4, 8, 16, 31};
      case '0': return {14, 17, 19, 21, 25, 17, 14};
      case '1': return {4, 12, 4, 4, 4, 4, 14};
      case '2': return {14, 17, 1, 2, 4, 8, 31};
      case '3': return {30, 1, 1, 14, 1, 1, 30};
      case '4': return {2, 6, 10, 18, 31, 2, 2};
      case '5': return {31, 16, 16, 30, 1, 1, 30};
      case '6': return {14, 16, 16, 30, 17, 17, 14};
      case '7': return {31, 1, 2, 4, 8, 8, 8};
      case '8': return {14, 17, 17, 14, 17, 17, 14};
      case '9': return {14, 17, 17, 15, 1, 1, 14};
      case ':': return {0, 4, 4, 0, 4, 4, 0};
      case '-': return {0, 0, 0, 31, 0, 0, 0};
      case '.': return {0, 0, 0, 0, 0, 12, 12};
      default: return {};
    }
  }

  class bitmap_shaper final : public text_shaper
  {
  public:
    auto shape(const text_request &request) const -> shaped_text override
    {
      shaped_text result;
      const scalar advance = request.point_size * 6.0 / 7.0;
      result.measured = {advance * static_cast<scalar>(request.text.size()), request.point_size};
      for (const unsigned char character : request.text)
        result.glyphs.push_back({character, advance, {}});
      return result;
    }
  };

  auto draw_text(draw_list &list, const text_shaper &shaper, const point origin,
                 const std::string &value, const scalar height, const color paint) -> void
  {
    const auto shaped = shaper.shape({value, "Sagan Bitmap", height, 10000.0,
                                      text_direction::left_to_right, "en"});
    const scalar pixel = height / 7.0;
    scalar cursor = origin.x;
    for (const glyph &glyph_value : shaped.glyphs)
    {
      const auto rows = glyph_rows(static_cast<char>(glyph_value.identifier));
      for (std::size_t row = 0; row < rows.size(); ++row)
        for (std::size_t column = 0; column < 5; ++column)
          if ((rows[row] & (1U << (4U - column))) != 0)
            list.fill({cursor + static_cast<scalar>(column) * pixel,
                       origin.y + static_cast<scalar>(row) * pixel,
                       pixel, pixel}, paint);
      cursor += glyph_value.advance;
    }
  }

  struct demo_layout
  {
    rectangle scene_bounds;
    std::vector<control> controls;
  };

  [[maybe_unused]] auto compose(draw_list &list, bitmap_shaper &shaper, input_router &input,
               const bool modal_visible, const size canvas_size) -> demo_layout
  {
    const rectangle canvas{0.0, 0.0, canvas_size.width, canvas_size.height};
    list.fill(canvas, background);
    const auto columns = linear_layout(canvas, axis::horizontal,
      {{180.0, 208.0, 240.0, 0.0, 0.0}, {400.0, 700.0, 2000.0, 1.0, 0.0}},
      12.0, {16.0, 16.0, 16.0, 16.0});
    list.fill(columns[0], panel);
    list.fill(columns[1], scene);
    draw_text(list, shaper, {32.0, 34.0}, "SAGAN UI", 21.0, white);
    draw_text(list, shaper, {32.0, 72.0}, "GPU DEMO", 14.0, accent);
    draw_text(list, shaper, {32.0, 136.0}, "MEASURED VIEWS", 10.0, muted);
    draw_text(list, shaper, {32.0, 157.0}, "KM TO LOGICAL", 12.0, white);
    draw_text(list, shaper, {32.0, 216.0}, "TAB: FOCUS", 10.0, muted);
    draw_text(list, shaper, {32.0, 237.0}, "ENTER: ACTIVATE", 10.0, muted);
    draw_text(list, shaper, {32.0, 258.0}, "P: PAUSE", 10.0, muted);
    draw_text(list, shaper, {32.0, 279.0}, "ESC: QUIT", 10.0, muted);

    const rectangle viewport = inset(columns[1], {24.0, 64.0, 24.0, 58.0});
    list.push_clip(viewport);
    const auto solar_view = orthographic_transform::with_horizontal_span(
      viewport, {75000000000.0, 0.0}, 200000000000.0);
    const point sun_at = solar_view.to_logical({0.0, 0.0});
    const point earth_at = solar_view.to_logical({149600000000.0, 0.0});
    list.fill({sun_at.x - 30.0, sun_at.y - 30.0, 60.0, 60.0}, sun);
    list.fill({earth_at.x - 10.0, earth_at.y - 10.0, 20.0, 20.0}, earth);
    draw_text(list, shaper, {sun_at.x - 18.0, sun_at.y + 42.0}, "SUN", 9.0, sun);
    draw_text(list, shaper, {earth_at.x - 18.0, earth_at.y + 18.0}, "EARTH", 9.0, earth);

    const scalar fifty_million_km = 50000000000.0;
    const scalar scale_width = fifty_million_km / solar_view.metres_per_logical_x();
    const scalar scale_x = viewport.x + 20.0;
    const scalar scale_y = viewport.y + viewport.height - 24.0;
    list.fill({scale_x, scale_y, scale_width, 3.0}, focus_color);
    list.fill({scale_x, scale_y - 4.0, 2.0, 11.0}, focus_color);
    list.fill({scale_x + scale_width - 2.0, scale_y - 4.0, 2.0, 11.0}, focus_color);
    draw_text(list, shaper, {scale_x, scale_y - 18.0}, "50000000 KM", 9.0, white);

    const scalar inset_width = std::min<scalar>(300.0, viewport.width * 0.46);
    const rectangle inset_bounds{viewport.x + viewport.width - inset_width - 16.0,
                                 viewport.y + viewport.height - 108.0,
                                 inset_width, 92.0};
    list.fill({inset_bounds.x - 3.0, inset_bounds.y - 3.0,
               inset_bounds.width + 6.0, inset_bounds.height + 6.0}, focus_color);
    list.fill(inset_bounds, panel);
    const rectangle inset_view = inset(inset_bounds, {12.0, 28.0, 12.0, 10.0});
    const auto moon_view = orthographic_transform::with_horizontal_span(
      inset_view, {192200000.0, 0.0}, 1000000000.0);
    const point inset_earth = moon_view.to_logical({0.0, 0.0});
    const point inset_moon = moon_view.to_logical({384400000.0, 0.0});
    list.fill({inset_earth.x - 7.0, inset_earth.y - 7.0, 14.0, 14.0}, earth);
    list.fill({inset_moon.x - 4.0, inset_moon.y - 4.0, 8.0, 8.0}, moon);
    draw_text(list, shaper, {inset_bounds.x + 10.0, inset_bounds.y + 8.0},
              "EARTH-MOON: 1000000 KM WIDE", 8.0, white);
    list.pop_clip();
    draw_text(list, shaper, {columns[1].x + 24.0, columns[1].y + 20.0},
              "SOLAR VIEW: 200000000 KM WIDE", 12.0, white);
    draw_text(list, shaper, {columns[1].x + 24.0, columns[1].y + columns[1].height - 30.0},
              "RESIZE REFLOWS UI - SCALE BARS KEEP KM", 10.0, muted);

    std::vector<control> controls;
    if (modal_visible)
    {
      const scalar modal_width = std::min<scalar>(360.0, columns[1].width - 48.0);
      const scalar modal_height = std::min<scalar>(290.0, columns[1].height - 48.0);
      const rectangle modal_bounds{columns[1].x + (columns[1].width - modal_width) / 2.0,
                                   columns[1].y + (columns[1].height - modal_height) / 2.0,
                                   modal_width, modal_height};
      list.fill({modal_bounds.x - 4.0, modal_bounds.y - 4.0,
                 modal_bounds.width + 8.0, modal_bounds.height + 8.0}, focus_color);
      list.fill(modal_bounds, modal);
      draw_text(list, shaper, {modal_bounds.x + 40.0, modal_bounds.y + 33.0},
                "PAUSED", 24.0, white);
      const std::array<std::string, 3> labels{"RESUME", "SETTINGS", "QUIT"};
      for (std::size_t index = 0; index < labels.size(); ++index)
      {
        const rectangle bounds{modal_bounds.x + 60.0,
                               modal_bounds.y + 95.0 + 58.0 * static_cast<scalar>(index),
                               modal_bounds.width - 120.0, 42.0};
        const bool focused = input.focused_identifier() == labels[index];
        list.fill(bounds, focused ? button_focus : button);
        draw_text(list, shaper, {bounds.x + 24.0, bounds.y + 12.0},
                  labels[index], 14.0, white);
        controls.push_back({labels[index], bounds, true, true});
      }
    }
    else
    {
      const rectangle pause_button{columns[1].x + columns[1].width - 124.0,
                                   columns[1].y + 14.0, 100.0, 32.0};
      list.fill(pause_button, button);
      draw_text(list, shaper, {pause_button.x + 15.0, pause_button.y + 9.0},
                "PAUSE", 12.0, white);
      controls.push_back({"PAUSE", pause_button, true, true});
    }
    return {viewport, std::move(controls)};
  }

  class gpu_compositor
  {
    SDL_Window *window{};
    SDL_GPUDevice *device{};
    SDL_GPUTexture *target{};
    SDL_GPUTexture *palette_texture{};
    SDL_GPUTransferBuffer *download{};
    std::unique_ptr<sagan_render::scene_gpu::indexed_sphere_pass> scene_pass;
    std::vector<sagan_render::scene_gpu::sphere_draw> scene_draws;
    bool claimed{};
    std::uint32_t target_width{};
    std::uint32_t target_height{};

    auto palette_index(const color value) const -> std::uint32_t
    {
      const auto found = std::find(palette.begin(), palette.end(), value);
      if (found == palette.end()) throw std::runtime_error("Draw color is absent from GPU palette");
      return static_cast<std::uint32_t>(found - palette.begin());
    }

    auto create_target(const std::uint32_t width, const std::uint32_t height) -> void
    {
      if (width == 0 || height == 0) return;
      if (target) SDL_ReleaseGPUTexture(device, target);
      if (download) SDL_ReleaseGPUTransferBuffer(device, download);
      target = nullptr;
      download = nullptr;

      SDL_GPUTextureCreateInfo target_info{};
      target_info.type = SDL_GPU_TEXTURETYPE_2D;
      target_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      target_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
      target_info.width = width;
      target_info.height = height;
      target_info.layer_count_or_depth = 1;
      target_info.num_levels = 1;
      target_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
      target = SDL_CreateGPUTexture(device, &target_info);
      if (!target) fail("Could not create UI target");

      SDL_GPUTransferBufferCreateInfo download_info{};
      download_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
      download_info.size = width * height * bytes_per_pixel;
      download = SDL_CreateGPUTransferBuffer(device, &download_info);
      if (!download) fail("Could not create UI download buffer");
      target_width = width;
      target_height = height;
    }

  public:
    gpu_compositor(const std::string &title = "Sagan Render UI GPU Demo",
                   std::uint32_t requested_width = initial_canvas_width,
                   std::uint32_t requested_height = initial_canvas_height)
    {
      if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL video");
      if (const char *width_value = std::getenv("SAGAN_RENDER_LOGICAL_WIDTH"))
        requested_width = static_cast<std::uint32_t>(std::strtoul(width_value, nullptr, 10));
      if (const char *height_value = std::getenv("SAGAN_RENDER_LOGICAL_HEIGHT"))
        requested_height = static_cast<std::uint32_t>(std::strtoul(height_value, nullptr, 10));
      window = SDL_CreateWindow(title.c_str(), requested_width, requested_height,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
      if (!window) fail("Could not create demo window");
      if (!SDL_SetWindowMinimumSize(window, 640, 400))
        fail("Could not set demo minimum size");
      device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
                                   SDL_GPU_SHADERFORMAT_MSL, false, platform_driver());
      if (!device) fail("Could not create GPU device");
      if (!SDL_ClaimWindowForGPUDevice(device, window)) fail("Could not claim demo window");
      claimed = true;

      SDL_GPUTextureCreateInfo palette_info{};
      palette_info.type = SDL_GPU_TEXTURETYPE_2D;
      palette_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      palette_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
      palette_info.width = palette.size();
      palette_info.height = 1;
      palette_info.layer_count_or_depth = 1;
      palette_info.num_levels = 1;
      palette_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
      palette_texture = SDL_CreateGPUTexture(device, &palette_info);
      if (!palette_texture) fail("Could not create UI palette texture");
      create_target(requested_width, requested_height);

      SDL_GPUTransferBufferCreateInfo upload_info{};
      upload_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
      upload_info.size = palette.size() * bytes_per_pixel;
      SDL_GPUTransferBuffer *upload = SDL_CreateGPUTransferBuffer(device, &upload_info);
      if (!upload) fail("Could not create UI palette upload buffer");
      auto *mapped = static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(device, upload, false));
      if (!mapped) fail("Could not map UI palette upload buffer");
      std::memcpy(mapped, palette.data(), upload_info.size);
      SDL_UnmapGPUTransferBuffer(device, upload);
      SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire palette command buffer");
      SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
      const SDL_GPUTextureTransferInfo source{upload, 0, static_cast<std::uint32_t>(palette.size()), 1};
      const SDL_GPUTextureRegion destination{palette_texture, 0, 0, 0, 0, 0,
                                             static_cast<std::uint32_t>(palette.size()), 1, 1};
      SDL_UploadToGPUTexture(copy, &source, &destination, false);
      SDL_EndGPUCopyPass(copy);
      SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
      if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
        fail("Could not upload UI palette");
      SDL_ReleaseGPUFence(device, fence);
      SDL_ReleaseGPUTransferBuffer(device, upload);
    }

    ~gpu_compositor()
    {
      if (device) SDL_WaitForGPUIdle(device);
      scene_pass.reset();
      if (download) SDL_ReleaseGPUTransferBuffer(device, download);
      if (palette_texture) SDL_ReleaseGPUTexture(device, palette_texture);
      if (target) SDL_ReleaseGPUTexture(device, target);
      if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
      if (device) SDL_DestroyGPUDevice(device);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }

    auto native_window() const -> SDL_Window * { return window; }
    auto driver() const -> const char * { return SDL_GetGPUDeviceDriver(device); }

    auto begin_scene() -> void { scene_draws.clear(); }

    auto mesh_sphere(sagan_render::scene_gpu::sphere_draw draw) -> void
    {
      if (!scene_pass)
        scene_pass = std::make_unique<sagan_render::scene_gpu::indexed_sphere_pass>(device);
      scene_draws.push_back(std::move(draw));
    }

    auto resize(const std::uint32_t width, const std::uint32_t height) -> bool
    {
      if (width == 0 || height == 0 ||
          (width == target_width && height == target_height)) return false;
      SDL_WaitForGPUIdle(device);
      create_target(width, height);
      return true;
    }

    auto render(const draw_list &list, const bool repaint,
                const std::string &capture = {}) -> void
    {
      SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire UI command buffer");
      if (repaint)
      {
        SDL_GPUColorTargetInfo color_target{};
        color_target.texture = target;
        color_target.clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
        color_target.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *clear = SDL_BeginGPURenderPass(commands, &color_target, 1, nullptr);
        if (!clear) fail("Could not clear UI target");
        SDL_EndGPURenderPass(clear);

        for (const fill_rectangle &fill : list.fills())
        {
          const std::uint32_t index = palette_index(fill.paint);
          const auto x = static_cast<std::uint32_t>(std::lround(fill.bounds.x));
          const auto y = static_cast<std::uint32_t>(std::lround(fill.bounds.y));
          const auto width = static_cast<std::uint32_t>(std::lround(fill.bounds.width));
          const auto height = static_cast<std::uint32_t>(std::lround(fill.bounds.height));
          if (width == 0 || height == 0) continue;
          SDL_GPUBlitInfo blit{};
          blit.source = {palette_texture, 0, 0, index, 0, 1, 1};
          blit.destination = {target, 0, 0, x, y, width, height};
          blit.load_op = SDL_GPU_LOADOP_LOAD;
          blit.filter = SDL_GPU_FILTER_NEAREST;
          SDL_BlitGPUTexture(commands, &blit);
        }
        if (scene_pass && !scene_draws.empty())
        {
          const sagan::render::LightingUniform lighting{
            {0.28F, 0.28F, 0.3F, 0.0F},
            {-0.45F, 0.55F, 0.7F, 0.95F},
            {1.0F, 0.95F, 0.86F, 0.0F}};
          std::size_t first{};
          while (first < scene_draws.size())
          {
            std::size_t last = first + 1;
            const auto viewport = scene_draws[first].target;
            while (last < scene_draws.size() &&
                   scene_draws[last].target.x == viewport.x &&
                   scene_draws[last].target.y == viewport.y &&
                   scene_draws[last].target.width == viewport.width &&
                   scene_draws[last].target.height == viewport.height)
              ++last;
            const std::vector<sagan_render::scene_gpu::sphere_draw> view_draws{
              scene_draws.begin() + static_cast<std::ptrdiff_t>(first),
              scene_draws.begin() + static_cast<std::ptrdiff_t>(last)};
            scene_pass->render(
              commands, target, target_width, target_height, view_draws, lighting);
            first = last;
          }
        }
      }

      SDL_GPUTexture *swapchain{};
      std::uint32_t swapchain_width{};
      std::uint32_t swapchain_height{};
      if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window, &swapchain,
                                                  &swapchain_width, &swapchain_height))
        fail("Could not acquire UI swapchain texture");
      if (swapchain)
      {
        SDL_GPUBlitInfo present{};
        present.source = {target, 0, 0, 0, 0, target_width, target_height};
        present.destination = {swapchain, 0, 0, 0, 0, swapchain_width, swapchain_height};
        present.load_op = SDL_GPU_LOADOP_DONT_CARE;
        present.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &present);
      }

      if (!capture.empty())
      {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        const SDL_GPUTextureRegion source{target, 0, 0, 0, 0, 0,
                                          target_width, target_height, 1};
        const SDL_GPUTextureTransferInfo destination{download, 0, target_width, target_height};
        SDL_DownloadFromGPUTexture(copy, &source, &destination);
        SDL_EndGPUCopyPass(copy);
      }

      SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
      if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
        fail("Could not finish UI frame");
      SDL_ReleaseGPUFence(device, fence);
      if (capture.empty()) return;

      auto *pixels = static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(device, download, false));
      if (!pixels) fail("Could not map UI capture");
      std::uint32_t white_pixels{};
      std::uint32_t modal_pixels{};
      std::uint32_t earth_pixels{};
      for (std::uint32_t index = 0; index < target_width * target_height; ++index)
      {
        const color value{pixels[index * bytes_per_pixel], pixels[index * bytes_per_pixel + 1],
                          pixels[index * bytes_per_pixel + 2], pixels[index * bytes_per_pixel + 3]};
        if (value == white) ++white_pixels;
        if (value == modal) ++modal_pixels;
        if (value == earth ||
            (value.blue > value.red + 25 && value.blue > value.green + 20))
          ++earth_pixels;
      }
      const char *require_earth_value = std::getenv("SAGAN_RENDER_UI_REQUIRE_EARTH");
      const bool require_earth = !require_earth_value || std::string_view{require_earth_value} != "0";
      const char *minimum_white_value = std::getenv("SAGAN_RENDER_UI_MIN_WHITE");
      const std::uint32_t minimum_white = minimum_white_value && *minimum_white_value
        ? static_cast<std::uint32_t>(std::strtoul(minimum_white_value, nullptr, 10)) : 1000;
      if (white_pixels < minimum_white || (require_earth && earth_pixels < 200))
        throw std::runtime_error("UI capture is missing expected text or measured-scene pixels");
      SDL_Surface *surface = SDL_CreateSurfaceFrom(target_width, target_height,
        SDL_PIXELFORMAT_RGBA32, pixels, target_width * bytes_per_pixel);
      if (!surface || !SDL_SaveBMP(surface, capture.c_str())) fail("Could not save UI capture");
      SDL_DestroySurface(surface);
      SDL_UnmapGPUTransferBuffer(device, download);
      std::cout << "SAGAN_UI_PIXELS white=" << white_pixels
                << " modal=" << modal_pixels << " earth=" << earth_pixels << '\n';
    }
  };
}

#ifndef SAGAN_RENDER_UI_BRIDGE
int main()
{
  try
  {
    gpu_compositor gpu;
    bitmap_shaper shaper;
    input_router input;
    const char *paused_value = std::getenv("SAGAN_RENDER_START_PAUSED");
    bool modal_visible = paused_value && std::string_view{paused_value} == "1";
    bool running = true;
    bool first = true;
    bool controls_dirty = true;
    bool repaint = true;
    int logical_width = initial_canvas_width;
    int logical_height = initial_canvas_height;
    const char *capture_value = std::getenv("SAGAN_RENDER_UI_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value
      ? capture_value : "build/ui-gpu-demo/ui-gpu-demo.bmp";
    const char *resize_capture_value = std::getenv("SAGAN_RENDER_UI_RESIZE_CAPTURE_BMP");
    const std::string resize_capture = resize_capture_value && *resize_capture_value
      ? resize_capture_value : std::string{};
    bool resized_captured = false;
    bool synthetic_resize_pending = false;
    const char *autoclose_value = std::getenv("SAGAN_RENDER_AUTOCLOSE_MS");
    const std::uint64_t autoclose = autoclose_value && *autoclose_value
      ? std::strtoull(autoclose_value, nullptr, 10) : 0;
    const std::uint64_t deadline = autoclose == 0 ? 0 : SDL_GetTicks() + autoclose;

    while (running && (deadline == 0 || SDL_GetTicks() < deadline))
    {
      SDL_Event event{};
      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
          running = false;
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
          if (event.key.key == SDLK_ESCAPE) running = false;
          else if (event.key.key == SDLK_P)
          {
            modal_visible = !modal_visible;
            controls_dirty = true;
            repaint = true;
          }
          else if (event.key.key == SDLK_TAB)
          {
            input.focus_next((event.key.mod & SDL_KMOD_SHIFT) != 0);
            repaint = true;
          }
          else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            input.activate_focused();
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
          input.pointer_down({event.button.x, event.button.y});
          repaint = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
          input.pointer_up({event.button.x, event.button.y});
        }
      }

      int current_width{};
      int current_height{};
      if (synthetic_resize_pending)
      {
        current_width = 800;
        current_height = 600;
        synthetic_resize_pending = false;
      }
      else
        SDL_GetWindowSize(gpu.native_window(), &current_width, &current_height);
      bool resized_this_frame = false;
      if (current_width > 0 && current_height > 0 &&
          (current_width != logical_width || current_height != logical_height))
      {
        logical_width = current_width;
        logical_height = current_height;
        gpu.resize(static_cast<std::uint32_t>(logical_width),
                   static_cast<std::uint32_t>(logical_height));
        controls_dirty = true;
        repaint = true;
        resized_this_frame = true;
      }

      if (const auto action = input.take_activation())
      {
        if (*action == "RESUME" || *action == "PAUSE")
        {
          modal_visible = !modal_visible;
          controls_dirty = true;
          repaint = true;
        }
        if (*action == "QUIT") running = false;
      }

      const size canvas_size{static_cast<scalar>(logical_width),
                             static_cast<scalar>(logical_height)};
      draw_list list{{0.0, 0.0, canvas_size.width, canvas_size.height}};
      const demo_layout layout = compose(list, shaper, input, modal_visible, canvas_size);
      if (controls_dirty)
      {
        input.set_controls(layout.controls);
        if (!input.focused_identifier()) input.focus_next();
        list = draw_list{{0.0, 0.0, canvas_size.width, canvas_size.height}};
        compose(list, shaper, input, modal_visible, canvas_size);
        controls_dirty = false;
        repaint = true;
      }
      const std::string frame_capture = first ? capture
        : (resized_this_frame && !resize_capture.empty() && !resized_captured
            ? resize_capture : std::string{});
      gpu.render(list, repaint, frame_capture);
      if (frame_capture == resize_capture && !resize_capture.empty()) resized_captured = true;
      if (first && !resize_capture.empty()) synthetic_resize_pending = true;
      first = false;
      repaint = false;
      SDL_Delay(16);
    }
    std::cout << "SAGAN_UI_DEMO driver=" << gpu.driver()
              << " initial_logical=960x540 solar_span_km=200000000"
              << " lunar_span_km=1000000 responsive=1 capture=" << capture
              << " cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
#else
namespace
{
  std::unique_ptr<gpu_compositor> bridge_gpu;
  std::unique_ptr<draw_list> bridge_list;
  bitmap_shaper bridge_shaper;
  std::vector<std::string> bridge_keys;
  std::vector<std::string> bridge_delayed_keys;
  std::uint64_t bridge_delayed_keys_at{};
  bool bridge_delayed_keys_delivered{};
  bool bridge_running{};
  bool bridge_pointer_down{};
  bool bridge_pointer_up{};
  bool bridge_pointer_test_override{};
  bool bridge_orbit_dragging{};
  double bridge_pointer_x{};
  double bridge_pointer_y{};
  double bridge_orbit_delta_x{};
  double bridge_orbit_delta_y{};
  double bridge_scroll_y{};
  std::uint64_t bridge_deadline{};
  std::uint64_t bridge_started_at{};
  double bridge_elapsed_override{-1.0};
  bool bridge_captured{};
  std::string bridge_driver;
  std::int64_t bridge_width{};
  std::int64_t bridge_height{};

  auto bridge_color(const std::int64_t red, const std::int64_t green,
                    const std::int64_t blue) -> color
  {
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
      throw std::invalid_argument("UI color channels must be between 0 and 255");
    return {static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green),
            static_cast<std::uint8_t>(blue), 255};
  }

  auto remember_key(const std::string &value) -> void
  {
    if (std::find(bridge_keys.begin(), bridge_keys.end(), value) == bridge_keys.end())
      bridge_keys.push_back(value);
  }

  auto remember_keys(const std::string_view values) -> void
  {
    std::size_t first{};
    while (first < values.size())
    {
      const std::size_t comma = values.find(',', first);
      const std::size_t last = comma == std::string_view::npos ? values.size() : comma;
      if (last > first) remember_key(std::string{values.substr(first, last - first)});
      if (comma == std::string_view::npos) break;
      first = comma + 1;
    }
  }

  auto update_bridge_size() -> void
  {
    int width{};
    int height{};
    SDL_GetWindowSize(bridge_gpu->native_window(), &width, &height);
    if (width > 0 && height > 0)
    {
      bridge_width = width;
      bridge_height = height;
      bridge_gpu->resize(static_cast<std::uint32_t>(width),
                         static_cast<std::uint32_t>(height));
    }
  }
}

auto sagan_5f5f72656e6465725f75695f6f70656e(
  const std::string &title, const std::int64_t width, const std::int64_t height) -> bool
{
  if (width < 640 || height < 400) throw std::invalid_argument("UI window is too small");
  bridge_gpu = std::make_unique<gpu_compositor>(
    title, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
  bridge_driver = bridge_gpu->driver();
  bridge_running = true;
  bridge_captured = false;
  bridge_keys.clear();
  bridge_delayed_keys.clear();
  bridge_delayed_keys_at = 0;
  bridge_delayed_keys_delivered = false;
  update_bridge_size();
  const char *autoclose_value = std::getenv("SAGAN_RENDER_AUTOCLOSE_MS");
  const std::uint64_t autoclose = autoclose_value && *autoclose_value
    ? std::strtoull(autoclose_value, nullptr, 10) : 0;
  bridge_deadline = autoclose == 0 ? 0 : SDL_GetTicks() + autoclose;
  bridge_started_at = SDL_GetTicks();
  const char *elapsed_value = std::getenv("SAGAN_RENDER_ELAPSED_SECONDS");
  bridge_elapsed_override = elapsed_value && *elapsed_value
    ? std::strtod(elapsed_value, nullptr) : -1.0;
  const char *test_key = std::getenv("SAGAN_RENDER_TEST_KEY");
  if (test_key && *test_key) remember_key(test_key);
  const char *test_keys = std::getenv("SAGAN_RENDER_TEST_KEYS");
  if (test_keys && *test_keys) remember_keys(test_keys);
  const char *delayed_keys = std::getenv("SAGAN_RENDER_TEST_DELAYED_KEYS");
  const char *delayed_keys_at = std::getenv("SAGAN_RENDER_TEST_DELAYED_KEYS_AFTER_MS");
  if (delayed_keys && *delayed_keys && delayed_keys_at && *delayed_keys_at)
  {
    std::size_t first{};
    const std::string_view values{delayed_keys};
    while (first < values.size())
    {
      const std::size_t comma = values.find(',', first);
      const std::size_t last = comma == std::string_view::npos ? values.size() : comma;
      if (last > first) bridge_delayed_keys.emplace_back(values.substr(first, last - first));
      if (comma == std::string_view::npos) break;
      first = comma + 1;
    }
    bridge_delayed_keys_at = std::strtoull(delayed_keys_at, nullptr, 10);
  }
  const char *test_pointer_x = std::getenv("SAGAN_RENDER_TEST_POINTER_X");
  const char *test_pointer_y = std::getenv("SAGAN_RENDER_TEST_POINTER_Y");
  if (test_pointer_x && *test_pointer_x && test_pointer_y && *test_pointer_y)
  {
    bridge_pointer_test_override = true;
    bridge_pointer_x = std::strtod(test_pointer_x, nullptr);
    bridge_pointer_y = std::strtod(test_pointer_y, nullptr);
    const char *pointer_action = std::getenv("SAGAN_RENDER_TEST_POINTER_ACTION");
    if (pointer_action && std::string_view{pointer_action} == "down")
      bridge_pointer_down = true;
    else if (!pointer_action || std::string_view{pointer_action} != "move")
    {
      bridge_pointer_down = true;
      bridge_pointer_up = true;
    }
  }
  const char *orbit_x = std::getenv("SAGAN_RENDER_TEST_ORBIT_DX");
  const char *orbit_y = std::getenv("SAGAN_RENDER_TEST_ORBIT_DY");
  if (orbit_x && *orbit_x) bridge_orbit_delta_x = std::strtod(orbit_x, nullptr);
  if (orbit_y && *orbit_y) bridge_orbit_delta_y = std::strtod(orbit_y, nullptr);
  const char *scroll_y = std::getenv("SAGAN_RENDER_TEST_SCROLL_Y");
  if (scroll_y && *scroll_y) bridge_scroll_y = std::strtod(scroll_y, nullptr);
  return true;
}

auto sagan_5f5f72656e6465725f75695f706f6c6c() -> bool
{
  if (!bridge_gpu || !bridge_running) return false;
  if (!bridge_delayed_keys_delivered && !bridge_delayed_keys.empty() &&
      SDL_GetTicks() - bridge_started_at >= bridge_delayed_keys_at)
  {
    for (const auto &key : bridge_delayed_keys) remember_key(key);
    bridge_delayed_keys_delivered = true;
  }
  SDL_Event event{};
  while (SDL_PollEvent(&event))
  {
    if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
      bridge_running = false;
    else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
    {
      if (event.key.key == SDLK_ESCAPE) { remember_key("escape"); bridge_running = false; }
      else if (event.key.key == SDLK_TAB)
        remember_key((event.key.mod & SDL_KMOD_SHIFT) != 0 ? "shift-tab" : "tab");
      else if (event.key.key == SDLK_RETURN) remember_key("enter");
      else if (event.key.key == SDLK_SPACE) remember_key("space");
      else if (event.key.key == SDLK_P) remember_key("p");
      else if (event.key.key == SDLK_F) remember_key("f");
      else if (event.key.key == SDLK_R) remember_key("r");
      else if (event.key.key == SDLK_LEFT) remember_key("left");
      else if (event.key.key == SDLK_RIGHT) remember_key("right");
      else if (event.key.key == SDLK_UP) remember_key("up");
      else if (event.key.key == SDLK_DOWN) remember_key("down");
      else if (event.key.key == SDLK_A) remember_key("a");
      else if (event.key.key == SDLK_D) remember_key("d");
      else if (event.key.key == SDLK_W) remember_key("w");
      else if (event.key.key == SDLK_S) remember_key("s");
      else if (event.key.key == SDLK_Q) remember_key("q");
      else if (event.key.key == SDLK_E) remember_key("e");
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
      bridge_pointer_x = event.button.x;
      bridge_pointer_y = event.button.y;
      if (event.button.button == SDL_BUTTON_LEFT) bridge_pointer_down = true;
      if (event.button.button == SDL_BUTTON_RIGHT)
      {
        bridge_orbit_dragging = true;
        SDL_CaptureMouse(true);
      }
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
      bridge_pointer_x = event.button.x;
      bridge_pointer_y = event.button.y;
      if (event.button.button == SDL_BUTTON_LEFT) bridge_pointer_up = true;
      if (event.button.button == SDL_BUTTON_RIGHT)
      {
        bridge_orbit_dragging = false;
        SDL_CaptureMouse(false);
      }
    }
    else if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
      if (!bridge_pointer_test_override)
      {
        bridge_pointer_x = event.motion.x;
        bridge_pointer_y = event.motion.y;
      }
      if (bridge_orbit_dragging)
      {
        bridge_orbit_delta_x += event.motion.xrel;
        bridge_orbit_delta_y += event.motion.yrel;
      }
    }
    else if (event.type == SDL_EVENT_MOUSE_WHEEL)
    {
      bridge_scroll_y += event.wheel.y;
    }
  }
  update_bridge_size();
  if (bridge_deadline != 0 && SDL_GetTicks() >= bridge_deadline) bridge_running = false;
  return bridge_running;
}

auto sagan_5f5f72656e6465725f75695f636c6f7365() -> void
{
  bridge_list.reset();
  bridge_gpu.reset();
  const char *kind_value = std::getenv("SAGAN_RENDER_DEMO_KIND");
  if (kind_value && std::string_view{kind_value} == "loading")
    std::cout << "SAGAN_LOADING_DEMO language=sagan driver=" << bridge_driver
              << " logical=" << bridge_width << 'x' << bridge_height
              << " cleanup=1\n";
  else if (kind_value && std::string_view{kind_value} == "toolbar")
    std::cout << "SAGAN_TOOLBAR_DEMO language=sagan driver=" << bridge_driver
              << " logical=" << bridge_width << 'x' << bridge_height
              << " cleanup=1\n";
  else if (kind_value && std::string_view{kind_value} == "scene")
    std::cout << "SAGAN_SCENE_DEMO language=sagan driver=" << bridge_driver
              << " logical=" << bridge_width << 'x' << bridge_height
              << " precision_origin_metres=1e15 cleanup=1\n";
  else
    std::cout << "SAGAN_UI_DEMO language=sagan driver=" << bridge_driver
              << " logical=" << bridge_width << 'x' << bridge_height
              << " solar_span_km=200000000 lunar_span_km=1000000 cleanup=1\n";
}

auto sagan_5f5f72656e6465725f75695f7769647468() -> double
{
  return static_cast<double>(bridge_width);
}
auto sagan_5f5f72656e6465725f75695f686569676874() -> double
{
  return static_cast<double>(bridge_height);
}

auto sagan_5f5f72656e6465725f656c61707365645f7365636f6e6473() -> double
{
  if (bridge_elapsed_override >= 0.0) return bridge_elapsed_override;
  return static_cast<double>(SDL_GetTicks() - bridge_started_at) / 1000.0;
}

auto sagan_5f5f72656e6465725f75695f626567696e() -> void
{
  if (!bridge_gpu) throw std::runtime_error("UI begin requires an open window");
  update_bridge_size();
  bridge_list = std::make_unique<draw_list>(
    rectangle{0.0, 0.0, static_cast<double>(bridge_width), static_cast<double>(bridge_height)});
  bridge_gpu->begin_scene();
}

auto sagan_5f5f72656e6465725f75695f66696c6c(
  const double x, const double y, const double width, const double height,
  const std::int64_t red, const std::int64_t green, const std::int64_t blue) -> void
{
  if (!bridge_list) throw std::runtime_error("UI fill requires begin");
  bridge_list->fill({x, y, width, height}, bridge_color(red, green, blue));
}

auto sagan_5f5f72656e6465725f75695f6c696e65(
  const double first_x, const double first_y,
  const double second_x, const double second_y, const double thickness,
  const double clip_x, const double clip_y,
  const double clip_width, const double clip_height,
  const std::int64_t red, const std::int64_t green, const std::int64_t blue) -> void
{
  if (!bridge_list) throw std::runtime_error("UI line requires begin");
  if (!std::isfinite(first_x) || !std::isfinite(first_y) ||
      !std::isfinite(second_x) || !std::isfinite(second_y) ||
      !std::isfinite(thickness) || thickness <= 0.0 ||
      !std::isfinite(clip_x) || !std::isfinite(clip_y) ||
      !std::isfinite(clip_width) || !std::isfinite(clip_height) ||
      clip_width <= 0.0 || clip_height <= 0.0)
    throw std::invalid_argument("UI line requires finite coordinates and positive thickness");
  const double delta_x = second_x - first_x;
  const double delta_y = second_y - first_y;
  const auto steps = static_cast<std::uint32_t>(
    std::max(1.0, std::ceil(std::max(std::abs(delta_x), std::abs(delta_y)))));
  const color paint = bridge_color(red, green, blue);
  for (std::uint32_t step = 0; step <= steps; ++step)
  {
    const double amount = static_cast<double>(step) / static_cast<double>(steps);
    const double x = first_x + delta_x * amount;
    const double y = first_y + delta_y * amount;
    if (x >= clip_x && y >= clip_y &&
        x < clip_x + clip_width && y < clip_y + clip_height)
      bridge_list->fill({x - thickness / 2.0, y - thickness / 2.0,
                         thickness, thickness}, paint);
  }
}

auto sagan_5f5f72656e6465725f75695f74657874(
  const double x, const double y, const std::string &value, const double height,
  const std::int64_t red, const std::int64_t green, const std::int64_t blue) -> void
{
  if (!bridge_list) throw std::runtime_error("UI text requires begin");
  draw_text(*bridge_list, bridge_shaper, {x, y}, value, height,
            bridge_color(red, green, blue));
}

auto sagan_5f5f72656e6465725f75695f6d6573685f737068657265(
  const double viewport_x, const double viewport_y,
  const double viewport_width, const double viewport_height,
  const double camera_x, const double camera_y, const double camera_z,
  const double forward_x, const double forward_y, const double forward_z,
  const double up_x, const double up_y, const double up_z,
  const double field_of_view, const double near_distance,
  const double far_distance, const double body_x, const double body_y,
  const double body_z, const double radius, const std::int64_t appearance,
  const double minimum_radius, const double center_offset_x,
  const double center_offset_y, const bool selected) -> void
{
  if (!bridge_gpu || !bridge_list)
    throw std::runtime_error("UI mesh sphere requires begin");
  const sagan_render::scene::direction3 forward{forward_x, forward_y, forward_z};
  const sagan_render::scene::length3 relative{
    body_x - camera_x, body_y - camera_y, body_z - camera_z};
  // The current sphere transform projects from its center. A very large body
  // may intersect the near half-space while its center is behind the camera;
  // that case is not a valid perspective mesh draw and must be culled rather
  // than forwarded to prepare_sphere_draw as a fatal contract violation.
  if (!sagan_render::scene::mesh_center_is_projectable(
        relative, forward, near_distance))
    return;
  sagan::render::MaterialUniform material{};
  if (appearance == 1)
    material = {{0.95F, 0.48F, 0.03F, 1.0F}, {0.43F, 0.22F, 0.01F, 0.55F}};
  else if (appearance == 2)
    material = {{0.04F, 0.18F, 0.92F, 1.0F}, {0.0F, 0.01F, 0.03F, 0.7F}};
  else
    material = {{0.48F, 0.52F, 0.58F, 1.0F}, {0.0F, 0.0F, 0.0F, 0.85F}};
  if (selected)
  {
    material.emissive_linear_and_roughness.x += 0.04F;
    material.emissive_linear_and_roughness.y += 0.08F;
    material.emissive_linear_and_roughness.z += 0.14F;
  }
  bridge_gpu->mesh_sphere({
    {static_cast<std::uint64_t>(appearance), {body_x, body_y, body_z}, radius, ""},
    {{camera_x, camera_y, camera_z}, {forward_x, forward_y, forward_z},
     {up_x, up_y, up_z}, field_of_view, near_distance, far_distance},
    {static_cast<float>(viewport_x), static_cast<float>(viewport_y),
     static_cast<float>(viewport_width), static_cast<float>(viewport_height)},
    {minimum_radius, center_offset_x, center_offset_y}, material});
}

auto sagan_5f5f72656e6465725f75695f70726573656e74() -> void
{
  if (!bridge_gpu || !bridge_list) throw std::runtime_error("UI present requires begin");
  const char *capture_value = std::getenv("SAGAN_RENDER_UI_CAPTURE_BMP");
  const char *capture_delay_value = std::getenv("SAGAN_RENDER_UI_CAPTURE_AFTER_MS");
  const std::uint64_t capture_delay = capture_delay_value && *capture_delay_value
    ? std::strtoull(capture_delay_value, nullptr, 10) : 0;
  const bool capture_ready = SDL_GetTicks() - bridge_started_at >= capture_delay;
  const std::string capture = !bridge_captured && capture_ready && capture_value && *capture_value
    ? capture_value : std::string{};
  bridge_gpu->render(*bridge_list, true, capture);
  if (!capture.empty()) bridge_captured = true;
}

auto sagan_5f5f72656e6465725f75695f6b65795f70726573736564(
  const std::string &key) -> bool
{
  const auto found = std::find(bridge_keys.begin(), bridge_keys.end(), key);
  if (found == bridge_keys.end()) return false;
  bridge_keys.erase(found);
  return true;
}

auto sagan_5f5f72656e6465725f75695f706f696e7465725f70726573736564() -> bool
{
  const bool result = bridge_pointer_down;
  bridge_pointer_down = false;
  return result;
}

auto sagan_5f5f72656e6465725f75695f706f696e7465725f72656c6561736564() -> bool
{
  const bool result = bridge_pointer_up;
  bridge_pointer_up = false;
  return result;
}

auto sagan_5f5f72656e6465725f75695f706f696e7465725f78() -> double
{
  return bridge_pointer_x;
}

auto sagan_5f5f72656e6465725f75695f706f696e7465725f79() -> double
{
  return bridge_pointer_y;
}

auto sagan_5f5f72656e6465725f75695f6f726269745f64656c74615f78() -> double
{
  const double result = bridge_orbit_delta_x;
  bridge_orbit_delta_x = 0.0;
  return result;
}

auto sagan_5f5f72656e6465725f75695f6f726269745f64656c74615f79() -> double
{
  const double result = bridge_orbit_delta_y;
  bridge_orbit_delta_y = 0.0;
  return result;
}

auto sagan_5f5f72656e6465725f75695f7363726f6c6c5f79() -> double
{
  const double result = bridge_scroll_y;
  bridge_scroll_y = 0.0;
  return result;
}
#endif
