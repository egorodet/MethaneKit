CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 12.2.0
    VERSION 12.2.0
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)

if(MSVC AND # For VS2022 build disable warnings
   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.30" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.50") 
    target_compile_options(fmt PUBLIC
        /wd4127 # conditional expression is constant 
    )
endif()
