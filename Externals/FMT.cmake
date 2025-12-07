CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 12.1.0
    VERSION 12.1.0
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)
