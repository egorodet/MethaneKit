if(APPLE_IOS OR APPLE_TVOS)
    # Console applications are not supported on iOS / tvOS
    return()
endif()

set(TARGET MethaneConsoleCompute)

include(MethaneShaders)
include(MethaneModules)

add_executable(${TARGET}
    ConsoleApp.h
    ConsoleApp.cpp
    ConsoleComputeApp.h
    ConsoleComputeApp.cpp
    Shaders/GameOfLifeRules.h
)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneKit
        ftxui::component # FTXUI
)

add_methane_shaders_source(
    TARGET ${TARGET}
    SOURCE Shaders/GameOfLife.hlsl
    VERSION 6_0
    TYPES
        comp=MainCS
)

add_methane_shaders_library(${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Apps
)

install(
    TARGETS ${TARGET}
    RUNTIME
    DESTINATION Apps
    COMPONENT Runtime
)

get_target_property(METHANE_PREREQUISITE_MODULES MethaneKit PREREQUISITE_MODULES)
add_prerequisite_binaries(${TARGET} "${METHANE_PREREQUISITE_MODULES}" "$<TARGET_FILE_DIR:${TARGET}>" TRUE)
