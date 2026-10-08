struct VertexInput {
  float3 position : TEXCOORD0;
  float3 normal : TEXCOORD1;
  float2 uv : TEXCOORD2;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 normal : TEXCOORD0;
  float2 uv : TEXCOORD1;
  float3 local_position : TEXCOORD2;
};

Texture2D surface_texture : register(t0, space2);
SamplerState surface_sampler : register(s0, space2);
Texture2D detail_texture : register(t1, space2);
SamplerState detail_sampler : register(s1, space2);

cbuffer CameraUniform : register(b0, space1) {
  row_major float4x4 model_view_projection;
  row_major float4x4 normal_matrix;
};

cbuffer MaterialUniform : register(b0, space3) {
  float4 base_color_linear;
  float4 emissive_linear_and_roughness;
};

cbuffer LightingUniform : register(b1, space3) {
  float4 ambient_linear;
  float4 direction_to_light_and_intensity;
  float4 light_color_linear;
};

cbuffer SurfaceLodUniform : register(b2, space3) {
  float4 local_center_and_angular_width;
  float4 local_east_and_enabled;
  float4 local_north_and_blend;
};

VertexOutput VSMain(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0), model_view_projection);
  output.normal = normalize(mul(float4(input.normal, 0.0), normal_matrix).xyz);
  output.uv = input.uv;
  output.local_position = input.position;
  return output;
}

float4 PSMain(VertexOutput input) : SV_Target0 {
  float cosine = max(0.0, dot(normalize(input.normal),
                              direction_to_light_and_intensity.xyz));
  float diffuse = cosine * direction_to_light_and_intensity.w;
  float3 global_albedo = surface_texture.Sample(surface_sampler, input.uv).rgb;
  float angular_width = max(local_center_and_angular_width.w, 0.000001);
  float3 from_center = input.local_position - local_center_and_angular_width.xyz;
  float2 local_uv = float2(
    0.5 + dot(from_center, local_east_and_enabled.xyz) / angular_width,
    0.5 - dot(from_center, local_north_and_blend.xyz) / angular_width);
  float edge_distance = 0.5 - max(abs(local_uv.x - 0.5), abs(local_uv.y - 0.5));
  float detail_weight = local_east_and_enabled.w * smoothstep(
    0.0, local_north_and_blend.w, edge_distance);
  float3 local_albedo = detail_texture.Sample(detail_sampler, local_uv).rgb;
  float3 albedo = lerp(global_albedo, local_albedo, detail_weight) *
    base_color_linear.rgb;
  float3 lit = albedo *
    (ambient_linear.rgb + light_color_linear.rgb * diffuse);
  return float4(lit + emissive_linear_and_roughness.rgb,
                base_color_linear.a);
}
