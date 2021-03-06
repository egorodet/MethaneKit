/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/FenceVK.cpp
Vulkan fence implementation.

******************************************************************************/

#include "FenceVK.h"
#include "CommandQueueVK.h"
#include "DeviceVK.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <nowide/convert.hpp>

namespace Methane::Graphics
{

Ptr<Fence> Fence::Create(CommandQueue& command_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<FenceVK>(static_cast<CommandQueueBase&>(command_queue));
}

FenceVK::FenceVK(CommandQueueBase& command_queue)
    : FenceBase(command_queue)
{
    META_FUNCTION_TASK();
}

void FenceVK::Signal()
{
    META_FUNCTION_TASK();

    FenceBase::Signal();
}

void FenceVK::WaitOnCpu()
{
    META_FUNCTION_TASK();

    FenceBase::WaitOnCpu();
}

void FenceVK::WaitOnGpu(CommandQueue& wait_on_command_queue)
{
    META_FUNCTION_TASK();
    FenceBase::WaitOnGpu(wait_on_command_queue);
}

void FenceVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (ObjectBase::GetName() == name)
        return;

   ObjectBase::SetName(name);
}

CommandQueueVK& FenceVK::GetCommandQueueVK()
{
    META_FUNCTION_TASK();
    return static_cast<CommandQueueVK&>(GetCommandQueue());
}

} // namespace Methane::Graphics
