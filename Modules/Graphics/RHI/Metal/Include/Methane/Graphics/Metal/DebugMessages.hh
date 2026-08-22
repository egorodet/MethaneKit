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

FILE: Methane/Graphics/Metal/DebugMessages.hh
Metal debug messages printing to the platform debug output.

******************************************************************************/

#pragma once

#import <Metal/Metal.h>

namespace Methane::Graphics::Metal
{

// Returns true when the Metal API validation layer or GPU shader validation is enabled with the
// MTL_DEBUG_LAYER / MTL_SHADER_VALIDATION environment variables, which are also set by the Xcode
// scheme diagnostics options and by the Build/Unix/CI/RunApplicationsTest.sh test script.
[[nodiscard]] bool IsDebugLayerEnabled();

// Creates MTLLogState which prints GPU shader log messages to the platform debug output as soon as they are received.
// Returns nil when the debug layer is disabled or when MTLLogState is not available (macOS 15.0, iOS 18.0, tvOS 18.0).
// Untyped 'id' is used to keep this header compatible with SDKs which do not declare the MTLLogState protocol.
[[nodiscard]] id CreateDebugLogState(const id<MTLDevice>& mtl_device);

// Creates MTLCommandQueue with the debug log state attached to it, when the log state is not nil.
[[nodiscard]] id<MTLCommandQueue> CreateDebugCommandQueue(const id<MTLDevice>& mtl_device, const id& mtl_log_state);

// Creates MTLCommandBuffer with per-encoder execution status error reporting enabled,
// when the Metal debug layer is enabled.
[[nodiscard]] id<MTLCommandBuffer> CreateDebugCommandBuffer(const id<MTLCommandQueue>& mtl_command_queue);

// Prints execution error and GPU shader validation messages of the completed command buffer to the platform
// debug output. Does nothing when the command buffer has neither an error nor shader validation messages.
void PrintCommandBufferDebugMessages(const id<MTLCommandBuffer>& mtl_command_buffer);

} // namespace Methane::Graphics::Metal
