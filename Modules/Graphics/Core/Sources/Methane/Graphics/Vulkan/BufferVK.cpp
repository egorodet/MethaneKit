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

FILE: Methane/Graphics/Vulkan/BufferVK.mm
Vulkan implementation of the buffer interface.

******************************************************************************/

#include "BufferVK.h"

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/ContextBase.h>
#include <Methane/Instrumentation.h>

#include <magic_enum.hpp>
#include <iterator>

namespace Methane::Graphics
{

Ptr<Buffer> Buffer::CreateVertexBuffer(const Context& context, Data::Size size, Data::Size stride)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Vertex, Usage::None, size, stride, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<const ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateIndexBuffer(const Context& context, Data::Size size, PixelFormat format)
{
    META_FUNCTION_TASK();
    const Buffer::Settings settings{ Buffer::Type::Index, Usage::None, size, GetPixelSize(format), format, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<const ContextBase&>(context), settings);
}

Ptr<Buffer> Buffer::CreateConstantBuffer(const Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    const Usage usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::None);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Private };
    return std::make_shared<BufferVK>(dynamic_cast<const ContextBase&>(context), settings, descriptor_by_usage);
}

Ptr<Buffer> Buffer::CreateVolatileBuffer(const Context& context, Data::Size size, bool addressable, const DescriptorByUsage& descriptor_by_usage)
{
    META_FUNCTION_TASK();
    using namespace magic_enum::bitwise_operators;
    const Usage usage_mask = Usage::ShaderRead | (addressable ? Usage::Addressable : Usage::None);
    const Buffer::Settings settings{ Buffer::Type::Constant, usage_mask, size, 0U, PixelFormat::Unknown, Buffer::StorageMode::Managed };
    return std::make_shared<BufferVK>(dynamic_cast<const ContextBase&>(context), settings, descriptor_by_usage);
}

Data::Size Buffer::GetAlignedBufferSize(Data::Size size) noexcept
{
    META_FUNCTION_TASK();
    return size;
}

BufferVK::BufferVK(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage)
    : ResourceVK(context, settings, descriptor_by_usage)
{
    META_FUNCTION_TASK();
    InitializeDefaultDescriptors();
}

void BufferVK::SetName(const std::string& name)
{
    META_FUNCTION_TASK();
    ResourceVK::SetName(name);
}

void BufferVK::SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue)
{
    META_FUNCTION_TASK();
    ResourceVK::SetData(sub_resources, sync_cmd_queue);
}

Ptr<BufferSet> BufferSet::Create(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
{
    META_FUNCTION_TASK();
    return std::make_shared<BufferSetVK>(buffers_type, buffer_refs);
}

BufferSetVK::BufferSetVK(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs)
    : BufferSetBase(buffers_type, buffer_refs)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
