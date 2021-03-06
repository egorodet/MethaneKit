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

FILE: Methane/Graphics/Metal/BlitCommandListVK.cpp
Vulkan implementation of the blit command list interface.

******************************************************************************/

#include "BlitCommandListVK.h"
#include "CommandQueueVK.h"

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

namespace Methane::Graphics
{

Ptr<BlitCommandList> BlitCommandList::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<BlitCommandListVK>(static_cast<CommandQueueBase&>(command_queue));
}

BlitCommandListVK::BlitCommandListVK(CommandQueueBase& command_queue)
    : CommandListBase(command_queue, CommandList::Type::Blit)
{
    META_FUNCTION_TASK();
}

void BlitCommandListVK::Reset(DebugGroup* p_debug_group)
{
    META_FUNCTION_TASK();
    CommandListBase::Reset(p_debug_group);
}

void BlitCommandListVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    CommandListBase::SetName(name);
}

void BlitCommandListVK::PushDebugGroup(DebugGroup& debug_group)
{
    META_FUNCTION_TASK();
    CommandListBase::PushDebugGroup(debug_group);
}

void BlitCommandListVK::PopDebugGroup()
{
    META_FUNCTION_TASK();
    CommandListBase::PopDebugGroup();
}

void BlitCommandListVK::Commit()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE(IsCommitted());
    CommandListBase::Commit();
}

void BlitCommandListVK::SetResourceBarriers(const Resource::Barriers&)
{
    META_FUNCTION_NOT_IMPLEMENTED();
}

void BlitCommandListVK::Execute(uint32_t frame_index, const CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandListBase::Execute(frame_index, completed_callback);
}

CommandQueueVK& BlitCommandListVK::GetCommandQueueVK() noexcept
{
    META_FUNCTION_TASK();
    return static_cast<class CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
