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

FILE: Methane/Graphics/Program.h
Methane program interface: represents a collection of shaders set on graphics 
pipeline via state object and used to create resource binding objects.

******************************************************************************/

#pragma once

#include "Shader.h"
#include "Object.h"

#include <Methane/Memory.hpp>
#include <Methane/Graphics/Types.h>

#include <magic_enum.hpp>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

//#define PROGRAM_IGNORE_MISSING_ARGUMENTS

namespace Methane::Graphics
{

struct Context;
struct CommandList;

struct Program : virtual Object
{
    struct InputBufferLayout
    {
        enum class StepType
        {
            Undefined,
            PerVertex,
            PerInstance,
        };

        using ArgumentSemantics = std::vector<std::string>;

        ArgumentSemantics argument_semantics;
        StepType          step_type = StepType::PerVertex;
        uint32_t          step_rate = 1;
    };
    
    using InputBufferLayouts = std::vector<InputBufferLayout>;

    class Argument
    {
    public:
        class NotFoundException : public std::invalid_argument
        {
        public:
            NotFoundException(const Program& program, const Argument& argument);

            [[nodiscard]] const Program&  GetProgram() const noexcept  { return m_program; }
            [[nodiscard]] const Argument& GetArgument() const noexcept { return *m_argument_ptr; }

        private:
            const Program& m_program;
            UniquePtr<Argument> m_argument_ptr;
        };

        struct Hash
        {
            [[nodiscard]] size_t operator()(const Argument& arg) const { return arg.m_hash; }
        };

        Argument(Shader::Type shader_type, const std::string& argument_name) noexcept;

        [[nodiscard]] Shader::Type       GetShaderType() const noexcept { return m_shader_type; }
        [[nodiscard]] const std::string& GetName() const noexcept       { return m_name; }
        [[nodiscard]] size_t             GetHash() const noexcept       { return m_hash; }

        [[nodiscard]] bool operator==(const Argument& other) const noexcept;
        [[nodiscard]] explicit operator std::string() const noexcept;

    private:
        Shader::Type m_shader_type;
        std::string  m_name;
        size_t       m_hash;
    };

    using Arguments = std::unordered_set<Argument, Argument::Hash>;

    class ArgumentAccessor : public Argument
    {
    public:
        enum class Type : uint32_t
        {
            Constant      = 1U << 0U,
            FrameConstant = 1U << 1U,
            Mutable       = 1U << 2U,
        };

        ArgumentAccessor(Shader::Type shader_type, const std::string& argument_name, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;
        ArgumentAccessor(const Argument& argument, Type accessor_type = Type::Mutable, bool addressable = false) noexcept;

        [[nodiscard]] Type   GetAccessorType() const noexcept  { return m_accessor_type; }
        [[nodiscard]] size_t GetAccessorIndex() const noexcept { return magic_enum::enum_index(m_accessor_type).value(); }
        [[nodiscard]] bool   IsAddressable() const noexcept    { return m_addressable; }
        [[nodiscard]] bool   IsConstant() const noexcept       { return m_accessor_type == Type::Constant; }
        [[nodiscard]] bool   IsFrameConstant() const noexcept  { return m_accessor_type == Type::FrameConstant; }

    private:
        Type m_accessor_type = Type::Mutable;
        bool m_addressable   = false;
    };

    using ArgumentAccessors = std::unordered_set<ArgumentAccessor, ArgumentAccessor::Hash>;
    static ArgumentAccessors::const_iterator FindArgumentAccessor(const ArgumentAccessors& argument_accessors, const Argument& argument);
    using Shaders = Ptrs<Shader>;

    // Program settings
    struct Settings
    {
        Shaders            shaders;
        InputBufferLayouts input_buffer_layouts;
        ArgumentAccessors  argument_accessors;
        PixelFormats       color_formats;
        PixelFormat        depth_format = PixelFormat::Unknown;
    };

    // Create Program instance
    [[nodiscard]] static Ptr<Program> Create(Context& context, const Settings& settings);

    // Program interface
    [[nodiscard]] virtual const Settings&      GetSettings() const = 0;
    [[nodiscard]] virtual const Shader::Types& GetShaderTypes() const = 0;
    [[nodiscard]] virtual const Ptr<Shader>&   GetShader(Shader::Type shader_type) const = 0;
};

} // namespace Methane::Graphics
