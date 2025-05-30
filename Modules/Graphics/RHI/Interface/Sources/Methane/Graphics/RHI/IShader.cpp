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

FILE: Methane/Graphics/RHI/IShader.cpp
Methane shader interface: defines programmable stage of the graphics pipeline.

******************************************************************************/

#include <Methane/Graphics/RHI/IShader.h>
#include <Methane/Graphics/RHI/IContext.h>

#include <Methane/Instrumentation.h>

#include <sstream>

namespace Methane::Graphics::Rhi
{

ShaderMacroDefinition::ShaderMacroDefinition(std::string name)
    : name(std::move(name))
{ }

ShaderMacroDefinition::ShaderMacroDefinition(std::string name, std::string value)
    : name(std::move(name))
    , value(std::move(value))
{ }

Ptr<IShader> IShader::Create(Type type, const IContext& context, const Settings& settings)
{
    return context.CreateShader(type, settings);
}

std::string ShaderMacroDefinition::ToString(std::span<const ShaderMacroDefinition> macro_definitions, std::string_view splitter) noexcept
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    bool is_first_definition = true;
    for(const ShaderMacroDefinition& macro_definition : macro_definitions)
    {
        if (!is_first_definition)
            ss << splitter;

        ss << macro_definition.name << "=" << macro_definition.value;
        is_first_definition = false;
    }
    return ss.str();
}

} // namespace Methane::Graphics::Rhi
