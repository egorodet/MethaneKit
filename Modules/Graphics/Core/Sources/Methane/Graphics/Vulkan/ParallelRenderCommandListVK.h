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

FILE: Methane/Graphics/Vulkan/ParallelRenderCommandListVK.h
Vulkan implementation of the parallel render command list interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/ParallelRenderCommandListBase.h>

namespace Methane::Graphics
{

class CommandQueueVK;
class BufferVK;
class RenderPassVK;

class ParallelRenderCommandListVK final : public ParallelRenderCommandListBase
{
public:
    ParallelRenderCommandListVK(CommandQueueBase& command_queue, RenderPassBase& render_pass);

    // ParallelRenderCommandList interface
    void Reset(DebugGroup* p_debug_group = nullptr) override;
    void ResetWithState(RenderState& render_state, DebugGroup* p_debug_group = nullptr) override;
    void SetBeginningResourceBarriers(const Resource::Barriers&) override;
    void SetEndingResourceBarriers(const Resource::Barriers&) override;

    // CommandList interface
    void Commit() override;

    // CommandListBase interface
    void Execute(uint32_t frame_index, const CompletedCallback& completed_callback = {}) override;

    // Object interface
    void SetName(const std::string& label) override;

private:
    CommandQueueVK& GetCommandQueueVK() noexcept;
    RenderPassVK&   GetPassVK();
};

} // namespace Methane::Graphics
