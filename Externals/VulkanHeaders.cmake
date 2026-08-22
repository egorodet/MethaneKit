if(TARGET Vulkan::Headers)
    # Vulkan::Headers target can be added by find_package(Vulkan) when VULKAN_SDK environment variable defines valid path
    return()
endif()

CPMAddPackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY MethanePowered/VulkanHeaders
    GIT_TAG vulkan-sdk-1.4.357.0
    VERSION 1.4.357.0
)
