set(TARGET MethanePlatformApp)

include(MethaneModules)

get_module_dirs("Methane/Platform")

if (WIN32)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppWin.h
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/AppWin.cpp
    )

elseif(APPLE)

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppMac.hh
        ${SOURCES_PLATFORM_DIR}/AppDelegate.hh
        ${SOURCES_PLATFORM_DIR}/WindowDelegate.hh
        ${SOURCES_PLATFORM_DIR}/AppViewController.hh
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/AppMac.mm
        ${SOURCES_PLATFORM_DIR}/AppDelegate.mm
        ${SOURCES_PLATFORM_DIR}/WindowDelegate.mm
        ${SOURCES_PLATFORM_DIR}/AppViewController.mm
    )

    set(PLATFORM_LIBRARIES
        "-framework AppKit"
    )

else() # Linux

    set(PLATFORM_HEADERS
        ${INCLUDE_PLATFORM_DIR}/AppLin.h
    )

    set(PLATFORM_SOURCES
        ${SOURCES_PLATFORM_DIR}/AppLin.cpp
    )

endif()

set(HEADERS ${PLATFORM_HEADERS}
    ${INCLUDE_DIR}/App.h
    ${INCLUDE_DIR}/AppBase.h
    ${INCLUDE_DIR}/AppController.h
)

set(SOURCES ${PLATFORM_SOURCES}
    ${SOURCES_DIR}/AppBase.cpp
    ${SOURCES_DIR}/AppController.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        CLI11
        MethanePrimitives
        MethaneInstrumentation
        MethaneDataTypes
        MethanePlatformAppView
        MethanePlatformInput
    PRIVATE
        MethaneBuildOptions
        MethanePrecompiledHeaders
        MethanePlatformUtils
        TaskFlow
        nowide
        ${PLATFORM_LIBRARIES}
)

target_compile_definitions(${TARGET}
    PRIVATE
    METHANE_RENDER_APP
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

