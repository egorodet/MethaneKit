set(TARGET MethanePlatformUtils)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if(WIN32)
    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/Utils.h
    )
elseif(APPLE)
    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/Types.hh
        ${INCLUDE_PLATFORM_DIR}/Utils.hh
    )
endif()

set(HEADERS
    ${INCLUDE_DIR}/Utils.h
    ${INCLUDE_DIR}/Logger.h
    ${PLATFORM_HEADERS}
)

set(PLATFORM_SOURCES
    ${SOURCES_PLATFORM_DIR}/Utils.${CPP_EXT}
)

set(SOURCES
    ${SOURCES_DIR}/Utils.cpp
    ${PLATFORM_SOURCES}
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneBuildOptions
        MethaneCommonPrecompiledHeaders
        MethaneInstrumentation
        nowide
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneCommonPrecompiledHeaders)
endif()

if(APPLE)

    target_sources(${TARGET}
        PRIVATE
            ${INCLUDE_PLATFORM_DIR}/Types.hh
    )

    target_link_libraries(${TARGET}
        PUBLIC
            "-framework Foundation"
    )

endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Platform
        PUBLIC_HEADER "${HEADERS}"
)

if (APPLE)
    # Disable precompiled headers on MacOS for Objective-C files:
    set_source_files_properties(
        ${PLATFORM_SOURCES}
        PROPERTIES
            COMPILE_FLAGS -fobjc-arc
            SKIP_PRECOMPILE_HEADERS ON
    )
endif()

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)

