#pragma once

#include "material_contract.hpp"
#include "scene_mesh_contract.hpp"

#include <SDL3/SDL.h>

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
    static constexpr std::uint32_t latitude_segments = 16;
    static constexpr std::uint32_t longitude_segments = 32;
    std::vector<vertex> vertices;
    std::vector<std::uint16_t> indices;
    std::vector<vertex> box_vertices;
    std::vector<std::uint16_t> box_indices;

    SDL_GPUDevice *device{};
    SDL_GPUGraphicsPipeline *pipeline{};
    SDL_GPUBuffer *vertex_buffer{};
    SDL_GPUBuffer *index_buffer{};
    SDL_GPUBuffer *box_vertex_buffer{};
    SDL_GPUBuffer *box_index_buffer{};
    SDL_GPUTexture *depth{};
    std::uint32_t depth_width{};
    std::uint32_t depth_height{};
    static constexpr std::uint32_t map_width = longitude_segments;
    static constexpr std::uint32_t map_height = latitude_segments;
    using albedo_pixels = std::array<std::array<float, 3>, map_width * map_height>;
    albedo_pixels earth_albedo{};
    albedo_pixels moon_albedo{};

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
          vertices.push_back({{x, y, z}, {x, y, z}});
        }
      }
      for (std::uint32_t latitude = 0; latitude < latitude_segments; ++latitude)
      {
        for (std::uint32_t longitude = 0; longitude < longitude_segments; ++longitude)
        {
          const auto first = static_cast<std::uint16_t>(
            latitude * (longitude_segments + 1) + longitude);
          const auto second = static_cast<std::uint16_t>(first + longitude_segments + 1);
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
            {face_value.normal[0], face_value.normal[1], face_value.normal[2]}});
        }
        box_indices.insert(box_indices.end(), {
          base, static_cast<std::uint16_t>(base + 1), static_cast<std::uint16_t>(base + 2),
          base, static_cast<std::uint16_t>(base + 2), static_cast<std::uint16_t>(base + 3)});
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

    static auto load_albedo(const std::string &path) -> albedo_pixels
    {
      std::ifstream input(path, std::ios::binary);
      std::string magic;
      std::uint32_t width{};
      std::uint32_t height{};
      std::uint32_t maximum{};
      input >> magic >> width >> height >> maximum;
      if (!input || magic != "P6" || width != map_width ||
          height != map_height || maximum != 255)
        throw std::runtime_error("Invalid planetary albedo map " + path);
      input.get();
      std::array<std::uint8_t, map_width * map_height * 3> encoded{};
      input.read(reinterpret_cast<char *>(encoded.data()), encoded.size());
      if (!input)
        throw std::runtime_error("Incomplete planetary albedo map " + path);
      albedo_pixels result{};
      for (std::size_t pixel = 0; pixel < result.size(); ++pixel)
      {
        for (std::size_t channel = 0; channel < 3; ++channel)
        {
          // The bootstrap material target is UNORM rather than sRGB and does
          // not yet encode linear fragment output for display. Preserve the
          // source's display values here so real albedo remains legible; the
          // sampled-texture shader milestone will own proper transfer curves.
          result[pixel][channel] =
            static_cast<float>(encoded[pixel * 3 + channel]) / 255.0F;
        }
      }
      return result;
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
      const auto vertex_bytes = static_cast<std::uint32_t>(vertices.size() * sizeof(vertex));
      const auto index_bytes = static_cast<std::uint32_t>(indices.size() * sizeof(std::uint16_t));
      const auto box_vertex_bytes = static_cast<std::uint32_t>(
        box_vertices.size() * sizeof(vertex));
      const auto box_index_bytes = static_cast<std::uint32_t>(
        box_indices.size() * sizeof(std::uint16_t));
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
      SDL_GPUTransferBufferCreateInfo transfer{
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        vertex_bytes + index_bytes + box_vertex_bytes + box_index_bytes, 0};
      auto *upload = SDL_CreateGPUTransferBuffer(device, &transfer);
      if (!vertex_buffer || !index_buffer || !box_vertex_buffer ||
          !box_index_buffer || !upload)
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
      earth_albedo = load_albedo(
        "assets/planetary/earth_blue_marble_32x16.ppm");
      moon_albedo = load_albedo(
        "assets/planetary/moon_lro_32x16.ppm");
      create_pipeline();
      upload_mesh();
    }

    ~indexed_sphere_pass()
    {
      if (depth) SDL_ReleaseGPUTexture(device, depth);
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
        if (draw.kind == mesh_kind::sphere &&
            draw.albedo_map != surface_map::none)
        {
          const auto &map = draw.albedo_map == surface_map::earth_blue_marble
            ? earth_albedo : moon_albedo;
          for (std::uint32_t latitude = 0; latitude < latitude_segments; ++latitude)
          {
            for (std::uint32_t longitude = 0; longitude < longitude_segments;
                 ++longitude)
            {
              // NASA maps are centered on zero longitude; the procedural
              // sphere begins at +X, so offset its first cell by half a map.
              const std::uint32_t map_longitude =
                (longitude + longitude_segments / 2) % longitude_segments;
              const auto &sample = map[latitude * map_width + map_longitude];
              auto material = draw.material;
              material.base_color_linear.x = sample[0];
              material.base_color_linear.y = sample[1];
              material.base_color_linear.z = sample[2];
              const float visibility_floor =
                draw.albedo_map == surface_map::earth_blue_marble ? 1.0F : 0.12F;
              material.emissive_linear_and_roughness.x +=
                sample[0] * visibility_floor;
              material.emissive_linear_and_roughness.y +=
                sample[1] * visibility_floor;
              material.emissive_linear_and_roughness.z +=
                sample[2] * visibility_floor;
              SDL_PushGPUFragmentUniformData(
                commands, 0, &material, sizeof(material));
              const std::uint32_t first_index =
                (latitude * longitude_segments + longitude) * 6;
              SDL_DrawGPUIndexedPrimitives(
                render_pass, 6, 1, first_index, 0, 0);
            }
          }
        }
        else
        {
          SDL_PushGPUFragmentUniformData(
            commands, 0, &draw.material, sizeof(draw.material));
          SDL_DrawGPUIndexedPrimitives(
            render_pass, selected_index_count, 1, 0, 0, 0);
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
