struct VertexInput {
  float3 position : TEXCOORD0;
  float3 normal : TEXCOORD1;
};

struct VertexOutput {
  float4 position : SV_Position;
  float3 normal : TEXCOORD0;
};

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

VertexOutput VSMain(VertexInput input) {
  VertexOutput output;
  output.position = mul(float4(input.position, 1.0), model_view_projection);
  output.normal = normalize(mul(float4(input.normal, 0.0), normal_matrix).xyz);
  return output;
}

float4 PSMain(VertexOutput input) : SV_Target0 {
  float cosine = max(0.0, dot(normalize(input.normal),
                              direction_to_light_and_intensity.xyz));
  float diffuse = cosine * direction_to_light_and_intensity.w;
  float3 lit = base_color_linear.rgb *
    (ambient_linear.rgb + light_color_linear.rgb * diffuse);
  return float4(lit + emissive_linear_and_roughness.rgb,
                base_color_linear.a);
}
