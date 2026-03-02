include(MethaneModules)

CPMAddPackage(
    NAME DirectXShaderCompilerBinary
    GITHUB_REPOSITORY MethanePowered/DirectXShaderCompilerBinary
    GIT_TAG update_dxc_v1-9-2602
    #VERSION 1.9.2602
)

get_platform_arch_dir(PLATFORM_ARCH_DIR CPP_EXT)
if(APPLE)
    set(PLATFORM_ARCH_DIR MacOS)
endif()

set(DXC_BINARY_DIR "${DirectXShaderCompilerBinary_SOURCE_DIR}/binaries/${PLATFORM_ARCH_DIR}/bin" PARENT_SCOPE)
