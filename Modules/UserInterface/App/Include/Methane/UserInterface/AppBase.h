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

FILE: Methane/UserInterface/AppBase.h
Base implementation of the Methane user interface application.

******************************************************************************/

#pragma once

#include "IApp.h"

#include <Methane/UserInterface/FontLibrary.h>
#include <Methane/UserInterface/HeadsUpDisplay.h>
#include <Methane/Checks.hpp>

#include <string_view>

namespace Methane::Platform
{

struct IApp;

} // namespace Methane::Platform

namespace Methane::UserInterface
{

namespace gfx = Methane::Graphics;
namespace rhi = Methane::Graphics::Rhi;

class Font;
class Badge;
class Text;
class Panel;
class Context;

class AppBase // NOSONAR - custom destructor is required
{
public:
    explicit AppBase(const IApp::Settings& ui_app_settings);
    AppBase(const AppBase&) = delete;

    AppBase& operator=(const AppBase&) = delete;
    AppBase& operator=(AppBase&&) = delete;

protected:
    void InitUI(const Platform::IApp& app, const rhi::CommandQueue& render_cmd_queue, const rhi::RenderPattern& render_pattern, const gfx::FrameSize& frame_size);
    void ReleaseUI();
    bool ResizeUI(const gfx::FrameSize& frame_size, bool is_minimized);
    bool UpdateUI() const;
    void RenderOverlay(const rhi::RenderCommandList& cmd_list) const;

    bool SetHeadsUpDisplayUIMode(HeadsUpDisplayMode heads_up_display_mode);
    bool SetHelpText(std::string_view help_str);
    bool SetParametersText(std::string_view parameters_str);

    [[nodiscard]] const Data::IProvider& GetFontProvider() const noexcept;
    [[nodiscard]] const FontContext& GetFontContext() const noexcept           { return m_font_context; }
    [[nodiscard]] bool IsHelpTextDisplayed() const noexcept                    { return !m_help_columns.first.text_str.empty(); }
    [[nodiscard]] bool IsParametersTextDisplayed() const noexcept              { return !m_parameters.text_str.empty(); }
    [[nodiscard]] Font& GetMainFont();

    [[nodiscard]] const IApp::Settings& GetAppSettings() const noexcept        { return m_app_settings; }

    [[nodiscard]] HeadsUpDisplay::Settings& GetHeadsUpDisplaySettings()        { return m_app_settings.hud_settings; }
    [[nodiscard]] HeadsUpDisplay*           GetHeadsUpDisplay() const noexcept { return m_hud_ptr.get(); }

    [[nodiscard]] IApp::Settings& GetAppSettings() noexcept                    { return m_app_settings; }
    [[nodiscard]] const Context&  GetUIContext() const                         { META_CHECK_NOT_NULL(m_ui_context_ptr); return *m_ui_context_ptr; }
    [[nodiscard]] Context&        GetUIContext()                               { META_CHECK_NOT_NULL(m_ui_context_ptr); return *m_ui_context_ptr; }

private:
    struct TextPanel
    {
        std::string   text_str;
        std::string   text_name;
        Ptr<Panel>    panel_ptr;
        Ptr<TextItem> text_ptr;

        void Update(const FrameSize& frame_size) const;
        void Draw(const rhi::RenderCommandList& cmd_list, const rhi::CommandListDebugGroup* debug_group_ptr) const;
        void Reset(bool forget_text_string);
    };

    using HelpTextPanels = std::pair<TextPanel, TextPanel>;

    bool UpdateTextPanel(TextPanel& text_panel);
    void UpdateHelpTextPosition() const;
    void UpdateParametersTextPosition() const;

    IApp::Settings      m_app_settings;
    FontContext         m_font_context;
    UniquePtr<Context>  m_ui_context_ptr;
    UnitSize            m_frame_size;
    UnitPoint           m_text_margins;
    UnitPoint           m_window_padding;
    Ptr<Badge>          m_logo_badge_ptr;
    Ptr<HeadsUpDisplay> m_hud_ptr;
    Opt<Font>           m_main_font_opt;
    std::string         m_help_text_str;
    HelpTextPanels      m_help_columns;
    TextPanel           m_parameters;
};

} // namespace Methane::UserInterface
