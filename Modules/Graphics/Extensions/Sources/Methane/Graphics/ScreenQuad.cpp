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

FILE: Methane/Graphics/ScreenQuad.cpp
Screen Quad rendering primitive.

******************************************************************************/

#include <Methane/Graphics/ScreenQuad.h>

#include <Methane/Graphics/Mesh/QuadMesh.hpp>
#include <Methane/Graphics/RenderCommandList.h>
#include <Methane/Graphics/TypeConverters.hpp>
#include <Methane/Data/AppResourceProviders.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

struct SHADER_STRUCT_ALIGN ScreenQuadConstants
{
    SHADER_FIELD_ALIGN Color4f blend_color;
};

struct ScreenQuadVertex
{
    Mesh::Position position;
    Mesh::TexCoord texcoord;

    inline static const Mesh::VertexLayout layout {
        Mesh::VertexField::Position,
        Mesh::VertexField::TexCoord,
    };
};

ScreenQuad::ScreenQuad(RenderContext& context, Settings settings)
    : ScreenQuad(context, nullptr, settings)
{
}

ScreenQuad::ScreenQuad(RenderContext& context, Ptr<Texture> texture_ptr, Settings settings)
    : m_settings(std::move(settings))
    , m_context(context)
    , m_texture_ptr(std::move(texture_ptr))
{
    META_FUNCTION_TASK();
    if (m_settings.texture_mode != TextureMode::Disabled && !m_texture_ptr)
        throw std::invalid_argument("Screen-quad texture can not be empty when quad texturing is enabled.");

    static const QuadMesh<ScreenQuadVertex> quad_mesh(ScreenQuadVertex::layout, 2.f, 2.f);
    const RenderContext::Settings&          context_settings              = context.GetSettings();
    const Shader::MacroDefinitions          ps_macro_definitions          = GetPixelShaderMacroDefinitions(m_settings.texture_mode);
    Program::ArgumentDescriptions           program_argument_descriptions = {
        { { Shader::Type::Pixel, "g_constants" }, Program::Argument::Modifiers::None }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        program_argument_descriptions.emplace(Shader::Type::Pixel, "g_texture", Program::Argument::Modifiers::None);
        program_argument_descriptions.emplace(Shader::Type::Pixel, "g_sampler", Program::Argument::Modifiers::Constant);
    }

    std::stringstream quad_name_ss;
    quad_name_ss << "Screen-Quad";
    if (m_settings.alpha_blending_enabled)
        quad_name_ss << " with Alpha-Blending";
    if (!ps_macro_definitions.empty())
        quad_name_ss << " " << Shader::ConvertMacroDefinitionsToString(ps_macro_definitions);

    const std::string s_state_name = quad_name_ss.str() + " Render State";
    m_render_state_ptr = std::dynamic_pointer_cast<RenderState>(m_context.GetObjectsRegistry().GetGraphicsObject(s_state_name));
    if (!m_render_state_ptr)
    {
        RenderState::Settings state_settings;
        state_settings.program_ptr = Program::Create(context,
            Program::Settings
            {
                Program::Shaders
                {
                    Shader::CreateVertex(context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadVS" }, { } }),
                    Shader::CreatePixel( context, { Data::ShaderProvider::Get(), { "ScreenQuad", "QuadPS" }, ps_macro_definitions }),
                },
                Program::InputBufferLayouts
                {
                    Program::InputBufferLayout
                    {
                        Program::InputBufferLayout::ArgumentSemantics { quad_mesh.GetVertexLayout().GetSemantics() }
                    }
                },
                program_argument_descriptions,
                PixelFormats
                {
                    context_settings.color_format
                },
                context_settings.depth_stencil_format
            }
        );
        state_settings.program_ptr->SetName(quad_name_ss.str() + " Shading");
        state_settings.depth.enabled                                        = false;
        state_settings.depth.write_enabled                                  = false;
        state_settings.rasterizer.is_front_counter_clockwise                = true;
        state_settings.blending.render_targets[0].blend_enabled             = m_settings.alpha_blending_enabled;
        state_settings.blending.render_targets[0].source_rgb_blend_factor   = RenderState::Blending::Factor::SourceAlpha;
        state_settings.blending.render_targets[0].dest_rgb_blend_factor     = RenderState::Blending::Factor::OneMinusSourceAlpha;
        state_settings.blending.render_targets[0].source_alpha_blend_factor = RenderState::Blending::Factor::Zero;
        state_settings.blending.render_targets[0].dest_alpha_blend_factor   = RenderState::Blending::Factor::Zero;

        m_render_state_ptr = RenderState::Create(context, state_settings);
        m_render_state_ptr->SetName(s_state_name);

        m_context.GetObjectsRegistry().AddGraphicsObject(*m_render_state_ptr);
    }

    m_view_state_ptr = ViewState::Create({
        { GetFrameViewport(m_settings.screen_rect)    },
        { GetFrameScissorRect(m_settings.screen_rect) }
    });

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        static const std::string s_sampler_name = "Screen-Quad Sampler";
        m_texture_sampler_ptr = std::dynamic_pointer_cast<Sampler>(m_context.GetObjectsRegistry().GetGraphicsObject(s_sampler_name));
        if (!m_texture_sampler_ptr)
        {
            m_texture_sampler_ptr = Sampler::Create(context, {
                { Sampler::Filter::MinMag::Linear     },
                { Sampler::Address::Mode::ClampToZero },
            });
            m_texture_sampler_ptr->SetName(s_sampler_name);
            m_context.GetObjectsRegistry().AddGraphicsObject(*m_texture_sampler_ptr);
        }

        m_texture_ptr->SetName(m_settings.name + " Screen-Quad Texture");
    }

    static const std::string s_vertex_buffer_name = "Screen-Quad Vertex Buffer";
    Ptr<Buffer> vertex_buffer_ptr = std::dynamic_pointer_cast<Buffer>(m_context.GetObjectsRegistry().GetGraphicsObject(s_vertex_buffer_name));
    if (!vertex_buffer_ptr)
    {
        vertex_buffer_ptr = Buffer::CreateVertexBuffer(context, static_cast<Data::Size>(quad_mesh.GetVertexDataSize()), static_cast<Data::Size>(quad_mesh.GetVertexSize()));
        vertex_buffer_ptr->SetName(s_vertex_buffer_name);
        vertex_buffer_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetVertices().data()),
                static_cast<Data::Size>(quad_mesh.GetVertexDataSize())
            }
        });
        m_context.GetObjectsRegistry().AddGraphicsObject(*vertex_buffer_ptr);
    }

    m_vertex_buffer_set_ptr = BufferSet::CreateVertexBuffers({ *vertex_buffer_ptr });

    static const std::string s_index_buffer_name = "Screen-Quad Index Buffer";
    m_index_buffer_ptr = std::dynamic_pointer_cast<Buffer>(m_context.GetObjectsRegistry().GetGraphicsObject(s_index_buffer_name));
    if (!m_index_buffer_ptr)
    {
        m_index_buffer_ptr = Buffer::CreateIndexBuffer(context, static_cast<Data::Size>(quad_mesh.GetIndexDataSize()), GetIndexFormat(quad_mesh.GetIndex(0)));
        m_index_buffer_ptr->SetName(s_index_buffer_name);
        m_index_buffer_ptr->SetData({
            {
                reinterpret_cast<Data::ConstRawPtr>(quad_mesh.GetIndices().data()),
                static_cast<Data::Size>(quad_mesh.GetIndexDataSize())
            }
        });
        m_context.GetObjectsRegistry().AddGraphicsObject(*m_index_buffer_ptr);
    }

    const Data::Size const_buffer_size = static_cast<Data::Size>(sizeof(ScreenQuadConstants));
    m_const_buffer_ptr = Buffer::CreateConstantBuffer(context, Buffer::GetAlignedBufferSize(const_buffer_size));
    m_const_buffer_ptr->SetName(m_settings.name + " Screen-Quad Constants Buffer");

    ProgramBindings::ResourceLocationsByArgument program_binding_resource_locations = {
        { { Shader::Type::Pixel, "g_constants" }, { { m_const_buffer_ptr    } } }
    };

    if (m_settings.texture_mode != TextureMode::Disabled)
    {
        program_binding_resource_locations.emplace(Program::Argument(Shader::Type::Pixel, "g_texture"), Resource::Locations{ { m_texture_ptr         } });
        program_binding_resource_locations.emplace(Program::Argument(Shader::Type::Pixel, "g_sampler"), Resource::Locations{ { m_texture_sampler_ptr } });
    }

    m_const_program_bindings_ptr = ProgramBindings::Create(m_render_state_ptr->GetSettings().program_ptr, program_binding_resource_locations);

    UpdateConstantsBuffer();
}

void ScreenQuad::SetBlendColor(const Color4f& blend_color)
{
    META_FUNCTION_TASK();
    if (m_settings.blend_color == blend_color)
        return;

    m_settings.blend_color  = blend_color;

    UpdateConstantsBuffer();
}

void ScreenQuad::SetScreenRect(const FrameRect& screen_rect, const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    if (m_settings.screen_rect == screen_rect)
        return;

    m_settings.screen_rect = screen_rect;

    m_view_state_ptr->SetViewports({ GetFrameViewport(screen_rect) });
    m_view_state_ptr->SetScissorRects({ GetFrameScissorRect(screen_rect, render_attachment_size) });
}

void ScreenQuad::SetAlphaBlendingEnabled(bool alpha_blending_enabled)
{
    META_FUNCTION_TASK();
    if (m_settings.alpha_blending_enabled == alpha_blending_enabled)
        return;

    m_settings.alpha_blending_enabled = alpha_blending_enabled;

    RenderState::Settings state_settings = m_render_state_ptr->GetSettings();
    state_settings.blending.render_targets[0].blend_enabled = alpha_blending_enabled;
    m_render_state_ptr->Reset(state_settings);
}

void ScreenQuad::SetTexture(Ptr<Texture> texture_ptr)
{
    META_FUNCTION_TASK();
    if (m_settings.texture_mode == TextureMode::Disabled)
        throw std::logic_error("Can not set texture of screen quad with Disabled texture mode.");

    if (m_texture_ptr.get() == texture_ptr.get())
        return;

    if (!texture_ptr)
        throw std::invalid_argument("Can not set null texture to screen quad.");

    m_texture_ptr = texture_ptr;

    const Ptr<ProgramBindings::ArgumentBinding>& texture_binding_ptr = m_const_program_bindings_ptr->Get({ Shader::Type::Pixel, "g_texture" });
    if (!texture_binding_ptr)
        throw std::logic_error("Can not find screen quad texture argument binding.");

    texture_binding_ptr->SetResourceLocations({ { m_texture_ptr } });
}

const Texture& ScreenQuad::GetTexture() const noexcept
{
    META_FUNCTION_TASK();
    assert(!!m_texture_ptr);
    return *m_texture_ptr;
}

void ScreenQuad::Draw(RenderCommandList& cmd_list, CommandList::DebugGroup* p_debug_group) const
{
    META_FUNCTION_TASK();
    cmd_list.Reset(m_render_state_ptr, p_debug_group);
    cmd_list.SetViewState(*m_view_state_ptr);
    cmd_list.SetProgramBindings(*m_const_program_bindings_ptr);
    cmd_list.SetVertexBuffers(*m_vertex_buffer_set_ptr);
    cmd_list.DrawIndexed(RenderCommandList::Primitive::Triangle, *m_index_buffer_ptr);
}

void ScreenQuad::UpdateConstantsBuffer() const
{
    META_FUNCTION_TASK();
    const ScreenQuadConstants constants {
        m_settings.blend_color
    };

    m_const_buffer_ptr->SetData({
        {
            reinterpret_cast<Data::ConstRawPtr>(&constants),
            static_cast<Data::Size>(sizeof(constants))
        }
    });
}

Shader::MacroDefinitions ScreenQuad::GetPixelShaderMacroDefinitions(TextureMode texture_mode)
{
    META_FUNCTION_TASK();
    Shader::MacroDefinitions macro_definitions;

    switch(texture_mode)
    {
    case TextureMode::Disabled:
        macro_definitions.emplace_back("TEXTURE_DISABLED", "");
        break;

    case TextureMode::RgbaFloat:
        break;

    case TextureMode::RFloatToAlpha:
        macro_definitions.emplace_back("TTEXEL", "float");
        macro_definitions.emplace_back("RMASK", "r");
        macro_definitions.emplace_back("WMASK", "a");
        break;
    };

    return macro_definitions;
}

} // namespace Methane::Graphics
