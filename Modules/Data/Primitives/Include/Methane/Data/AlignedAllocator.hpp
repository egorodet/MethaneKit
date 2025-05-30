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

FILE: Methane/Data/AlignedAllocator.hpp
Aligned memory allocator to be used in STL containers, like std::vector.

******************************************************************************/

#pragma once

#include <stdlib.h>
#include <exception>
#include <string>

#ifdef WIN32
#include <malloc.h>
#endif

namespace Methane::Data
{

template <typename T, size_t N = 16>
class AlignedAllocator
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type =  std::ptrdiff_t;

    using pointer = T*;
    using const_pointer = const T*;

    using reference = T&;
    using const_reference = const T&;
    
    template <typename T2>
    struct rebind
    {
        using other = AlignedAllocator<T2, N>;
    };

    AlignedAllocator() = default;

    template <typename T2>
    explicit AlignedAllocator(const AlignedAllocator<T2, N>&) noexcept { }

    static pointer address(reference r)             { return &r; }
    static const_pointer address(const_reference r) { return &r; }

    static pointer allocate(size_type n)
    {
        const size_t allocate_size = n * sizeof(value_type);
#ifdef WIN32
        return static_cast<pointer>(_aligned_malloc(allocate_size, N));
#else
        void* memory_ptr = nullptr;
        if (posix_memalign(&memory_ptr, N, allocate_size))
            throw std::bad_alloc();
        return static_cast<pointer>(memory_ptr);
#endif
    }

    void deallocate(pointer p, size_type) const
    {
#ifdef WIN32
        _aligned_free(p); // NOSONAR
#else
        free(p); // NOSONAR
#endif
    }

    void construct(pointer p, const value_type& value) const
    {
        new(p) value_type(value); // NOSONAR
    }

    void destroy(pointer p) const
    {
        p->~value_type(); // NOSONAR
    }

    [[nodiscard]] size_type max_size() const noexcept
    {
        return size_type(-1) / sizeof(value_type);
    }

    friend bool operator==(const AlignedAllocator&, const AlignedAllocator&) noexcept = default;
};

} // namespace Methane::Data
