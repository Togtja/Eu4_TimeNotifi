set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw-3.3.3)

set(ENABLE_EXTERNAL_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory(libsndfile-1.0.31)

add_subdirectory(openal-soft-1.21.1)