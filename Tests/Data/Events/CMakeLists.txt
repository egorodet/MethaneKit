include(CTest)
include(Catch)

set(TARGET MethaneDataEventsTest)

set(SOURCES
    Main.cpp
    EventWrappers.hpp
    EventsTest.cpp
)

# Events benchmark is disabled in Debug builds to let them run faster
if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(SOURCES ${SOURCES}
        EventsBenchmark.cpp
    )
endif()

add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET}
    PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:CATCH_CONFIG_ENABLE_BENCHMARKING>
)

target_precompile_headers(${TARGET} REUSE_FROM MethanePrecompiledHeaders)

target_link_libraries(${TARGET}
    PRIVATE
        MethaneDataEvents
        MethaneBuildOptions
        MethanePrecompiledHeaders
        $<$<BOOL:${METHANE_TRACY_PROFILING_ENABLED}>:TracyClient>
        Catch2
)

if (METHANE_RUN_TESTS_DURING_BUILD)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CTEST_EXE} -C $<CONFIGURATION> --output-on-failure
    )
endif()

set_target_properties(${TARGET}
    PROPERTIES
        FOLDER Tests
)

install(TARGETS ${TARGET}
    RUNTIME
        DESTINATION Tests
        COMPONENT Test
)

catch_discover_tests(${TARGET})
