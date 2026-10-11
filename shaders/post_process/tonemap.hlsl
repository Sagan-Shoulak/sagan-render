struct VertexOutput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

Texture2D scene_color : register(t0, space2);
SamplerState scene_sampler : register(s0, space2);

cbuffer PostProcessUniform : register(b0, space3) {
  float4 exposure_and_inverse_gamma;
};

VertexOutput VSMain(uint vertex_id : SV_VertexID) {
  const float2 positions[3] = {
    float2(-1.0, -1.0),
    float2(3.0, -1.0),
    float2(-1.0, 3.0)
  };
  VertexOutput output;
  float2 position = positions[vertex_id];
  output.position = float4(position, 0.0, 1.0);
  output.uv = position * float2(0.5, -0.5) + 0.5;
  return output;
}

float4 PSMain(VertexOutput input) : SV_Target0 {
  float exposure = max(exposure_and_inverse_gamma.x, 0.0);
  float inverse_gamma = max(exposure_and_inverse_gamma.y, 0.0001);
  float3 scene_linear = max(scene_color.Sample(scene_sampler, input.uv).rgb, 0.0);
  float3 mapped = 1.0 - exp(-scene_linear * exposure);
  float3 display_color = pow(mapped, inverse_gamma);
  return float4(display_color, 1.0);
}
