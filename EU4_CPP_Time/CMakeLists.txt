set(EXEC_NAME eu4_notifi)
add_executable(${EXEC_NAME})


include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/external_headers/ImGui/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/external_headers/stb/CMakeLists.txt)

#add_library(injection SHARED ${CMAKE_CURRENT_LIST_DIR}/src/injection.cpp)

target_include_directories(
    ${EXEC_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/external_headers
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

add_custom_command(TARGET ${EXEC_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/Resources/"
        $<TARGET_FILE_DIR:${EXEC_NAME}>/Resources/)

#TODO: Attempt to find the Dll Before we copy
if(WIN32)
    add_custom_command(TARGET ${EXEC_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${PROJECT_BINARY_DIR}/_deps/openal-soft_external-build/OpenAL32.dll"
            $<TARGET_FILE_DIR:${EXEC_NAME}>)
endif()