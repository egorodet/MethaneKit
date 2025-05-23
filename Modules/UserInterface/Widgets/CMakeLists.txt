set(TARGET MethaneUserInterfaceWidgets)

include(MethaneResources)

get_module_dirs("Methane/UserInterface")

set(HEADERS
    ${INCLUDE_DIR}/Widgets.h
    ${INCLUDE_DIR}/Badge.h
    ${INCLUDE_DIR}/Panel.h
    ${INCLUDE_DIR}/TextItem.h
    ${INCLUDE_DIR}/HeadsUpDisplay.h
)

set(SOURCES
    ${SOURCES_DIR}/Badge.cpp
    ${SOURCES_DIR}/Panel.cpp
    ${SOURCES_DIR}/TextItem.cpp
    ${SOURCES_DIR}/HeadsUpDisplay.cpp
)

add_library(${TARGET} STATIC
    ${HEADERS}
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        MethaneUserInterfaceTypes
        MethaneUserInterfaceTypography
        MethaneGraphicsPrimitives
        MethanePlatformInputKeyboard
    PRIVATE
        MethaneBuildOptions
        MethaneMathPrecompiledHeaders
        MethaneInstrumentation
        magic_enum
)

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
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
        FOLDER Modules/UserInterface
        PUBLIC_HEADER "${HEADERS}"
)

install(TARGETS ${TARGET}
    PUBLIC_HEADER
        DESTINATION ${INSTALL_INCLUDE_DIR}
        COMPONENT Development
    ARCHIVE
        DESTINATION lib
        COMPONENT Development
)


