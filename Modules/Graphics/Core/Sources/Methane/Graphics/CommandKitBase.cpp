/******************************************************************************

Copyright 2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/CommandKitBase.h
Methane command kit implementation.

******************************************************************************/

#include "CommandKitBase.h"
#include "RenderCommandListBase.h"
#include "CommandQueueBase.h"

#include <Methane/Graphics/Fence.h>
#include <Methane/Graphics/CommandKit.h>
#include <Methane/Graphics/BlitCommandList.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <fmt/format.h>

namespace Methane::Graphics
{

static constexpr uint32_t g_max_cmd_lists_count = 32U;

static uint32_t GetCommandListSetId(const std::vector<uint32_t>& cmd_list_ids)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(cmd_list_ids.size(), g_max_cmd_lists_count, "too many command lists in a set");
    uint32_t set_id = 0;
    for(uint32_t cmd_list_id : cmd_list_ids)
    {
        META_CHECK_ARG_LESS_DESCR(cmd_list_id, g_max_cmd_lists_count, "no more than 32 command lists are supported in one command kit");
        set_id += 1 << cmd_list_id;
    }
    return set_id;
}

Ptr<CommandKit> CommandKit::Create(const Context& context, CommandList::Type cmd_list_type)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandKitBase>(context, cmd_list_type);
}

Ptr<CommandKit> CommandKit::Create(CommandQueue& cmd_queue)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandKitBase>(cmd_queue);
}

CommandKitBase::CommandKitBase(const Context& context, CommandList::Type cmd_list_type)
    : m_context(context)
    , m_cmd_list_type(cmd_list_type)
{
    META_FUNCTION_TASK();
}

CommandKitBase::CommandKitBase(CommandQueue& cmd_queue)
    : ObjectBase(cmd_queue.GetName())
    , m_context(cmd_queue.GetContext())
    , m_cmd_list_type(cmd_queue.GetCommandListType())
    , m_cmd_queue_ptr(static_cast<CommandQueueBase&>(cmd_queue).GetCommandQueuePtr())
{
    META_FUNCTION_TASK();
}

void CommandKitBase::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ObjectBase::SetName(name);

    if (m_cmd_queue_ptr)
        m_cmd_queue_ptr->SetName(fmt::format("{} Command Queue", GetName()));

    for(size_t cmd_list_id = 0; cmd_list_id < m_cmd_list_ptrs.size(); ++cmd_list_id)
    {
        const Ptr<CommandList>& cmd_list_ptr = m_cmd_list_ptrs[cmd_list_id];
        if (cmd_list_ptr)
            cmd_list_ptr->SetName(fmt::format("{} Command List {}", GetName(), cmd_list_id));
    }

    for(size_t fence_id = 0; fence_id < m_fence_ptrs.size(); ++fence_id)
    {
        const Ptr<Fence>& fence_ptr = m_fence_ptrs[fence_id];
        if (fence_ptr)
            fence_ptr->SetName(fmt::format("{} Fence {}", GetName(), fence_id));
    }
}

CommandQueue& CommandKitBase::GetQueue() const
{
    META_FUNCTION_TASK();
    if (m_cmd_queue_ptr)
        return *m_cmd_queue_ptr;

    m_cmd_queue_ptr = CommandQueue::Create(m_context, m_cmd_list_type);
    m_cmd_queue_ptr->SetName(fmt::format("{} Command Queue", GetName()));
    return *m_cmd_queue_ptr;
}

bool CommandKitBase::HasList(uint32_t cmd_list_id) const noexcept
{
    META_FUNCTION_TASK();
    return cmd_list_id < m_cmd_list_ptrs.size() && m_cmd_list_ptrs[cmd_list_id];
}

bool CommandKitBase::HasListWithState(CommandList::State cmd_list_state, uint32_t cmd_list_id) const noexcept
{
    META_FUNCTION_TASK();
    return cmd_list_id < m_cmd_list_ptrs.size() && m_cmd_list_ptrs[cmd_list_id] && m_cmd_list_ptrs[cmd_list_id]->GetState() == cmd_list_state;
}

CommandList& CommandKitBase::GetList(uint32_t cmd_list_id = 0U) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(cmd_list_id, g_max_cmd_lists_count, "no more than 32 command lists are supported in one command kit");
    if (cmd_list_id >= m_cmd_list_ptrs.size())
        m_cmd_list_ptrs.resize(cmd_list_id + 1);

    Ptr<CommandList>& cmd_list_ptr = m_cmd_list_ptrs[cmd_list_id];
    if (cmd_list_ptr)
        return *cmd_list_ptr;

    switch (m_cmd_list_type)
    {
    case CommandList::Type::Blit:   cmd_list_ptr = BlitCommandList::Create(GetQueue()); break;
    case CommandList::Type::Render: cmd_list_ptr = RenderCommandListBase::CreateForSynchronization(GetQueue()); break;
    default:                        META_UNEXPECTED_ARG(m_cmd_list_type);
    }

    cmd_list_ptr->SetName(fmt::format("{} Utility Command List {}", GetName(), cmd_list_id));
    return *cmd_list_ptr;
}

CommandList& CommandKitBase::GetListForEncoding(uint32_t cmd_list_id, std::string_view debug_group_name) const
{
    META_FUNCTION_TASK();
    CommandList& cmd_list = GetList(cmd_list_id);

    // FIXME: loop with wait timeout iterations is used to workaround sporadic deadlock on command list wait for completion
    //        reproduced at high rate of resource updates (in typography tutorial)
    while(cmd_list.GetState() == CommandList::State::Executing)
        cmd_list.WaitUntilCompleted(16);

    if (cmd_list.GetState() == CommandList::State::Pending)
    {
        if (debug_group_name.empty())
        {
            cmd_list.Reset();
        }
        else
        {
            META_DEBUG_GROUP_CREATE_VAR(s_debug_region_name, fmt::format("{}"));
            cmd_list.Reset(s_debug_region_name.get());
        }
    }

    return cmd_list;
}

CommandListSet& CommandKitBase::GetListSet(const std::vector<uint32_t>& cmd_list_ids) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_EMPTY(cmd_list_ids);
    const uint32_t cmd_list_set_id = GetCommandListSetId(cmd_list_ids);

    Ptr<CommandListSet>& cmd_list_set_ptr = m_cmd_list_set_by_id[cmd_list_set_id];
    if (cmd_list_set_ptr && cmd_list_set_ptr->GetCount() == cmd_list_ids.size())
        return *cmd_list_set_ptr;

    Refs<CommandList> command_list_refs;
    for(uint32_t cmd_list_id : cmd_list_ids)
    {
        command_list_refs.emplace_back(GetList(cmd_list_id));
    }

    cmd_list_set_ptr = CommandListSet::Create(command_list_refs);
    return *cmd_list_set_ptr;
}

Fence& CommandKitBase::GetFence(uint32_t fence_id) const
{
    META_FUNCTION_TASK();
    if (fence_id >= m_fence_ptrs.size())
        m_fence_ptrs.resize(fence_id + 1);

    Ptr<Fence>& fence_ptr = m_fence_ptrs[fence_id];

    if (fence_ptr)
        return *fence_ptr;

    fence_ptr = Fence::Create(GetQueue());
    fence_ptr->SetName(fmt::format("{} Fence", GetName()));
    return *fence_ptr;
}

} // namespace Methane::Graphics
