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

FILE: Methane/Graphics/Metal/Types.hh
Methane graphics types converters to Metal native types.

******************************************************************************/

#pragma once

#include <Methane/Graphics/Types.h>
#include <Methane/Graphics/Color.hpp>
#include <Methane/Graphics/Rect.hpp>

#import <Metal/Metal.h>

#include <vector>

#ifdef APPLE_MACOS

#import <Foundation/Foundation.h>

using NativeRect = NSRect;
#define MakeNativeRect NSMakeRect

#else // APPLE_MACOS

#include <CoreGraphics/CGGeometry.h>
using NativeRect = CGRect;
#define MakeNativeRect CGRectMake

#endif // APPLE_MACOS

namespace Methane::Graphics::Metal
{

class TypeConverter
{
public:
    static MTLIndexType DataFormatToMetalIndexType(PixelFormat data_format);
    static MTLPixelFormat DataFormatToMetalPixelType(PixelFormat data_format);
    static MTLVertexFormat MetalDataTypeToVertexFormat(MTLDataType data_type, bool normalized = false);
    static uint32_t ByteSizeOfDataType(MTLDataType data_type);
    static uint32_t ByteSizeOfVertexFormat(MTLVertexFormat vertex_format);
    static MTLClearColor ColorToMetalClearColor(const Color4F& color) noexcept;
    static NativeRect RectToNS(const FrameRect& rect) noexcept;
    static NativeRect CreateNSRect(const FrameSize& size, const Point2I& origin = Point2I(0, 0)) noexcept;
    static FrameRect RectFromNS(const NativeRect& rect) noexcept;
    static MTLCompareFunction CompareFunctionToMetal(Compare compare_func);

private:
    TypeConverter() = default;
};

} // namespace Methane::Graphics::Metal
