/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/DirectX/ComputeCommandList.h
DirectX 12 implementation of the compute command list interface.

******************************************************************************/

#pragma once

#include "CommandList.hpp"

#include <Methane/Graphics/Base/ComputeCommandList.h>

namespace Methane::Graphics::DirectX
{

class ComputeCommandList final // NOSONAR - inheritance hierarchy depth is greater than 5
    : public CommandList<Base::ComputeCommandList>
{
public:
    explicit ComputeCommandList(Base::CommandQueue& cmd_buffer);

    // ICommandList interface
    void Reset(IDebugGroup* debug_group_ptr = nullptr) override;

    // IComputeCommandList interface
    void Dispatch(const Rhi::ThreadGroupsCount& thread_groups_count) override;

private:
    DescriptorHeap& m_gpu_shader_resources_descriptor_heap;
};

} // namespace Methane::Graphics::DirectX
