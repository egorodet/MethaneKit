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

FILE: Methane/Graphics/Metal/Texture.mm
Metal implementation of the texture interface.

******************************************************************************/

#include <Methane/Graphics/Metal/Texture.hh>
#include <Methane/Graphics/Metal/RenderContext.hh>
#include <Methane/Graphics/Metal/TransferCommandList.hh>
#include <Methane/Graphics/Metal/Types.hh>

#include <Methane/Graphics/RHI/ICommandKit.h>
#include <Methane/Platform/Apple/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

#include <algorithm>

namespace Methane::Graphics::Metal
{

static MTLTextureType GetNativeTextureType(Rhi::TextureDimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    using enum Rhi::TextureDimensionType;
    case Tex1D:             return MTLTextureType1D;
    case Tex1DArray:        return MTLTextureType1DArray;
    case Tex2D:             return MTLTextureType2D;
    case Tex2DArray:        return MTLTextureType2DArray;
    case Tex2DMultisample:  return MTLTextureType2DMultisample;
    // TODO: add support for MTLTextureType2DMultisampleArray
    case Cube:              return MTLTextureTypeCube;
    case CubeArray:         return MTLTextureTypeCubeArray;
    case Tex3D:             return MTLTextureType3D;
    // TODO: add support for MTLTextureTypeTextureBuffer
    default: META_UNEXPECTED_RETURN(dimension_type, MTLTextureType1D);
    }
}

static MTLRegion GetTextureRegion(const Dimensions& dimensions, Rhi::TextureDimensionType dimension_type)
{
    META_FUNCTION_TASK();
    switch(dimension_type)
    {
    using enum Rhi::TextureDimensionType;
    case Tex1D:
    case Tex1DArray:
             return MTLRegionMake1D(0, dimensions.GetWidth());
    case Tex2D:
    case Tex2DArray:
    case Tex2DMultisample:
    case Cube:
    case CubeArray:
             return MTLRegionMake2D(0, 0, dimensions.GetWidth(), dimensions.GetHeight());
    case Tex3D:
             return MTLRegionMake3D(0, 0, 0, dimensions.GetWidth(), dimensions.GetHeight(), dimensions.GetDepth());
    default: META_UNEXPECTED_RETURN(dimension_type, MTLRegion{});
    }
}

Texture::Texture(const Base::Context& context, const Settings& settings)
    : Resource(context, settings)
    , m_mtl_texture(settings.type == ITexture::Type::FrameBuffer
                      ? nil // actual frame buffer texture descriptor is set in UpdateFrameBuffer()
                      : [GetMetalContext().GetMetalDevice().GetNativeDevice()  newTextureWithDescriptor:GetNativeTextureDescriptor()])
{
    META_FUNCTION_TASK();
    SetNativeResourceUsage(ConvertResourceUsageMaskToMetal(settings.usage_mask));
}

bool Texture::SetName(std::string_view name)
{
    META_FUNCTION_TASK();
    if (!Resource::SetName(name))
        return false;

    m_mtl_texture.label = [[NSString alloc] initWithUTF8String:name.data()];
    return true;
}

void Texture::SetData(Rhi::ICommandQueue& target_cmd_queue, const SubResources& sub_resources)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_mtl_texture);
    META_CHECK_EQUAL(m_mtl_texture.storageMode, MTLStorageModePrivate);

    Base::Texture::SetData(target_cmd_queue, sub_resources);

    TransferCommandList& transfer_command_list = dynamic_cast<TransferCommandList&>(GetBaseContext().GetUploadCommandKit().GetListForEncoding());
    transfer_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_NOT_NULL(mtl_blit_encoder);

    const Settings& settings        = GetSettings();
    const uint32_t  bytes_per_row   = settings.dimensions.GetWidth()  * GetPixelSize(settings.pixel_format);
    const uint32_t  bytes_per_image = settings.dimensions.GetHeight() * bytes_per_row;
    const MTLRegion texture_region  = GetTextureRegion(settings.dimensions, settings.dimension_type);

    for(const SubResource& sub_resource : sub_resources)
    {
        uint32_t slice = 0;
        switch(settings.dimension_type)
        {
            using enum Rhi::TextureDimensionType;
            case Tex1DArray:
            case Tex2DArray:
                slice = sub_resource.GetIndex().GetArrayIndex();
                break;
            case Cube:
                slice = sub_resource.GetIndex().GetDepthSlice();
                break;
            case CubeArray:
                slice = sub_resource.GetIndex().GetDepthSlice() + sub_resource.GetIndex().GetArrayIndex() * 6;
                break;
            default:
                slice = 0;
        }

        [mtl_blit_encoder copyFromBuffer:GetUploadSubresourceBuffer(sub_resource, GetSubresourceCount())
                            sourceOffset:0
                       sourceBytesPerRow:bytes_per_row
                     sourceBytesPerImage:bytes_per_image
                              sourceSize:texture_region.size
                               toTexture:m_mtl_texture
                        destinationSlice:slice
                        destinationLevel:sub_resource.GetIndex().GetMipLevel()
                       destinationOrigin:texture_region.origin];
    }

    if (settings.mipmapped && sub_resources.size() < GetSubresourceCount().GetRawCount())
    {
        GenerateMipLevels(transfer_command_list);
    }

    GetBaseContext().RequestDeferredAction(Rhi::IContext::DeferredAction::UploadResources);
}

Rhi::SubResource Texture::GetData(Rhi::ICommandQueue&, const SubResource::Index& sub_resource_index, const BytesRangeOpt& data_range)
{
    META_FUNCTION_TASK();
    META_CHECK_NOT_NULL(m_mtl_texture);
    META_CHECK_EQUAL(m_mtl_texture.storageMode, MTLStorageModePrivate);

    ValidateSubResource(sub_resource_index, data_range);

    TransferCommandList& transfer_command_list = dynamic_cast<TransferCommandList&>(GetBaseContext().GetUploadCommandKit().GetListForEncoding());
    transfer_command_list.RetainResource(*this);

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_NOT_NULL(mtl_blit_encoder);

    const Settings& settings        = GetSettings();
    const uint32_t  bytes_per_row   = settings.dimensions.GetWidth()  * GetPixelSize(settings.pixel_format);
    const uint32_t  bytes_per_image = settings.dimensions.GetHeight() * bytes_per_row;
    const MTLRegion texture_region  = GetTextureRegion(settings.dimensions, settings.dimension_type);

    const id<MTLBuffer> mtl_read_back_buffer = GetReadBackBuffer(bytes_per_image);
    [mtl_blit_encoder copyFromTexture: m_mtl_texture
                          sourceSlice: sub_resource_index.GetDepthSlice()
                          sourceLevel: sub_resource_index.GetMipLevel()
                         sourceOrigin: texture_region.origin
                           sourceSize: texture_region.size
                             toBuffer: mtl_read_back_buffer
                    destinationOffset: 0U
               destinationBytesPerRow: bytes_per_row
             destinationBytesPerImage: bytes_per_image];

    GetBaseContext().UploadResources();

    auto* data_ptr = reinterpret_cast<std::byte*>([mtl_read_back_buffer contents]);
    Data::Size data_size = static_cast<Data::Size>([mtl_read_back_buffer length]);
    if (data_range.has_value())
    {
        META_CHECK_LESS_DESCR(data_range->GetEnd(), data_size, "provided texture subresource data range is out of bounds");
        data_ptr += data_range->GetStart();
        data_size = data_range->GetLength();
    }

    return Rhi::SubResource(Data::Bytes(data_ptr, data_ptr + data_size), sub_resource_index, data_range);
}

void Texture::UpdateFrameBuffer()
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL_DESCR(GetSettings().type, Rhi::TextureType::FrameBuffer, "unable to update frame buffer on non-FB texture");
    m_mtl_texture = [GetMetalRenderContext().GetNativeDrawable() texture];
}

void Texture::GenerateMipLevels(TransferCommandList& transfer_command_list)
{
    META_FUNCTION_TASK();
    META_DEBUG_GROUP_CREATE_VAR(s_debug_group, "Texture MIPs Generation");
    transfer_command_list.Reset(s_debug_group.get());

    const id<MTLBlitCommandEncoder>& mtl_blit_encoder = transfer_command_list.GetNativeCommandEncoder();
    META_CHECK_NOT_NULL(mtl_blit_encoder);
    META_CHECK_NOT_NULL(m_mtl_texture);

    [mtl_blit_encoder generateMipmapsForTexture: m_mtl_texture];
}

const RenderContext& Texture::GetMetalRenderContext() const
{
    META_FUNCTION_TASK();
    META_CHECK_EQUAL_DESCR(GetBaseContext().GetType(), Rhi::ContextType::Render, "incompatible context type");
    return static_cast<const RenderContext&>(GetMetalContext());
}

MTLTextureUsage Texture::GetNativeTextureUsage()
{
    META_FUNCTION_TASK();
    NSUInteger texture_usage = MTLTextureUsageUnknown;
    const Settings& settings = GetSettings();
    
    if (settings.usage_mask.HasAnyBit(Usage::ShaderRead))
        texture_usage |= MTLTextureUsageShaderRead;
    
    if (settings.usage_mask.HasAnyBit(Usage::ShaderWrite))
        texture_usage |= MTLTextureUsageShaderWrite;
    
    if (settings.usage_mask.HasAnyBit(Usage::RenderTarget))
        texture_usage |= MTLTextureUsageRenderTarget;

    return texture_usage;
}

MTLTextureDescriptor* Texture::GetNativeTextureDescriptor()
{
    META_FUNCTION_TASK();

    const Settings& settings = GetSettings();
    const MTLPixelFormat mtl_pixel_format = TypeConverter::DataFormatToMetalPixelType(settings.pixel_format);
    const BOOL is_tex_mipmapped = MacOS::ConvertToNsBool(settings.mipmapped);

    MTLTextureDescriptor* mtl_tex_desc = nil;
    switch(settings.dimension_type)
    {
    using enum Rhi::TextureDimensionType;
    case Tex2D:
        mtl_tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_pixel_format
                                                                          width:settings.dimensions.GetWidth()
                                                                         height:settings.dimensions.GetHeight()
                                                                      mipmapped:is_tex_mipmapped];
        break;

    case Cube:
        mtl_tex_desc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:mtl_pixel_format
                                                                             size:settings.dimensions.GetWidth()
                                                                        mipmapped:is_tex_mipmapped];
        break;

    case Tex1D:
    case Tex1DArray:
    case Tex2DArray:
    case Tex2DMultisample:
    case CubeArray:
    case Tex3D:
        mtl_tex_desc                    = [[MTLTextureDescriptor alloc] init];
        mtl_tex_desc.pixelFormat        = mtl_pixel_format;
        mtl_tex_desc.textureType        = GetNativeTextureType(settings.dimension_type);
        mtl_tex_desc.width              = settings.dimensions.GetWidth();
        mtl_tex_desc.height             = settings.dimensions.GetHeight();
        mtl_tex_desc.depth              = settings.dimension_type == Rhi::TextureDimensionType::Tex3D
                                        ? settings.dimensions.GetDepth()
                                        : 1U;
        mtl_tex_desc.arrayLength        = settings.array_length;
        mtl_tex_desc.mipmapLevelCount   = GetSubresourceCount().GetMipLevelsCount();
        break;

    default: META_UNEXPECTED(settings.dimension_type);
    }

    if (!mtl_tex_desc)
        return nil;

    mtl_tex_desc.resourceOptions = MTLResourceStorageModePrivate;
    mtl_tex_desc.usage = GetNativeTextureUsage();

    return mtl_tex_desc;
}

} // namespace Methane::Graphics::Metal
