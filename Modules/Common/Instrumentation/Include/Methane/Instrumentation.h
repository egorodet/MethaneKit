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

FILE: Methane/Instrumentation.h
Common header for instrumentation of the Methane Kit modules with ITT macroses,
Defines common ITT domain required for instrumentation.

NOTE:
    This header is force included first in every source file,
    which is linked with cmake interface target MethaneInstrumentation,
    when METHANE_TRACY_PROFILING_ENABLED = ON

******************************************************************************/

#pragma once

#include "IttApiHelper.h"
#include "ScopeTimer.h"

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define __GCC_COMPILER__
#endif

#ifdef __GCC_COMPILER__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>
#endif

#ifdef __GCC_COMPILER__
#pragma GCC diagnostic pop
#endif

#include <string_view>

#if defined(ITT_INSTRUMENTATION_ENABLED) || defined(TRACY_ENABLE)
#define META_INSTRUMENTATION_ENABLED
#endif

namespace Methane
{
void SetThreadName(std::string_view name);
}

constexpr const char* g_methane_itt_domain_name = "Methane Kit";

ITT_DOMAIN_EXTERN();

#ifdef TRACY_ENABLE

#define TRACY_MUTEX(mutex_type) tracy::Lockable<mutex_type>
#define TRACY_SET_THREAD_NAME(name) tracy::SetThreadName(name)

#if defined(TRACY_ZONE_CALL_STACK_DEPTH) && TRACY_ZONE_CALL_STACK_DEPTH > 0

#define TRACY_ZONE_SCOPED() ZoneScopedS(TRACY_ZONE_CALL_STACK_DEPTH)
#define TRACY_ZONE_SCOPED_NAME(name) ZoneScopedNS(name, TRACY_ZONE_CALL_STACK_DEPTH)

#else // defined(TRACY_ZONE_CALL_STACK_DEPTH) && TRACY_ZONE_CALL_STACK_DEPTH > 0

#define TRACY_ZONE_SCOPED() ZoneScoped
#define TRACY_ZONE_SCOPED_NAME(name) ZoneScopedN(name)

#endif // defined(TRACY_ZONE_CALL_STACK_DEPTH) && TRACY_ZONE_CALL_STACK_DEPTH > 0

#else // ifdef TRACY_ENABLE

#define TRACY_MUTEX(mutex_type) mutex_type
#define TRACY_SET_THREAD_NAME(name)
#define TRACY_ZONE_SCOPED()
#define TRACY_ZONE_SCOPED_NAME(name)

#define TracyMessage(S, N)
#define TracyLockable(M, V) M V
#define LockableBase(M) M
#define FrameMark
#define TracyCFrameMarkStart(name)
#define TracyCFrameMarkEnd(name)

#endif // ifdef TRACY_ENABLE

#ifdef META_INSTRUMENTATION_ENABLED

#define META_CPU_FRAME_DELIMITER(/* uint32_t */ frame_buffer_index, /* uint32_t */ frame_index) \
    FrameMark; \
    ITT_PROCESS_MARKER("Methane-Frame-Delimiter"); \
    ITT_MARKER_ARG("Frame-Buffer-Index", static_cast<int64_t>(frame_buffer_index)); \
    ITT_MARKER_ARG("Frame-Index", static_cast<int64_t>(frame_index))

#define META_CPU_FRAME_START(/*const char* */name) \
    TracyCFrameMarkStart(name)

#define META_CPU_FRAME_END(/*const char* */name) \
    TracyCFrameMarkEnd(name)

#define META_SCOPE_TASK(/*const char* */name) \
    TRACY_ZONE_SCOPED_NAME(name); \
    ITT_SCOPE_TASK(name)

#define META_FUNCTION_TASK() \
    TRACY_ZONE_SCOPED(); \
    ITT_FUNCTION_TASK()

#define META_GLOBAL_MARKER(/*const char* */name) \
    ITT_GLOBAL_MARKER(name)
#define META_PROCESS_MARKER(/*const char* */name) \
    ITT_PROCESS_MARKER(name)
#define META_THREAD_MARKER(/*const char* */name) \
    ITT_THREAD_MARKER(name)
#define META_TASK_MARKER(/*const char* */name) \
    ITT_TASK_MARKER(name)

#define META_FUNCTION_GLOBAL_MARKER() \
    ITT_FUNCTION_GLOBAL_MARKER()
#define META_FUNCTION_PROCESS_MARKER() \
    ITT_FUNCTION_PROCESS_MARKER()
#define META_FUNCTION_THREAD_MARKER() \
    ITT_FUNCTION_THREAD_MARKER()
#define META_FUNCTION_TASK_MARKER() \
    ITT_FUNCTION_TASK_MARKER()

#define META_THREAD_NAME(/*const char* */name) \
    TRACY_SET_THREAD_NAME(name); \
    ITT_THREAD_NAME(name); \
    Methane::SetThreadName(name)

#else // ifdef META_INSTRUMENTATION_ENABLED

#define META_CPU_FRAME_DELIMITER(/* uint32_t */ frame_buffer_index, /* uint32_t */ frame_index)
#define META_CPU_FRAME_START(/*const char* */name)
#define META_CPU_FRAME_END(/*const char* */name)
#define META_SCOPE_TASK(/*const char* */name)
#define META_FUNCTION_TASK()
#define META_GLOBAL_MARKER(/*const char* */name)
#define META_PROCESS_MARKER(/*const char* */name)
#define META_THREAD_MARKER(/*const char* */name)
#define META_TASK_MARKER(/*const char* */name)
#define META_FUNCTION_GLOBAL_MARKER()
#define META_FUNCTION_PROCESS_MARKER()
#define META_FUNCTION_THREAD_MARKER()
#define META_FUNCTION_TASK_MARKER()
#define META_THREAD_NAME(/*const char* */name)

#endif // ifdef META_INSTRUMENTATION_ENABLED

#ifdef METHANE_LOGGING_ENABLED

#include <Methane/Platform/Utils.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#define META_LOG(/*std::string_view*/message, ...) \
    Methane::Platform::PrintToDebugOutput(fmt::format(message, ## __VA_ARGS__))

#else // ifdef METHANE_LOGGING_ENABLED

#define META_LOG(/*const std::string& */message, ...)

#endif // ifdef METHANE_LOGGING_ENABLED
