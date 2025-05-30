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

FILE: TypographyAppController.cpp
Typography application controller.

******************************************************************************/

#include "TypographyAppController.h"
#include "TypographyApp.h"

#include <Methane/Checks.hpp>

namespace Methane::Tutorials
{

TypographyAppController::TypographyAppController(TypographyApp& typography_app, const ActionByKeyboardState& action_by_keyboard_state)
    : Controller("TYPOGRAPHY SETTINGS")
    , pin::Keyboard::ActionControllerBase<TypographyAppAction>(action_by_keyboard_state, {})
    , m_typography_app(typography_app)
{ }

void TypographyAppController::OnKeyboardChanged(pin::Keyboard::Key key, pin::Keyboard::KeyState key_state,
                                               const pin::Keyboard::StateChange& state_change)
{
    META_FUNCTION_TASK();
    pin::Keyboard::ActionControllerBase<TypographyAppAction>::OnKeyboardChanged(key, key_state, state_change);
}

void TypographyAppController::OnKeyboardStateAction(TypographyAppAction action)
{
    META_FUNCTION_TASK();
    constexpr double s_text_update_interval_delta = 0.01;
    gui::Text::Layout text_layout = m_typography_app.GetSettings().text_layout;

    switch(action)
    {
    using enum TypographyAppAction;

    case SwitchTextWrapMode:
        text_layout.wrap = magic_enum::enum_value<gui::Text::Wrap>(
            (magic_enum::enum_integer(text_layout.wrap) + 1) % magic_enum::enum_count<gui::Text::Wrap>());
        m_typography_app.SetTextLayout(text_layout);
        break;

    case SwitchTextHorizontalAlignment:
        text_layout.horizontal_alignment = magic_enum::enum_value<gui::Text::HorizontalAlignment>(
            (magic_enum::enum_integer(text_layout.horizontal_alignment) + 1) % magic_enum::enum_count<gui::Text::HorizontalAlignment>());
        m_typography_app.SetTextLayout(text_layout);
        break;

    case SwitchTextVerticalAlignment:
        text_layout.vertical_alignment = magic_enum::enum_value<gui::Text::VerticalAlignment>(
            (magic_enum::enum_integer(text_layout.vertical_alignment) + 1) % magic_enum::enum_count<gui::Text::VerticalAlignment>());
        m_typography_app.SetTextLayout(text_layout);
        break;

    case SwitchIncrementalTextUpdate:
        m_typography_app.SetIncrementalTextUpdate(!m_typography_app.GetSettings().is_incremental_text_update);
        break;

    case SwitchTypingDirection:
        m_typography_app.SetForwardTypingDirection(!m_typography_app.GetSettings().is_forward_typing_direction);
        break;

    case SpeedupTyping:
        m_typography_app.SetTextUpdateInterval(
            std::max(s_text_update_interval_delta, m_typography_app.GetSettings().typing_update_interval_sec - s_text_update_interval_delta));
        break;

    case SlowdownTyping:
        m_typography_app.SetTextUpdateInterval( m_typography_app.GetSettings().typing_update_interval_sec + s_text_update_interval_delta);
        break;

    default:
        META_UNEXPECTED(action);
    }
}

std::string TypographyAppController::GetKeyboardActionName(TypographyAppAction action) const
{
    META_FUNCTION_TASK();
    switch(action)
    {
    using enum TypographyAppAction;
    case SwitchTextWrapMode:            return "switch text wrap mode";
    case SwitchTextHorizontalAlignment: return "switch horizontal text alignment";
    case SwitchTextVerticalAlignment:   return "switch vertical text alignment";
    case SwitchIncrementalTextUpdate:   return "switch incremental text update";
    case SwitchTypingDirection:         return "switch typing direction";
    case SpeedupTyping:                 return "speedup typing";
    case SlowdownTyping:                return "slowdown typing";
    default:                                                 META_UNEXPECTED_RETURN(action, "");
    }
}

pin::IHelpProvider::HelpLines TypographyAppController::GetHelp() const
{
    META_FUNCTION_TASK();
    return GetKeyboardHelp();
}

} // namespace Methane::Tutorials
