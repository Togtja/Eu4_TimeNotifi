message(STATUS "Could not find libsndfile package so we are getting it from GitHub instead.")

# Declare where to find spdlog and what version to use
FetchContent_Declare(
    openal-soft_external
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG 1.21.1
    GIT_PROGRESS TRUE
)

# Populate it for building
FetchContent_MakeAvailable(openal-soft_external)