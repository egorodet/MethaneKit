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

FILE: Methane/Graphics/DirectX12/BufferDX.h
DirectX 12 implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "BlitCommandListDX.h"
#include "ResourceDX.hpp"

#include <Methane/Graphics/BufferBase.h>
#include <Methane/Graphics/DescriptorHeap.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>
#include <fmt/format.h>
#include <d3dx12.h>

namespace Methane::Graphics
{

template<typename TViewNative, typename... ExtraViewArgs>
class BufferDX final : public ResourceDX<BufferBase>
{
public:
    BufferDX(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage, ExtraViewArgs... view_args)
        : ResourceDX(context, settings, descriptor_by_usage)
    {
        META_FUNCTION_TASK();
        using namespace magic_enum::bitwise_operators;

        const bool is_private_storage  = settings.storage_mode == Buffer::StorageMode::Private;
        const bool is_read_back_buffer = magic_enum::flags::enum_contains(settings.usage_mask & Usage::ReadBack);

        const D3D12_HEAP_TYPE     normal_heap_type = is_private_storage  ? D3D12_HEAP_TYPE_DEFAULT  : D3D12_HEAP_TYPE_UPLOAD;
        const D3D12_HEAP_TYPE            heap_type = is_read_back_buffer ? D3D12_HEAP_TYPE_READBACK : normal_heap_type;
        const D3D12_RESOURCE_STATES resource_state = (is_read_back_buffer || is_private_storage) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
        const CD3DX12_RESOURCE_DESC  resource_desc = CD3DX12_RESOURCE_DESC::Buffer(settings.size);

        InitializeDefaultDescriptors();
        InitializeCommittedResource(resource_desc, heap_type, resource_state);
        InitializeView(view_args...);

        if (is_private_storage)
        {
            m_cp_upload_resource = CreateCommittedResource(resource_desc, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
        }
    }

    // Object overrides
    void SetName(const std::string& name) override
    {
        META_FUNCTION_TASK();
        ResourceDX::SetName(name);

        if (m_cp_upload_resource)
        {
            m_cp_upload_resource->SetName(nowide::widen(fmt::format("{} Upload Resource", name)).c_str());
        }
    }

    // Resource overrides
    void SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue) override
    {
        META_FUNCTION_TASK();
        ResourceDX::SetData(sub_resources, sync_cmd_queue);

        const CD3DX12_RANGE zero_read_range(0U, 0U);
        const bool is_private_storage  = GetSettings().storage_mode == Buffer::StorageMode::Private;
        ID3D12Resource& d3d12_resource = is_private_storage ? *m_cp_upload_resource.Get() : GetNativeResourceRef();
        for(const SubResource& sub_resource : sub_resources)
        {
            ValidateSubResource(sub_resource);

            // Using zero range, since we're not going to read this resource on CPU
            const Data::Index sub_resource_raw_index = sub_resource.GetIndex().GetRawIndex(GetSubresourceCount());
            Data::RawPtr p_sub_resource_data = nullptr;
            ThrowIfFailed(
                d3d12_resource.Map(sub_resource_raw_index, &zero_read_range,
                                   reinterpret_cast<void**>(&p_sub_resource_data)),
                GetContextDX().GetDeviceDX().GetNativeDevice().Get()
            );

            META_CHECK_ARG_NOT_NULL_DESCR(p_sub_resource_data, "failed to map buffer subresource");
            stdext::checked_array_iterator target_data_it(p_sub_resource_data, sub_resource.GetDataSize());
            std::copy(sub_resource.GetDataPtr(), sub_resource.GetDataEndPtr(), target_data_it);

            if (sub_resource.HasDataRange())
            {
                const CD3DX12_RANGE write_range(sub_resource.GetDataRange().GetStart(), sub_resource.GetDataRange().GetEnd());
                d3d12_resource.Unmap(sub_resource_raw_index, &write_range);
            }
            else
            {
                d3d12_resource.Unmap(sub_resource_raw_index, nullptr);
            }
        }

        if (!is_private_storage)
            return;

        // In case of private GPU storage, copy buffer data from intermediate upload resource to the private GPU resource
        const BlitCommandListDX& upload_cmd_list = PrepareResourceUpload(sync_cmd_queue);
        upload_cmd_list.GetNativeCommandList().CopyResource(GetNativeResource(), m_cp_upload_resource.Get());
        GetContext().RequestDeferredAction(Context::DeferredAction::UploadResources);
    }

    SubResource GetData(const SubResource::Index& sub_resource_index = SubResource::Index(), const std::optional<BytesRange>& data_range = {}) override
    {
        META_FUNCTION_TASK();

        using namespace magic_enum::bitwise_operators;
        META_CHECK_ARG_DESCR(GetUsage(), magic_enum::flags::enum_contains(GetUsage() & Usage::ReadBack),
                             "getting buffer data from GPU is allowed for buffers with CPU Read-back flag only");

        ValidateSubResource(sub_resource_index, data_range);

        const Data::Index sub_resource_raw_index = sub_resource_index.GetRawIndex(GetSubresourceCount());
        const Data::Index data_start  = data_range ? data_range->GetStart()  : 0U;
        const Data::Index data_length = data_range ? data_range->GetLength() : GetSubResourceDataSize(sub_resource_index);
        const Data::Index data_end    = data_start + data_length;

        ID3D12Resource& d3d12_resource = GetNativeResourceRef();
        const CD3DX12_RANGE read_range(data_start, data_start + data_length);
        Data::RawPtr p_sub_resource_data = nullptr;
        ThrowIfFailed(
            d3d12_resource.Map(sub_resource_raw_index, &read_range,
                               reinterpret_cast<void**>(&p_sub_resource_data)),
            GetContextDX().GetDeviceDX().GetNativeDevice().Get()
        );

        META_CHECK_ARG_NOT_NULL_DESCR(p_sub_resource_data, "failed to map buffer subresource");

        stdext::checked_array_iterator source_data_it(p_sub_resource_data, data_end);
        Data::Bytes sub_resource_data(data_length, {});
        std::copy(source_data_it + data_start, source_data_it + data_end, sub_resource_data.begin());

        const CD3DX12_RANGE zero_write_range(0, 0);
        d3d12_resource.Unmap(sub_resource_raw_index, &zero_write_range);

        return SubResource(std::move(sub_resource_data), sub_resource_index, data_range);
    }

    const TViewNative& GetNativeView() const { return m_buffer_view; }

private:
    void InitializeView(ExtraViewArgs...);

    // NOTE: in case of resource context placed in descriptor heap, m_buffer_view field holds context descriptor instead of context
    TViewNative                 m_buffer_view;
    wrl::ComPtr<ID3D12Resource> m_cp_upload_resource;
};

struct ReadBackBufferView { };

using VertexBufferDX   = BufferDX<D3D12_VERTEX_BUFFER_VIEW, Data::Size>;
using IndexBufferDX    = BufferDX<D3D12_INDEX_BUFFER_VIEW, PixelFormat>;
using ConstantBufferDX = BufferDX<D3D12_CONSTANT_BUFFER_VIEW_DESC>;
using ReadBackBufferDX = BufferDX<ReadBackBufferView>;

class BufferSetDX final : public BufferSetBase
{
public:
    BufferSetDX(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs);

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& GetNativeVertexBufferViews() const;

private:
    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_vertex_buffer_views;
};

} // namespace Methane::Graphics
