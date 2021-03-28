set(EXEC_NAME EU4_Notifi)
add_executable(${EXEC_NAME})

include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/ImGui/CMakeLists.txt)

target_include_directories(
    ${EXEC_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/ImGui
)
target_link_libraries(
    ${EXEC_NAME}
    PRIVATE
    OpenGL::GL
    glfw
    sndfile
    OpenAL
    spdlog::spdlog
    glad
)

target_compile_features(
    ${EXEC_NAME}
    PRIVATE
    cxx_std_20
)