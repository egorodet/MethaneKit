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

FILE: Methane/Graphics/Vulkan/DescriptorHeapVK.mm
Vulkan "dummy" implementation of the descriptor heap.

******************************************************************************/

#include "DescriptorHeapVK.h"

#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

Ptr<DescriptorHeap> DescriptorHeap::Create(const ContextBase& context, const Settings& settings)
{
    META_FUNCTION_TASK();
    auto descriptor_heap_ptr = std::make_shared<DescriptorHeapVK>(context, settings);
    if (settings.size > 0)
    {
        descriptor_heap_ptr->Allocate();
    }
    return descriptor_heap_ptr;
}

DescriptorHeapVK::DescriptorHeapVK(const ContextBase& context, const Settings& settings)
    : DescriptorHeap(context, settings)
{
    META_FUNCTION_TASK();
}

DescriptorHeapVK::~DescriptorHeapVK()
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
