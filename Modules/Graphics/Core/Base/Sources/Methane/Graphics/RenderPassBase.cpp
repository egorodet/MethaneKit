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

FILE: Methane/Graphics/RenderPassBase.cpp
Base implementation of the render pass interface.

******************************************************************************/

#include <Methane/Graphics/RenderPassBase.h>
#include <Methane/Graphics/RenderContextBase.h>
#include <Methane/Graphics/TextureBase.h>
#include <Methane/Graphics/RenderCommandListBase.h>

#include <Methane/Data/BitMaskHelpers.hpp>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <magic_enum.hpp>

namespace Methane::Graphics
{

RenderPatternBase::RenderPatternBase(RenderContextBase& render_context, const Settings& settings)
    : m_render_context_ptr(std::dynamic_pointer_cast<RenderContextBase>(render_context.GetPtr()))
    , m_settings(settings)
{
    META_FUNCTION_TASK();
}

const IRenderContext& RenderPatternBase::GetRenderContext() const noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

IRenderContext& RenderPatternBase::GetRenderContext() noexcept
{
    META_FUNCTION_TASK();
    return *m_render_context_ptr;
}

Data::Size RenderPatternBase::GetAttachmentCount() const noexcept
{
    META_FUNCTION_TASK();
    auto attachment_count = static_cast<Data::Size>(m_settings.color_attachments.size());
    if (m_settings.depth_attachment)
        attachment_count++;
    if (m_settings.stencil_attachment)
        attachment_count++;
    return attachment_count;
}

AttachmentFormats RenderPatternBase::GetAttachmentFormats() const noexcept
{
    META_FUNCTION_TASK();
    AttachmentFormats attachment_formats;

    attachment_formats.colors.reserve(m_settings.color_attachments.size());
    std::transform(m_settings.color_attachments.begin(), m_settings.color_attachments.end(), std::back_inserter(attachment_formats.colors),
                   [](const ColorAttachment& color_attachment) { return color_attachment.format; });

    if (m_settings.depth_attachment)
        attachment_formats.depth = m_settings.depth_attachment->format;

    if (m_settings.stencil_attachment)
        attachment_formats.stencil = m_settings.stencil_attachment->format;

    return attachment_formats;
}

RenderPassBase::RenderPassBase(RenderPatternBase& render_pattern, const Settings& settings, bool update_attachment_states)
    : m_pattern_base_ptr(std::dynamic_pointer_cast<RenderPatternBase>(render_pattern.GetBasePtr()))
    , m_settings(settings)
    , m_update_attachment_states(update_attachment_states)
{
    META_FUNCTION_TASK();
    InitAttachmentStates();
}

bool RenderPassBase::Update(const RenderPassSettings& settings)
{
    META_FUNCTION_TASK();
    if (m_settings == settings)
        return false;

    m_settings = settings;

    m_non_frame_buffer_attachment_textures.clear();
    m_color_attachment_textures.clear();
    m_p_depth_attachment_texture = nullptr;

    InitAttachmentStates();
    return true;
}

void RenderPassBase::ReleaseAttachmentTextures()
{
    META_FUNCTION_TASK();
    m_non_frame_buffer_attachment_textures.clear();
    m_settings.attachments.clear();
}

void RenderPassBase::Begin(RenderCommandListBase&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_FALSE_DESCR(m_is_begun, "can not begin pass which was begun already and was not ended");

    if (m_update_attachment_states)
    {
        SetAttachmentStates(IResource::State::RenderTarget, IResource::State::DepthWrite);
    }
    m_is_begun = true;
}

void RenderPassBase::End(RenderCommandListBase&)
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_TRUE_DESCR(m_is_begun, "can not end render pass, which was not begun");

    if (m_update_attachment_states && GetPatternBase().GetSettings().is_final_pass)
    {
        SetAttachmentStates(IResource::State::Present, { });
    }
    m_is_begun = false;
}

void RenderPassBase::InitAttachmentStates() const
{
    META_FUNCTION_TASK();
    const bool             is_final_pass          = GetPatternBase().GetSettings().is_final_pass;
    const IResource::State color_attachment_state = is_final_pass ? IResource::State::Present : IResource::State::RenderTarget;
    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        if (color_texture_ref.get().GetState() == IResource::State::Common ||
            color_texture_ref.get().GetState() == IResource::State::Undefined)
            color_texture_ref.get().SetState(color_attachment_state);
    }
}

void RenderPassBase::SetAttachmentStates(const std::optional<IResource::State>& color_state,
                                         const std::optional<IResource::State>& depth_state) const
{
    META_FUNCTION_TASK();
    if (color_state)
    {
        for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
        {
            color_texture_ref.get().SetState(*color_state);
        }
    }

    if (depth_state)
    {
        if (TextureBase* p_depth_texture = GetDepthAttachmentTexture();
            p_depth_texture)
        {
            p_depth_texture->SetState(*depth_state);
        }
    }
}

void RenderPassBase::SetAttachmentStates(const std::optional<IResource::State>& color_state,
                                         const std::optional<IResource::State>& depth_state,
                                         Ptr<IResourceBarriers>& transition_barriers_ptr,
                                         RenderCommandListBase& render_command_list) const
{
    META_FUNCTION_TASK();
    bool attachment_states_changed = false;

    if (color_state)
    {
        for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
        {
            attachment_states_changed |= color_texture_ref.get().SetState(*color_state, transition_barriers_ptr);
        }
    }

    if (depth_state)
    {
        if (TextureBase* p_depth_texture = GetDepthAttachmentTexture();
            p_depth_texture)
        {
            attachment_states_changed |= p_depth_texture->SetState(*depth_state, transition_barriers_ptr);
        }
    }

    if (transition_barriers_ptr && attachment_states_changed)
    {
        render_command_list.SetResourceBarriers(*transition_barriers_ptr);
    }
}

const ITexture::View& RenderPassBase::GetAttachmentTextureView(const RenderPassAttachment& attachment) const
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_LESS_DESCR(attachment.attachment_index, m_settings.attachments.size(),
                              "attachment index is out of bounds of render pass attachments array");
    return m_settings.attachments[attachment.attachment_index];
}

const Refs<TextureBase>& RenderPassBase::GetColorAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_color_attachment_textures.empty())
        return m_color_attachment_textures;

    const ColorAttachments& color_attachments = GetPatternBase().GetSettings().color_attachments;
    m_color_attachment_textures.reserve(color_attachments.size());
    for (const ColorAttachment& color_attach : color_attachments)
    {
        m_color_attachment_textures.push_back(static_cast<TextureBase&>(GetAttachmentTextureView(color_attach).GetTexture()));
    }
    return m_color_attachment_textures;
}

TextureBase* RenderPassBase::GetDepthAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_depth_attachment_texture)
        return m_p_depth_attachment_texture;

    const Opt<RenderPassDepthAttachment>& depth_attachment_opt = GetPatternBase().GetSettings().depth_attachment;
    if (!depth_attachment_opt)
        return nullptr;

    m_p_depth_attachment_texture = static_cast<TextureBase*>(GetAttachmentTextureView(*depth_attachment_opt).GetTexturePtr().get());
    return m_p_depth_attachment_texture;
}

TextureBase* RenderPassBase::GetStencilAttachmentTexture() const
{
    META_FUNCTION_TASK();
    if (m_p_stencil_attachment_texture)
        return m_p_stencil_attachment_texture;

    const Opt<StencilAttachment>& stencil_attachment_opt = GetPatternBase().GetSettings().stencil_attachment;
    if (!stencil_attachment_opt)
        return nullptr;

    m_p_stencil_attachment_texture = static_cast<TextureBase*>(GetAttachmentTextureView(*stencil_attachment_opt).GetTexturePtr().get());
    return m_p_stencil_attachment_texture;
}

const Ptrs<TextureBase>& RenderPassBase::GetNonFrameBufferAttachmentTextures() const
{
    META_FUNCTION_TASK();
    if (!m_non_frame_buffer_attachment_textures.empty())
        return m_non_frame_buffer_attachment_textures;

    m_non_frame_buffer_attachment_textures.reserve(m_settings.attachments.size());

    for (const Ref<TextureBase>& color_texture_ref : GetColorAttachmentTextures())
    {
        Ptr<TextureBase> color_attachment_ptr = color_texture_ref.get().GetPtr<TextureBase>();
        if (color_attachment_ptr->GetSettings().type == ITexture::Type::FrameBuffer)
            continue;

        m_non_frame_buffer_attachment_textures.emplace_back(std::move(color_attachment_ptr));
    }

    if (TextureBase* depth_texture_ptr = GetDepthAttachmentTexture();
        depth_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(depth_texture_ptr->GetPtr<TextureBase>());
    }

    if (TextureBase* stencil_texture_ptr = GetStencilAttachmentTexture();
        stencil_texture_ptr)
    {
        m_non_frame_buffer_attachment_textures.emplace_back(stencil_texture_ptr->GetPtr<TextureBase>());
    }

    return m_non_frame_buffer_attachment_textures;
}

} // namespace Methane::Graphics
