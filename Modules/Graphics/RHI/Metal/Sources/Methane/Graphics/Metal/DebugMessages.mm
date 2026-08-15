/******************************************************************************

Copyright 2026 Evgeny Gorodetskiy

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

FILE: Methane/Graphics/Metal/DebugMessages.mm
Metal debug messages printing to the platform debug output.

Messages are printed with the same severity and category prefix as it is done by the Vulkan debug
utils messenger callback in Methane::Graphics::Vulkan::System and by the DirectX 12 debug message
callback in Methane::Graphics::DirectX::Device, so that validation messages of all graphics APIs
are recognized with the same patterns in the Build/Unix/CI/RunApplicationsTest.sh test script.

Note that unlike DirectX 12 (ID3D12InfoQueue1) and Vulkan (VK_EXT_debug_utils), Metal does not provide
any callback to intercept messages of the API validation layer: these messages are printed by Metal
itself either with NSLog or as an assertion failure, depending on the MTL_DEBUG_LAYER_ERROR_MODE and
MTL_DEBUG_LAYER_WARNING_MODE environment variables. Metal makes available programmatically only the
GPU shader validation messages (MTLFunctionLog, MTLLogState) and the command buffer execution errors
(MTLCommandBuffer.error), which are printed to the platform debug output here.

******************************************************************************/

#include <Methane/Graphics/Metal/DebugMessages.hh>

#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>

namespace Methane::Graphics::Metal
{

// Size of the GPU buffer used by MTLLogState to store shader log messages
static const NSInteger g_log_state_buffer_size = 16 * 1024;

static bool IsEnvironmentVariableEnabled(const char* variable_name)
{
    META_FUNCTION_TASK();
    const char* variable_value = std::getenv(variable_name); // NOSONAR - environment is not modified by Methane
    return variable_value && std::string_view(variable_value) != "0";
}

static std::string GetNsStringOrDefault(NSString* ns_string, std::string_view default_value)
{
    META_FUNCTION_TASK();
    return ns_string ? MacOS::ConvertFromNsString(ns_string) : std::string(default_value);
}

static std::string_view GetLogLevelSeverityName(NSInteger log_level)
{
    META_FUNCTION_TASK();
    // MTLLogLevel values are used as integers, because the MTLLogLevel enum is available only since macOS 15.0
    switch (log_level) // NOSONAR - do not use magic_enum, because severity names are formatted for logging
    {
    case 5:  return "Error";    // MTLLogLevelFault
    case 4:  return "Error";    // MTLLogLevelError
    case 3:  return "Warning";  // MTLLogLevelNotice
    case 2:  return "Info";     // MTLLogLevelInfo
    case 1:  return "Info";     // MTLLogLevelDebug
    default: return "Unknown";  // MTLLogLevelUndefined
    }
}

static std::string_view GetCommandBufferErrorName(NSInteger error_code)
{
    META_FUNCTION_TASK();
    switch (error_code) // NOSONAR - do not use magic_enum, because error names are formatted for logging
    {
    case MTLCommandBufferErrorNone:            return "None";
    case MTLCommandBufferErrorInternal:        return "Internal";
    case MTLCommandBufferErrorTimeout:         return "Timeout";
    case MTLCommandBufferErrorPageFault:       return "PageFault";
    case MTLCommandBufferErrorAccessRevoked:   return "AccessRevoked";
    case MTLCommandBufferErrorNotPermitted:    return "NotPermitted";
    case MTLCommandBufferErrorOutOfMemory:     return "OutOfMemory";
    case MTLCommandBufferErrorInvalidResource: return "InvalidResource";
    case MTLCommandBufferErrorMemoryless:      return "Memoryless";
#ifdef APPLE_MACOS
    case MTLCommandBufferErrorDeviceRemoved:   return "DeviceRemoved";
#endif
    case MTLCommandBufferErrorStackOverflow:   return "StackOverflow";
    default:                                   return "Unknown";
    }
}

static std::string_view GetEncoderErrorStateName(MTLCommandEncoderErrorState error_state)
{
    META_FUNCTION_TASK();
    switch (error_state) // NOSONAR - do not use magic_enum, because state names are formatted for logging
    {
    case MTLCommandEncoderErrorStateCompleted: return "Completed";
    case MTLCommandEncoderErrorStateAffected:  return "Affected";
    case MTLCommandEncoderErrorStatePending:   return "Pending";
    case MTLCommandEncoderErrorStateFaulted:   return "Faulted";
    default:                                   return "Unknown";
    }
}

static void PrintCommandBufferError(const id<MTLCommandBuffer>& mtl_command_buffer, NSError* ns_error)
{
    META_FUNCTION_TASK();
    std::stringstream ss;
    ss << "Error Metal CommandBuffer:" << std::endl;
    ss << "\t- commandBuffer:   " << GetNsStringOrDefault(mtl_command_buffer.label, "unnamed") << std::endl;
    ss << "\t- errorDomain:     " << GetNsStringOrDefault(ns_error.domain, "") << std::endl;
    ss << "\t- errorCode:       " << ns_error.code << " (" << GetCommandBufferErrorName(ns_error.code) << ")" << std::endl;
    ss << "\t- message:         " << GetNsStringOrDefault(ns_error.localizedDescription, "") << std::endl;

    NSArray<id<MTLCommandBufferEncoderInfo>>* mtl_encoder_infos = ns_error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
    if (mtl_encoder_infos.count > 0U)
    {
        ss << "\t- Encoders:" << std::endl;
        NSUInteger encoder_index = 0U;
        for(const id<MTLCommandBufferEncoderInfo> mtl_encoder_info in mtl_encoder_infos)
        {
            ss << "\t\t- Encoder " << encoder_index << ":" << std::endl;
            ss << "\t\t\t- label:      " << GetNsStringOrDefault(mtl_encoder_info.label, "unnamed") << std::endl;
            ss << "\t\t\t- errorState: " << GetEncoderErrorStateName(mtl_encoder_info.errorState) << std::endl;
            for(NSString* mtl_debug_signpost in mtl_encoder_info.debugSignposts)
            {
                ss << "\t\t\t- signpost:   " << GetNsStringOrDefault(mtl_debug_signpost, "") << std::endl;
            }
            encoder_index++;
        }
    }

    Methane::Platform::PrintToDebugOutput(ss.str());
}

static void PrintCommandBufferShaderLogs(const id<MTLCommandBuffer>& mtl_command_buffer)
{
    META_FUNCTION_TASK();
    // MTLFunctionLog messages are produced by the GPU shader validation layer enabled with MTL_SHADER_VALIDATION=1
    for(const id<MTLFunctionLog> mtl_function_log in mtl_command_buffer.logs)
    {
        std::stringstream ss;
        ss << "Error Metal ShaderValidation:" << std::endl;
        ss << "\t- commandBuffer:   " << GetNsStringOrDefault(mtl_command_buffer.label, "unnamed") << std::endl;
        ss << "\t- encoderLabel:    " << GetNsStringOrDefault(mtl_function_log.encoderLabel, "unnamed") << std::endl;

        if (const id<MTLFunctionLogDebugLocation> mtl_debug_location = mtl_function_log.debugLocation;
            mtl_debug_location)
        {
            ss << "\t- functionName:    " << GetNsStringOrDefault(mtl_debug_location.functionName, "unknown") << std::endl;
            ss << "\t- sourceLocation:  " << GetNsStringOrDefault(mtl_debug_location.URL.absoluteString, "unknown")
               << ":" << mtl_debug_location.line << ":" << mtl_debug_location.column << std::endl;
        }

        ss << "\t- message:         " << GetNsStringOrDefault([mtl_function_log description], "") << std::endl;
        Methane::Platform::PrintToDebugOutput(ss.str());
    }
}

bool IsDebugLayerEnabled()
{
    META_FUNCTION_TASK();
    // Environment variables are checked only once, because they are not changed during the application run
    static const bool s_debug_layer_enabled = IsEnvironmentVariableEnabled("MTL_DEBUG_LAYER") ||
                                              IsEnvironmentVariableEnabled("MTL_SHADER_VALIDATION");
    return s_debug_layer_enabled;
}

id CreateDebugLogState(const id<MTLDevice>& mtl_device)
{
    META_FUNCTION_TASK();
    if (!IsDebugLayerEnabled())
        return nil;

    if (@available(macOS 15.0, iOS 18.0, tvOS 18.0, *))
    {
        MTLLogStateDescriptor* mtl_log_state_desc = [[MTLLogStateDescriptor alloc] init];
        mtl_log_state_desc.level = MTLLogLevelDebug;
        mtl_log_state_desc.bufferSize = g_log_state_buffer_size;

        NSError* ns_error = nil;
        const id<MTLLogState> mtl_log_state = [mtl_device newLogStateWithDescriptor:mtl_log_state_desc error:&ns_error];
        if (!mtl_log_state)
        {
            std::stringstream ss;
            ss << "Warning Metal DebugLayer:" << std::endl;
            ss << "\t- message:         failed to create GPU shader log state: "
               << GetNsStringOrDefault(ns_error.localizedDescription, "unknown error") << std::endl;
            Methane::Platform::PrintToDebugOutput(ss.str());
            return nil;
        }

        [mtl_log_state addLogHandler:^(NSString* subsystem, NSString* category, MTLLogLevel log_level, NSString* message)
        {
            std::stringstream ss;
            ss << GetLogLevelSeverityName(log_level) << " Metal ShaderLog:" << std::endl;
            ss << "\t- subSystem:       " << GetNsStringOrDefault(subsystem, "") << std::endl;
            ss << "\t- category:        " << GetNsStringOrDefault(category, "") << std::endl;
            ss << "\t- message:         " << GetNsStringOrDefault(message, "") << std::endl;
            Methane::Platform::PrintToDebugOutput(ss.str());
        }];

        return mtl_log_state;
    }

    return nil;
}

id<MTLCommandQueue> CreateDebugCommandQueue(const id<MTLDevice>& mtl_device, const id& mtl_log_state)
{
    META_FUNCTION_TASK();
    if (mtl_log_state)
    {
        if (@available(macOS 15.0, iOS 18.0, tvOS 18.0, *))
        {
            MTLCommandQueueDescriptor* mtl_command_queue_desc = [[MTLCommandQueueDescriptor alloc] init];
            mtl_command_queue_desc.logState = mtl_log_state;
            return [mtl_device newCommandQueueWithDescriptor:mtl_command_queue_desc];
        }
    }
    return [mtl_device newCommandQueue];
}

id<MTLCommandBuffer> CreateDebugCommandBuffer(const id<MTLCommandQueue>& mtl_command_queue)
{
    META_FUNCTION_TASK();
    // Command buffers inherit the shader log state from the command queue they are created with,
    // so it does not need to be set in the command buffer descriptor here.
    if (!IsDebugLayerEnabled())
        return [mtl_command_queue commandBuffer];

    // Per-encoder execution status is requested to print labels of the command encoders affected by an execution
    // error. It is enabled only along with the debug layer, because it may increase CPU, GPU and memory overhead.
    MTLCommandBufferDescriptor* mtl_command_buffer_desc = [[MTLCommandBufferDescriptor alloc] init];
    mtl_command_buffer_desc.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    return [mtl_command_queue commandBufferWithDescriptor:mtl_command_buffer_desc];
}

void PrintCommandBufferDebugMessages(const id<MTLCommandBuffer>& mtl_command_buffer)
{
    META_FUNCTION_TASK();
    if (NSError* ns_error = mtl_command_buffer.error;
        ns_error)
    {
        PrintCommandBufferError(mtl_command_buffer, ns_error);
    }

    if (IsDebugLayerEnabled())
    {
        PrintCommandBufferShaderLogs(mtl_command_buffer);
    }
}

} // namespace Methane::Graphics::Metal
