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

FILE: Methane/Data/Math.hpp
Math primitive functions.

******************************************************************************/

#pragma once

#include <type_traits>
#include <cstdlib>
#include <cmath>
#include <numbers>

#include <Methane/Checks.hpp>

namespace Methane::Data
{

template<typename T, typename V> requires std::is_arithmetic_v<T> && std::is_arithmetic_v<V>
constexpr T RoundCast(V value) noexcept
{
    if constexpr (std::is_same_v<T, V>)
        return value;
    else
    {
        if constexpr (std::is_integral_v<T> && std::is_floating_point_v<V>)
            return static_cast<T>(std::round(value));
        else
            return static_cast<T>(value);
    }
}

template<typename T>
requires(std::is_arithmetic_v<T>)
constexpr bool IsPowerOfTwo(T value) noexcept
{
    return value > 0 && (value & (value - 1)) == 0;
}

template<typename T>
requires(std::is_arithmetic_v<T>)
constexpr T AlignUp(T value, T alignment)
{
    META_CHECK_TRUE_DESCR(IsPowerOfTwo(alignment), "alignment {} must be a power of two", alignment);
    return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T>
requires(std::is_arithmetic_v<T>)
constexpr T AbsSubtract(T a, T b) noexcept
{
    return a >= b ? a - b : b - a;
}

template<typename T>
requires(std::is_arithmetic_v<T>)
constexpr T DivCeil(T numerator, T denominator) noexcept
{
    if constexpr (std::is_signed_v<T>)
    {
        std::div_t res = std::div(static_cast<int32_t>(numerator), static_cast<int32_t>(denominator));
        if (res.rem)
            return res.quot >= 0 ? (res.quot + 1) : (res.quot - 1);

        return res.quot;
    }
    else
    {
        return numerator > 0 ? (1 + (numerator - 1) / denominator) : 0;
    }
}

template<std::floating_point T>
[[nodiscard]] constexpr T DegreeToRadians(T degrees) noexcept
{
    return degrees * std::numbers::pi_v<T> / T(180.);
}

template<std::floating_point T>
[[nodiscard]] constexpr T RadiansToDegrees(T degrees) noexcept
{
    return degrees * T(180.) / std::numbers::pi_v<T>;
}

} // namespace Methane::Data
