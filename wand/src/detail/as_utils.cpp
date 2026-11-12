#include "wand/detail/as_utils.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "wand/util.hpp"
#include "wand/detail/buffer_utils.hpp"


namespace wand::detail {
auto ToD3D12(
  BuildAccelerationStructureInputs const& inputs,
  std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometry_storage
) -> D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS {
  auto ret = D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS{};

  ret.Flags = std::visit([](auto const& in) { return in.flags; }, inputs);
  ret.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

  std::visit(Overloaded{
    [&](BuildBottomLevelAsInputs const& blas_in) {
      geometry_storage.clear();
      geometry_storage.reserve(blas_in.geometries.size());

      std::ranges::transform(blas_in.geometries, std::back_inserter(geometry_storage),
        [](TriangleGeometry const& tri_geom) {
          auto geom_desc = D3D12_RAYTRACING_GEOMETRY_DESC{};
          geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
          geom_desc.Flags = tri_geom.flags;

          assert(tri_geom.vertices.GetRegion().stride != 0 && "Vertex buffer stride cannot be 0.");
          assert(tri_geom.vertices.GetRegion().size % tri_geom.vertices.GetRegion().stride == 0 &&
            "Vertex buffer size must be multiple of stride.");
          geom_desc.Triangles.VertexFormat = tri_geom.vertex_format;
          geom_desc.Triangles.VertexBuffer.StartAddress = GetGpuAddress(tri_geom.vertices);
          geom_desc.Triangles.VertexBuffer.StrideInBytes = tri_geom.vertices.GetRegion().stride;
          geom_desc.Triangles.VertexCount = ClampCast<UINT>(
            tri_geom.vertices.GetRegion().size / tri_geom.vertices.GetRegion().stride);

          if (tri_geom.indices) {
            assert((tri_geom.index_format == DXGI_FORMAT_R32_UINT || tri_geom.index_format == DXGI_FORMAT_R16_UINT) &&
              "Index format must be u32 or u16.");
            auto const index_size = tri_geom.index_format == DXGI_FORMAT_R32_UINT ? 4u : 2u;
            assert((tri_geom.indices->GetRegion().stride == 0 || tri_geom.indices->GetRegion().stride == index_size)
              && "Index buffer stride must match format.");
            assert(tri_geom.indices->GetRegion().size % index_size == 0 &&
              "Index buffer size must be multiple of element size.");
            geom_desc.Triangles.IndexFormat = tri_geom.index_format;
            geom_desc.Triangles.IndexBuffer = GetGpuAddress(*tri_geom.indices);
            geom_desc.Triangles.IndexCount = ClampCast<UINT>(tri_geom.indices->GetRegion().size / index_size);
          }

          if (tri_geom.transform) {
            geom_desc.Triangles.Transform3x4 = GetGpuAddress(*tri_geom.transform);
          }

          return geom_desc;
        });

      ret.pGeometryDescs = geometry_storage.data();
      ret.NumDescs = ClampCast<UINT>(geometry_storage.size());
      ret.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    },
    [&](BuildTopLevelAsInputs const& tlas_in) {
      auto constexpr element_size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
      assert((tlas_in.instances.GetRegion().stride == 0 || tlas_in.instances.GetRegion().stride == element_size) &&
        "Instance buffer stride must be 0 or sizeof(D3D12_RAYTRACING_INSTANCE_DESC).");
      assert(tlas_in.instances.GetRegion().size % element_size == 0 &&
        "Instance buffer size must be multiple of sizeof(D3D12_RAYTRACING_INSTANCE_DESC).");
      assert((tlas_in.instances.GetRegion().offset % D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT) == 0 &&
        "Instance buffer must be aligned to D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT.");
      ret.InstanceDescs = GetGpuAddress(tlas_in.instances);
      ret.NumDescs = ClampCast<UINT>(tlas_in.instances.GetRegion().size / element_size);
      ret.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    }
  }, inputs);

  return ret;
}
}
