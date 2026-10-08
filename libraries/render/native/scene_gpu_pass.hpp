#pragma once

#include "material_contract.hpp"
#include "scene_mesh_contract.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
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
  struct vertex
  {
    float position[3];
    float normal[3];
    float uv[2];
  };

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
    sagan::render::MaterialUniform material;
  };

  struct box_draw
  {
    scene::render_item item;
    scene::length3 half_extents;
    scene::scalar yaw_radians{};
    scene::camera camera;
    target_viewport target;
    sagan::render::MaterialUniform material;
  };

  struct oriented_box_draw
  {
    scene::render_item item;
    scene::length3 half_extents;
    scene::direction3 axis_x;
    scene::direction3 axis_y;
    scene::direction3 axis_z;
    scene::camera camera;
    target_viewport target;
    sagan::render::MaterialUniform material;
  };

  enum class mesh_kind { sphere, box };
  enum class surface_map { none, earth_blue_marble, moon_lro };

  struct mesh_draw
  {
    mesh_kind kind{mesh_kind::sphere};
    scene::render_item item;
    scene::length3 half_extents{};
    scene::scalar yaw_radians{};
    scene::camera camera;
    target_viewport target;
    sagan::render::MaterialUniform material;
    bool oriented{};
    scene::direction3 axis_x{};
    scene::direction3 axis_y{};
    scene::direction3 axis_z{};
    surface_map albedo_map{surface_map::none};
  };

  class indexed_sphere_pass
  {
    static constexpr std::uint32_t latitude_segments = 64;
    static constexpr std::uint32_t longitude_segments = 128;
    std::vector<vertex> vertices;
    std::vector<std::uint16_t> indices;
    std::vector<vertex> box_vertices;
    std::vector<std::uint16_t> box_indices;
    std::vector<vertex> surface_patch_vertices;
    std::vector<std::uint16_t> surface_patch_indices;

    SDL_GPUDevice *device{};
    SDL_GPUGraphicsPipeline *pipeline{};
    SDL_GPUBuffer *vertex_buffer{};
    SDL_GPUBuffer *index_buffer{};
    SDL_GPUBuffer *box_vertex_buffer{};
    SDL_GPUBuffer *box_index_buffer{};
    SDL_GPUBuffer *surface_patch_vertex_buffer{};
    SDL_GPUBuffer *surface_patch_index_buffer{};
    SDL_GPUTexture *depth{};
    SDL_GPUTexture *white_texture{};
    SDL_GPUTexture *earth_texture{};
    SDL_GPUTexture *moon_texture{};
    SDL_GPUTexture *moon_detail_texture{};
    SDL_GPUSampler *surface_sampler{};
    SDL_GPUSampler *local_detail_sampler{};
    std::uint32_t depth_width{};
    std::uint32_t depth_height{};

    struct image
    {
      std::uint32_t width{};
      std::uint32_t height{};
      std::vector<std::uint8_t> rgba;
    };

    struct surface_lod_uniform
    {
      float center_and_angular_width[4]{};
      float east_and_enabled[4]{};
      float north_and_blend[4]{};
    };

    struct shader_format
    {
      SDL_GPUShaderFormat format;
      const char *extension;
    };

    static auto fail(const std::string &message) -> void
    {
      throw std::runtime_error(message + ": " + SDL_GetError());
    }

    auto build_sphere_mesh() -> void
    {
      constexpr float pi = 3.14159265358979323846F;
      vertices.reserve((latitude_segments + 1) * (longitude_segments + 1));
      indices.reserve(latitude_segments * longitude_segments * 6);
      for (std::uint32_t latitude = 0; latitude <= latitude_segments; ++latitude)
      {
        const float polar = pi * static_cast<float>(latitude) /
                            static_cast<float>(latitude_segments);
        const float ring = std::sin(polar);
        const float y = std::cos(polar);
        for (std::uint32_t longitude = 0; longitude <= longitude_segments; ++longitude)
        {
          const float azimuth = 2.0F * pi * static_cast<float>(longitude) /
                                static_cast<float>(longitude_segments);
          const float x = ring * std::cos(azimuth);
          const float z = ring * std::sin(azimuth);
          // NASA's equirectangular maps are centered on zero longitude. The
          // procedural sphere starts at +X, so the half-turn offset preserves
          // the same body-fixed orientation used by the earlier albedo grid.
          const float u = static_cast<float>(longitude) /
              static_cast<float>(longitude_segments) + 0.5F;
          const float v = static_cast<float>(latitude) /
              static_cast<float>(latitude_segments);
          vertices.push_back({{x, y, z}, {x, y, z}, {u, v}});
        }
      }
      for (std::uint32_t latitude = 0; latitude < latitude_segments; ++latitude)
      {
        for (std::uint32_t longitude = 0; longitude < longitude_segments;
             ++longitude)
        {
          const auto first = static_cast<std::uint16_t>(
            latitude * (longitude_segments + 1) + longitude);
          const auto second = static_cast<std::uint16_t>(
            first + longitude_segments + 1);
          indices.insert(indices.end(), {
            first, second, static_cast<std::uint16_t>(first + 1),
            static_cast<std::uint16_t>(first + 1), second,
            static_cast<std::uint16_t>(second + 1)});
        }
      }
    }

    auto build_box_mesh() -> void
    {
      constexpr std::array<std::array<float, 3>, 8> corners{{
        {{-1.0F, -1.0F, -1.0F}}, {{1.0F, -1.0F, -1.0F}},
        {{1.0F, 1.0F, -1.0F}}, {{-1.0F, 1.0F, -1.0F}},
        {{-1.0F, -1.0F, 1.0F}}, {{1.0F, -1.0F, 1.0F}},
        {{1.0F, 1.0F, 1.0F}}, {{-1.0F, 1.0F, 1.0F}}
      }};
      struct face { std::array<std::uint8_t, 4> corner; std::array<float, 3> normal; };
      constexpr std::array<face, 6> faces{{
        {{{0, 3, 2, 1}}, {{0.0F, 0.0F, -1.0F}}},
        {{{4, 5, 6, 7}}, {{0.0F, 0.0F, 1.0F}}},
        {{{0, 1, 5, 4}}, {{0.0F, -1.0F, 0.0F}}},
        {{{1, 2, 6, 5}}, {{1.0F, 0.0F, 0.0F}}},
        {{{2, 3, 7, 6}}, {{0.0F, 1.0F, 0.0F}}},
        {{{3, 0, 4, 7}}, {{-1.0F, 0.0F, 0.0F}}}
      }};
      for (const auto &face_value : faces)
      {
        const auto base = static_cast<std::uint16_t>(box_vertices.size());
        for (const auto corner_index : face_value.corner)
        {
          const auto &corner = corners[corner_index];
          box_vertices.push_back({
            {corner[0], corner[1], corner[2]},
            {face_value.normal[0], face_value.normal[1], face_value.normal[2]},
            {0.0F, 0.0F}});
        }
        box_indices.insert(box_indices.end(), {
          base, static_cast<std::uint16_t>(base + 1), static_cast<std::uint16_t>(base + 2),
          base, static_cast<std::uint16_t>(base + 2), static_cast<std::uint16_t>(base + 3)});
      }
    }

    auto build_surface_patch_mesh() -> void
    {
      constexpr std::uint32_t segments = 255;
      constexpr double moon_radius_metres = 1737400.0;
      // Cover beyond the roughly 83 km lunar horizon visible from the local
      // camera's 2 km maximum distance. Quadratic spacing keeps dense vertices
      // around the base while allowing one patch to replace the coarse globe.
      constexpr double patch_half_width_metres = 131072.0;
      constexpr std::array<double, 3> center{{
        0.8660254037835535, 0.0, 0.49999999999996425}};
      constexpr std::array<double, 3> east{{0.0, 1.0, 0.0}};
      constexpr std::array<double, 3> north{{
        -0.49999999999996425, 0.0, 0.8660254037835535}};
      surface_patch_vertices.reserve((segments + 1) * (segments + 1));
      surface_patch_indices.reserve(segments * segments * 6);
      for (std::uint32_t row = 0; row <= segments; ++row)
      {
        const double north_normalized = -1.0 +
          2.0 * static_cast<double>(row) / static_cast<double>(segments);
        const double north_metres = std::copysign(
          north_normalized * north_normalized * patch_half_width_metres,
          north_normalized);
        for (std::uint32_t column = 0; column <= segments; ++column)
        {
          const double east_normalized = -1.0 +
            2.0 * static_cast<double>(column) /
              static_cast<double>(segments);
          const double east_metres = std::copysign(
            east_normalized * east_normalized * patch_half_width_metres,
            east_normalized);
          double x = center[0] + east[0] * east_metres / moon_radius_metres +
            north[0] * north_metres / moon_radius_metres;
          double y = center[1] + east[1] * east_metres / moon_radius_metres +
            north[1] * north_metres / moon_radius_metres;
          double z = center[2] + east[2] * east_metres / moon_radius_metres +
            north[2] * north_metres / moon_radius_metres;
          const double length = std::sqrt(x * x + y * y + z * z);
          x /= length;
          y /= length;
          z /= length;
          constexpr double detail_period_metres = 16384.0;
          const float u = static_cast<float>(
            0.5 + east_metres / detail_period_metres);
          const float v = static_cast<float>(
            0.5 - north_metres / detail_period_metres);
          // Close terrain uses marker-relative metre coordinates. Keeping the
          // vertex magnitude near the local patch avoids quantizing metre-scale
          // height against a 1,737,400-metre unit-sphere transform.
          surface_patch_vertices.push_back({{
            static_cast<float>((x - center[0]) * moon_radius_metres),
            static_cast<float>((y - center[1]) * moon_radius_metres),
            static_cast<float>((z - center[2]) * moon_radius_metres)},
            {static_cast<float>(x), static_cast<float>(y),
             static_cast<float>(z)}, {u, v}});
        }
      }
      for (std::uint32_t row = 0; row < segments; ++row)
      {
        for (std::uint32_t column = 0; column < segments; ++column)
        {
          const auto first = static_cast<std::uint16_t>(
            row * (segments + 1) + column);
          const auto second = static_cast<std::uint16_t>(first + segments + 1);
          surface_patch_indices.insert(surface_patch_indices.end(), {
            first, second, static_cast<std::uint16_t>(first + 1),
            static_cast<std::uint16_t>(first + 1), second,
            static_cast<std::uint16_t>(second + 1)});
        }
      }
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

    static auto load_image(const std::string &path) -> image
    {
      std::ifstream input(path, std::ios::binary);
      std::string magic;
      std::uint32_t width{};
      std::uint32_t height{};
      std::uint32_t maximum{};
      input >> magic >> width >> height >> maximum;
      if (!input || magic != "P6" || width == 0 || height == 0 || maximum != 255)
        throw std::runtime_error("Invalid planetary albedo map " + path);
      input.get();
      std::vector<std::uint8_t> encoded(
        static_cast<std::size_t>(width) * height * 3);
      input.read(
        reinterpret_cast<char *>(encoded.data()),
        static_cast<std::streamsize>(encoded.size()));
      if (!input)
        throw std::runtime_error("Incomplete planetary albedo map " + path);
      image result{width, height, {}};
      result.rgba.resize(static_cast<std::size_t>(width) * height * 4);
      for (std::size_t pixel = 0; pixel < encoded.size() / 3; ++pixel)
      {
        for (std::size_t channel = 0; channel < 3; ++channel)
          result.rgba[pixel * 4 + channel] = encoded[pixel * 3 + channel];
        result.rgba[pixel * 4 + 3] = 255;
      }
      return result;
    }

    auto load_shader(const std::string &root, const char *stage,
                     const SDL_GPUShaderStage shader_stage,
                     const std::uint32_t uniform_buffers,
                     const std::uint32_t samplers,
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
      info.num_samplers = samplers;
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
        root, "vert", SDL_GPU_SHADERSTAGE_VERTEX, 1, 0, selected);
      auto *fragment_shader = load_shader(
        root, "frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 3, 2, selected);
      const SDL_GPUVertexBufferDescription buffer_description{
        0, sizeof(vertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
      const std::array<SDL_GPUVertexAttribute, 3> attributes{{
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(vertex, position)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(vertex, normal)},
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(vertex, uv)}
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

    auto upload_texture(const image &source) -> SDL_GPUTexture *
    {
      std::uint32_t mip_levels = 1;
      for (std::uint32_t extent = std::max(source.width, source.height);
           extent > 1; extent /= 2)
        ++mip_levels;
      SDL_GPUTextureCreateInfo texture_info{};
      texture_info.type = SDL_GPU_TEXTURETYPE_2D;
      texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER |
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
      texture_info.width = source.width;
      texture_info.height = source.height;
      texture_info.layer_count_or_depth = 1;
      texture_info.num_levels = mip_levels;
      texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
      auto *texture = SDL_CreateGPUTexture(device, &texture_info);
      SDL_GPUTransferBufferCreateInfo transfer_info{};
      transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
      transfer_info.size = source.rgba.size();
      auto *upload = SDL_CreateGPUTransferBuffer(device, &transfer_info);
      if (!texture || !upload) fail("Could not create planetary texture upload");
      auto *mapped = static_cast<std::uint8_t *>(
        SDL_MapGPUTransferBuffer(device, upload, false));
      if (!mapped) fail("Could not map planetary texture upload");
      std::memcpy(mapped, source.rgba.data(), source.rgba.size());
      SDL_UnmapGPUTransferBuffer(device, upload);
      auto *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire planetary texture commands");
      auto *copy = SDL_BeginGPUCopyPass(commands);
      const SDL_GPUTextureTransferInfo source_region{
        upload, 0, source.width, source.height};
      const SDL_GPUTextureRegion destination{
        texture, 0, 0, 0, 0, 0, source.width, source.height, 1};
      SDL_UploadToGPUTexture(copy, &source_region, &destination, false);
      SDL_EndGPUCopyPass(copy);
      if (mip_levels > 1)
        SDL_GenerateMipmapsForGPUTexture(commands, texture);
      auto *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
      if (!fence || !SDL_WaitForGPUFences(device, true, &fence, 1))
        fail("Could not upload planetary texture");
      SDL_ReleaseGPUFence(device, fence);
      SDL_ReleaseGPUTransferBuffer(device, upload);
      return texture;
    }

    auto create_surface_textures() -> void
    {
      const image white{1, 1, {255, 255, 255, 255}};
      white_texture = upload_texture(white);
      earth_texture = upload_texture(load_image(
        "assets/planetary/earth_blue_marble_1024x512.ppm"));
      moon_texture = upload_texture(load_image(
        "assets/planetary/moon_lro_2048x1024.ppm"));
      moon_detail_texture = upload_texture(load_image(
        "assets/planetary/moon_shackleton_rim_2048x2048.ppm"));
      SDL_GPUSamplerCreateInfo sampler_info{};
      sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
      sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
      sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
      sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
      sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
      sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
      sampler_info.max_anisotropy = 8.0F;
      sampler_info.min_lod = 0.0F;
      sampler_info.max_lod = 16.0F;
      sampler_info.enable_anisotropy = true;
      surface_sampler = SDL_CreateGPUSampler(device, &sampler_info);
      if (!surface_sampler) fail("Could not create planetary surface sampler");
      sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
      sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
      local_detail_sampler = SDL_CreateGPUSampler(device, &sampler_info);
      if (!local_detail_sampler)
        fail("Could not create local detail sampler");
    }

    auto upload_mesh() -> void
    {
      SDL_GPUBufferCreateInfo buffer{};
      buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
      const auto vertex_bytes = static_cast<std::uint32_t>(vertices.size() * sizeof(vertex));
      const auto index_bytes = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint16_t));
      const auto box_vertex_bytes = static_cast<std::uint32_t>(
        box_vertices.size() * sizeof(vertex));
      const auto box_index_bytes = static_cast<std::uint32_t>(
        box_indices.size() * sizeof(std::uint16_t));
      const auto surface_patch_vertex_bytes = static_cast<std::uint32_t>(
        surface_patch_vertices.size() * sizeof(vertex));
      const auto surface_patch_index_bytes = static_cast<std::uint32_t>(
        surface_patch_indices.size() * sizeof(std::uint16_t));
      buffer.size = vertex_bytes;
      vertex_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX;
      buffer.size = index_bytes;
      index_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
      buffer.size = box_vertex_bytes;
      box_vertex_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX;
      buffer.size = box_index_bytes;
      box_index_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
      buffer.size = surface_patch_vertex_bytes;
      surface_patch_vertex_buffer = SDL_CreateGPUBuffer(device, &buffer);
      buffer.usage = SDL_GPU_BUFFERUSAGE_INDEX;
      buffer.size = surface_patch_index_bytes;
      surface_patch_index_buffer = SDL_CreateGPUBuffer(device, &buffer);
      SDL_GPUTransferBufferCreateInfo transfer{
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        vertex_bytes + index_bytes + box_vertex_bytes + box_index_bytes +
          surface_patch_vertex_bytes + surface_patch_index_bytes, 0};
      auto *upload = SDL_CreateGPUTransferBuffer(device, &transfer);
      if (!vertex_buffer || !index_buffer || !box_vertex_buffer ||
          !box_index_buffer || !surface_patch_vertex_buffer ||
          !surface_patch_index_buffer || !upload)
        fail("Could not create indexed scene buffers");
      auto *mapped = static_cast<std::uint8_t *>(
        SDL_MapGPUTransferBuffer(device, upload, false));
      if (!mapped) fail("Could not map indexed scene upload");
      std::memcpy(mapped, vertices.data(), vertex_bytes);
      std::memcpy(mapped + vertex_bytes, indices.data(), index_bytes);
      std::memcpy(mapped + vertex_bytes + index_bytes,
                  box_vertices.data(), box_vertex_bytes);
      std::memcpy(mapped + vertex_bytes + index_bytes + box_vertex_bytes,
                  box_indices.data(), box_index_bytes);
      std::memcpy(
        mapped + vertex_bytes + index_bytes + box_vertex_bytes + box_index_bytes,
        surface_patch_vertices.data(), surface_patch_vertex_bytes);
      std::memcpy(
        mapped + vertex_bytes + index_bytes + box_vertex_bytes + box_index_bytes +
          surface_patch_vertex_bytes,
        surface_patch_indices.data(), surface_patch_index_bytes);
      SDL_UnmapGPUTransferBuffer(device, upload);
      auto *commands = SDL_AcquireGPUCommandBuffer(device);
      if (!commands) fail("Could not acquire indexed scene upload commands");
      auto *copy = SDL_BeginGPUCopyPass(commands);
      SDL_GPUTransferBufferLocation source{upload, 0};
      SDL_GPUBufferRegion destination{vertex_buffer, 0, vertex_bytes};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset = vertex_bytes;
      destination = {index_buffer, 0, index_bytes};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset = vertex_bytes + index_bytes;
      destination = {box_vertex_buffer, 0, box_vertex_bytes};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset = vertex_bytes + index_bytes + box_vertex_bytes;
      destination = {box_index_buffer, 0, box_index_bytes};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset = vertex_bytes + index_bytes + box_vertex_bytes +
        box_index_bytes;
      destination = {
        surface_patch_vertex_buffer, 0, surface_patch_vertex_bytes};
      SDL_UploadToGPUBuffer(copy, &source, &destination, false);
      source.offset += surface_patch_vertex_bytes;
      destination = {
        surface_patch_index_buffer, 0, surface_patch_index_bytes};
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
      build_sphere_mesh();
      build_box_mesh();
      build_surface_patch_mesh();
      create_pipeline();
      upload_mesh();
      create_surface_textures();
    }

    ~indexed_sphere_pass()
    {
      if (depth) SDL_ReleaseGPUTexture(device, depth);
      if (surface_patch_index_buffer)
        SDL_ReleaseGPUBuffer(device, surface_patch_index_buffer);
      if (surface_patch_vertex_buffer)
        SDL_ReleaseGPUBuffer(device, surface_patch_vertex_buffer);
      if (local_detail_sampler)
        SDL_ReleaseGPUSampler(device, local_detail_sampler);
      if (surface_sampler) SDL_ReleaseGPUSampler(device, surface_sampler);
      if (moon_detail_texture)
        SDL_ReleaseGPUTexture(device, moon_detail_texture);
      if (moon_texture) SDL_ReleaseGPUTexture(device, moon_texture);
      if (earth_texture) SDL_ReleaseGPUTexture(device, earth_texture);
      if (white_texture) SDL_ReleaseGPUTexture(device, white_texture);
      if (box_index_buffer) SDL_ReleaseGPUBuffer(device, box_index_buffer);
      if (box_vertex_buffer) SDL_ReleaseGPUBuffer(device, box_vertex_buffer);
      if (index_buffer) SDL_ReleaseGPUBuffer(device, index_buffer);
      if (vertex_buffer) SDL_ReleaseGPUBuffer(device, vertex_buffer);
      if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    }

    indexed_sphere_pass(const indexed_sphere_pass &) = delete;
    auto operator=(const indexed_sphere_pass &) -> indexed_sphere_pass & = delete;

    auto render(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *color,
                const std::uint32_t width, const std::uint32_t height,
                const std::vector<mesh_draw> &draws,
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
      SDL_PushGPUFragmentUniformData(commands, 1, &lighting, sizeof(lighting));
      for (const auto &draw : draws)
      {
        const scene::viewport logical{draw.target.width, draw.target.height};
        sagan::render::CameraUniform camera_uniform{};
        SDL_GPUBuffer *selected_vertex_buffer = vertex_buffer;
        SDL_GPUBuffer *selected_index_buffer = index_buffer;
        std::size_t selected_index_count = indices.size();
        if (draw.kind == mesh_kind::sphere)
          camera_uniform = scene::prepare_sphere_draw(
            draw.item, draw.camera, logical).camera_uniform;
        else
        {
          camera_uniform = draw.oriented
            ? scene::prepare_oriented_box_draw(
                draw.item, draw.half_extents, draw.axis_x, draw.axis_y,
                draw.axis_z, draw.camera, logical).camera_uniform
            : scene::prepare_box_draw(
                draw.item, draw.half_extents, draw.yaw_radians,
                draw.camera, logical).camera_uniform;
          selected_vertex_buffer = box_vertex_buffer;
          selected_index_buffer = box_index_buffer;
          selected_index_count = box_indices.size();
        }
        const SDL_GPUBufferBinding vertex_binding{selected_vertex_buffer, 0};
        const SDL_GPUBufferBinding index_binding{selected_index_buffer, 0};
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
        SDL_BindGPUIndexBuffer(
          render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        const SDL_GPUViewport viewport{
          draw.target.x, draw.target.y, draw.target.width, draw.target.height,
          0.0F, 1.0F};
        const SDL_Rect scissor{
          static_cast<int>(draw.target.x), static_cast<int>(draw.target.y),
          static_cast<int>(draw.target.width), static_cast<int>(draw.target.height)};
        SDL_SetGPUViewport(render_pass, &viewport);
        SDL_SetGPUScissor(render_pass, &scissor);
        SDL_PushGPUVertexUniformData(
          commands, 0, &camera_uniform, sizeof(camera_uniform));
        auto material = draw.material;
        SDL_GPUTexture *surface = white_texture;
        SDL_GPUTexture *detail = white_texture;
        surface_lod_uniform surface_lod{};
        bool draw_surface_patch = false;
        if (draw.albedo_map == surface_map::earth_blue_marble)
          surface = earth_texture;
        else if (draw.albedo_map == surface_map::moon_lro)
        {
          surface = moon_texture;
          const double camera_dx =
            draw.camera.position.x_metres - draw.item.position.x_metres;
          const double camera_dy =
            draw.camera.position.y_metres - draw.item.position.y_metres;
          const double camera_dz =
            draw.camera.position.z_metres - draw.item.position.z_metres;
          const double camera_altitude = std::sqrt(
            camera_dx * camera_dx + camera_dy * camera_dy +
            camera_dz * camera_dz) - draw.item.radius_metres;
          if (camera_altitude < 100000.0)
          {
            constexpr float marker_latitude_sine = 0.5F;
            constexpr float marker_latitude_cosine = 0.8660254038F;
            constexpr float detail_width_metres = 16384.0F;
            const float angular_width = detail_width_metres /
              static_cast<float>(draw.item.radius_metres);
            surface_lod.center_and_angular_width[0] = marker_latitude_cosine;
            surface_lod.center_and_angular_width[2] = marker_latitude_sine;
            surface_lod.center_and_angular_width[3] = angular_width;
            surface_lod.east_and_enabled[1] = 1.0F;
            surface_lod.east_and_enabled[3] = 1.0F;
            surface_lod.north_and_blend[0] = -marker_latitude_sine;
            surface_lod.north_and_blend[2] = marker_latitude_cosine;
            surface_lod.north_and_blend[3] = 0.08F;
            detail = moon_detail_texture;
            draw_surface_patch = camera_altitude < 5000.0;
          }
        }
        if (draw.albedo_map != surface_map::none)
          material.base_color_linear = {
            1.0F, 1.0F, 1.0F, draw.material.base_color_linear.w};
        const std::array<SDL_GPUTextureSamplerBinding, 2> surface_bindings{{
          {surface, surface_sampler}, {detail, surface_sampler}}};
        SDL_BindGPUFragmentSamplers(
          render_pass, 0, surface_bindings.data(), surface_bindings.size());
        SDL_PushGPUFragmentUniformData(
          commands, 0, &material, sizeof(material));
        SDL_PushGPUFragmentUniformData(
          commands, 2, &surface_lod, sizeof(surface_lod));
        if (!draw_surface_patch)
          SDL_DrawGPUIndexedPrimitives(
            render_pass, selected_index_count, 1, 0, 0, 0);
        if (draw_surface_patch)
        {
          constexpr scene::direction3 patch_center{
            0.8660254037835535, 0.0, 0.49999999999996425};
          constexpr double marker_altitude_metres = 10.0;
          constexpr double foundation_datum_metres = 0.25;
          scene::render_item patch_anchor = draw.item;
          // Match the demo's marker construction and altitude removal exactly.
          // Reassociating this as center + radius changes the rounded anchor at
          // the demo's 1e15-metre precision origin by enough to clip foundations.
          patch_anchor.position.x_metres +=
            patch_center.x *
              (draw.item.radius_metres + marker_altitude_metres);
          patch_anchor.position.y_metres +=
            patch_center.y *
              (draw.item.radius_metres + marker_altitude_metres);
          patch_anchor.position.z_metres +=
            patch_center.z *
              (draw.item.radius_metres + marker_altitude_metres);
          patch_anchor.position.x_metres -=
            patch_center.x * marker_altitude_metres;
          patch_anchor.position.y_metres -=
            patch_center.y * marker_altitude_metres;
          patch_anchor.position.z_metres -=
            patch_center.z * marker_altitude_metres;
          patch_anchor.position.x_metres -=
            patch_center.x * foundation_datum_metres;
          patch_anchor.position.y_metres -=
            patch_center.y * foundation_datum_metres;
          patch_anchor.position.z_metres -=
            patch_center.z * foundation_datum_metres;
          camera_uniform = scene::prepare_oriented_box_draw(
            patch_anchor, {1.0, 1.0, 1.0},
            {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
            draw.camera, logical).camera_uniform;
          SDL_PushGPUVertexUniformData(
            commands, 0, &camera_uniform, sizeof(camera_uniform));
          surface_lod_uniform patch_surface_lod{};
          SDL_PushGPUFragmentUniformData(
            commands, 2, &patch_surface_lod, sizeof(patch_surface_lod));
          // The close patch uses metre-based repeating UVs and never samples
          // the low-resolution global Moon map. This keeps the Shackleton
          // detail scale stable without exposing its square blend boundary.
          const std::array<SDL_GPUTextureSamplerBinding, 2>
            patch_surface_bindings{{
              {moon_detail_texture, local_detail_sampler},
              {moon_detail_texture, local_detail_sampler}}};
          SDL_BindGPUFragmentSamplers(
            render_pass, 0, patch_surface_bindings.data(),
            patch_surface_bindings.size());
          const SDL_GPUBufferBinding patch_vertex_binding{
            surface_patch_vertex_buffer, 0};
          const SDL_GPUBufferBinding patch_index_binding{
            surface_patch_index_buffer, 0};
          SDL_BindGPUVertexBuffers(
            render_pass, 0, &patch_vertex_binding, 1);
          SDL_BindGPUIndexBuffer(
            render_pass, &patch_index_binding,
            SDL_GPU_INDEXELEMENTSIZE_16BIT);
          SDL_DrawGPUIndexedPrimitives(
            render_pass, surface_patch_indices.size(), 1, 0, 0, 0);
        }
      }
      SDL_EndGPURenderPass(render_pass);
    }

    auto render(SDL_GPUCommandBuffer *commands, SDL_GPUTexture *color,
                const std::uint32_t width, const std::uint32_t height,
                const std::vector<sphere_draw> &draws,
                const sagan::render::LightingUniform &lighting) -> void
    {
      std::vector<mesh_draw> meshes;
      meshes.reserve(draws.size());
      for (const auto &draw : draws)
        meshes.push_back({mesh_kind::sphere, draw.item, {}, 0.0,
                          draw.camera, draw.target, draw.material});
      render(commands, color, width, height, meshes, lighting);
    }
  };
}
