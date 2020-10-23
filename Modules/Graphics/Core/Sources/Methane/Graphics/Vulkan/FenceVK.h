/******************************************************************************

Copyright 2020 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Vulkan/FenceVK.h
Vulkan fence implementation.

******************************************************************************/

#pragma once

#include <Methane/Graphics/FenceBase.h>

namespace Methane::Graphics
{

class CommandQueueVK;

class FenceVK final : public FenceBase
{
public:
    FenceVK(CommandQueueBase& command_queue);
    ~FenceVK();

    // Fence overrides
    void Signal() override;
    void WaitOnCpu() override;
    void WaitOnGpu(CommandQueue& wait_on_command_queue) override;

    // Object override
    void SetName(const std::string& name) noexcept override;

private:
    CommandQueueVK& GetCommandQueueVK();

    // TODO: native fence object
};

} // namespace Methane::Graphics
