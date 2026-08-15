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

FILE: Methane/Platform/Utils.cpp
Methane platform utility functions

******************************************************************************/

#include <Methane/Platform/Utils.h>
#include <Methane/Instrumentation.h>

#include <atomic>

namespace Methane::Platform
{

static std::atomic<bool>& GetPrintToConsoleEnabledFlag() noexcept
{
    static std::atomic<bool> s_print_to_console_enabled(false);
    return s_print_to_console_enabled;
}

void SetPrintToConsoleEnabled(bool print_to_console_enabled) noexcept
{
    META_FUNCTION_TASK();
    GetPrintToConsoleEnabledFlag().store(print_to_console_enabled, std::memory_order_relaxed);
}

bool IsPrintToConsoleEnabled() noexcept
{
    return GetPrintToConsoleEnabledFlag().load(std::memory_order_relaxed);
}

inline void SplitInChunks(const std::string_view str, size_t max_chunk_size, std::vector<std::string_view>& output)
{
    if (str.empty())
    {
        output.emplace_back(str);
        return;
    }
    for (size_t i = 0; i < str.size(); i += max_chunk_size)
    {
        output.emplace_back(str.data() + i, std::min(str.size() - i, max_chunk_size));
    }
}

std::vector<std::string_view> SplitString(const std::string_view str, const char delimiter, bool with_empty_parts, size_t max_chunk_size)
{
    META_FUNCTION_TASK();
    std::vector<std::string_view> parts;
    size_t begin_part_index = 0;

    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] != delimiter)
            continue;

        if (!with_empty_parts && begin_part_index == i)
        {
            begin_part_index = i + 1;
            continue;
        }

        SplitInChunks(std::string_view(str.data() + begin_part_index, i - begin_part_index), max_chunk_size, parts);
        begin_part_index = i + 1;
    }

    if (begin_part_index < str.length() - 1)
        SplitInChunks(std::string_view(str.data() + begin_part_index, str.length() - begin_part_index), max_chunk_size, parts);

    return parts;
}

} // namespace Methane::Platform
