#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  constexpr std::uint32_t capture_width = 256;
  constexpr std::uint32_t capture_height = 256;
  constexpr std::uint32_t bytes_per_pixel = 4;

  auto fail(const std::string &operation) -> void
  {
    throw std::runtime_error(operation + ": " + SDL_GetError());
  }

  struct GpuState
  {
    SDL_Window *window{};
    SDL_GPUDevice *device{};
    SDL_GPUTexture *target{};
    SDL_GPUTransferBuffer *download{};
    SDL_GPUGraphicsPipeline *pipeline{};
    bool window_claimed{};

    ~GpuState()
    {
      if (device) SDL_WaitForGPUIdle(device);
      if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
      if (download) SDL_ReleaseGPUTransferBuffer(device, download);
      if (target) SDL_ReleaseGPUTexture(device, target);
      if (window_claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
      if (device) SDL_DestroyGPUDevice(device);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

  struct ShaderSelection
  {
    SDL_GPUShaderFormat format{};
    const char *extension{};
    const char *entrypoint{};
    const char *name{};
  };

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

  auto choose_shader_format(SDL_GPUDevice *device) -> ShaderSelection
  {
    const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    if (formats & SDL_GPU_SHADERFORMAT_DXIL)
      return {SDL_GPU_SHADERFORMAT_DXIL, "dxil", "main", "DXIL"};
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
      return {SDL_GPU_SHADERFORMAT_SPIRV, "spv", "main", "SPIR-V"};
    if (formats & SDL_GPU_SHADERFORMAT_MSL)
      return {SDL_GPU_SHADERFORMAT_MSL, "msl", "main0", "MSL"};
    throw std::runtime_error("Selected SDL GPU backend exposes no supported foundation shader format");
  }

  auto load_file(const std::string &path) -> std::vector<std::uint8_t>
  {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("Could not open shader: " + path);
    const auto length = stream.tellg();
    if (length <= 0) throw std::runtime_error("Shader is empty: " + path);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    stream.seekg(0);
    if (!stream.read(reinterpret_cast<char *>(bytes.data()), length))
      throw std::runtime_error("Could not read shader: " + path);
    return bytes;
  }

  auto load_shader(SDL_GPUDevice *device, const std::string &directory,
                   const std::string &stem, const ShaderSelection &selection,
                   const SDL_GPUShaderStage stage) -> SDL_GPUShader *
  {
    const std::string path = directory + '/' + stem + '.' + selection.extension;
    const auto code = load_file(path);
    SDL_GPUShaderCreateInfo info{};
    info.code_size = code.size();
    info.code = code.data();
    info.entrypoint = selection.entrypoint;
    info.format = selection.format;
    info.stage = stage;
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &info);
    if (!shader) fail("Could not create " + stem + " shader");
    return shader;
  }

  auto create_pipeline(SDL_GPUDevice *device, const std::string &shader_directory,
                       const ShaderSelection &selection) -> SDL_GPUGraphicsPipeline *
  {
    SDL_GPUShader *vertex = load_shader(device, shader_directory,
      "RawTriangle.vert", selection, SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader *fragment{};
    SDL_GPUGraphicsPipeline *pipeline{};
    try
    {
      fragment = load_shader(device, shader_directory,
        "SolidColor.frag", selection, SDL_GPU_SHADERSTAGE_FRAGMENT);
      SDL_GPUColorTargetDescription color_target{};
      color_target.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      SDL_GPUGraphicsPipelineCreateInfo info{};
      info.vertex_shader = vertex;
      info.fragment_shader = fragment;
      info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
      info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
      info.target_info.color_target_descriptions = &color_target;
      info.target_info.num_color_targets = 1;
      pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
      if (!pipeline) fail("Could not create graphics pipeline");
    }
    catch (...)
    {
      if (fragment) SDL_ReleaseGPUShader(device, fragment);
      SDL_ReleaseGPUShader(device, vertex);
      throw;
    }
    SDL_ReleaseGPUShader(device, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    return pipeline;
  }

  struct PixelEvidence
  {
    std::uint32_t colored{};
    std::uint32_t red_dominant{};
    std::uint32_t green_dominant{};
    std::uint32_t blue_dominant{};
  };

  auto inspect_pixels(const std::uint8_t *pixels) -> PixelEvidence
  {
    PixelEvidence evidence{};
    for (std::uint32_t index = 0; index < capture_width * capture_height; ++index)
    {
      const auto red = pixels[index * bytes_per_pixel];
      const auto green = pixels[index * bytes_per_pixel + 1];
      const auto blue = pixels[index * bytes_per_pixel + 2];
      if (red + green + blue > 180) ++evidence.colored;
      if (red > green + 40 && red > blue + 40) ++evidence.red_dominant;
      if (green > red + 40 && green > blue + 40) ++evidence.green_dominant;
      if (blue > red + 40 && blue > green + 40) ++evidence.blue_dominant;
    }
    if (evidence.colored < 10000 || evidence.red_dominant < 500 ||
        evidence.green_dominant < 500 || evidence.blue_dominant < 500)
      throw std::runtime_error("GPU capture did not contain the expected RGB triangle");
    return evidence;
  }
}

int main()
{
  GpuState state{};
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL video");
    state.window = SDL_CreateWindow("Sagan Render GPU Foundation", 960, 540,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!state.window) fail("Could not create SDL window");

    const SDL_GPUShaderFormat available_formats = SDL_GPU_SHADERFORMAT_DXIL |
      SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL;
    state.device = SDL_CreateGPUDevice(available_formats, false, platform_driver());
    if (!state.device) fail("Could not create SDL GPU device");
    if (!SDL_ClaimWindowForGPUDevice(state.device, state.window))
      fail("Could not claim window for SDL GPU device");
    state.window_claimed = true;

    const ShaderSelection selection = choose_shader_format(state.device);
    const char *shader_value = std::getenv("SAGAN_RENDER_SHADER_DIR");
    const std::string shader_directory = shader_value && *shader_value
      ? shader_value : "build/dependencies/sdl-gpu-foundation-shaders";
    state.pipeline = create_pipeline(state.device, shader_directory, selection);

    SDL_GPUTextureCreateInfo texture_info{};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_info.width = capture_width;
    texture_info.height = capture_height;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    state.target = SDL_CreateGPUTexture(state.device, &texture_info);
    if (!state.target) fail("Could not create offscreen color target");

    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    transfer_info.size = capture_width * capture_height * bytes_per_pixel;
    state.download = SDL_CreateGPUTransferBuffer(state.device, &transfer_info);
    if (!state.download) fail("Could not create GPU download buffer");

    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(state.device);
    if (!commands) fail("Could not acquire GPU command buffer");

    SDL_GPUColorTargetInfo target_info{};
    target_info.texture = state.target;
    target_info.clear_color = SDL_FColor{0.05F, 0.09F, 0.16F, 1.0F};
    target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(commands, &target_info, 1, nullptr);
    if (!render_pass) fail("Could not begin GPU render pass");
    SDL_BindGPUGraphicsPipeline(render_pass, state.pipeline);
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    SDL_GPUTexture *swapchain{};
    std::uint32_t swapchain_width{};
    std::uint32_t swapchain_height{};
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, state.window, &swapchain,
                                                &swapchain_width, &swapchain_height))
      fail("Could not acquire GPU swapchain texture");
    if (!swapchain) throw std::runtime_error("SDL GPU returned no swapchain texture");

    SDL_GPUBlitInfo blit{};
    blit.source = {state.target, 0, 0, 0, 0, capture_width, capture_height};
    blit.destination = {swapchain, 0, 0, 0, 0, swapchain_width, swapchain_height};
    blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter = SDL_GPU_FILTER_LINEAR;
    SDL_BlitGPUTexture(commands, &blit);

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(commands);
    if (!copy_pass) fail("Could not begin GPU copy pass");
    const SDL_GPUTextureRegion source{
      state.target, 0, 0, 0, 0, 0, capture_width, capture_height, 1
    };
    const SDL_GPUTextureTransferInfo destination{
      state.download, 0, capture_width, capture_height
    };
    SDL_DownloadFromGPUTexture(copy_pass, &source, &destination);
    SDL_EndGPUCopyPass(copy_pass);

    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (!fence) fail("Could not submit GPU command buffer");
    if (!SDL_WaitForGPUFences(state.device, true, &fence, 1))
      fail("Could not wait for GPU frame fence");
    SDL_ReleaseGPUFence(state.device, fence);

    auto *pixels = static_cast<std::uint8_t *>(
      SDL_MapGPUTransferBuffer(state.device, state.download, false));
    if (!pixels) fail("Could not map GPU capture buffer");
    const PixelEvidence evidence = inspect_pixels(pixels);

    const char *capture_value = std::getenv("SAGAN_RENDER_GPU_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value
      ? capture_value : "build/sdl-gpu/sdl-gpu-triangle.bmp";
    SDL_Surface *surface = SDL_CreateSurfaceFrom(capture_width, capture_height,
      SDL_PIXELFORMAT_RGBA32, pixels, capture_width * bytes_per_pixel);
    if (!surface) fail("Could not wrap GPU capture as an SDL surface");
    if (!SDL_SaveBMP(surface, capture.c_str())) fail("Could not save GPU capture");
    SDL_DestroySurface(surface);
    SDL_UnmapGPUTransferBuffer(state.device, state.download);

    std::cout << "SDL_GPU_PROBE device=1 driver=" << SDL_GetGPUDeviceDriver(state.device)
              << " shader=" << selection.name << " command_buffer=1 render_pass=1"
              << " swapchain=1 triangle=1 capture=" << capture << '\n';
    std::cout << "SDL_GPU_PIXELS colored=" << evidence.colored
              << " red=" << evidence.red_dominant
              << " green=" << evidence.green_dominant
              << " blue=" << evidence.blue_dominant << '\n';

    const std::uint64_t deadline = SDL_GetTicks() + 1200;
    bool running = true;
    while (running && SDL_GetTicks() < deadline)
    {
      SDL_Event event{};
      while (SDL_PollEvent(&event))
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
          running = false;
      SDL_Delay(8);
    }
    std::cout << "SDL_GPU_PROBE cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
