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
 
 FILE: Methane/Platform/MouseActionControllerBase.hpp
 Base implementation of the mouse actions controller.
 
 ******************************************************************************/

#pragma once

#include <Methane/Platform/Input/IHelpProvider.h>
#include <Methane/Platform/Input/Mouse.h>

#include <magic_enum/magic_enum.hpp>
#include <ranges>
#include <map>

namespace Methane::Platform::Input::Mouse
{

template<typename ActionEnum>
class ActionControllerBase
{
public:
    using ActionByMouseButton = std::map<Button, ActionEnum>;
    
    explicit ActionControllerBase(const ActionByMouseButton& action_by_mouse_button)
        : m_action_by_mouse_button(action_by_mouse_button)
    { }

    virtual ~ActionControllerBase() = default;

    [[nodiscard]]
    size_t GetMouseActionsCount() const noexcept { return m_action_by_mouse_button.size(); }

    [[nodiscard]]
    Input::IHelpProvider::HelpLines GetMouseHelp() const
    {
        META_FUNCTION_TASK();
        Input::IHelpProvider::HelpLines help_lines;
        if (m_action_by_mouse_button.empty())
            return help_lines;
        
        help_lines.reserve(m_action_by_mouse_button.size());
        for (const ActionEnum action : magic_enum::enum_values<ActionEnum>())
        {
            const auto action_by_mouse_button_it = std::ranges::find_if(m_action_by_mouse_button,
                [action](const std::pair<Button, ActionEnum>& button_and_action)
                { return button_and_action.second == action; }
            );
            if (action_by_mouse_button_it == m_action_by_mouse_button.end())
                continue;
            
            help_lines.push_back({
                std::string(Mouse::ButtonConverter(action_by_mouse_button_it->first).ToString()),
                GetMouseActionName(action_by_mouse_button_it->second)
            });
        }
        
        return help_lines;
    }
    
protected:
    // Mouse::ActionControllerBase interface
    [[nodiscard]] virtual std::string GetMouseActionName(ActionEnum action) const = 0;

    [[nodiscard]]
    ActionEnum GetMouseActionByButton(Button mouse_button) const
    {
        META_FUNCTION_TASK();
        const auto action_by_mouse_button_it = m_action_by_mouse_button.find(mouse_button);
        return (action_by_mouse_button_it != m_action_by_mouse_button.end())
              ? action_by_mouse_button_it->second : ActionEnum::None;
    }

private:
    ActionByMouseButton m_action_by_mouse_button;
};

} // namespace Methane::Platform::Input::Mouse
