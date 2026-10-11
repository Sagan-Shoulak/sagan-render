#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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

  struct Vertex { float position[3]; float normal[3]; float uv[2]; };
  constexpr std::array<Vertex, 6> vertices{{
    {{0, 1, 0}, {0, 1, 0}, {0.5F, 0}}, {{1, 0, 0}, {1, 0, 0}, {1, 0.5F}},
    {{0, 0, 1}, {0, 0, 1}, {0.75F, 0.5F}},
    {{-1, 0, 0}, {-1, 0, 0}, {0, 0.5F}},
    {{0, 0, -1}, {0, 0, -1}, {0.25F, 0.5F}},
    {{0, -1, 0}, {0, -1, 0}, {0.5F, 1}}
  }};
  constexpr std::array<std::uint16_t, 24> indices{{
    0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4,
    5, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1
  }};

  struct alignas(16) CameraUniform { float transform[16]; float normal[16]; };
  struct alignas(16) MaterialUniform { float base[4]; float emissive_roughness[4]; };
  struct alignas(16) LightingUniform { float ambient[4]; float direction_intensity[4]; float color[4]; };
  struct alignas(16) SurfaceLodUniform { float center_width[4]; float east_enabled[4]; float north_blend[4]; };
  static_assert(sizeof(CameraUniform) == 128);
  static_assert(sizeof(MaterialUniform) == 32);
  static_assert(sizeof(LightingUniform) == 48);
  static_assert(sizeof(SurfaceLodUniform) == 48);

  auto fail(const std::string &message) -> void
  {
    throw std::runtime_error(message + ": " + SDL_GetError());
  }

  struct State
  {
    SDL_Window *window{};
    SDL_GPUDevice *device{};
    SDL_GPUTexture *color{};
    SDL_GPUTexture *depth{};
    SDL_GPUTexture *white{};
    SDL_GPUSampler *sampler{};
    SDL_GPUBuffer *vertex{};
    SDL_GPUBuffer *index{};
    SDL_GPUTransferBuffer *upload{};
    SDL_GPUTransferBuffer *download{};
    SDL_GPUGraphicsPipeline *pipeline{};
    bool claimed{};
    ~State()
    {
      if (device) SDL_WaitForGPUIdle(device);
      if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
      if (download) SDL_ReleaseGPUTransferBuffer(device, download);
      if (upload) SDL_ReleaseGPUTransferBuffer(device, upload);
      if (index) SDL_ReleaseGPUBuffer(device, index);
      if (vertex) SDL_ReleaseGPUBuffer(device, vertex);
      if (sampler) SDL_ReleaseGPUSampler(device, sampler);
      if (white) SDL_ReleaseGPUTexture(device, white);
      if (depth) SDL_ReleaseGPUTexture(device, depth);
      if (color) SDL_ReleaseGPUTexture(device, color);
      if (claimed) SDL_ReleaseWindowFromGPUDevice(device, window);
      if (device) SDL_DestroyGPUDevice(device);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

  struct ShaderFormat { SDL_GPUShaderFormat format; const char *extension; const char *name; };

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
    throw std::runtime_error("Backend did not expose its required material shader format");
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
    const auto bytes = read_file(root + "/lit_mesh." + stage + "." + format.extension);
    SDL_GPUShaderCreateInfo info{};
    info.code_size = bytes.size();
    info.code = bytes.data();
    info.entrypoint = entrypoint;
    info.format = format.format;
    info.stage = shader_stage;
    info.num_uniform_buffers = uniforms;
    info.num_samplers = samplers;
    auto *shader = SDL_CreateGPUShader(device, &info);
    if (!shader) fail(std::string("Could not create ") + stage + " material shader");
    return shader;
  }

  auto create_pipeline(SDL_GPUDevice *device, const std::string &root,
                       const ShaderFormat &format) -> SDL_GPUGraphicsPipeline *
  {
    auto *vertex_shader = load_shader(
      device, root, "vert", "VSMain", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0, format);
    auto *fragment_shader = load_shader(
      device, root, "frag", "PSMain", SDL_GPU_SHADERSTAGE_FRAGMENT, 3, 2, format);
    SDL_GPUVertexBufferDescription buffer_description{0, sizeof(Vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    const std::array<SDL_GPUVertexAttribute, 3> attributes{{
      {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, position)},
      {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, normal)},
      {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, uv)}
    }};
    SDL_GPUColorTargetDescription color_description{};
    color_description.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = vertex_shader;
    info.fragment_shader = fragment_shader;
    info.vertex_input_state = {&buffer_description, 1, attributes.data(), attributes.size()};
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    info.depth_stencil_state.enable_depth_test = true;
    info.depth_stencil_state.enable_depth_write = true;
    info.target_info.color_target_descriptions = &color_description;
    info.target_info.num_color_targets = 1;
    info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    info.target_info.has_depth_stencil_target = true;
    auto *pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
    SDL_ReleaseGPUShader(device, fragment_shader);
    SDL_ReleaseGPUShader(device, vertex_shader);
    if (!pipeline) fail("Could not create depth-tested material pipeline");
    return pipeline;
  }

  auto camera(float scale, float x, float z) -> CameraUniform
  {
    CameraUniform result{};
    result.transform[0] = scale; result.transform[5] = scale; result.transform[10] = scale;
    result.transform[12] = x; result.transform[14] = z; result.transform[15] = 1;
    result.normal[0] = 1; result.normal[5] = 1; result.normal[10] = 1; result.normal[15] = 1;
    return result;
  }

  auto material(float red, float green, float blue) -> MaterialUniform
  {
    return {{red, green, blue, 1}, {0, 0, 0, 0.5F}};
  }

  auto lighting() -> LightingUniform
  {
    return {{0.28F, 0.28F, 0.28F, 0}, {0, 0, -1, 0.9F}, {1, 1, 1, 0}};
  }

  struct Evidence { std::uint32_t blue{}; std::uint32_t orange{}; std::uint32_t lit{}; };
  auto inspect(const std::uint8_t *pixels) -> Evidence
  {
    Evidence result{};
    for (std::uint32_t i = 0; i < width * height; ++i)
    {
      const auto r = pixels[i * 4]; const auto g = pixels[i * 4 + 1]; const auto b = pixels[i * 4 + 2];
      if (r + g + b > 100) ++result.lit;
      if (b > r + 25 && b > g + 25) ++result.blue;
      if (r > b + 25 && r > g + 10) ++result.orange;
    }
    if (result.lit < 1500 || result.blue < 150 || result.orange < 150)
      throw std::runtime_error("Material capture lacks expected lit blue and orange geometry");
    return result;
  }
}

int main()
{
  State state{};
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL");
    state.window = SDL_CreateWindow("Sagan Material Depth", 640, 480, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!state.window) fail("Could not create window");
    state.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_MSL, false, driver());
    if (!state.device) fail("Could not create GPU device");
    if (!SDL_ClaimWindowForGPUDevice(state.device, state.window)) fail("Could not claim window");
    state.claimed = true;
    const auto format = shader_format(state.device);
    const char *root_value = std::getenv("SAGAN_RENDER_MATERIAL_SHADER_DIR");
    const std::string root = root_value && *root_value ? root_value : "shaders/generated/material";
    state.pipeline = create_pipeline(state.device, root, format);

    SDL_GPUTextureCreateInfo texture{};
    texture.type = SDL_GPU_TEXTURETYPE_2D; texture.width = width; texture.height = height;
    texture.layer_count_or_depth = 1; texture.num_levels = 1; texture.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    state.color = SDL_CreateGPUTexture(state.device, &texture);
    texture.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    texture.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    state.depth = SDL_CreateGPUTexture(state.device, &texture);
    texture.width = 1; texture.height = 1;
    texture.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    state.white = SDL_CreateGPUTexture(state.device, &texture);
    SDL_GPUSamplerCreateInfo sampler_info{};
    state.sampler = SDL_CreateGPUSampler(state.device, &sampler_info);
    if (!state.color || !state.depth || !state.white || !state.sampler)
      fail("Could not create material targets or sampler");

    SDL_GPUBufferCreateInfo buffer{};
    buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX; buffer.size = sizeof(vertices);
    state.vertex = SDL_CreateGPUBuffer(state.device, &buffer);
    buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX; buffer.size = sizeof(indices);
    state.index = SDL_CreateGPUBuffer(state.device, &buffer);
    SDL_GPUTransferBufferCreateInfo transfer{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      static_cast<std::uint32_t>(sizeof(vertices) + sizeof(indices)), 0};
    state.upload = SDL_CreateGPUTransferBuffer(state.device, &transfer);
    transfer = {SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, width * height * pixel_bytes, 0};
    state.download = SDL_CreateGPUTransferBuffer(state.device, &transfer);
    if (!state.vertex || !state.index || !state.upload || !state.download) fail("Could not create material buffers");
    auto *mapped = static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(state.device, state.upload, false));
    if (!mapped) fail("Could not map upload buffer");
    std::memcpy(mapped, vertices.data(), sizeof(vertices));
    std::memcpy(mapped + sizeof(vertices), indices.data(), sizeof(indices));
    SDL_UnmapGPUTransferBuffer(state.device, state.upload);

    auto *commands = SDL_AcquireGPUCommandBuffer(state.device);
    if (!commands) fail("Could not acquire commands");
    auto *copy = SDL_BeginGPUCopyPass(commands);
    SDL_GPUTransferBufferLocation source{state.upload, 0};
    SDL_GPUBufferRegion destination{state.vertex, 0, sizeof(vertices)};
    SDL_UploadToGPUBuffer(copy, &source, &destination, false);
    source.offset = sizeof(vertices); destination = {state.index, 0, sizeof(indices)};
    SDL_UploadToGPUBuffer(copy, &source, &destination, false);
    SDL_EndGPUCopyPass(copy);

    SDL_GPUColorTargetInfo white_target{};
    white_target.texture = state.white;
    white_target.clear_color = {1, 1, 1, 1};
    white_target.load_op = SDL_GPU_LOADOP_CLEAR;
    white_target.store_op = SDL_GPU_STOREOP_STORE;
    auto *white_pass = SDL_BeginGPURenderPass(commands, &white_target, 1, nullptr);
    SDL_EndGPURenderPass(white_pass);

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = state.color; color_target.clear_color = {0.02F, 0.03F, 0.06F, 1};
    color_target.load_op = SDL_GPU_LOADOP_CLEAR; color_target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPUDepthStencilTargetInfo depth_target{};
    depth_target.texture = state.depth; depth_target.clear_depth = 1;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR; depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
    auto *pass = SDL_BeginGPURenderPass(commands, &color_target, 1, &depth_target);
    SDL_BindGPUGraphicsPipeline(pass, state.pipeline);
    const SDL_GPUBufferBinding vertex_binding{state.vertex, 0}, index_binding{state.index, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
    SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    const std::array<SDL_GPUTextureSamplerBinding, 2> surface_bindings{{
      {state.white, state.sampler}, {state.white, state.sampler}}};
    SDL_BindGPUFragmentSamplers(pass, 0, surface_bindings.data(), surface_bindings.size());
    const auto light = lighting();
    SDL_PushGPUFragmentUniformData(commands, 1, &light, sizeof(light));
    const SurfaceLodUniform surface_lod{};
    SDL_PushGPUFragmentUniformData(commands, 2, &surface_lod, sizeof(surface_lod));
    const auto near_camera = camera(0.38F, -0.1F, 0.48F);
    const auto near_material = material(0.05F, 0.2F, 0.9F);
    SDL_PushGPUVertexUniformData(commands, 0, &near_camera, sizeof(near_camera));
    SDL_PushGPUFragmentUniformData(commands, 0, &near_material, sizeof(near_material));
    SDL_DrawGPUIndexedPrimitives(pass, indices.size(), 1, 0, 0, 0);
    const auto far_camera = camera(0.32F, 0.1F, 0.68F);
    const auto far_material = material(0.95F, 0.25F, 0.04F);
    SDL_PushGPUVertexUniformData(commands, 0, &far_camera, sizeof(far_camera));
    SDL_PushGPUFragmentUniformData(commands, 0, &far_material, sizeof(far_material));
    SDL_DrawGPUIndexedPrimitives(pass, indices.size(), 1, 0, 0, 0);
    SDL_EndGPURenderPass(pass);

    copy = SDL_BeginGPUCopyPass(commands);
    const SDL_GPUTextureRegion color_region{state.color, 0, 0, 0, 0, 0, width, height, 1};
    const SDL_GPUTextureTransferInfo download_info{state.download, 0, width, height};
    SDL_DownloadFromGPUTexture(copy, &color_region, &download_info);
    SDL_EndGPUCopyPass(copy);
    auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
    if (!fence || !SDL_WaitForGPUFences(state.device, true, &fence, 1)) fail("Could not finish material frame");
    SDL_ReleaseGPUFence(state.device, fence);
    auto *pixels = static_cast<std::uint8_t *>(SDL_MapGPUTransferBuffer(state.device, state.download, false));
    if (!pixels) fail("Could not map material capture");
    const auto evidence = inspect(pixels);
    const char *capture_value = std::getenv("SAGAN_RENDER_MATERIAL_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value ? capture_value : "build/material-depth/material-depth.bmp";
    auto *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * pixel_bytes);
    if (!surface || !SDL_SaveBMP(surface, capture.c_str())) fail("Could not save material capture");
    SDL_DestroySurface(surface);
    SDL_UnmapGPUTransferBuffer(state.device, state.download);
    std::cout << "MATERIAL_DEPTH driver=" << SDL_GetGPUDeviceDriver(state.device)
              << " shader=" << format.name << " indexed=1 depth=1 uniforms=3 blue="
              << evidence.blue << " orange=" << evidence.orange << " cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
