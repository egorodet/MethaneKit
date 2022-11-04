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

FILE: Methane/Graphics/IProgramBindings.cpp
Methane program bindings interface for resources binding to program arguments.

******************************************************************************/

#include <Methane/Graphics/IProgramBindings.h>

#include <Methane/Instrumentation.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

template<>
struct fmt::formatter<Methane::Graphics::ProgramArgument>
{
    [[nodiscard]] constexpr auto parse(const format_parse_context& ctx) const { return ctx.end(); }

    template<typename FormatContext>
    auto format(const Methane::Graphics::ProgramArgument& program_argument, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", static_cast<std::string>(program_argument));
    }
};

namespace Methane::Graphics
{

ProgramArgumentConstantModificationException::ProgramArgumentConstantModificationException(const IProgram::Argument& argument)
    : std::logic_error(fmt::format("Can not modify constant argument binding '{}' of {} shaders.",
                                   argument.GetName(), magic_enum::enum_name(argument.GetShaderType())))
{
    META_FUNCTION_TASK();
}

ProgramBindingsUnboundArgumentsException::ProgramBindingsUnboundArgumentsException(const IProgram& program, const IProgram::Arguments& unbound_arguments)
    : std::runtime_error(fmt::format("Some arguments of program '{}' are not bound to any resource:\n{}", program.GetName(), unbound_arguments))
    , m_program(program)
    , m_unbound_arguments(unbound_arguments)
{
    META_FUNCTION_TASK();
}

} // namespace Methane::Graphics
