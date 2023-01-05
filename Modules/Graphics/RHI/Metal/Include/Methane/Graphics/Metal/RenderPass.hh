/******************************************************************************

Copyright 2019-2021 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/RenderPass.hh
Metal implementation of the render pass interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/RenderPass.h>

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

struct IContext;
class RenderPattern;

class RenderPass final : public Base::RenderPass
{
public:
    RenderPass(RenderPattern& render_pattern, const Settings& settings);

    // IRenderPass interface
    bool Update(const Settings& settings) override;
    
    void Reset();
    
    MTLRenderPassDescriptor* GetNativeDescriptor(bool reset);

private:
    const IContext& GetMetalContext() const noexcept;
    
    MTLRenderPassDescriptor* m_mtl_pass_descriptor;
};

} // namespace Methane::Graphics::Metal
