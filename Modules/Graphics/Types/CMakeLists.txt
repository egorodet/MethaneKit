set(TARGET MethaneGraphicsTypes)

include(MethaneModules)

get_module_dirs("Methane/Graphics")

set(HEADERS
    ${INCLUDE_DIR}/Types.h
    ${INCLUDE_DIR}/TypeConverters.hpp
    ${INCLUDE_DIR}/TypeTraits.hpp
    ${INCLUDE_DIR}/Point.hpp
    ${INCLUDE_DIR}/Rect.hpp
    ${INCLUDE_DIR}/Volume.hpp
    ${INCLUDE_DIR}/Color.hpp
)

set(SOURCES
    ${SOURCES_DIR}/Types.cpp
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

if(METHANE_PRECOMPILED_HEADERS_ENABLED)
    target_precompile_headers(${TARGET} REUSE_FROM MethaneMathPrecompiledHeaders)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        MethaneDataTypes
        HLSLpp
    PRIVATE
        MethaneBuildOptions
        MethaneInstrumentation
        MethaneMathPrecompiledHeaders
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES  ${HEADERS} ${SOURCES})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Modules/Graphics
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

