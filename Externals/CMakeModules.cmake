CPMAddPackage(
    NAME CMakeModules
    GITHUB_REPOSITORY MethanePowered/CMakeModules
    GIT_TAG 471d10b21f836b8a59119b60b28aae67e2a51b8e # last commit from 'methane' branch
    DOWNLOAD_ONLY YES
)

list(APPEND CMAKE_MODULE_PATH "${CMakeModules_SOURCE_DIR}")