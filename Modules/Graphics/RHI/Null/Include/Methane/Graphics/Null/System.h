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

FILE: Methane/Graphics/Null/System.h
Null implementation of the system interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/System.h>

namespace Methane::Graphics::Null
{

class System final
    : public Base::System
{
public:
    System() = default;

    // ISystem interface
    void CheckForChanges() override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Methane::Platform::AppEnvironment& app_env, const Rhi::DeviceCaps& required_device_caps) override;
    const Ptrs<Rhi::IDevice>& UpdateGpuDevices(const Rhi::DeviceCaps& required_device_caps) override;

    using Base::System::RequestRemoveDevice;
    using Base::System::RemoveDevice;
};

} // namespace Methane::Graphics::Null
