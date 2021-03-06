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

FILE: Methane/Platform/MacOS/Utils.mm
MacOS platform utility functions.

******************************************************************************/

#include <Methane/Platform/MacOS/Utils.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <stdexcept>
#include <string_view>

namespace Methane::Platform
{

void PrintToDebugOutput(__attribute__((unused)) std::string_view msg)
{
    META_FUNCTION_TASK();
    TracyMessage(msg.data(), msg.size());
}

std::string GetExecutableDir()
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN("");
}

std::string GetExecutableFileName()
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN("");
}

std::string GetResourceDir()
{
    META_FUNCTION_NOT_IMPLEMENTED_RETURN("");
}

} // namespace Methane::Platform
