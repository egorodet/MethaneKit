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

FILE: Methane/StbImage.h
Methane STB Image wrapper header to workaround some issues with stb_image.h
NOTE: STB target should be linked where STB is actually used.

******************************************************************************/

#pragma once

#if defined(_MSC_VER) // MSVC compiler

#pragma warning(push)
#pragma warning(disable: 4505) // unreferenced function with internal linkage has been removed

#elif defined(__clang__) // Clang or AppleClang compiler

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

#elif defined(__GNUC__) // GCC compiler

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// Disable stb_image SSE2 path: with GCC in Debug build and a precompiled header, the always-inline
// _mm_slli_si128/_mm_srli_si128 intrinsics fail in Ninja-Multiconfig Debug builds,
// causing "the last argument must be an 8-bit immediate" errors.
#ifndef NDEBUG
#define STBI_NO_SIMD
#endif

#endif // End of compiler branching

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_STDIO

#include <stb_image.h>

#if defined(_MSC_VER) // MSVC compiler

#pragma warning(pop)

#elif defined(__clang__) // Clang or AppleClang compiler

#pragma clang diagnostic pop

#elif defined(__GNUC__) // GCC compiler

#pragma GCC diagnostic pop

#endif // End of compiler branching
