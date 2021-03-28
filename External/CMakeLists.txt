include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Include All Dependencies
include(glfw.cmake)
include(openal-soft.cmake)
include(libsndfile.cmake)
include(spdlog.cmake)
# Set FC to be disconnected so we stop downloading
set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "This has been reset to ON (set to OFF to download again)" FORCE)

add_subdirectory(glad)