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

FILE: Methane/Graphics/Vulkan/Shader.cpp
Vulkan implementation of the shader interface.

******************************************************************************/

#include <Methane/Graphics/Vulkan/Shader.h>
#include <Methane/Graphics/Vulkan/Program.h>
#include <Methane/Graphics/Vulkan/IContext.h>
#include <Methane/Graphics/Vulkan/Device.h>
#include <Methane/Graphics/Vulkan/ProgramBindings.h>

#include <Methane/Data/IProvider.h>
#include <Methane/Graphics/Base/Context.h>
#include <Methane/Instrumentation.h>

#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>

namespace Methane::Graphics::Vulkan
{

// Number of words in the SPIRV byte code header, which is followed by the instructions stream
constexpr size_t g_spirv_header_words_count = 5U;

static bool IsHlslReflectionDecoration(uint32_t decoration)
{
    META_FUNCTION_TASK();
    return decoration == spv::DecorationHlslCounterBufferGOOGLE ||
           decoration == spv::DecorationHlslSemanticGOOGLE ||
           decoration == spv::DecorationUserTypeGOOGLE;
}

static bool IsHlslReflectionExtension(const char* extension_name)
{
    META_FUNCTION_TASK();
    return std::string_view(extension_name) == "SPV_GOOGLE_hlsl_functionality1" ||
           std::string_view(extension_name) == "SPV_GOOGLE_user_type";
}

// Shaders are compiled from HLSL to SPIRV with the '-fspv-reflect' DXC option, which decorates SPIRV byte code
// with the HLSL reflection information (semantic names, user types and counter buffers) declared by the
// SPV_GOOGLE_hlsl_functionality1 and SPV_GOOGLE_user_type extensions. HLSL semantic names are used by Methane
// to match vertex shader inputs with the program input buffer layouts (see InitializeVertexInputDescriptions),
// but the shader module can be created only when the corresponding device extensions are supported by the driver.
// So the reflection information is removed from the byte code copy used for the shader module creation, to avoid
// validation error VUID-VkShaderModuleCreateInfo-pCode-08742, while the original byte code is kept for reflection.
static std::vector<uint32_t> RemoveHlslReflectionFromSpirv(const uint32_t* spirv_data_ptr, size_t spirv_words_count)
{
    META_FUNCTION_TASK();
    if (spirv_words_count < g_spirv_header_words_count)
        throw InvalidArgumentException<size_t>(std::source_location::current(), "spirv_words_count",
                                              spirv_words_count, "SPIRV byte code is shorter than its header");

    std::vector<uint32_t> stripped_spirv;
    stripped_spirv.reserve(spirv_words_count);
    stripped_spirv.assign(spirv_data_ptr, spirv_data_ptr + g_spirv_header_words_count);

    for (size_t word_index = g_spirv_header_words_count; word_index < spirv_words_count;)
    {
        const uint32_t instruction    = spirv_data_ptr[word_index];
        const auto     op_code        = static_cast<spv::Op>(instruction & spv::OpCodeMask);
        const size_t   op_words_count = instruction >> spv::WordCountShift;
        if (op_words_count == 0U || op_words_count > spirv_words_count - word_index)
            throw InvalidArgumentException<size_t>(std::source_location::current(), "op_words_count", op_words_count,
                                                  "SPIRV instruction word count is zero or exceeds the byte code bounds");

        bool is_reflection_instruction = false;
        switch (op_code) // NOSONAR - do not add default case to switch
        {
        case spv::OpExtension:
            is_reflection_instruction = IsHlslReflectionExtension(reinterpret_cast<const char*>(&spirv_data_ptr[word_index + 1])); // NOSONAR
            break;

        case spv::OpDecorateId:
        case spv::OpDecorateString:
            is_reflection_instruction = op_words_count > 2U && IsHlslReflectionDecoration(spirv_data_ptr[word_index + 2]);
            break;

        case spv::OpMemberDecorateString:
            is_reflection_instruction = op_words_count > 3U && IsHlslReflectionDecoration(spirv_data_ptr[word_index + 3]);
            break;

        default:
            break;
        }

        if (!is_reflection_instruction)
        {
            stripped_spirv.insert(stripped_spirv.end(),
                                  spirv_data_ptr + word_index,
                                  spirv_data_ptr + word_index + op_words_count);
        }
        word_index += op_words_count;
    }

    return stripped_spirv;
}

static vk::VertexInputRate ConvertInputBufferLayoutStepTypeToVertexInputRate(Rhi::IProgram::InputBufferLayout::StepType step_type)
{
    META_FUNCTION_TASK();
    using StepType = Rhi::IProgram::InputBufferLayout::StepType;
    switch(step_type)
    {
    using enum vk::VertexInputRate;
    case StepType::PerVertex:   return eVertex;
    case StepType::PerInstance: return eInstance;
    default:                    META_UNEXPECTED_RETURN(step_type, vk::VertexInputRate::eVertex);
    }
}

static vk::Format GetFloatVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    using enum vk::Format;
    case 1: return eR32Sfloat;
    case 2: return eR32G32Sfloat;
    case 3: return eR32G32B32Sfloat;
    case 4: return eR32G32B32A32Sfloat;
    default: META_UNEXPECTED_RETURN(vector_size, eUndefined);
    }
}

static vk::Format GetSignedIntegerVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    using enum vk::Format;
    case 1: return eR32Sint;
    case 2: return eR32G32Sint;
    case 3: return eR32G32B32Sint;
    case 4: return eR32G32B32A32Sint;
    default: META_UNEXPECTED_RETURN(vector_size, eUndefined);
    }
}

static vk::Format GetUnsignedIntegerVectorFormat(uint32_t vector_size)
{
    META_FUNCTION_TASK();
    switch (vector_size)
    {
    using enum vk::Format;
    case 1: return eR32Uint;
    case 2: return eR32G32Uint;
    case 3: return eR32G32B32Uint;
    case 4: return eR32G32B32A32Uint;
    default: META_UNEXPECTED_RETURN(vector_size, eUndefined);
    }
}

static vk::Format GetVertexAttributeFormatFromSpirvType(const spirv_cross::SPIRType& attribute_type)
{
    META_FUNCTION_TASK();
    switch(attribute_type.basetype)
    {
    case spirv_cross::SPIRType::Float: return GetFloatVectorFormat(attribute_type.vecsize);
    case spirv_cross::SPIRType::UInt:  return GetSignedIntegerVectorFormat(attribute_type.vecsize);
    case spirv_cross::SPIRType::Int:   return GetUnsignedIntegerVectorFormat(attribute_type.vecsize);
    default:                           META_UNEXPECTED_RETURN(attribute_type.basetype, vk::Format::eUndefined);
    }
}

static uint32_t GetArraySize(const spirv_cross::SPIRType& resource_type) noexcept
{
    META_FUNCTION_TASK();
    if (resource_type.array.empty())
        return 1;

    return resource_type.array.front()
           ? resource_type.array.front()
           : std::numeric_limits<uint32_t>::max();
}

static Rhi::IResource::Type ConvertDescriptorTypeToResourceType(vk::DescriptorType vk_descriptor_type)
{
    META_FUNCTION_TASK();
    switch(vk_descriptor_type)
    {
    using enum vk::DescriptorType;
    using enum Rhi::ResourceType;
    case eUniformBuffer:
    case eStorageBuffer:
    case eInlineUniformBlock:
        return Buffer;

    case eStorageImage:
    case eSampledImage:
        return Texture;

    case eSampler:
        return Sampler;

    default:
        META_UNEXPECTED_RETURN(vk_descriptor_type, Buffer);
    }
}

static vk::DescriptorType UpdateDescriptorType(vk::DescriptorType vk_shader_descriptor_type, const Rhi::ProgramArgumentAccessor& argument_accessor)
{
    META_FUNCTION_TASK();
    if (!argument_accessor.IsAddressable())
        return vk_shader_descriptor_type;

    switch(vk_shader_descriptor_type)
    {
    using enum vk::DescriptorType;
    case eUniformBuffer: return eUniformBufferDynamic;
    case eStorageBuffer: return eStorageBufferDynamic;
    default: META_UNEXPECTED_RETURN_DESCR(vk_shader_descriptor_type, vk_shader_descriptor_type,
                                          "addressable arguments support only Uniform or Storage buffers");
    }
}

static void AddSpirvResourcesToArgumentBindings(const spirv_cross::Compiler& spirv_compiler,
                                                const spirv_cross::SmallVector<spirv_cross::Resource>& spirv_resources,
                                                const vk::DescriptorType vk_descriptor_type,
                                                const Rhi::ProgramArgumentAccessors& argument_accessors,
                                                const Shader& shader,
                                                Ptrs<Base::ProgramArgumentBinding>& argument_bindings)
{
    META_FUNCTION_TASK();
    if (spirv_resources.begin() == spirv_resources.end())
        return;

    const Rhi::IResource::Type resource_type = ConvertDescriptorTypeToResourceType(vk_descriptor_type);\
    const Rhi::ShaderType shader_type = shader.GetType();

    for (const spirv_cross::Resource& resource : spirv_resources)
    {
        const spirv_cross::SPIRType& spirv_type = spirv_compiler.get_type(resource.type_id);
        const uint32_t array_size = GetArraySize(spirv_type);
        const uint32_t buffer_size = spirv_type.basetype == spirv_cross::SPIRType::BaseType::Struct
                                   ? static_cast<uint32_t>(spirv_compiler.get_declared_struct_size(spirv_type))
                                   : 0U;

        ProgramBindings::ArgumentBinding::ByteCodeMap byte_code_map{ shader_type };
        if (vk_descriptor_type != vk::DescriptorType::eInlineUniformBlock)
        {
            META_CHECK_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationDescriptorSet, byte_code_map.descriptor_set_offset));
            META_CHECK_TRUE(spirv_compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationBinding, byte_code_map.binding_offset));
        }

        const uint32_t descriptor_set_id = spirv_compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        const Rhi::ProgramArgumentAccessType arg_access_type = Rhi::ProgramArgumentAccessor::GetTypeByRegisterSpace(descriptor_set_id);
        const Rhi::ProgramArgumentValueType arg_value_type = vk_descriptor_type == vk::DescriptorType::eInlineUniformBlock
                                                           ? Rhi::ProgramArgumentValueType::RootConstantValue
                                                           : Rhi::ProgramArgumentValueType::ResourceView;

        const Rhi::ProgramArgument shader_argument(shader_type, shader.GetCachedArgName(spirv_compiler.get_name(resource.id)));
        const Rhi::ProgramArgumentAccessor* argument_accessor_ptr = Rhi::IProgram::FindArgumentAccessor(argument_accessors, shader_argument);
        const Rhi::ProgramArgumentAccessor argument_acc = argument_accessor_ptr
                                                          ? *argument_accessor_ptr
                                                          : Rhi::ProgramArgumentAccessor(shader_argument, arg_access_type, arg_value_type);

        argument_bindings.push_back(std::make_shared<ProgramBindings::ArgumentBinding>(
            shader.GetContext(),
            ProgramArgumentBindingSettings
            {
                Rhi::ProgramArgumentBindingSettings
                {
                    argument_acc,
                    resource_type,
                    array_size,
                    buffer_size
                },
                UpdateDescriptorType(vk_descriptor_type, argument_acc),
                { std::move(byte_code_map) }
            }
        ));

        META_LOG("  - '{}' with descriptor type {}, array size {};",
                 shader_argument.GetName(),
                 vk::to_string(vk_descriptor_type),
                 array_size);
    }
}

Shader::Shader(Rhi::ShaderType shader_type, const Base::Context& context, const Settings& settings)
    : Base::Shader(shader_type, context, settings)
    , m_vk_context(dynamic_cast<const IContext&>(context))
    , m_byte_code_chunk(settings.data_provider.GetData(fmt::format("{}.spirv", GetCompiledEntryFunctionName(settings))))
{ }

Shader::~Shader() = default;

Ptrs<Base::ProgramArgumentBinding> Shader::GetArgumentBindings(const Rhi::ProgramArgumentAccessors& argument_accessors) const
{
    META_FUNCTION_TASK();
    const Rhi::IShader::Settings& shader_settings = GetSettings();
    META_UNUSED(shader_settings);

    META_LOG("{} shader '{}' ({}) with argument bindings:",
             magic_enum::enum_name(GetType()),
             shader_settings.entry_function.function_name,
             Rhi::ShaderMacroDefinition::ToString(shader_settings.compile_definitions));

    Ptrs<Base::ProgramArgumentBinding> argument_bindings;
    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const auto add_spirv_resources_to_argument_bindings = [this, &spirv_compiler, &argument_accessors, &argument_bindings]
                                                          (const spirv_cross::SmallVector<spirv_cross::Resource>& spirv_resources,
                                                           const vk::DescriptorType vk_descriptor_type)
    {
        AddSpirvResourcesToArgumentBindings(spirv_compiler, spirv_resources, vk_descriptor_type, argument_accessors, *this, argument_bindings);
    };

    // Get only resources that are statically used in SPIRV-code (skip all resources that are never accessed by the shader)
    const spirv_cross::ShaderResources spirv_resources = spirv_compiler.get_shader_resources(spirv_compiler.get_active_interface_variables());

    add_spirv_resources_to_argument_bindings(spirv_resources.push_constant_buffers, vk::DescriptorType::eInlineUniformBlock);
    add_spirv_resources_to_argument_bindings(spirv_resources.uniform_buffers,       vk::DescriptorType::eUniformBuffer);
    add_spirv_resources_to_argument_bindings(spirv_resources.storage_buffers,       vk::DescriptorType::eStorageBuffer);
    add_spirv_resources_to_argument_bindings(spirv_resources.storage_images,        vk::DescriptorType::eStorageImage);
    add_spirv_resources_to_argument_bindings(spirv_resources.sampled_images,        vk::DescriptorType::eCombinedImageSampler);
    add_spirv_resources_to_argument_bindings(spirv_resources.separate_images,       vk::DescriptorType::eSampledImage);
    add_spirv_resources_to_argument_bindings(spirv_resources.separate_samplers,     vk::DescriptorType::eSampler);
    // TODO: add support for spirv_resources.atomic_counters, vk::DescriptorType::eMutableVALVE

    if (argument_bindings.empty())
    {
        META_LOG("  - No argument bindings.");
    }

    return argument_bindings;
}

const vk::ShaderModule& Shader::GetNativeModule() const
{
    META_FUNCTION_TASK();
    if (m_vk_unique_module)
        return m_vk_unique_module.get();

    const Device& device = m_vk_context.GetVulkanDevice();
    if (device.IsExtensionSupported(VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME) &&
        device.IsExtensionSupported(VK_GOOGLE_USER_TYPE_EXTENSION_NAME))
    {
        m_vk_unique_module = device.GetNativeDevice().createShaderModuleUnique(
            vk::ShaderModuleCreateInfo(
                vk::ShaderModuleCreateFlags{},
                m_byte_code_chunk.GetDataSize(),
                m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>())
        );
    }
    else
    {
        // Device does not support HLSL reflection extensions, so it is removed from the shader module byte code
        const std::vector<uint32_t> stripped_byte_code = RemoveHlslReflectionFromSpirv(m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>(),
                                                                                       m_byte_code_chunk.GetDataSize<uint32_t>());
        m_vk_unique_module = device.GetNativeDevice().createShaderModuleUnique(
            vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags{}, stripped_byte_code)
        );
    }
    return m_vk_unique_module.get();
}

const spirv_cross::Compiler& Shader::GetNativeCompiler() const
{
    META_FUNCTION_TASK();
    if (m_spirv_compiler_ptr)
        return *m_spirv_compiler_ptr;

    m_spirv_compiler_ptr = std::make_unique<spirv_cross::Compiler>(m_byte_code_chunk.AsConstChunk().GetDataPtr<uint32_t>(),
                                                                   m_byte_code_chunk.GetDataSize<uint32_t>());
    return *m_spirv_compiler_ptr;
}

vk::PipelineShaderStageCreateInfo Shader::GetNativeStageCreateInfo() const
{
    META_FUNCTION_TASK();
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags{},
        ConvertTypeToStageFlagBits(GetType()),
        GetNativeModule(),
        GetSettings().entry_function.function_name.c_str()
    );
}

vk::PipelineVertexInputStateCreateInfo Shader::GetNativeVertexInputStateCreateInfo(const Program& program)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(GetType(), Rhi::ShaderType::Vertex);
    std::lock_guard lock(m_mutex);

    if (!m_vertex_input_initialized)
        InitializeVertexInputDescriptions(program);

    return vk::PipelineVertexInputStateCreateInfo(
        vk::PipelineVertexInputStateCreateFlags{},
        m_vertex_input_binding_descriptions,
        m_vertex_input_attribute_descriptions
    );
}

Data::MutableChunk& Shader::GetMutableByteCode() noexcept
{
    META_FUNCTION_TASK();
    m_vk_unique_module.reset();
    m_spirv_compiler_ptr.reset();
    return m_byte_code_chunk;
}

void Shader::InitializeVertexInputDescriptions(const Program& program)
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL(GetType(), Rhi::ShaderType::Vertex);
    META_CHECK_FALSE_DESCR(m_vertex_input_initialized, "vertex input descriptions are already initialized");

    const Rhi::IShader::Settings              & shader_settings      = GetSettings();
    const Base::Program::InputBufferLayouts& input_buffer_layouts = program.GetSettings().input_buffer_layouts;
    m_vertex_input_binding_descriptions.reserve(input_buffer_layouts.size());

    uint32_t input_buffer_index = 0U;
    for(const Rhi::IProgram::InputBufferLayout& input_buffer_layout : input_buffer_layouts)
    {
        m_vertex_input_binding_descriptions.emplace_back(
            input_buffer_index,
            0U, // stride is auto calculated by vertex attributes
            ConvertInputBufferLayoutStepTypeToVertexInputRate(input_buffer_layout.step_type)
        );
        input_buffer_index++;
    }

    const spirv_cross::Compiler& spirv_compiler = GetNativeCompiler();
    const spirv_cross::ShaderResources shader_resources = spirv_compiler.get_shader_resources();

#ifdef METHANE_LOGGING_ENABLED
    std::stringstream log_ss;
    log_ss << magic_enum::enum_name(GetType())
           << " shader '" << shader_settings.entry_function.function_name
           << "' (" << Rhi::ShaderMacroDefinition::ToString(shader_settings.compile_definitions)
           << ") input layout:" << std::endl;
    if (shader_resources.stage_inputs.empty())
        log_ss << " - No stage inputs." << std::endl;
#else
    META_UNUSED(shader_settings);
#endif

    m_vertex_input_attribute_descriptions.reserve(shader_resources.stage_inputs.size());
    for(const spirv_cross::Resource& input_resource : shader_resources.stage_inputs)
    {
        const bool has_semantic = spirv_compiler.has_decoration(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const bool has_location = spirv_compiler.has_decoration(input_resource.id, spv::DecorationLocation);
        META_CHECK_TRUE(has_semantic && has_location);

        const std::string&           semantic_name    = spirv_compiler.get_decoration_string(input_resource.id, spv::DecorationHlslSemanticGOOGLE);
        const uint32_t               input_location   = spirv_compiler.get_decoration(input_resource.id, spv::DecorationLocation);
        const spirv_cross::SPIRType& attribute_type   = spirv_compiler.get_type(input_resource.base_type_id);
        const vk::Format             attribute_format = GetVertexAttributeFormatFromSpirvType(attribute_type);

        const uint32_t buffer_index = GetProgramInputBufferIndexByArgumentSemantic(program, semantic_name);
        META_CHECK_LESS(buffer_index, m_vertex_input_binding_descriptions.size());
        vk::VertexInputBindingDescription& input_binding_desc = m_vertex_input_binding_descriptions[buffer_index];

        m_vertex_input_attribute_descriptions.emplace_back(
            input_location,
            buffer_index,
            attribute_format,
            input_binding_desc.stride
        );

#ifdef METHANE_LOGGING_ENABLED
        log_ss << "  - Input semantic name '" << semantic_name
               << "' location " << input_location
               << " buffer " << buffer_index
               << " binding " << input_binding_desc.binding
               << " with attribute format " << vk::to_string(attribute_format)
               << ";" << std::endl;
#endif

        // Tight packing of attributes in vertex buffer is assumed
        input_binding_desc.stride += attribute_type.vecsize * 4;
    }

    META_LOG("{}", log_ss.str());

    m_vertex_input_initialized = true;
}

vk::ShaderStageFlagBits Shader::ConvertTypeToStageFlagBits(Rhi::ShaderType shader_type)
{
    META_FUNCTION_TASK();
    switch(shader_type)
    {
    using enum Rhi::ShaderType;
    using enum vk::ShaderStageFlagBits;
    case All:     return eAll;
    case Vertex:  return eVertex;
    case Pixel:   return eFragment;
    case Compute: return eCompute;
    default: META_UNEXPECTED_RETURN(shader_type, eAll);
    }
}

} // namespace Methane::Graphics::Vulkan
