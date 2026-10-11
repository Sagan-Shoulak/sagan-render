#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  constexpr std::uint32_t width = 256;
  constexpr std::uint32_t height = 256;
  constexpr std::uint32_t pixel_bytes = 4;

  auto fail(const std::string &message) -> void
  {
    throw std::runtime_error(message + ": " + SDL_GetError());
  }

  struct State
  {
    SDL_Window *window{};
    SDL_GPUDevice *device{};
    SDL_GPUTexture *scene{};
    SDL_GPUTexture *output{};
    SDL_GPUSampler *sampler{};
    SDL_GPUTransferBuffer *download{};
    SDL_GPUGraphicsPipeline *pipeline{};
    bool claimed{};
    ~State()
    {
      if (device) SDL_WaitForGPUIdle(device);
      if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
      if (download) SDL_ReleaseGPUTransferBuffer(device, download);
      if (sampler) SDL_ReleaseGPUSampler(device, sampler);
      if (output) SDL_ReleaseGPUTexture(device, output);
      if (scene) SDL_ReleaseGPUTexture(device, scene);
      if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
      if (device) SDL_DestroyGPUDevice(device);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

  struct ShaderFormat
  {
    SDL_GPUShaderFormat format;
    const char *extension;
    const char *name;
  };

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

  auto shader_format(SDL_GPUDevice *device) -> ShaderFormat
  {
    const auto formats = SDL_GetGPUShaderFormats(device);
#if defined(_WIN32)
    if (formats & SDL_GPU_SHADERFORMAT_DXIL) return {SDL_GPU_SHADERFORMAT_DXIL, "dxil", "DXIL"};
#elif defined(__APPLE__)
    if (formats & SDL_GPU_SHADERFORMAT_MSL) return {SDL_GPU_SHADERFORMAT_MSL, "msl", "MSL"};
#else
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) return {SDL_GPU_SHADERFORMAT_SPIRV, "spv", "SPIR-V"};
#endif
    throw std::runtime_error("Backend did not expose its required post-process shader format");
  }

  auto read_file(const std::string &path) -> std::vector<std::uint8_t>
  {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Could not open shader " + path);
    const auto size = input.tellg();
    if (size <= 0) throw std::runtime_error("Shader is empty " + path);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    input.read(reinterpret_cast<char *>(bytes.data()), size);
    if (!input) throw std::runtime_error("Could not read shader " + path);
    return bytes;
  }

  auto load_shader(SDL_GPUDevice *device, const std::string &root, const char *stage,
                   const char *entrypoint, SDL_GPUShaderStage shader_stage,
                   std::uint32_t uniforms, std::uint32_t samplers,
                   const ShaderFormat &format) -> SDL_GPUShader *
  {
    const auto bytes = read_file(root + "/tonemap." + stage + "." + format.extension);
    SDL_GPUShaderCreateInfo info{};
    info.code_size = bytes.size();
    info.code = bytes.data();
    info.entrypoint = entrypoint;
    info.format = format.format;
    info.stage = shader_stage;
    info.num_uniform_buffers = uniforms;
    info.num_samplers = samplers;
    auto *shader = SDL_CreateGPUShader(device, &info);
    if (!shader) fail(std::string("Could not create ") + stage + " post-process shader");
    return shader;
  }

  auto create_pipeline(SDL_GPUDevice *device, const std::string &root,
                       const ShaderFormat &format) -> SDL_GPUGraphicsPipeline *
  {
    auto *vertex_shader = load_shader(
      device, root, "vert", "VSMain", SDL_GPU_SHADERSTAGE_VERTEX, 0, 0, format);
    auto *fragment_shader = load_shader(
      device, root, "frag", "PSMain", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 1, format);
    SDL_GPUColorTargetDescription color_description{};
    color_description.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertex_shader;
    info.fragment_shader = fragment_shader;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.target_info.color_target_descriptions = &color_description;
    info.target_info.num_color_targets = 1;
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, fragment_shader);
    SDL_ReleaseGPUShader(device, vertex_shader);
    if (!pipeline) fail("Could not create post-process pipeline");
    return pipeline;
  }

  auto inspect(const std::uint8_t *pixels) -> std::array<std::uint8_t, 3>
  {
    const auto offset = (height / 2 * width + width / 2) * pixel_bytes;
    const std::array<std::uint8_t, 3> center{
      pixels[offset], pixels[offset + 1], pixels[offset + 2]};
    if (center[0] < 210 || center[1] < 100 || center[1] > 150 ||
        center[2] < 45 || center[2] > 90 ||
        !(center[0] > center[1] && center[1] > center[2]))
      throw std::runtime_error("Tone-mapped center pixel is outside the reviewed range");
    return center;
  }
}

int main()
{
  State state{};
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL");
    state.window = SDL_CreateWindow("Sagan Post Process", 640, 480, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!state.window) fail("Could not create window");
    state.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_MSL, false, driver());
    if (!state.device) fail("Could not create GPU device");
    if (!SDL_ClaimWindowForGPUDevice(state.device, state.window)) fail("Could not claim window");
    state.claimed = true;
    const auto format = shader_format(state.device);
    const char *root_value = std::getenv("SAGAN_RENDER_POST_PROCESS_SHADER_DIR");
    const std::string root = root_value && *root_value ? root_value : "shaders/generated/post_process";
    state.pipeline = create_pipeline(state.device, root, format);

    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D;
    texture.width = width;
    texture.height = height;
    texture.layer_count_or_depth = 1;
    texture.num_levels = 1;
    texture.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    state.scene = SDL_CreateGPUTexture(state.device, &texture);
    state.output = SDL_CreateGPUTexture(state.device, &texture);
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    state.sampler = SDL_CreateGPUSampler(state.device, &sampler_info);
    SDL_GPUTransferBufferCreateInfo transfer{
      SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, width * height * pixel_bytes, 0};
    state.download = SDL_CreateGPUTransferBuffer(state.device, &transfer);
    if (!state.scene || !state.output || !state.sampler || !state.download)
      fail("Could not create post-process resources");

    auto *commands = SDL_AcquireGPUCommandBuffer(state.device);
    if (!commands) fail("Could not acquire commands");
    SDL_GPUColorTargetInfo scene_target{};
    scene_target.texture = state.scene;
    scene_target.clear_color = {0.8F, 0.2F, 0.05F, 1.0F};
    scene_target.load_op = SDL_GPU_LOADOP_CLEAR;
    scene_target.store_op = SDL_GPU_STOREOP_STORE;
    auto *scene_pass = SDL_BeginGPURenderPass(commands, &scene_target, 1, nullptr);
    SDL_EndGPURenderPass(scene_pass);

    SDL_GPUColorTargetInfo output_target{};
    output_target.texture = state.output;
    output_target.clear_color = {0, 0, 0, 1};
    output_target.load_op = SDL_GPU_LOADOP_CLEAR;
    output_target.store_op = SDL_GPU_STOREOP_STORE;
    auto *post_pass = SDL_BeginGPURenderPass(commands, &output_target, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(post_pass, state.pipeline);
    const SDL_GPUTextureSamplerBinding scene_binding{state.scene, state.sampler};
    SDL_BindGPUFragmentSamplers(post_pass, 0, &scene_binding, 1);
    const std::array<float, 4> post_uniform{1.0F, 1.0F / 2.2F, 0.0F, 0.0F};
    SDL_PushGPUFragmentUniformData(commands, 0, post_uniform.data(), sizeof(post_uniform));
    SDL_DrawGPUPrimitives(post_pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(post_pass);

    auto *copy = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTextureRegion region{state.output, 0, 0, 0, 0, 0, width, height, 1};
    const SDL_GPUTextureTransferInfo download_info{state.download, 0, width, height};
    SDL_DownloadFromGPUTexture(copy, &region, &download_info);
    SDL_EndGPUCopyPass(copy);
    auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (!fence || !SDL_WaitForGPUFences(state.device, true, &fence, 1))
      fail("Could not finish post-process frame");
    SDL_ReleaseGPUFence(state.device, fence);

    auto *pixels = static_cast<std::uint8_t *>(
      SDL_MapGPUTransferBuffer(state.device, state.download, false));
    if (!pixels) fail("Could not map post-process capture");
    const auto center = inspect(pixels);
    const char *capture_value = std::getenv("SAGAN_RENDER_POST_PROCESS_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value ? capture_value :
      "build/post-process/post-process.bmp";
    auto *surface = SDL_CreateSurfaceFrom(
      width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * pixel_bytes);
    if (!surface || !SDL_SaveBMP(surface, capture.c_str())) fail("Could not save post-process capture");
    SDL_DestroySurface(surface);
    SDL_UnmapGPUTransferBuffer(state.device, state.download);
    std::cout << "POST_PROCESS driver=" << SDL_GetGPUDeviceDriver(state.device)
              << " shader=" << format.name
              << " passes=2 sampled_scene=1 tonemap=1 center="
              << static_cast<int>(center[0]) << ',' << static_cast<int>(center[1]) << ','
              << static_cast<int>(center[2]) << " cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
