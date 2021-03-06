set(TARGET MethaneGraphicsCore)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

if (WIN32)

    set(PLATFORM_SOURCES
        ${SOURCES_GRAPHICS_DIR}/TypesDX.h
        ${SOURCES_GRAPHICS_DIR}/TypesDX.cpp
        ${SOURCES_GRAPHICS_DIR}/DeviceDX.h
        ${SOURCES_GRAPHICS_DIR}/DeviceDX.cpp
        ${SOURCES_GRAPHICS_DIR}/FenceDX.h
        ${SOURCES_GRAPHICS_DIR}/FenceDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ContextDX.h
        ${SOURCES_GRAPHICS_DIR}/ContextDX.hpp
        ${SOURCES_GRAPHICS_DIR}/ShaderDX.h
        ${SOURCES_GRAPHICS_DIR}/ShaderDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramDX.h
        ${SOURCES_GRAPHICS_DIR}/ProgramDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsDX.h
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderContextDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderStateDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderStateDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceDX.h
        ${SOURCES_GRAPHICS_DIR}/ResourceDX.hpp
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersDX.h
        ${SOURCES_GRAPHICS_DIR}/ResourceBarriersDX.cpp
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapDX.h
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapDX.cpp
        ${SOURCES_GRAPHICS_DIR}/QueryBufferDX.h
        ${SOURCES_GRAPHICS_DIR}/QueryBufferDX.cpp
        ${SOURCES_GRAPHICS_DIR}/BufferDX.h
        ${SOURCES_GRAPHICS_DIR}/BufferDX.cpp
        ${SOURCES_GRAPHICS_DIR}/TextureDX.h
        ${SOURCES_GRAPHICS_DIR}/TextureDX.cpp
        ${SOURCES_GRAPHICS_DIR}/SamplerDX.h
        ${SOURCES_GRAPHICS_DIR}/SamplerDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderPassDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderPassDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandQueueDX.h
        ${SOURCES_GRAPHICS_DIR}/CommandQueueDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListDX.hpp
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListDX.cpp
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListDX.h
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListDX.cpp
    )

    set(PLATFORM_LIBRARIES
        D3DX12
        DirectXTex
        D3DCompilerToDXC
        d3d12
        dxgi
        dxguid
        Shcore
    )

elseif(APPLE)

    set(PLATFORM_SOURCES
        ${SOURCES_GRAPHICS_DIR}/TypesMT.hh
        ${SOURCES_GRAPHICS_DIR}/TypesMT.mm
        ${SOURCES_GRAPHICS_DIR}/DeviceMT.hh
        ${SOURCES_GRAPHICS_DIR}/DeviceMT.mm
        ${SOURCES_GRAPHICS_DIR}/FenceMT.hh
        ${SOURCES_GRAPHICS_DIR}/FenceMT.mm
        ${SOURCES_GRAPHICS_DIR}/ContextMT.h
        ${SOURCES_GRAPHICS_DIR}/ContextMT.hpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderContextMT.mm
        ${SOURCES_GRAPHICS_DIR}/ShaderMT.hh
        ${SOURCES_GRAPHICS_DIR}/ShaderMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramLibraryMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramLibraryMT.mm
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsMT.hh
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderStateMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderStateMT.mm
        ${SOURCES_GRAPHICS_DIR}/ResourceMT.hh
        ${SOURCES_GRAPHICS_DIR}/ResourceMT.mm
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapMT.hh
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapMT.mm
        ${SOURCES_GRAPHICS_DIR}/BufferMT.hh
        ${SOURCES_GRAPHICS_DIR}/BufferMT.mm
        ${SOURCES_GRAPHICS_DIR}/TextureMT.hh
        ${SOURCES_GRAPHICS_DIR}/TextureMT.mm
        ${SOURCES_GRAPHICS_DIR}/SamplerMT.hh
        ${SOURCES_GRAPHICS_DIR}/SamplerMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderPassMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderPassMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandQueueMT.hh
        ${SOURCES_GRAPHICS_DIR}/CommandQueueMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/CommandListMT.hpp
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListMT.mm
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListMT.hh
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListMT.mm
    )

    set(PLATFORM_LIBRARIES
        "-framework Metal"
    )

    # CLANG_ENABLE_OBJC_WEAK = YES
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            COMPILE_FLAGS -fobjc-weak
    )

else() # Linux

    set(PLATFORM_SOURCES
        ${SOURCES_GRAPHICS_DIR}/TypesVK.h
        ${SOURCES_GRAPHICS_DIR}/TypesVK.cpp
        ${SOURCES_GRAPHICS_DIR}/DeviceVK.h
        ${SOURCES_GRAPHICS_DIR}/DeviceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/FenceVK.h
        ${SOURCES_GRAPHICS_DIR}/FenceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ContextVK.h
        ${SOURCES_GRAPHICS_DIR}/ContextVK.hpp
        ${SOURCES_GRAPHICS_DIR}/ShaderVK.h
        ${SOURCES_GRAPHICS_DIR}/ShaderVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramVK.h
        ${SOURCES_GRAPHICS_DIR}/ProgramVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsVK.h
        ${SOURCES_GRAPHICS_DIR}/ProgramBindingsVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderContextVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderContextVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderStateVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderStateVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ResourceVK.h
        ${SOURCES_GRAPHICS_DIR}/ResourceVK.cpp
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapVK.h
        ${SOURCES_GRAPHICS_DIR}/DescriptorHeapVK.cpp  
        ${SOURCES_GRAPHICS_DIR}/BufferVK.h
        ${SOURCES_GRAPHICS_DIR}/BufferVK.cpp
        ${SOURCES_GRAPHICS_DIR}/TextureVK.h
        ${SOURCES_GRAPHICS_DIR}/TextureVK.cpp
        ${SOURCES_GRAPHICS_DIR}/SamplerVK.h
        ${SOURCES_GRAPHICS_DIR}/SamplerVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderPassVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderPassVK.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandQueueVK.h
        ${SOURCES_GRAPHICS_DIR}/CommandQueueVK.cpp
        ${SOURCES_GRAPHICS_DIR}/CommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/CommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/BlitCommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/RenderCommandListVK.cpp
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListVK.h
        ${SOURCES_GRAPHICS_DIR}/ParallelRenderCommandListVK.cpp
    )

endif()

set(HEADERS
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/Context.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/ProgramBindings.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/Resource.h
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/SubResource.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/CommandKit.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandList.h
    ${INCLUDE_DIR}/BlitCommandList.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
)

set(SOURCES ${PLATFORM_SOURCES}

    # Native alias headers
    ${SOURCES_DIR}/Native/ContextNT.h

    # Base implementation
    ${SOURCES_DIR}/ObjectBase.h
    ${SOURCES_DIR}/ObjectBase.cpp
    ${SOURCES_DIR}/DeviceBase.h
    ${SOURCES_DIR}/DeviceBase.cpp
    ${SOURCES_DIR}/ContextBase.h
    ${SOURCES_DIR}/ContextBase.cpp
    ${SOURCES_DIR}/RenderContextBase.h
    ${SOURCES_DIR}/RenderContextBase.cpp
    ${SOURCES_DIR}/FenceBase.h
    ${SOURCES_DIR}/FenceBase.cpp
    ${SOURCES_DIR}/ShaderBase.h
    ${SOURCES_DIR}/ShaderBase.cpp
    ${SOURCES_DIR}/ProgramBase.h
    ${SOURCES_DIR}/ProgramBase.cpp
    ${SOURCES_DIR}/ProgramBindingsBase.h
    ${SOURCES_DIR}/ProgramBindingsBase.cpp
    ${SOURCES_DIR}/RenderStateBase.h
    ${SOURCES_DIR}/RenderStateBase.cpp
    ${SOURCES_DIR}/SubResource.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/ResourceBase.h
    ${SOURCES_DIR}/ResourceBase.cpp
    ${SOURCES_DIR}/BufferBase.h
    ${SOURCES_DIR}/BufferBase.cpp
    ${SOURCES_DIR}/TextureBase.h
    ${SOURCES_DIR}/TextureBase.cpp
    ${SOURCES_DIR}/SamplerBase.h
    ${SOURCES_DIR}/SamplerBase.cpp
    ${SOURCES_DIR}/RenderPassBase.h
    ${SOURCES_DIR}/RenderPassBase.cpp
    ${SOURCES_DIR}/CommandKitBase.h
    ${SOURCES_DIR}/CommandKitBase.cpp
    ${SOURCES_DIR}/CommandQueueBase.h
    ${SOURCES_DIR}/CommandQueueBase.cpp
    ${SOURCES_DIR}/CommandListBase.h
    ${SOURCES_DIR}/CommandListBase.cpp
    ${SOURCES_DIR}/RenderCommandListBase.h
    ${SOURCES_DIR}/RenderCommandListBase.cpp
    ${SOURCES_DIR}/ParallelRenderCommandListBase.h
    ${SOURCES_DIR}/ParallelRenderCommandListBase.cpp

    # Other sources
    ${SOURCES_DIR}/DescriptorHeap.h
    ${SOURCES_DIR}/DescriptorHeap.cpp
    ${SOURCES_DIR}/ResourceManager.h
    ${SOURCES_DIR}/ResourceManager.cpp
    ${SOURCES_DIR}/QueryBuffer.h
    ${SOURCES_DIR}/QueryBuffer.cpp
    ${SOURCES_DIR}/CoreFormatters.hpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethanePrimitives
        MethaneDataProvider
        MethaneDataRangeSet
        MethaneDataEvents
        MethaneGraphicsTypes
        MethanePlatformAppView
    PRIVATE
        MethaneBuildOptions
        MethaneDataPrimitives
        MethaneGraphicsPrimitives
        MethanePlatformUtils
        MethaneInstrumentation
        MethanePrecompiledExtraHeaders
        TaskFlow
        nowide
        magic_enum
        ${PLATFORM_LIBRARIES}
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledExtraHeaders)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_compile_definitions(${TARGET}
    PUBLIC
        $<$<BOOL:${METHANE_COMMAND_DEBUG_GROUPS_ENABLED}>:METHANE_COMMAND_DEBUG_GROUPS_ENABLED>
    PRIVATE
        $<$<BOOL:${METHANE_GPU_INSTRUMENTATION_ENABLED}>:METHANE_GPU_INSTRUMENTATION_ENABLED>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
        PUBLIC_HEADER "${HEADERS}"
        PREREQUISITE_MODULES "DirectXCompiler"
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 6
)

if (APPLE)
    # Disable precompiled headers on MacOS for Objective-C files:
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION Lib
        COMPONENT Development
)
