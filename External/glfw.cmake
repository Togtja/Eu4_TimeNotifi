message(STATUS "Could not find glfw package so we are getting it from GitHub instead.")

# Declare where to find glfw and what version to use
FetchContent_Declare(
    glfw_external
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.3
    GIT_PROGRESS TRUE
)

# Populate it for building
FetchContent_MakeAvailable(glfw_external)