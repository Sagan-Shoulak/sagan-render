#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "../../libraries/render/native/scene_gpu_pass.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
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

  auto material(const float red, const float green, const float blue,
                const float emission = 0.0F) -> sagan::render::MaterialUniform
  {
    return {{red, green, blue, 1.0F},
            {red * emission, green * emission, blue * emission, 0.7F}};
  }

  struct evidence
  {
    std::uint32_t sun{};
    std::uint32_t earth{};
    std::uint32_t moon{};
  };

  auto inspect(const std::uint8_t *pixels) -> evidence
  {
    evidence result{};
    for (std::uint32_t index = 0; index < width * height; ++index)
    {
      const auto red = pixels[index * pixel_bytes];
      const auto green = pixels[index * pixel_bytes + 1];
      const auto blue = pixels[index * pixel_bytes + 2];
      if (red > 120 && green > 55 && red > blue * 2) ++result.sun;
      if (blue > 65 && blue > red * 2 && blue > green + 20) ++result.earth;
      if (red > 45 && green > 45 && blue > 45 &&
          std::abs(static_cast<int>(red) - static_cast<int>(green)) < 30 &&
          std::abs(static_cast<int>(green) - static_cast<int>(blue)) < 30)
        ++result.moon;
    }
    return result;
  }
}

int main()
{
  SDL_Window *window{};
  SDL_GPUDevice *device{};
  SDL_GPUTexture *color{};
  SDL_GPUTransferBuffer *download{};
  bool claimed{};
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize 3D scene probe");
    window = SDL_CreateWindow("Sagan 3D Sun-Earth-Moon", width, height,
                              SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) fail("Could not create 3D scene window");
    device = SDL_CreateGPUDevice(
      SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_MSL, false, driver());
    if (!device) fail("Could not create 3D scene device");
    if (!SDL_ClaimWindowForGPUDevice(device, window))
      fail("Could not claim 3D scene window");
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
    color = SDL_CreateGPUTexture(device, &texture);
    SDL_GPUTransferBufferCreateInfo transfer{
      SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, width * height * pixel_bytes, 0};
    download = SDL_CreateGPUTransferBuffer(device, &transfer);
    if (!color || !download) fail("Could not create 3D scene targets");

    auto sphere_pass = std::make_unique<sagan_render::scene_gpu::indexed_sphere_pass>(device);
    constexpr double origin = 1.0e15;
    constexpr double earth_distance = 149597870700.0;
    constexpr double moon_distance = 384400000.0;
    const sagan_render::scene::render_item sun{
      1, {origin, origin, origin}, 696340000.0, "Sun"};
    const sagan_render::scene::render_item earth{
      2, {origin + earth_distance, origin, origin}, 6371000.0, "Earth"};
    const sagan_render::scene::render_item moon{
      3, {origin + earth_distance + moon_distance, origin, origin},
      1737400.0, "Moon"};
    const sagan_render::scene::camera system_camera{
      {origin + earth_distance / 2.0, origin, origin + 200000000000.0},
      {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
      1.0471975511965976, 1000000000.0, 400000000000.0};
    const sagan_render::scene::camera lunar_camera{
      {origin + earth_distance + moon_distance / 2.0, origin,
       origin + 1000000000.0},
      {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
      1.0471975511965976, 1000000.0, 2000000000.0};
    const sagan_render::scene_gpu::target_viewport system_view{24, 24, 912, 492};
    const sagan_render::scene_gpu::target_viewport lunar_view{620, 330, 300, 160};
    const auto sun_material = material(0.95F, 0.48F, 0.03F, 0.45F);
    const auto earth_material = material(0.04F, 0.18F, 0.92F);
    const auto moon_material = material(0.48F, 0.52F, 0.58F);
    const sagan::render::LightingUniform lighting{
      {0.28F, 0.28F, 0.3F, 0.0F},
      {-0.45F, 0.55F, 0.7F, 0.95F},
      {1.0F, 0.95F, 0.86F, 0.0F}};

    auto *commands = SDL_AcquireGPUCommandBuffer(device);
    if (!commands) fail("Could not acquire 3D scene commands");
    SDL_GPUColorTargetInfo clear_target{};
    clear_target.texture = color;
    clear_target.clear_color = {0.008F, 0.016F, 0.035F, 1.0F};
    clear_target.load_op = SDL_GPU_LOADOP_CLEAR;
    clear_target.store_op = SDL_GPU_STOREOP_STORE;
    auto *clear = SDL_BeginGPURenderPass(commands, &clear_target, 1, nullptr);
    SDL_EndGPURenderPass(clear);

    const std::vector<sagan_render::scene_gpu::sphere_draw> system_draws{
      {sun, system_camera, system_view, {28.0}, sun_material},
      {earth, system_camera, system_view, {14.0}, earth_material},
      {moon, system_camera, system_view, {7.0}, moon_material}};
    sphere_pass->render(commands, color, width, height, system_draws, lighting);
    const std::vector<sagan_render::scene_gpu::sphere_draw> lunar_draws{
      {earth, lunar_camera, lunar_view, {18.0}, earth_material},
      {moon, lunar_camera, lunar_view, {10.0}, moon_material}};
    sphere_pass->render(commands, color, width, height, lunar_draws, lighting);

    auto *copy = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTextureRegion source{color, 0, 0, 0, 0, 0, width, height, 1};
    const SDL_GPUTextureTransferInfo destination{download, 0, width, height};
    SDL_DownloadFromGPUTexture(copy, &source, &destination);
    SDL_EndGPUCopyPass(copy);
    auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
      fail("Could not finish 3D scene frame");
    SDL_ReleaseGPUFence(device, fence);

    auto *pixels = static_cast<std::uint8_t *>(
      SDL_MapGPUTransferBuffer(device, download, false));
    if (!pixels) fail("Could not map 3D scene capture");
    const auto result = inspect(pixels);
    const char *capture_value = std::getenv("SAGAN_RENDER_SCENE_3D_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value
      ? capture_value : "build/scene-3d/scene-3d.bmp";
    auto *surface = SDL_CreateSurfaceFrom(
      width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * pixel_bytes);
    if (!surface || !SDL_SaveBMP(surface, capture.c_str()))
      fail("Could not save 3D scene capture");
    SDL_DestroySurface(surface);
    SDL_UnmapGPUTransferBuffer(device, download);
    if (result.sun < 300 || result.earth < 100 || result.moon < 40)
    {
      std::cerr << "SCENE_3D_EVIDENCE sun=" << result.sun
                << " earth=" << result.earth << " moon=" << result.moon << '\n';
      throw std::runtime_error(
        "3D scene capture lacks distinguishable Sun, Earth, or Moon meshes");
    }
    std::cout << "SCENE_3D driver=" << SDL_GetGPUDeviceDriver(device)
              << " indexed=1 depth=1 views=2 bodies=3 sun=" << result.sun
              << " earth=" << result.earth << " moon=" << result.moon
              << " precision_origin_metres=1e15 cleanup=1\n";

    SDL_WaitForGPUIdle(device);
    sphere_pass.reset();
    SDL_ReleaseGPUTransferBuffer(device, download);
    SDL_ReleaseGPUTexture(device, color);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    if (device) SDL_WaitForGPUIdle(device);
    if (download) SDL_ReleaseGPUTransferBuffer(device, download);
    if (color) SDL_ReleaseGPUTexture(device, color);
    if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
    if (device) SDL_DestroyGPUDevice(device);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
}
