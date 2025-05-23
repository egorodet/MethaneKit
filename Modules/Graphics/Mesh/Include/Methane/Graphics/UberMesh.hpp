/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Graphics/UberMesh.hpp
Uber mesh generator with customizable vertex type

******************************************************************************/

#pragma once

#include "BaseMesh.hpp"

#include <algorithm>

namespace Methane::Graphics
{

template<typename VType>
class UberMesh : public BaseMesh<VType>
{
public:
    using BaseMeshT = BaseMesh<VType>;

    explicit UberMesh(const Mesh::VertexLayout& vertex_layout)
        : BaseMeshT(Mesh::Type::Uber, vertex_layout)
    { }

    void AddSubMesh(const BaseMeshT& sub_mesh, bool adjust_indices)
    {
        META_FUNCTION_TASK();
        const typename BaseMeshT::Vertices& sub_vertices = sub_mesh.GetVertices();
        const Mesh::Indices& sub_indices = sub_mesh.GetIndices();

        m_subsets.emplace_back(sub_mesh.GetType(),
                               Mesh::Subset::Slice(BaseMeshT::GetVertexCount(), static_cast<Data::Size>(sub_vertices.size())),
                               Mesh::Subset::Slice(Mesh::GetIndexCount(),       static_cast<Data::Size>(sub_indices.size())),
                               adjust_indices);

        if (adjust_indices)
        {
            const Data::Size vertex_count = BaseMeshT::GetVertexCount();
            META_CHECK_LESS(vertex_count, std::numeric_limits<Mesh::Index>::max());

            const auto index_offset = static_cast<Mesh::Index>(vertex_count);
            std::ranges::transform(sub_indices, BaseMeshT::GetIndicesBackInserter(),
                           [index_offset](const Mesh::Index& index)
                           {
                               META_CHECK_LESS(index_offset, std::numeric_limits<Mesh::Index>::max() - index);
                               return static_cast<Mesh::Index>(index_offset + index);
                           });
        }
        else
        {
            BaseMeshT::AppendIndices(sub_indices);
        }

        BaseMeshT::AppendVertices(sub_vertices);
    }

    const Mesh::Subsets& GetSubsets() const                     { return m_subsets; }
    size_t               GetSubsetCount() const noexcept        { return m_subsets.size(); }
    const Mesh::Subset&  GetSubset(size_t subset_index) const
    {
        META_FUNCTION_TASK();
        META_CHECK_LESS(subset_index, m_subsets.size());
        return m_subsets[subset_index];
    }

    std::pair<const VType*, size_t> GetSubsetVertices(size_t subset_index) const
    {
        META_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { BaseMeshT::GetVertices().data() + subset.vertices.offset, subset.vertices.count };
    }

    std::pair<const Mesh::Index*, size_t> GetSubsetIndices(size_t subset_index) const
    {
        META_FUNCTION_TASK();
        const Mesh::Subset& subset = GetSubset(subset_index);
        return { Mesh::GetIndices().data() + subset.indices.offset, subset.indices.count };
    }

private:
    Mesh::Subsets m_subsets;
};

} // namespace Methane::Graphics
