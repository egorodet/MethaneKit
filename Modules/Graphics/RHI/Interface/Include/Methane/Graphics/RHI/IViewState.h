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

FILE: Methane/Graphics/RHI/IViewState.h
Methane view state interface: viewports and clipping rects setup.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Volume.hpp>
#include <Methane/Data/IEmitter.h>

namespace Methane::Graphics::Rhi
{

struct ViewSettings
{
    Viewports    viewports;
    ScissorRects scissor_rects;

    [[nodiscard]] friend auto operator<=>(const ViewSettings& left, const ViewSettings& right) noexcept = default;

    [[nodiscard]] explicit operator std::string() const;
};

struct IViewState;

struct IViewStateCallback
{
    virtual void OnViewStateChanged(IViewState& view_state) = 0;
    virtual void OnViewStateDestroyed(IViewState& view_state) = 0;

    virtual ~IViewStateCallback() = default;
};

struct IViewState
    : virtual Data::IEmitter<IViewStateCallback> // NOSONAR
{
    using Settings = ViewSettings;
    using ICallback = IViewStateCallback;

    // Create IViewState instance
    [[nodiscard]] static Ptr<IViewState> Create(const Settings& state_settings);

    // IViewState interface
    [[nodiscard]] virtual Ptr<IViewState> GetPtr() = 0;
    [[nodiscard]] virtual const Settings& GetSettings() const noexcept = 0;
    virtual bool Reset(const Settings& settings) = 0;
    virtual bool SetViewports(const Viewports& viewports) = 0;
    virtual bool SetScissorRects(const ScissorRects& scissor_rects) = 0;
};

} // namespace Methane::Graphics::Rhi
