#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "../libraries/render/native/ui_contract.hpp"
#include "../libraries/render/native/ui_draw_list.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
  using namespace sagan_render::ui;
  constexpr std::uint32_t canvas_width = 960;
  constexpr std::uint32_t canvas_height = 540;
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
  constexpr color white{245, 249, 255, 255};
  constexpr color muted{142, 164, 188, 255};
  constexpr color modal{36, 55, 82, 255};
  constexpr color button{38, 78, 116, 255};
  constexpr color button_focus{55, 125, 181, 255};
  constexpr std::array palette{
    background, panel, panel_light, scene, accent, focus_color, sun, earth,
    moon, white, muted, modal, button, button_focus
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

  auto compose(draw_list &list, bitmap_shaper &shaper, input_router &input,
               const bool modal_visible) -> demo_layout
  {
    const rectangle canvas{0.0, 0.0, canvas_width, canvas_height};
    list.fill(canvas, background);
    const auto columns = linear_layout(canvas, axis::horizontal,
      {{180.0, 208.0, 240.0, 0.0, 0.0}, {400.0, 700.0, 2000.0, 1.0, 0.0}},
      12.0, {16.0, 16.0, 16.0, 16.0});
    list.fill(columns[0], panel);
    list.fill(columns[1], scene);
    draw_text(list, shaper, {32.0, 34.0}, "SAGAN UI", 21.0, white);
    draw_text(list, shaper, {32.0, 72.0}, "GPU DEMO", 14.0, accent);
    draw_text(list, shaper, {32.0, 136.0}, "PHYSICAL SPAN", 10.0, muted);
    draw_text(list, shaper, {32.0, 157.0}, "400000 KM", 14.0, white);
    draw_text(list, shaper, {32.0, 216.0}, "TAB: FOCUS", 10.0, muted);
    draw_text(list, shaper, {32.0, 237.0}, "ENTER: ACTIVATE", 10.0, muted);
    draw_text(list, shaper, {32.0, 258.0}, "P: PAUSE", 10.0, muted);
    draw_text(list, shaper, {32.0, 279.0}, "ESC: QUIT", 10.0, muted);

    const rectangle viewport = inset(columns[1], {24.0, 56.0, 24.0, 58.0});
    list.push_clip(viewport);
    list.fill({viewport.x - 30.0, viewport.y + viewport.height - 2.0,
               viewport.width + 60.0, 2.0}, panel_light);
    const auto physical_view = orthographic_transform::with_horizontal_span(
      viewport, {0.0, 0.0}, 400000000.0);
    const point sun_at = physical_view.to_logical({0.0, 0.0});
    const point earth_at = physical_view.to_logical({145000000.0, 0.0});
    const point moon_at = physical_view.to_logical({160000000.0, 0.0});
    list.fill({sun_at.x - 30.0, sun_at.y - 30.0, 60.0, 60.0}, sun);
    list.fill({earth_at.x - 10.0, earth_at.y - 10.0, 20.0, 20.0}, earth);
    list.fill({moon_at.x - 4.0, moon_at.y - 4.0, 8.0, 8.0}, moon);
    list.pop_clip();
    draw_text(list, shaper, {columns[1].x + 24.0, columns[1].y + 20.0},
              "ORTHOGRAPHIC SCENE VIEW", 13.0, white);
    draw_text(list, shaper, {columns[1].x + 24.0, columns[1].y + columns[1].height - 30.0},
              "DISPLAY SCALE DOES NOT CHANGE KM", 10.0, muted);

    std::vector<control> controls;
    if (modal_visible)
    {
      const rectangle modal_bounds{390.0, 125.0, 360.0, 290.0};
      list.fill({modal_bounds.x - 4.0, modal_bounds.y - 4.0,
                 modal_bounds.width + 8.0, modal_bounds.height + 8.0}, focus_color);
      list.fill(modal_bounds, modal);
      draw_text(list, shaper, {430.0, 158.0}, "PAUSED", 24.0, white);
      const std::array<std::string, 3> labels{"RESUME", "SETTINGS", "QUIT"};
      for (std::size_t index = 0; index < labels.size(); ++index)
      {
        const rectangle bounds{450.0, 220.0 + 58.0 * static_cast<scalar>(index), 240.0, 42.0};
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
    bool claimed{};

    auto palette_index(const color value) const -> std::uint32_t
    {
      const auto found = std::find(palette.begin(), palette.end(), value);
      if (found == palette.end()) throw std::runtime_error("Draw color is absent from GPU palette");
      return static_cast<std::uint32_t>(found - palette.begin());
    }

  public:
    gpu_compositor()
    {
      if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL video");
      window = SDL_CreateWindow("Sagan Render UI GPU Demo", canvas_width, canvas_height,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
      if (!window) fail("Could not create demo window");
      device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
                                   SDL_GPU_SHADERFORMAT_MSL, false, platform_driver());
      if (!device) fail("Could not create GPU device");
      if (!SDL_ClaimWindowForGPUDevice(device, window)) fail("Could not claim demo window");
      claimed = true;

      SDL_GPUTextureCreateInfo target_info{};
      target_info.type = SDL_GPU_TEXTURETYPE_2D;
      target_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      target_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
      target_info.width = canvas_width;
      target_info.height = canvas_height;
      target_info.layer_count_or_depth = 1;
      target_info.num_levels = 1;
      target_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
      target = SDL_CreateGPUTexture(device, &target_info);
      if (!target) fail("Could not create UI target");

      SDL_GPUTextureCreateInfo palette_info = target_info;
      palette_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
      palette_info.width = palette.size();
      palette_info.height = 1;
      palette_texture = SDL_CreateGPUTexture(device, &palette_info);
      if (!palette_texture) fail("Could not create UI palette texture");

      SDL_GPUTransferBufferCreateInfo download_info{};
      download_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
      download_info.size = canvas_width * canvas_height * bytes_per_pixel;
      download = SDL_CreateGPUTransferBuffer(device, &download_info);
      if (!download) fail("Could not create UI download buffer");

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
        present.source = {target, 0, 0, 0, 0, canvas_width, canvas_height};
        present.destination = {swapchain, 0, 0, 0, 0, swapchain_width, swapchain_height};
        present.load_op = SDL_GPU_LOADOP_DONT_CARE;
        present.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &present);
      }

      if (!capture.empty())
      {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        const SDL_GPUTextureRegion source{target, 0, 0, 0, 0, 0,
                                          canvas_width, canvas_height, 1};
        const SDL_GPUTextureTransferInfo destination{download, 0, canvas_width, canvas_height};
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
      for (std::uint32_t index = 0; index < canvas_width * canvas_height; ++index)
      {
        const color value{pixels[index * bytes_per_pixel], pixels[index * bytes_per_pixel + 1],
                          pixels[index * bytes_per_pixel + 2], pixels[index * bytes_per_pixel + 3]};
        if (value == white) ++white_pixels;
        if (value == modal) ++modal_pixels;
        if (value == earth) ++earth_pixels;
      }
      if (white_pixels < 1000 || modal_pixels < 50000 || earth_pixels < 200)
        throw std::runtime_error("UI capture is missing expected text, modal, or scene pixels");
      SDL_Surface *surface = SDL_CreateSurfaceFrom(canvas_width, canvas_height,
        SDL_PIXELFORMAT_RGBA32, pixels, canvas_width * bytes_per_pixel);
      if (!surface || !SDL_SaveBMP(surface, capture.c_str())) fail("Could not save UI capture");
      SDL_DestroySurface(surface);
      SDL_UnmapGPUTransferBuffer(device, download);
      std::cout << "SAGAN_UI_PIXELS white=" << white_pixels
                << " modal=" << modal_pixels << " earth=" << earth_pixels << '\n';
    }
  };
}

int main()
{
  try
  {
    gpu_compositor gpu;
    bitmap_shaper shaper;
    input_router input;
    bool modal_visible = true;
    bool running = true;
    bool first = true;
    bool controls_dirty = true;
    bool repaint = true;
    const char *capture_value = std::getenv("SAGAN_RENDER_UI_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value
      ? capture_value : "build/ui-gpu-demo/ui-gpu-demo.bmp";
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
          int width{};
          int height{};
          SDL_GetWindowSize(gpu.native_window(), &width, &height);
          input.pointer_down({event.button.x * canvas_width / std::max(width, 1),
                              event.button.y * canvas_height / std::max(height, 1)});
          repaint = true;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
          int width{};
          int height{};
          SDL_GetWindowSize(gpu.native_window(), &width, &height);
          input.pointer_up({event.button.x * canvas_width / std::max(width, 1),
                            event.button.y * canvas_height / std::max(height, 1)});
        }
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

      draw_list list{{0.0, 0.0, canvas_width, canvas_height}};
      const demo_layout layout = compose(list, shaper, input, modal_visible);
      if (controls_dirty)
      {
        input.set_controls(layout.controls);
        if (!input.focused_identifier()) input.focus_next();
        list = draw_list{{0.0, 0.0, canvas_width, canvas_height}};
        compose(list, shaper, input, modal_visible);
        controls_dirty = false;
        repaint = true;
      }
      gpu.render(list, repaint, first ? capture : std::string{});
      first = false;
      repaint = false;
      SDL_Delay(16);
    }
    std::cout << "SAGAN_UI_DEMO driver=" << gpu.driver()
              << " logical=960x540 physical_span_km=400000 capture=" << capture
              << " cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
