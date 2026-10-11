name = "tonemap"
source = "tonemap.hlsl"
color_space = "linear-to-display"

[vertex]
entrypoint = "VSMain"
uniform_buffers = 0
samplers = 0
storage_textures = 0
storage_buffers = 0

[fragment]
entrypoint = "PSMain"
uniform_buffers = 1
samplers = 1
storage_textures = 0
storage_buffers = 0

[render_state]
primitive = "triangle-list"
depth_test = false
depth_write = false
blend = false
