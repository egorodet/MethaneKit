set(TARGET MethaneGraphicsRhiDirectX)

include(MethaneModules)

get_module_dirs("Methane/Graphics/DirectX")

list(APPEND HEADERS
    ${INCLUDE_DIR}/ErrorHandling.h
    ${INCLUDE_DIR}/Types.h
    ${INCLUDE_DIR}/Device.h
    ${INCLUDE_DIR}/System.h
    ${INCLUDE_DIR}/Fence.h
    ${INCLUDE_DIR}/IContext.h
    ${INCLUDE_DIR}/Context.hpp
    ${INCLUDE_DIR}/Shader.h
    ${INCLUDE_DIR}/Program.h
    ${INCLUDE_DIR}/ProgramArgumentBinding.h
    ${INCLUDE_DIR}/ProgramBindings.h
    ${INCLUDE_DIR}/RenderContext.h
    ${INCLUDE_DIR}/ComputeContext.h
    ${INCLUDE_DIR}/RenderState.h
    ${INCLUDE_DIR}/ViewState.h
    ${INCLUDE_DIR}/ComputeState.h
    ${INCLUDE_DIR}/IResource.h
    ${INCLUDE_DIR}/ResourceView.h
    ${INCLUDE_DIR}/Resource.hpp
    ${INCLUDE_DIR}/ResourceBarriers.h
    ${INCLUDE_DIR}/DescriptorManager.h
    ${INCLUDE_DIR}/DescriptorHeap.h
    ${INCLUDE_DIR}/Buffer.h
    ${INCLUDE_DIR}/BufferSet.h
    ${INCLUDE_DIR}/Texture.h
    ${INCLUDE_DIR}/Sampler.h
    ${INCLUDE_DIR}/QueryPool.h
    ${INCLUDE_DIR}/RenderPattern.h
    ${INCLUDE_DIR}/RenderPass.h
    ${INCLUDE_DIR}/CommandQueue.h
    ${INCLUDE_DIR}/CommandListSet.h
    ${INCLUDE_DIR}/CommandListDebugGroup.h
    ${INCLUDE_DIR}/ICommandList.h
    ${INCLUDE_DIR}/CommandList.hpp
    ${INCLUDE_DIR}/TransferCommandList.h
    ${INCLUDE_DIR}/ComputeCommandList.h
    ${INCLUDE_DIR}/RenderCommandList.h
    ${INCLUDE_DIR}/ParallelRenderCommandList.h
)

list(APPEND SOURCES
    ${SOURCES_DIR}/ErrorHandling.cpp
    ${SOURCES_DIR}/Types.cpp
    ${SOURCES_DIR}/Device.cpp
    ${SOURCES_DIR}/System.cpp
    ${SOURCES_DIR}/Fence.cpp
    ${SOURCES_DIR}/Shader.cpp
    ${SOURCES_DIR}/Program.cpp
    ${SOURCES_DIR}/ProgramArgumentBinding.cpp
    ${SOURCES_DIR}/ProgramBindings.cpp
    ${SOURCES_DIR}/RenderContext.cpp
    ${SOURCES_DIR}/RenderState.cpp
    ${SOURCES_DIR}/ViewState.cpp
    ${SOURCES_DIR}/ComputeState.cpp
    ${SOURCES_DIR}/IResource.cpp
    ${SOURCES_DIR}/ResourceView.cpp
    ${SOURCES_DIR}/ResourceBarriers.cpp
    ${SOURCES_DIR}/DescriptorManager.cpp
    ${SOURCES_DIR}/DescriptorHeap.cpp
    ${SOURCES_DIR}/Buffer.cpp
    ${SOURCES_DIR}/BufferSet.cpp
    ${SOURCES_DIR}/Texture.cpp
    ${SOURCES_DIR}/Sampler.cpp
    ${SOURCES_DIR}/QueryPool.cpp
    ${SOURCES_DIR}/RenderPattern.cpp
    ${SOURCES_DIR}/RenderPass.cpp
    ${SOURCES_DIR}/CommandQueue.cpp
    ${SOURCES_DIR}/CommandListSet.cpp
    ${SOURCES_DIR}/CommandListDebugGroup.cpp
    ${SOURCES_DIR}/TransferCommandList.cpp
    ${SOURCES_DIR}/ComputeCommandList.cpp
    ${SOURCES_DIR}/RenderCommandList.cpp
    ${SOURCES_DIR}/ParallelRenderCommandList.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneGraphicsRhiBase
        # DirectX Libs
        DirectX-Headers
    PRIVATE
        MethaneBuildOptions
        MethaneDataPrimitives
        MethanePlatformUtils
        MethaneInstrumentation
        TaskFlow
        nowide
        # DirectX Libs
        DirectXTex
        DirectXCompiler # Implements deprecated functions D3DCompileFromFile, D3DReflect
        d3d12
        dxgi
        dxguid
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    get_target_property(MATH_PRECOMPILE_HEADERS MethaneMathPrecompiledHeaders PRECOMPILE_HEADERS)
    target_precompile_headers(${TARGET}
        PRIVATE
        ${MATH_PRECOMPILE_HEADERS}
        <directx/d3d12.h>
    )
endif()

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        METHANE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}
        FOLDER Modules/Graphics/RHI
        UNITY_BUILD ${METHANE_UNITY_BUILD_ENABLED}
        UNITY_BUILD_BATCH_SIZE 4
)

set_target_properties(${TARGET} PROPERTIES PREREQUISITE_MODULES "DirectXCompiler")
