CPMAddPackage(
    NAME Tracy
    GITHUB_REPOSITORY MethanePowered/Tracy
    VERSION 0.14.0
    OPTIONS
        "TRACY_STATIC ON"
        "TRACY_ENABLE ${METHANE_TRACY_PROFILING_ENABLED}"
        "TRACY_ON_DEMAND ${METHANE_TRACY_PROFILING_ON_DEMAND}"
)

set_target_properties(TracyClient
    PROPERTIES
        FOLDER Externals
)

if(MSVC)
    target_compile_options(TracyClient
        PUBLIC
            /wd4366 # The result of the unary '&' operator may be unaligned
    )
else()
    target_compile_options(TracyClient
        PRIVATE
            -Wno-unused-result # ignoring return value of 'fscanf' declared with attribute ‘warn_unused_result’
            -Wno-deprecated-declarations # ignore warning: 'sprintf' is deprecated
    )
endif()
