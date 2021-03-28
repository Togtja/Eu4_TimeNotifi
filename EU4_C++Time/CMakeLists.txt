
#find_package(spdlog CONFIG REQUIRED)
set(EXEC_NAME EU4_Notifi)
add_executable(${EXEC_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)

add_subdirectory(ImGui)

target_include_directories(
    ${EXEC_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(
    ${EXEC_NAME}
    PRIVATE
    OpenGL::GL
    glfw
    sndfile
    OpenAL
    spdlog::spdlog
)

target_compile_features(
    ${EXEC_NAME}
    PRIVATE
    cxx_std_20
)