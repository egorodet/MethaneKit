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

FILE: Methane/Pimpl.h
Methane PIMPL common header.

******************************************************************************/

#pragma once

#include <Methane/Memory.hpp>

#ifdef META_PIMPL_INLINE

#include <Methane/Inline.hpp>

#define META_PIMPL_API META_INLINE

#else // META_PIMPL_INLINE

#define META_PIMPL_API

#endif // META_PIMPL_INLINE

#define META_PIMPL_NO_INLINE

#ifdef _DEBUG
// Comment this define to disable checks of pointer to implementation in all PIMPL methods
#define META_PIMPL_NULL_CHECK_ENABLED
#endif

#ifdef META_PIMPL_NULL_CHECK_ENABLED
#define META_PIMPL_NOEXCEPT
#else
#define META_PIMPL_NOEXCEPT noexcept
#endif

#define META_PIMPL_METHODS_DECLARE_MACRO(Class, API_MACRO) \
    API_MACRO ~Class(); \
    API_MACRO Class(const Class& other); \
    API_MACRO Class(Class&& other) noexcept; \
    API_MACRO Class& operator=(const Class& other); \
    API_MACRO Class& operator=(Class&& other) noexcept

#define META_PIMPL_METHODS_DECLARE(Class) META_PIMPL_METHODS_DECLARE_MACRO(Class, META_PIMPL_API)
#define META_PIMPL_METHODS_DECLARE_NO_INLINE(Class) META_PIMPL_METHODS_DECLARE_MACRO(Class, META_PIMPL_NO_INLINE)

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_MACRO(Class, API_MACRO) \
    API_MACRO Class(); \
    META_PIMPL_METHODS_DECLARE_MACRO(Class, API_MACRO)

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE(Class) META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_MACRO(Class, META_PIMPL_API)
#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_NO_INLINE(Class) META_PIMPL_DEFAULT_CONSTRUCT_METHODS_DECLARE_MACRO(Class, META_PIMPL_NO_INLINE)

#define META_PIMPL_METHODS_COMPARE_DEFAULT(Class) \
    [[nodiscard]] friend auto operator<=>(const Class& left, const Class& right) noexcept = default;

#define META_PIMPL_METHODS_IMPLEMENT(Class) \
    Class::~Class() = default; \
    Class::Class(const Class& other) = default; \
    Class::Class(Class&& other) noexcept = default; \
    Class& Class::operator=(const Class& other) = default; \
    Class& Class::operator=(Class&& other) noexcept = default

#define META_PIMPL_METHODS_COMPARE_INLINE(Class) \
    [[nodiscard]] friend auto operator<=>(const Class& left, const Class& right) noexcept \
    { if (left.IsInitialized() && right.IsInitialized()) \
          return std::addressof(left.GetInterface()) <=> std::addressof(right.GetInterface()); \
       if (!left.IsInitialized() && !right.IsInitialized()) \
           return std::strong_ordering::equal; \
       return left.IsInitialized() ? std::strong_ordering::greater : std::strong_ordering::less; \
    } \
    [[nodiscard]] friend bool operator==(const Class& left, const Class& right) noexcept \
    { return std::is_eq(left <=> right); }

#define META_PIMPL_DEFAULT_CONSTRUCT_METHODS_IMPLEMENT(Class) \
    Class::Class() = default; \
    META_PIMPL_METHODS_IMPLEMENT(Class)

