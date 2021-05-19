// C++ Libraries
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
// External Libraries
#include <ImGui/imgui_impl_glfw.h>
#include <ImGui/imgui_impl_opengl3.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>

#include <stb/stb_image.h>

// Own Libaries
#include "Constants.hpp"
#include "CrossMemory.hpp"
#include "NotificationSound.hpp"
#include "Utility.hpp"

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(WINDOWS_SIZE.x, WINDOWS_SIZE.y, "EU4 Time notification", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    NotificationSound sound("Resources/Audio/quest_complete.wav");
    std::atomic_bool play_sound = false;

    if (gladLoadGL() == 0) {
        spdlog::log(spdlog::level::critical, "Failed to initialize OpenGL loader!");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION.data());

    // Load texture
    std::array<EU4_Image, IMAGES_PATH.size()> images;
    for (size_t i = 0; i < IMAGES_PATH.size(); i++) {
        if (!load_texture(IMAGES_PATH[i].data(), images[i])) {
            spdlog::critical("Failed to load {}", IMAGES_PATH[i].data());
            return 1;
        }
    }

    // Pointer to a memory class, that include some nice memory functions
    std::unique_ptr<CrossMemory> mem = nullptr;

    std::vector<const uint8_t*> eu4_date_address{};
    bool foundEu4Memloc = false;
    bool increase_day   = false;

    std::vector<Eu4_Notification> notify_dates{};

    uint32_t nr_threads = MAX_THREADS;
    std::thread worker_thread;
    std::atomic_bool running     = false;
    std::atomic_bool exit_worker = false;

    // Display notification popup
    std::vector<std::string> popup_msg{};

    // The notification date (i.e user set date)
    Eu4Date user_date(11, 11, 1444);
    std::string eu4exe{DEFAULT_EU4_BIN};
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(WINDOWS_SIZE);
            ImGui::Begin("Hello, world!",
                         0,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

            // Current date is always gotten each loop
            Eu4Date current_date(11, 11, 1444); // SO it beginns

            // Find the eu4 Date:
            if (!foundEu4Memloc) {
                if (!increase_day) {
                    ImGui::Text("Eu4 excecutable name (typically eu4.exe):");
                    ImGui::PushItemWidth(200);
                    ImGui::InputText("##eu4 exe", &eu4exe);
                    ImGui::SameLine();
                    imgui_setting(window, images[0], nr_threads);
                    ImGui::Text("Enter the current EU4 Date:");
                }
                else {
                    ImGui::Text("Enter a different EU4 Date:");
                }

                get_user_date(user_date);
                current_date = user_date;

                if (running) {
                    ImGui::Text("Working on finding the date in the game");
                }
                else if (!running && ImGui::Button("Submit", {200, 20})) {
                    if (!mem) {
                        create_memory_class_for_eu4(mem, eu4exe);
                    }
                    if (mem) {
                        // Starts a thread(s) that searches the eu4 memory for a date
                        start_find_eu4date_in_memory(worker_thread,
                                                     nr_threads,
                                                     mem,
                                                     running,
                                                     user_date,
                                                     eu4_date_address,
                                                     foundEu4Memloc,
                                                     increase_day);
                    }
                }
            }
            else {
                update_current_date(mem, eu4_date_address, current_date);
                if (!running) {
                    if (worker_thread.joinable()) {
                        worker_thread.join();
                    }
                    // The worker thread will now always run, until someone say exit, then remember to set the running to false
                    running = true;
                    worker_thread =
                        std::thread(sound_player_thread, std::ref(sound), std::ref(play_sound), std::ref(exit_worker));
                }
            }

            current_date.correct_date();
            display_current_date(current_date);

            if (foundEu4Memloc) {
                add_notification(notify_dates, user_date, current_date);
            }

            manage_notifications(notify_dates, popup_msg, current_date, play_sound);
            notify_popup(popup_msg);

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(CLEAR_COLOR.x * CLEAR_COLOR.w, CLEAR_COLOR.y * CLEAR_COLOR.w, CLEAR_COLOR.z * CLEAR_COLOR.w, CLEAR_COLOR.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    // Join the sound playing worker thread
    exit_worker = true;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    return 0;
}
