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

FILE: Methane/Graphics/Types.cpp
Methane graphics type functions implementation.

******************************************************************************/

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Rect.hpp>
#include <Methane/Graphics/Volume.hpp>

#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>
#include <cstdint>

namespace Methane::Graphics
{

// Scissor rect of a frame rect is an intersection of the [offset, offset + dimension_size) range of that rect
// with the [0, render_attachment_size) range of the render attachment. Both bounds are computed in a signed
// 64-bit type, so that a frame rect moved partially or completely outside of the render attachment results in
// a clipped or empty scissor rect, instead of an unsigned integer underflow of its offset or extent.
// The underflowed extent used to be rejected by the graphics API validation, for example a UI text block
// laid out to the right of a narrow window produced a huge scissor extent close to 2^32.

[[nodiscard]]
inline uint32_t GetNormalizedDimensionOffset(int32_t offset, uint32_t render_attachment_size)
{
    return static_cast<uint32_t>(std::clamp<int64_t>(offset, 0, render_attachment_size));
}

[[nodiscard]]
inline uint32_t GetNormalizedDimensionSize(int32_t offset, uint32_t dimension_size, uint32_t render_attachment_size)
{
    const int64_t range_begin = std::max<int64_t>(offset, 0);
    const int64_t range_end   = std::min<int64_t>(static_cast<int64_t>(offset) + dimension_size, render_attachment_size);
    return range_end > range_begin ? static_cast<uint32_t>(range_end - range_begin) : 0U;
}

ScissorRect GetFrameScissorRect(const FrameRect& frame_rect, const FrameSize& render_attachment_size)
{
    META_FUNCTION_TASK();
    return {
        ScissorRect::Point(GetNormalizedDimensionOffset(frame_rect.origin.GetX(), render_attachment_size.GetWidth()),
                           GetNormalizedDimensionOffset(frame_rect.origin.GetY(), render_attachment_size.GetHeight())),
        ScissorRect::Size(GetNormalizedDimensionSize(frame_rect.origin.GetX(), frame_rect.size.GetWidth(),  render_attachment_size.GetWidth()),
                          GetNormalizedDimensionSize(frame_rect.origin.GetY(), frame_rect.size.GetHeight(), render_attachment_size.GetHeight()))
    };
}

ScissorRect GetFrameScissorRect(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    return ScissorRect {
        ScissorRect::Point(0U, 0U),
        ScissorRect::Size(frame_size.GetWidth(), frame_size.GetHeight())
    };
}

Viewport GetFrameViewport(const FrameRect& frame_rect)
{
    META_FUNCTION_TASK();
    return Viewport{
        Viewport::Point(static_cast<double>(frame_rect.origin.GetX()), static_cast<double>(frame_rect.origin.GetY()), 0.0),
        Viewport::Size(static_cast<double>(frame_rect.size.GetWidth()), static_cast<double>(frame_rect.size.GetHeight()), 1.0)
    };
}

Viewport GetFrameViewport(const FrameSize& frame_size)
{
    META_FUNCTION_TASK();
    return Viewport {
        Viewport::Point(0.0, 0.0, 0.0),
        Viewport::Size(static_cast<double>(frame_size.GetWidth()), static_cast<double>(frame_size.GetHeight()), 1.0)
    };
}

Data::Size GetPixelSize(PixelFormat pixel_format)
{
    META_FUNCTION_TASK();
    switch(pixel_format)
    {
    using enum PixelFormat;
    case RGBA8:
    case RGBA8Unorm:
    case RGBA8Unorm_sRGB:
    case BGRA8Unorm:
    case BGRA8Unorm_sRGB:
    case R32Float:
    case R32Uint:
    case R32Sint:
    case Depth32Float:
        return 4;

    case R16Float:
    case R16Uint:
    case R16Sint:
    case R16Unorm:
    case R16Snorm:
        return 2;

    case R8Uint:
    case R8Sint:
    case R8Unorm:
    case R8Snorm:
    case A8Unorm:
        return 1;

    default:
        META_UNEXPECTED_RETURN(pixel_format, 0);
    }
}

bool IsSrgbColorSpace(PixelFormat pixel_format) noexcept
{
    META_FUNCTION_TASK();
    switch (pixel_format)
    {
    using enum PixelFormat;
    case RGBA8Unorm_sRGB:
    case BGRA8Unorm_sRGB:
        return true;

    default:
        return false;
    }
}

bool IsDepthFormat(PixelFormat pixel_format) noexcept
{
    META_FUNCTION_TASK();
    return pixel_format == PixelFormat::Depth32Float;
}

} // namespace Methane::Graphics
