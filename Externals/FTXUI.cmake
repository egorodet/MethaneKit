CPMAddPackage(
    NAME FTXUI
    GITHUB_REPOSITORY MethanePowered/FTXUI
    VERSION 5.0.0
    OPTIONS
        "FTXUI_BUILD_DOCS OFF"
        "FTXUI_BUILD_EXAMPLES OFF"
        "FTXUI_BUILD_TESTS OFF"
        "FTXUI_ENABLE_INSTALL OFF"
)

set_target_properties(screen dom component
    PROPERTIES
    FOLDER Externals
)
