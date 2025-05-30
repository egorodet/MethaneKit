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

FILE: Methane/Graphics/Null/Resource.h
Null implementation of the resource interface.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Base/Context.h>
#include <Methane/Graphics/Base/Resource.h>

#include <type_traits>
#include <cassert>

namespace Methane::Graphics::Null
{

template<typename ResourceBaseType> requires std::is_base_of_v<Base::Resource, ResourceBaseType>
class Resource // NOSONAR - can not comply with rule of Zero: destructor is required
    : public ResourceBaseType
    , public virtual Rhi::IResource // NOSONAR
{
public:
    template<typename SettingsType>
    Resource(const Base::Context& context, const SettingsType& settings)
        : ResourceBaseType(context, settings, State::Undefined)
    { }

    Resource(const Resource&) = delete;
    Resource(Resource&&) = delete;

    ~Resource() override
    {
        META_FUNCTION_TASK();
        try
        {
            // Resource released callback has to be emitted before native resource is released
            Data::Emitter<Rhi::IResourceCallback>::Emit(&Rhi::IResourceCallback::OnResourceReleased, std::ref(*this));
        }
        catch(const std::exception& e)
        {
            META_UNUSED(e);
            META_LOG("WARNING: Unexpected error during resource destruction: {}", e.what());
            assert(false);
        }
    }

    bool operator=(const Resource&) = delete;
    bool operator=(Resource&&) = delete;

    const DescriptorByViewId& GetDescriptorByViewId() const noexcept final
    {
        static const DescriptorByViewId s_dummy_descriptor_by_view_id;
        return s_dummy_descriptor_by_view_id;
    }

    void RestoreDescriptorViews(const DescriptorByViewId&) final
    { /* Intentionally unimplemented */ }

    using Base::Resource::SetInitializedDataSize;
};

} // namespace Methane::Graphics::Null
