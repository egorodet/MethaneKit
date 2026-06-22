CPMAddPackage(
    NAME FMT
    GITHUB_REPOSITORY MethanePowered/FMT
    GIT_TAG 12.2.0
    VERSION 12.2.0
)

set_target_properties(fmt
    PROPERTIES
    FOLDER Externals
)
