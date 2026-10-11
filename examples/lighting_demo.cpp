#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "../libraries/render/native/scene_gpu_pass.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
  constexpr std::uint32_t width = 960;
  constexpr std::uint32_t height = 540;
  constexpr std::uint32_t pixel_bytes = 4;

  auto fail(const std::string &message) -> void
  {
    throw std::runtime_error(message + ": " + SDL_GetError());
  }

  auto driver() -> const char *
  {
#if defined(_WIN32)
    return "direct3d12";
#elif defined(__APPLE__)
    return "metal";
#else
    return "vulkan";
#endif
  }

  auto light_rig(const std::uint32_t count) -> sagan::render::LightingUniform
  {
    sagan::render::LightingUniform result{};
    result.ambient_linear = count == 0
      ? sagan::render::Float4{0.24F, 0.24F, 0.24F, 0.0F}
      : sagan::render::Float4{0.055F, 0.055F, 0.065F, 0.0F};
    result.direction_to_light_and_intensity = {{
      {-0.45F, 0.55F, 0.7F, 1.0F},
      {0.75F, -0.15F, 0.45F, 0.7F},
      {}, {}}};
    result.light_color_linear = {{
      {1.0F, 0.72F, 0.42F, 0.0F},
      {0.24F, 0.48F, 1.0F, 0.0F},
      {}, {}}};
    result.light_count_and_padding = {
      static_cast<float>(count), 0.0F, 0.0F, 0.0F};
    return result;
  }

  struct Evidence
  {
    std::uint32_t ambient{};
    std::uint32_t warm{};
    std::uint32_t cool{};
  };

  auto inspect(const std::uint8_t *pixels) -> Evidence
  {
    Evidence result{};
    for (std::uint32_t y = 80; y < 470; ++y)
    {
      for (std::uint32_t x = 0; x < width; ++x)
      {
        const auto offset = (y * width + x) * pixel_bytes;
        const auto red = pixels[offset];
        const auto green = pixels[offset + 1];
        const auto blue = pixels[offset + 2];
        if (x < 320 && red > 35 && green > 35 && blue > 35 &&
            std::abs(static_cast<int>(red) - static_cast<int>(blue)) < 18)
          ++result.ambient;
        if (x >= 320 && x < 640 && red > blue + 30 && red > green + 12)
          ++result.warm;
        if (x >= 640 && blue > red + 20 && blue > green + 12)
          ++result.cool;
      }
    }
    if (result.ambient < 1000 || result.warm < 1000 || result.cool < 250)
      throw std::runtime_error("Lighting demo capture lacks the three reviewed light responses");
    return result;
  }

  auto environment_u32(const char *name, const std::uint32_t fallback) -> std::uint32_t
  {
    const char *value = std::getenv(name);
    if (!value || !*value) return fallback;
    const auto parsed = std::strtoul(value, nullptr, 10);
    return parsed > UINT32_MAX ? fallback : static_cast<std::uint32_t>(parsed);
  }
}

int main()
{
  SDL_Window *window{};
  SDL_GPUDevice *device{};
  SDL_GPUTexture *target{};
  SDL_GPUTransferBuffer *download{};
  bool claimed{};
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize lighting demo");
    window = SDL_CreateWindow(
      "Sagan Lighting - Ambient | Warm Key | Key + Cool Fill",
      width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);
    if (!window) fail("Could not create lighting demo window");
    device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_MSL, false, driver());
    if (!device) fail("Could not create lighting demo device");
    if (!SDL_ClaimWindowForGPUDevice(device, window))
      fail("Could not claim lighting demo window");
    claimed = true;

    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    texture.width = width;
    texture.height = height;
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;
    texture.sample_count = SDL_GPU_SAMPLECOUNT_1;
    target = SDL_CreateGPUTexture(device, &texture);
    SDL_GPUTransferBufferCreateInfo transfer{
      SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, width * height * pixel_bytes, 0};
    download = SDL_CreateGPUTransferBuffer(device, &transfer);
    if (!target || !download) fail("Could not create lighting demo targets");

    auto sphere_pass = std::make_unique<sagan_render::scene_gpu::indexed_sphere_pass>(device);
    const sagan_render::scene::render_item sphere{
      1, {0.0, 0.0, 0.0}, 1.0, "Lighting reference sphere"};
    const sagan_render::scene::camera camera{
      {0.0, 0.0, 3.2}, {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
      0.85, 0.1, 20.0};
    const sagan::render::MaterialUniform neutral{
      {0.72F, 0.72F, 0.72F, 1.0F}, {0.0F, 0.0F, 0.0F, 0.7F}};
    const std::array<sagan_render::scene_gpu::target_viewport, 3> views{{
      {20, 70, 293, 400}, {333, 70, 294, 400}, {647, 70, 293, 400}}};
    const char *capture_value = std::getenv("SAGAN_RENDER_LIGHTING_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value ? capture_value : "";
    bool capture_pending = !capture.empty();

    auto render = [&]
    {
      auto *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire lighting demo commands");
      SDL_GPUColorTargetInfo clear_target{};
      clear_target.texture = target;
      clear_target.clear_color = {0.008F, 0.012F, 0.025F, 1.0F};
      clear_target.load_op = SDL_GPU_LOADOP_CLEAR;
      clear_target.store_op = SDL_GPU_STOREOP_STORE;
      auto *clear = SDL_BeginGPURenderPass(commands, &clear_target, 1, nullptr);
      SDL_EndGPURenderPass(clear);
      for (std::uint32_t index = 0; index < views.size(); ++index)
      {
        const std::vector<sagan_render::scene_gpu::sphere_draw> draws{{
          sphere, camera, views[index], neutral}};
        sphere_pass->render(commands, target, width, height, draws, light_rig(index));
      }

      SDL_GPUTexture *swapchain{};
      std::uint32_t swapchain_width{};
      std::uint32_t swapchain_height{};
      if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commands, window, &swapchain, &swapchain_width, &swapchain_height))
        fail("Could not acquire lighting demo swapchain");
      if (swapchain)
      {
        SDL_GPUBlitInfo present{};
        present.source = {target, 0, 0, 0, 0, width, height};
        present.destination = {
          swapchain, 0, 0, 0, 0, swapchain_width, swapchain_height};
        present.load_op = SDL_GPU_LOADOP_DONT_CARE;
        present.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(commands, &present);
      }
      if (capture_pending)
      {
        auto *copy = SDL_BeginGPUCopyPass(commands);
        const SDL_GPUTextureRegion source{
          target, 0, 0, 0, 0, 0, width, height, 1};
        const SDL_GPUTextureTransferInfo destination{
          download, 0, width, height};
        SDL_DownloadFromGPUTexture(copy, &source, &destination);
        SDL_EndGPUCopyPass(copy);
      }
      auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
      if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
        fail("Could not finish lighting demo frame");
      SDL_ReleaseGPUFence(device, fence);

      if (capture_pending)
      {
        auto *pixels = static_cast<std::uint8_t *>(
          SDL_MapGPUTransferBuffer(device, download, false));
        if (!pixels) fail("Could not map lighting demo capture");
        const auto evidence = inspect(pixels);
        auto *surface = SDL_CreateSurfaceFrom(
          width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * pixel_bytes);
        if (!surface || !SDL_SaveBMP(surface, capture.c_str()))
          fail("Could not save lighting demo capture");
        SDL_DestroySurface(surface);
        SDL_UnmapGPUTransferBuffer(device, download);
        capture_pending = false;
        std::cout << "LIGHTING_DEMO driver=" << SDL_GetGPUDeviceDriver(device)
                  << " panels=3 lights=0,1,2 ambient=" << evidence.ambient
                  << " warm=" << evidence.warm << " cool=" << evidence.cool << '\n';
      }
    };

    const auto autoclose_ms = environment_u32("SAGAN_RENDER_AUTOCLOSE_MS", 0);
    const auto started = SDL_GetTicks();
    bool running = true;
    while (running)
    {
      SDL_Event event{};
      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
          running = false;
      }
      render();
      if (autoclose_ms > 0 && SDL_GetTicks() - started >= autoclose_ms)
        running = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    SDL_WaitForGPUIdle(device);
    sphere_pass.reset();
    SDL_ReleaseGPUTransferBuffer(device, download);
    SDL_ReleaseGPUTexture(device, target);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "LIGHTING_DEMO cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    if (device) SDL_WaitForGPUIdle(device);
    if (download) SDL_ReleaseGPUTransferBuffer(device, download);
    if (target) SDL_ReleaseGPUTexture(device, target);
    if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
    if (device) SDL_DestroyGPUDevice(device);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
}
