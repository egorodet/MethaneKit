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

FILE: Methane/Graphics/Vulkan/SamplerVK.h
Vulkan implementation of the sampler interface.

******************************************************************************/

#pragma once

#include "ResourceVK.h"

#include <Methane/Graphics/SamplerBase.h>

namespace Methane::Graphics
{

struct IContextVK;

class SamplerVK final : public ResourceVK<SamplerBase>
{
public:
    SamplerVK(const ContextBase& context, const Settings& settings, const DescriptorByUsage& descriptor_by_usage);

    // Object interface
    void SetName(const std::string& name) override;
    
private:
    void ResetSamplerState();
};

} // namespace Methane::Graphics
