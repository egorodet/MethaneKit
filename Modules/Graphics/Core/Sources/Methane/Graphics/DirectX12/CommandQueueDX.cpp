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

FILE: Methane/Graphics/DirectX12/CommandQueueDX.cpp
DirectX 12 implementation of the command queue interface.

******************************************************************************/

#include "CommandQueueDX.h"
#include "CommandListDX.h"
#include "DeviceDX.h"
#include "BlitCommandListDX.h"
#include "RenderCommandListDX.h"
#include "ParallelRenderCommandListDX.h"
#include "QueryBufferDX.h"

#include <Methane/Graphics/ContextBase.h>
#include <Methane/Graphics/Windows/ErrorHandling.h>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <nowide/convert.hpp>
#include <magic_enum.hpp>
#include <stdexcept>
#include <cassert>

namespace Methane::Graphics
{

constexpr uint32_t g_max_timestamp_queries_count_per_frame = 1000;

Ptr<CommandQueue> CommandQueue::Create(const Context& context, CommandList::Type command_lists_type)
{
    META_FUNCTION_TASK();
    return std::make_shared<CommandQueueDX>(dynamic_cast<const ContextBase&>(context), command_lists_type);
}

static D3D12_COMMAND_LIST_TYPE GetNativeCommandListType(CommandList::Type command_list_type, Context::Options options)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;

    switch(command_list_type)
    {
    case CommandList::Type::Blit:
        return magic_enum::flags::enum_contains(options & Context::Options::BlitWithDirectQueueOnWindows)
             ? D3D12_COMMAND_LIST_TYPE_DIRECT
             : D3D12_COMMAND_LIST_TYPE_COPY;

    case CommandList::Type::Render:
    case CommandList::Type::ParallelRender:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;

    default:
        META_UNEXPECTED_ARG_RETURN(command_list_type, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
}

static wrl::ComPtr<ID3D12CommandQueue> CreateNativeCommandQueue(const DeviceDX& device, D3D12_COMMAND_LIST_TYPE command_list_type)
{
    META_FUNCTION_TASK();
    const wrl::ComPtr<ID3D12Device>& cp_device = device.GetNativeDevice();
    META_CHECK_ARG_NOT_NULL(cp_device);

    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = command_list_type;

    wrl::ComPtr<ID3D12CommandQueue> cp_command_queue;
    ThrowIfFailed(cp_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&cp_command_queue)), cp_device.Get());
    return cp_command_queue;
}

CommandQueueDX::CommandQueueDX(const ContextBase& context, CommandList::Type command_lists_type)
    : CommandQueueBase(context, command_lists_type)
    , m_cp_command_queue(CreateNativeCommandQueue(GetContextDX().GetDeviceDX(), GetNativeCommandListType(command_lists_type, context.GetOptions())))
    , m_execution_waiting_thread(&CommandQueueDX::WaitForExecution, this)
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    , m_timestamp_query_buffer_ptr(TimestampQueryBuffer::Create(*this, g_max_timestamp_queries_count_per_frame))
#endif
{
    META_FUNCTION_TASK();
#ifdef METHANE_GPU_INSTRUMENTATION_ENABLED
    TimestampQueryBufferDX& timestamp_query_buffer_dx = static_cast<TimestampQueryBufferDX&>(*m_timestamp_query_buffer_ptr);
    InitializeTracyGpuContext(
        Tracy::GpuContext::Settings(
            Tracy::GpuContext::Type::DirectX12,
            timestamp_query_buffer_dx.GetGpuCalibrationTimestamp(),
            Data::ConvertFrequencyToTickPeriod(timestamp_query_buffer_dx.GetGpuFrequency())
        )
    );
#endif
}

CommandQueueDX::~CommandQueueDX()
{
    META_FUNCTION_TASK();
    try
    {
        CompleteExecution();
    }
    catch(const std::exception& ex)
    {
        META_UNUSED(ex);
        META_LOG("WARNING: Command queue '{}' has failed to complete command list execution, exception occurred: {}", GetName(), ex.what());
        assert(false);
    }
    m_execution_waiting = false;
    m_execution_waiting_condition_var.notify_one();
    m_execution_waiting_thread.join();
}

void CommandQueueDX::Execute(CommandListSet& command_lists, const CommandList::CompletedCallback& completed_callback)
{
    META_FUNCTION_TASK();
    CommandQueueBase::Execute(command_lists, completed_callback);

    if (!m_execution_waiting)
    {
        m_execution_waiting_thread.join();
        META_CHECK_ARG_NOT_NULL_DESCR(m_execution_waiting_exception_ptr, "Command queue '{}' execution waiting thread has unexpectedly finished", GetName());
        std::rethrow_exception(m_execution_waiting_exception_ptr);
    }

    auto& dx_command_lists = static_cast<CommandListSetDX&>(command_lists);
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    m_executing_command_lists.push(std::static_pointer_cast<CommandListSetDX>(dx_command_lists.GetPtr()));
    m_execution_waiting_condition_var.notify_one();
}

void CommandQueueDX::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    if (name == GetName())
        return;

    CommandQueueBase::SetName(name);
    m_cp_command_queue->SetName(nowide::widen(name).c_str());
    m_name_changed = true;
}

void CommandQueueDX::CompleteExecution(const std::optional<Data::Index>& frame_index)
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);
    while (!m_executing_command_lists.empty() &&
          (!frame_index.has_value() || m_executing_command_lists.front()->GetExecutingOnFrameIndex() == *frame_index))
    {
        m_executing_command_lists.front()->Complete();
        m_executing_command_lists.pop();
    }
    m_execution_waiting_condition_var.notify_one();
}

const IContextDX& CommandQueueDX::GetContextDX() const noexcept
{
    META_FUNCTION_TASK();
    return static_cast<const IContextDX&>(GetContextBase());
}

ID3D12CommandQueue& CommandQueueDX::GetNativeCommandQueue()
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_cp_command_queue);
    return *m_cp_command_queue.Get();
}

void CommandQueueDX::WaitForExecution() noexcept
{
    try
    {
        do
        {
            std::unique_lock lock(m_execution_waiting_mutex);
            m_execution_waiting_condition_var.wait(lock,
                [this] { return !m_execution_waiting || !m_executing_command_lists.empty(); }
            );

            if (m_name_changed)
            {
                const std::string thread_name = fmt::format("{} Wait for Execution", GetName());
                META_THREAD_NAME(thread_name.c_str());
                m_name_changed = false;
            }

            while (!m_executing_command_lists.empty())
            {
                const Ptr<CommandListSetDX>& command_list_set_ptr = GetNextExecutingCommandListSet();
                if (!command_list_set_ptr)
                    break;

                command_list_set_ptr->GetExecutionCompletedFenceDX().WaitOnCpu();
                CompleteCommandListSetExecution(*command_list_set_ptr);
            }
        }
        while (m_execution_waiting);
    }
    catch (...)
    {
        m_execution_waiting_exception_ptr = std::current_exception();
        m_execution_waiting = false;
    }
}

const Ptr<CommandListSetDX>& CommandQueueDX::GetNextExecutingCommandListSet() const
{
    META_FUNCTION_TASK();
    std::scoped_lock lock_guard(m_executing_command_lists_mutex);

    static const Ptr<CommandListSetDX> s_empty_command_list_set_ptr;

    if (m_executing_command_lists.empty())
        return s_empty_command_list_set_ptr;

    META_CHECK_ARG_NOT_NULL(m_executing_command_lists.front());
    return m_executing_command_lists.front();
}

void CommandQueueDX::CompleteCommandListSetExecution(CommandListSetDX& executing_command_list_set)
{
    META_FUNCTION_TASK();
    std::unique_lock lock_guard(m_executing_command_lists_mutex);

    executing_command_list_set.Complete();

    if (!m_executing_command_lists.empty() && m_executing_command_lists.front().get() == std::addressof(executing_command_list_set))
    {
        m_executing_command_lists.pop();
    }
}

} // namespace Methane::Graphics
