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

FILE: Methane/Platform/Input/State.h
Aggregated application input state with controllers.

******************************************************************************/

#pragma once

#include "ControllersPool.h"

#include <Methane/Checks.hpp>
#include <Methane/Instrumentation.h>

namespace Methane::Platform::Input
{

class State final : public IActionController
{
public:
    State() = default;
    explicit State(const ControllersPool& controllers) : m_controllers(controllers) {}

    [[nodiscard]] const ControllersPool& GetControllers() const noexcept { return m_controllers; }

    void AddControllers(const Ptrs<Controller>& controllers) { m_controllers.insert(m_controllers.end(), controllers.begin(), controllers.end()); }

    [[nodiscard]] const Keyboard::State& GetKeyboardState() const noexcept { return m_keyboard_state; }
    [[nodiscard]] const Mouse::State&    GetMouseState() const noexcept    { return m_mouse_state; }

    // IActionController
    void OnMouseButtonChanged(Mouse::Button button, Mouse::ButtonState button_state) override;
    void OnMousePositionChanged(const Mouse::Position& mouse_position) override;
    void OnMouseScrollChanged(const Mouse::Scroll& mouse_scroll_delta) override;
    void OnMouseInWindowChanged(bool is_mouse_in_window) override;
    void OnKeyboardChanged(Keyboard::Key key, Keyboard::KeyState key_state) override;
    void OnModifiersChanged(Keyboard::Modifiers modifiers) override;

    void ReleaseAllKeys();

    template<typename ControllerT, typename = std::enable_if_t<std::is_base_of_v<Controller, ControllerT>>>
    [[nodiscard]] Refs<ControllerT> GetControllersOfType() const
    {
        META_FUNCTION_TASK();
        Refs<ControllerT> controllers;
        const std::type_info& controller_type  = typeid(ControllerT);
        for(const Ptr<Controller>& controller_ptr : m_controllers)
        {
            META_CHECK_ARG_NOT_NULL(controller_ptr);
            if (Controller& controller = *controller_ptr;
                typeid(controller) == controller_type)
                controllers.emplace_back(static_cast<ControllerT&>(controller));
        }
        return controllers;
    }

private:
    ControllersPool     m_controllers;
    Mouse::State        m_mouse_state;
    Keyboard::StateExt  m_keyboard_state;
};

} // namespace Methane::Platform::Input
