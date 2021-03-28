add_library(glad include/glad/glad.h src/glad.c)

message(status ${CMAKE_CURRENT_LIST_DIR}/include)
# Set include directories
target_include_directories(
    glad
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)