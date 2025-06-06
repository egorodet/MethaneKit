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

FILE: Methane/Graphics/Metal/Resource.mm
Metal implementation of the resource interface.

******************************************************************************/

#include <Methane/Graphics/Metal/Resource.hh>

#include <Methane/Graphics/Base/ResourceBarriers.h>

namespace Methane::Graphics::Metal
{

class ResourceBarriers final
    : public Base::ResourceBarriers
{
public:
    explicit ResourceBarriers(const Set& barriers)
        : Base::ResourceBarriers(barriers)
    { }
};

MTLResourceUsage ConvertResourceUsageToMetal(Rhi::ResourceUsage resource_usage)
{
    META_FUNCTION_TASK();
    switch(resource_usage)
    {
        using enum Rhi::ResourceUsage;
        case ShaderRead:  return MTLResourceUsageRead;
        case ShaderWrite: return MTLResourceUsageWrite;
        default: META_UNEXPECTED_DESCR(resource_usage, "resource usage can not be converted to Metal");
    }
}

MTLResourceUsage ConvertResourceUsageMaskToMetal(Rhi::ResourceUsageMask resource_usage_mask)
{
    META_FUNCTION_TASK();
    MTLResourceUsage mtl_resource_usage{};
    using enum Rhi::ResourceUsage;
    if (resource_usage_mask.HasBit(ShaderRead))
        mtl_resource_usage |= MTLResourceUsageRead;
    if (resource_usage_mask.HasBit(ShaderWrite))
        mtl_resource_usage |= MTLResourceUsageWrite;
    return mtl_resource_usage;
}

} // namespace Methane::Graphics::Metal

namespace Methane::Graphics::Rhi
{

Ptr<IResourceBarriers> IResourceBarriers::Create(const Set& barriers)
{
    META_FUNCTION_TASK();
    return std::make_shared<Metal::ResourceBarriers>(barriers);
}

} // namespace Methane::Graphics::Rhi
