float4 main(const float2 uv : TEXCOORD) : SV_Target {
  const Texture2D tex = ResourceDescriptorHeap[2];
  const SamplerState samp = SamplerDescriptorHeap[0];
  return tex.Sample(samp, uv);
}
