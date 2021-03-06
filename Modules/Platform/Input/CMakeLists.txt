set(TARGET MethanePlatformInput)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if(APPLE)

    set(PLATFORM_LIBRARIES
        "-framework AppKit"
    )

endif()

if (WIN32)

    set(PLATFORM_HEADERS
        ${INCLUDE_DIR}/Windows/Keyboard.h
    )
    set(PLATFORM_SOURCES
        ${SOURCES_DIR}/Windows/Keyboard.cpp
    )

endif()

if (APPLE)

    set(PLATFORM_SOURCES
        ${SOURCES_DIR}/MacOS/Keyboard.mm
    )

endif()

if (UNIX) # Linux & MacOS

    set(PLATFORM_HEADERS
        ${INCLUDE_DIR}/Unix/Keyboard.h
    )
    set(PLATFORM_SOURCES ${PLATFORM_SOURCES}
        ${SOURCES_DIR}/Unix/Keyboard.cpp
    )

endif()

set(HEADERS
    ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/Keyboard.h
    ${INCLUDE_DIR}/Mouse.h
    ${INCLUDE_DIR}/KeyboardActionControllerBase.hpp
    ${INCLUDE_DIR}/MouseActionControllerBase.hpp
    ${INCLUDE_DIR}/Input/HelpProvider.h
    ${INCLUDE_DIR}/Input/Controller.h
    ${INCLUDE_DIR}/Input/ControllersPool.h
    ${INCLUDE_DIR}/Input/State.h
    )

set(SOURCES
    ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/Keyboard.cpp
    ${SOURCES_DIR}/Mouse.cpp
    ${SOURCES_DIR}/Input/ControllersPool.cpp
    ${SOURCES_DIR}/Input/State.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneDataTypes
        MethaneInstrumentation
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledHeaders
        MethanePrimitives
        MethanePlatformUtils
        magic_enum
        ${PLATFORM_LIBRARIES}
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_include_directories(${TARGET}
    PRIVATE
        Sources
    PUBLIC
        Include
)

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
