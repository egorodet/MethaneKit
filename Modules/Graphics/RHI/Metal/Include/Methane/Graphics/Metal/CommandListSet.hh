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

FILE: Methane/Graphics/DirectX12/CommandListSet.h
Metal command list set implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/CommandListSet.h>

namespace Methane::Graphics::Metal
{

class CommandListSet final
    : public Base::CommandListSet
{
public:
    explicit CommandListSet(const Refs<Rhi::ICommandList>& command_list_refs, Opt<Data::Index> frame_index_opt);

    void WaitUntilCompleted(uint32_t timeout_ms) override;
};

} // namespace Methane::Graphics::Metal
