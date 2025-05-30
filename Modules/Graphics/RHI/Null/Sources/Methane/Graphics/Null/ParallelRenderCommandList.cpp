/******************************************************************************

Copyright 2023 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Null/ParallelRenderCommandList.cpp
Null implementation of the parallel render command list interface.

******************************************************************************/

#include <Methane/Graphics/Null/ParallelRenderCommandList.h>
#include <Methane/Graphics/Null/RenderCommandList.h>

namespace Methane::Graphics::Null
{

Ptr<Rhi::IRenderCommandList> ParallelRenderCommandList::CreateCommandList(bool)
{
    return std::make_shared<RenderCommandList>(*this);
}

void ParallelRenderCommandList::SetBeginningResourceBarriers(const Rhi::IResourceBarriers& barriers)
{
    m_beginning_barriers_ptr = &barriers;
}

void ParallelRenderCommandList::SetEndingResourceBarriers(const Rhi::IResourceBarriers& barriers)
{
    m_ending_barriers_ptr = &barriers;
}

} // namespace Methane::Graphics::Null
