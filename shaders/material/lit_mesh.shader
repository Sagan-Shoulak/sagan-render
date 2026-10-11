name = "lit_mesh"
source = "lit_mesh.hlsl"
color_space = "linear"

[vertex]
entrypoint = "VSMain"
uniform_buffers = 1
samplers = 0
storage_textures = 0
storage_buffers = 0

[fragment]
entrypoint = "PSMain"
uniform_buffers = 3
samplers = 2
storage_textures = 0
storage_buffers = 0

[vertex_input.position]
semantic = "TEXCOORD0"
format = "float3"

[vertex_input.normal]
semantic = "TEXCOORD1"
format = "float3"

[vertex_input.uv]
semantic = "TEXCOORD2"
format = "float2"
