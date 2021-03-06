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

FILE: Methane/Graphics/Vulkan/BufferVK.h
Vulkan implementation of the buffer interface.

******************************************************************************/

#pragma once

#include "ResourceVK.h"

#include <Methane/Graphics/BufferBase.h>
#include <Methane/Graphics/Types.h>

namespace Methane::Graphics
{

class BufferVK final : public ResourceVK<BufferBase>
{
public:
    BufferVK(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage = DescriptorByUsage());

    // Resource interface
    void SetData(const SubResources& sub_resources, CommandQueue* sync_cmd_queue) override;

    // Object interface
    void SetName(const std::string& name) override;
};

class BufferSetVK final : public BufferSetBase
{
public:
    BufferSetVK(Buffer::Type buffers_type, const Refs<Buffer>& buffer_refs);
};

} // namespace Methane::Graphics
