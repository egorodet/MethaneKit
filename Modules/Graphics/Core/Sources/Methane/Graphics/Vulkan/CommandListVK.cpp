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

FILE: Methane/Graphics/DirectX12/CommandListVK.cpp
Vulkan command lists sequence implementation

******************************************************************************/

#include "CommandListVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<CommandList::DebugGroup> CommandList::DebugGroup::Create(const std::string& name)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListVK::DebugGroupVK>(name);
}

CommandListVK::DebugGroupVK::DebugGroupVK(const std::string& name)
    : CommandListBase::DebugGroupBase(name)
{
    META_FUNCTION_TASK();
}

Ptr<CommandListSet> CommandListSet::Create(const Refs<CommandList>& command_list_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandListSetVK>(command_list_refs);
}

CommandListSetVK::CommandListSetVK(const Refs<CommandList>& command_list_refs)
    : CommandListSetBase(command_list_refs)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
