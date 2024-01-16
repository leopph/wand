struct Output {
  float2 uv : TEXCOORD;
  float4 pos : SV_Position;
};

Output main(const uint vertex_id : SV_VertexID) {
  const StructuredBuffer<float4> vertices = ResourceDescriptorHeap[0];
  const StructuredBuffer<float2> uvs = ResourceDescriptorHeap[1];
  
  Output o;
  o.pos = vertices[vertex_id];
  o.uv = uvs[vertex_id];
  return o;
}
