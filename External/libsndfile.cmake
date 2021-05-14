message(STATUS "Could not find libsndfile package so we are getting it from GitHub instead.")

# Declare where to find libsndfile and what version to use
FetchContent_Declare(
    libsndfile_external
    GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
    GIT_TAG 1.0.31
    GIT_PROGRESS TRUE
)

# Populate it for building
FetchContent_MakeAvailable(libsndfile_external)