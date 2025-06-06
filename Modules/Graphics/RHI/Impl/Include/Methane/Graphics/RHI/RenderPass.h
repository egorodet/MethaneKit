/******************************************************************************

Copyright 2022 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/RHI/RenderPass.h
Methane RenderPass PIMPL wrappers for direct calls to final implementation.

******************************************************************************/

#pragma once

#include <Methane/Pimpl.h>

#include <Methane/Graphics/RHI/IRenderPass.h>

namespace Methane::Graphics::META_GFX_NAME
{
class RenderPass;
}

namespace Methane::Graphics::Rhi
{

class RenderPattern;

class RenderPass // NOSONAR - constructors and assignment operators are required to use forward declared Impl and Ptr<Impl> in header
{
public:
    using Interface         = IRenderPass;
    using Pattern           = RenderPattern;
    using Attachment        = RenderPassAttachment;
    using ColorAttachment   = RenderPassColorAttachment;
    using ColorAttachments  = RenderPassColorAttachments;
    using DepthAttachment   = RenderPassDepthAttachment;
    using StencilAttachment = RenderPassStencilAttachment;
    using AccessMask        = RenderPassAccessMask;
    using Access            = RenderPassAccess;
    using Settings          = RenderPassSettings;
    using ICallback         = IRenderPassCallback;

    META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(RenderPass);
    META_PIMPL_METHODS_COMPARE_INLINE(RenderPass);

    META_PIMPL_API explicit RenderPass(const Ptr<IRenderPass>& interface_ptr);
    META_PIMPL_API explicit RenderPass(IRenderPass& interface_ref);
    META_PIMPL_API RenderPass(const Pattern& render_pattern, const Settings& settings);

    META_PIMPL_API bool IsInitialized() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API IRenderPass& GetInterface() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API Ptr<IRenderPass> GetInterfacePtr() const META_PIMPL_NOEXCEPT;

    // IObject interface methods
    META_PIMPL_API bool SetName(std::string_view name) const;
    META_PIMPL_API std::string_view GetName() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IObjectCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IObjectCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IObjectCallback>& receiver) const;

    // IRenderPass interface methods
    META_PIMPL_API RenderPattern GetPattern() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API const Settings& GetSettings() const META_PIMPL_NOEXCEPT;
    META_PIMPL_API bool Update(const Settings& settings) const META_PIMPL_NOEXCEPT;
    META_PIMPL_API void ReleaseAttachmentTextures() const META_PIMPL_NOEXCEPT;

    // Data::IEmitter<IRenderPassCallback> interface methods
    META_PIMPL_API void Connect(Data::Receiver<IRenderPassCallback>& receiver) const;
    META_PIMPL_API void Disconnect(Data::Receiver<IRenderPassCallback>& receiver) const;

private:
    using Impl = Methane::Graphics::META_GFX_NAME::RenderPass;

    Ptr<Impl> m_impl_ptr;
};

} // namespace Methane::Graphics::Rhi

#ifdef META_PIMPL_INLINE

#include <Methane/Graphics/RHI/RenderPass.cpp>

#endif // META_PIMPL_INLINE
