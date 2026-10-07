#pragma once

#include "material_contract.hpp"
#include "scene_mesh_contract.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sagan_render::scene_gpu
{
  struct vertex { float position[3]; float normal[3]; };

  struct target_viewport
  {
    float x{};
    float y{};
    float width{};
    float height{};
  };

  struct sphere_draw
  {
    scene::render_item item;
    scene::camera camera;
    target_viewport target;
    scene::sphere_presentation presentation;
    sagan::render::MaterialUniform material;
  };

  class indexed_sphere_pass
  {
    static constexpr std::array<vertex, 6> vertices{{
      {{0, 1, 0}, {0, 1, 0}}, {{1, 0, 0}, {1, 0, 0}},
      {{0, 0, 1}, {0, 0, 1}}, {{-1, 0, 0}, {-1, 0, 0}},
      {{0, 0, -1}, {0, 0, -1}}, {{0, -1, 0}, {0, -1, 0}}
    }};
    static constexpr std::array<std::uint16_t, 24> indices{{
      0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4,
      5, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1
    }};

    SDL_GPUDevice *device{};
    SDL_GPUGraphicsPipeline *pipeline{};
    SDL_GPUBuffer *vertex_buffer{};
    SDL_GPUBuffer *index_buffer{};
    SDL_GPUTexture *depth{};
    std::uint32_t depth_width{};
    std::uint32_t depth_height{};

    struct shader_format
    {
      SDL_GPUShaderFormat format;
      const char *extension;
    };

    static auto fail(const std::string &message) -> void
    {
      throw std::runtime_error(message + ": " + SDL_GetError());
    }

    auto select_format() const -> shader_format
    {
      const auto formats = SDL_GetGPUShaderFormats(device);
#if defined(_WIN32)
      if (formats & SDL_GPU_SHADERFORMAT_DXIL)
        return {SDL_GPU_SHADERFORMAT_DXIL, "dxil"};
#elif defined(__APPLE__)
      if (formats & SDL_GPU_SHADERFORMAT_MSL)
        return {SDL_GPU_SHADERFORMAT_MSL, "msl"};
#else
      if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
        return {SDL_GPU_SHADERFORMAT_SPIRV, "spv"};
#endif
      throw std::runtime_error("Scene backend lacks its required material shader format");
    }

    static auto read_file(const std::string &path) -> std::vector<std::uint8_t>
    {
      std::ifstream input(path, std::ios::binary | std::ios::ate);
      if (!input) throw std::runtime_error("Could not open scene shader " + path);
      const auto size = input.tellg();
      if (size <= 0) throw std::runtime_error("Scene shader is empty " + path);
      std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
      input.seekg(0);
      input.read(reinterpret_cast<char *>(bytes.data()), size);
      if (!input) throw std::runtime_error("Could not read scene shader " + path);
      return bytes;
    }

    auto load_shader(const std::string &root, const char *stage,
                     const SDL_GPUShaderStage shader_stage,
                     const std::uint32_t uniform_buffers,
                     const shader_format selected) const -> SDL_GPUShader *
    {
      const auto bytes = read_file(root + "/lit_mesh." + stage + "." + selected.extension);
      SDL_GPUShaderCreateInfo info{};
      info.code_size = bytes.size();
      info.code = bytes.data();
      info.entrypoint = shader_stage == SDL_GPU_SHADERSTAGE_VERTEX ? "VSMain" : "PSMain";
      info.format = selected.format;
      info.stage = shader_stage;
      info.num_uniform_buffers = uniform_buffers;
      auto *shader = SDL_CreateGPUShader(device, &info);
      if (!shader) fail(std::string("Could not create scene ") + stage + " shader");
      return shader;
    }

    auto create_pipeline() -> void
    {
      const char *root_value = std::getenv("SAGAN_RENDER_MATERIAL_SHADER_DIR");
      const std::string root = root_value && *root_value
        ? root_value : "shaders/generated/material";
      const auto selected = select_format();
      auto *vertex_shader = load_shader(
        root, "vert", SDL_GPU_SHADERSTAGE_VERTEX, 1, selected);
      auto *fragment_shader = load_shader(
        root, "frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 2, selected);
      const SDL_GPUVertexBufferDescription buffer_description{
        0, sizeof(vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
      const std::array<SDL_GPUVertexAttribute, 2> attributes{{
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(vertex, position)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(vertex, normal)}
      }};
      SDL_GPUColorTargetDescription color_description{};
      color_description.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      SDL_GPUGraphicsPipelineCreateInfo info{};
      info.vertex_shader = vertex_shader;
      info.fragment_shader = fragment_shader;
      info.vertex_input_state = {
        &buffer_description, 1, attributes.data(), attributes.size()};
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
      pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
      SDL_ReleaseGPUShader(device, fragment_shader);
      SDL_ReleaseGPUShader(device, vertex_shader);
      if (!pipeline) fail("Could not create indexed scene pipeline");
    }

    auto upload_mesh() -> void
    {
      SDL_GPUBufferCreateInfo buffer{};
      buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
      buffer.size = sizeof(vertices);
      vertex_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX;
      buffer.size = sizeof(indices);
      index_buffer = SDL_CreateGPUBuffer(device, &buffer);
      SDL_GPUTransferBufferCreateInfo transfer{
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        static_cast<std::uint32_t>(sizeof(vertices) + sizeof(indices)), 0};
      auto *upload = SDL_CreateGPUTransferBuffer(device, &transfer);
      if (!vertex_buffer || !index_buffer || !upload)
        fail("Could not create indexed scene buffers");
      auto *mapped = static_cast<std::uint8_t *>(
        SDL_MapGPUTransferBuffer(device, upload, false));
      if (!mapped) fail("Could not map indexed scene upload");
      std::memcpy(mapped, vertices.data(), sizeof(vertices));
      std::memcpy(mapped + sizeof(vertices), indices.data(), sizeof(indices));
      SDL_UnmapGPUTransferBuffer(device, upload);
      auto *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire indexed scene upload commands");
      auto *copy = SDL_BeginGPUCopyPass(commands);
      SDL_GPUTransferBufferLocation source{upload, 0};
      SDL_GPUBufferRegion destination{vertex_buffer, 0, sizeof(vertices)};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset = sizeof(vertices);
      destination = {index_buffer, 0, sizeof(indices)};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      SDL_EndGPUCopyPass(copy);
      auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
      if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
        fail("Could not upload indexed scene mesh");
      SDL_ReleaseGPUFence(device, fence);
      SDL_ReleaseGPUTransferBuffer(device, upload);
    }

    auto ensure_depth(const std::uint32_t width, const std::uint32_t height) -> void
    {
      if (depth && depth_width == width && depth_height == height) return;
      if (depth) SDL_ReleaseGPUTexture(device, depth);
      SDL_GPUTextureCreateInfo info{};
      info.type = SDL_GPU_TEXTURETYPE_2D;
      info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
      info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
      info.width = width;
      info.height = height;
      info.layer_count_or_depth = 1;
      info.num_levels = 1;
      info.sample_count = SDL_GPU_SAMPLECOUNT_1;
      depth = SDL_CreateGPUTexture(device, &info);
      if (!depth) fail("Could not create indexed scene depth target");
      depth_width = width;
      depth_height = height;
    }

  public:
    explicit indexed_sphere_pass(SDL_GPUDevice *value) : device{value}
    {
      create_pipeline();
      upload_mesh();
    }

    ~indexed_sphere_pass()
    {
      if (depth) SDL_ReleaseGPUTexture(device, depth);
      if (index_buffer) SDL_ReleaseGPUBuffer(device, index_buffer);
      if (vertex_buffer) SDL_ReleaseGPUBuffer(device, vertex_buffer);
      if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }

    indexed_sphere_pass(const indexed_sphere_pass &) = delete;
    auto operator=(const indexed_sphere_pass &) -> indexed_sphere_pass & = delete;

    auto render(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *color,
                const std::uint32_t width, const std::uint32_t height,
                const std::vector<sphere_draw> &draws,
                const sagan::render::LightingUniform &lighting) -> void
    {
      if (draws.empty()) return;
      ensure_depth(width, height);
      SDL_GPUColorTargetInfo color_target{};
      color_target.texture = color;
      color_target.load_op = SDL_GPU_LOADOP_LOAD;
      color_target.store_op = SDL_GPU_STOREOP_STORE;
      SDL_GPUDepthStencilTargetInfo depth_target{};
      depth_target.texture = depth;
      depth_target.clear_depth = 1.0F;
      depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
      depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;
      auto *render_pass = SDL_BeginGPURenderPass(
        commands, &color_target, 1, &depth_target);
      if (!render_pass) fail("Could not begin indexed scene pass");
      SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
      const SDL_GPUBufferBinding vertex_binding{vertex_buffer, 0};
      const SDL_GPUBufferBinding index_binding{index_buffer, 0};
      SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
      SDL_BindGPUIndexBuffer(
        render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
      SDL_PushGPUFragmentUniformData(commands, 1, &lighting, sizeof(lighting));
      for (const auto &draw : draws)
      {
        const scene::viewport logical{draw.target.width, draw.target.height};
        const auto prepared = scene::prepare_sphere_draw(
          draw.item, draw.camera, logical, draw.presentation);
        const SDL_GPUViewport viewport{
          draw.target.x, draw.target.y, draw.target.width, draw.target.height,
          0.0F, 1.0F};
        const SDL_Rect scissor{
          static_cast<int>(draw.target.x), static_cast<int>(draw.target.y),
          static_cast<int>(draw.target.width), static_cast<int>(draw.target.height)};
        SDL_SetGPUViewport(render_pass, &viewport);
        SDL_SetGPUScissor(render_pass, &scissor);
        SDL_PushGPUVertexUniformData(
          commands, 0, &prepared.camera_uniform, sizeof(prepared.camera_uniform));
        SDL_PushGPUFragmentUniformData(
          commands, 0, &draw.material, sizeof(draw.material));
        SDL_DrawGPUIndexedPrimitives(
          render_pass, indices.size(), 1, 0, 0, 0);
      }
      SDL_EndGPURenderPass(render_pass);
    }
  };
}
