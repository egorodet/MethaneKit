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

FILE: Methane/Graphics/DirectX12/RenderCommandListDX.cpp
DirectX 12 implementation of the render command list interface.

******************************************************************************/

#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"
#include "RenderStateDX.h"
#include "RenderPassDX.h"
#include "CommandQueueDX.h"
#include "DeviceDX.h"
#include "ProgramDX.h"
#include "BufferDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>

#include <magic_enum.hpp>
#include <d3dx12.h>

namespace Methane::Graphics
{

static D3D12_PRIMITIVE_TOPOLOGY PrimitiveToDXTopology(RenderCommandList::Primitive primitive)
{
    META_FUNCTION_TASK();
    switch (primitive)
    {
    case RenderCommandList::Primitive::Point:          return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RenderCommandList::Primitive::Line:           return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RenderCommandList::Primitive::LineStrip:      return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RenderCommandList::Primitive::Triangle:       return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RenderCommandList::Primitive::TriangleStrip:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:                                           META_UNEXPECTED_ARG_RETURN(primitive, D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
    }
}

Ptr<RenderCommandList> RenderCommandList::Create(CommandQueue& cmd_queue, RenderPass& render_pass)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<CommandQueueBase&>(cmd_queue), static_cast<RenderPassBase&>(render_pass));
}

Ptr<RenderCommandList> RenderCommandList::Create(ParallelRenderCommandList& parallel_render_command_list)
{
    META_FUNCTION_TASK();
    return std::make_shared<RenderCommandListDX>(static_cast<ParallelRenderCommandListBase&>(parallel_render_command_list));
}

RenderCommandListDX::RenderCommandListDX(CommandQueueBase& cmd_buffer, RenderPassBase& render_pass)
    : CommandListDX<RenderCommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_buffer, render_pass)
{
    META_FUNCTION_TASK();
}

RenderCommandListDX::RenderCommandListDX(ParallelRenderCommandListBase& parallel_render_command_list)
    : CommandListDX<RenderCommandListBase>(D3D12_COMMAND_LIST_TYPE_DIRECT, parallel_render_command_list)
{
    META_FUNCTION_TASK();
}

void RenderCommandListDX::ResetNative(const Ptr<RenderStateDX>& render_state_ptr)
{
    META_FUNCTION_TASK();
    if (!IsNativeCommitted())
        return;

    SetNativeCommitted(false);
    SetCommandListState(CommandList::State::Encoding);

    ID3D12PipelineState* p_dx_initial_state = render_state_ptr ? render_state_ptr->GetNativePipelineState().Get() : nullptr;
    ID3D12CommandAllocator& dx_cmd_allocator = GetNativeCommandAllocatorRef();
    ID3D12Device* p_native_device = GetCommandQueueDX().GetContextDX().GetDeviceDX().GetNativeDevice().Get();
    ThrowIfFailed(dx_cmd_allocator.Reset(), p_native_device);
    ThrowIfFailed(GetNativeCommandListRef().Reset(&dx_cmd_allocator, p_dx_initial_state), p_native_device);

    // Insert beginning GPU timestamp query
    if (HasBeginTimestampQuery())
        GetBeginTimestampQuery().InsertTimestamp();

    if (!render_state_ptr)
        return;

    using namespace magic_enum::bitwise_operators;
    DrawingState& drawing_state = GetDrawingState();
    drawing_state.render_state_ptr    = render_state_ptr;
    drawing_state.render_state_groups = RenderState::Groups::Program
                                      | RenderState::Groups::Rasterizer
                                      | RenderState::Groups::DepthStencil;
}

void RenderCommandListDX::ResetRenderPass()
{
    META_FUNCTION_TASK();
    RenderPassDX& pass_dx = GetPassDX();

    if (IsParallel())
    {
        pass_dx.SetNativeDescriptorHeaps(*this);
        pass_dx.SetNativeRenderTargets(*this);
    }
    else if (!pass_dx.IsBegun())
    {
        pass_dx.Begin(*this);
    }
}

void RenderCommandListDX::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative();
    RenderCommandListBase::Reset(p_debug_group);
    ResetRenderPass();
}

void RenderCommandListDX::ResetWithState(RenderState& render_state, DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    ResetNative(std::static_pointer_cast<RenderStateDX>(static_cast<RenderStateBase&>(render_state).GetBasePtr()));
    RenderCommandListBase::ResetWithState(render_state, p_debug_group);
    ResetRenderPass();
}

void RenderCommandListDX::SetVertexBuffers(BufferSet& vertex_buffers)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    RenderCommandListBase::SetVertexBuffers(vertex_buffers);

    DrawingState& drawing_state = GetDrawingState();
    if (!magic_enum::flags::enum_contains(drawing_state.changes & DrawingState::Changes::VertexBuffers))
        return;

    BufferSetDX& dx_vertex_buffer_set = static_cast<BufferSetDX&>(vertex_buffers);
    if (dx_vertex_buffer_set.SetState(Resource::State::VertexAndConstantBuffer) && dx_vertex_buffer_set.GetSetupTransitionBarriers())
    {
        SetResourceBarriers(*dx_vertex_buffer_set.GetSetupTransitionBarriers());
    }

    const std::vector<D3D12_VERTEX_BUFFER_VIEW>& vertex_buffer_views = dx_vertex_buffer_set.GetNativeVertexBufferViews();
    GetNativeCommandListRef().IASetVertexBuffers(0, static_cast<UINT>(vertex_buffer_views.size()), vertex_buffer_views.data());
    drawing_state.changes &= ~DrawingState::Changes::VertexBuffers;
}

void RenderCommandListDX::DrawIndexed(Primitive primitive, Buffer& index_buffer,
                                      uint32_t index_count, uint32_t start_index, uint32_t start_vertex,
                                      uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    auto& dx_index_buffer = static_cast<IndexBufferDX&>(index_buffer);
    if (!index_count)
    {
        index_count = dx_index_buffer.GetFormattedItemsCount();
    }

    RenderCommandListBase::DrawIndexed(primitive, index_buffer, index_count, start_index, start_vertex, instance_count, start_instance);

    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    DrawingState& drawing_state = GetDrawingState();
    if (magic_enum::flags::enum_contains(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }
    if (magic_enum::flags::enum_contains(drawing_state.changes & DrawingState::Changes::IndexBuffer))
    {
        Ptr<Resource::Barriers>& buffer_setup_barriers_ptr = dx_index_buffer.GetSetupTransitionBarriers();
        if (dx_index_buffer.SetState(Resource::State::IndexBuffer, buffer_setup_barriers_ptr) && buffer_setup_barriers_ptr)
        {
            SetResourceBarriers(*buffer_setup_barriers_ptr);
        }
        dx_command_list.IASetIndexBuffer(&dx_index_buffer.GetNativeView());
        drawing_state.changes &= ~DrawingState::Changes::IndexBuffer;
    }
    dx_command_list.DrawIndexedInstanced(index_count, instance_count, start_index, start_vertex, start_instance);
}

void RenderCommandListDX::Draw(Primitive primitive, uint32_t vertex_count, uint32_t start_vertex,
                               uint32_t instance_count, uint32_t start_instance)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    RenderCommandListBase::Draw(primitive, vertex_count, start_vertex, instance_count, start_instance);

    ID3D12GraphicsCommandList& dx_command_list = GetNativeCommandListRef();
    if (DrawingState& drawing_state = GetDrawingState();
        magic_enum::flags::enum_contains(drawing_state.changes & DrawingState::Changes::PrimitiveType))
    {
        const D3D12_PRIMITIVE_TOPOLOGY primitive_topology = PrimitiveToDXTopology(primitive);
        dx_command_list.IASetPrimitiveTopology(primitive_topology);
        drawing_state.changes &= ~DrawingState::Changes::PrimitiveType;
    }
    dx_command_list.DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
}

void RenderCommandListDX::Commit()
{
    META_FUNCTION_TASK();
    if (IsParallel())
    {
        CommandListDX<RenderCommandListBase>::Commit();
        return;
    }

    if (RenderPassDX& pass_dx = GetPassDX();
        pass_dx.IsBegun())
    {
        pass_dx.End(*this);
    }

    CommandListDX<RenderCommandListBase>::Commit();
}

RenderPassDX& RenderCommandListDX::GetPassDX()
{
    META_FUNCTION_TASK();
    return static_cast<RenderPassDX&>(GetPass());
}

} // namespace Methane::Graphics
