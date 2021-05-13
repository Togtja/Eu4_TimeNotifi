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

// TODO: Move a lot into functions / classes
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
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only

    // Create window with graphics context

    GLFWwindow* window = glfwCreateWindow(WINDOWS_SIZE.x, WINDOWS_SIZE.y, "EU4 Time notification", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    NotificationSound sound("Resources/Audio/quest_complete.wav");

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
    // ImGui::StyleColorsClassic();

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

    // Our state
    bool show_demo_window = true;

    std::string current_month{MONTH_NAME[10]};
    bool foundEu4Memloc = false;
    bool increase_day   = false;

    std::atomic_bool play_sound  = false;
    std::atomic_bool exit_worker = false;

    // Pointer to a memory class, that include some nice memory functions
    std::unique_ptr<CrossMemory> mem = nullptr;

    std::vector<const uint8_t*> eu4_date_address{};
    std::vector<Eu4_Notification> notify_dates{};

    // Max concurrent thread -2 because (worker thread + current thread)

    uint32_t nr_threads = MAX_THREADS;
    std::thread worker_thread;
    std::atomic_bool running = false;

    // Display notification popup
    bool display_notification_popup = false;
    std::vector<std::string> popup_msg{};

    // The notification date (i.e user set date)
    Eu4Date eu4date(11, 11, 1444);

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

            ImGui::Image((void*)&images[0].texture, ImVec2(images[0].width, images[0].height));
            ImGui::Text("Look Above a perfectly fine photo, but below it get scuffed");
            ImGui::Image((void*)&images[0].texture, ImVec2(images[0].width, images[0].height));

            Eu4Date current_date(11, 11, 1444); // SO it beginns

            current_month = MONTH_NAME[eu4date.get_date().month - 1];

            // Find the eu4 Date:
            if (!foundEu4Memloc) {
                static std::string eu4exe{"eu4.exe"};
                if (!increase_day) {
                    ImGui::Text("Eu4 excecutable name (typically eu4.exe):");
                    ImGui::InputText("##eu4 exe", &eu4exe);
                    ImGui::Text("Enter the current EU4 Date:");
                }
                else {
                    ImGui::Text("Enter a different EU4 Date:");
                }

                ImGui::PushItemWidth(100);
                auto [temp_day, temp_month, temp_year] = eu4date.get_date();

                ImGui::InputScalar("##eu4 day", ImGuiDataType_U8, &temp_day, &U8_1STEP);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 month", current_month.c_str(), 0)) {
                    for (int i = 0; i < MONTH_NAME.size(); i++) {
                        bool is_selected = (eu4date.get_date().month == i + 1);
                        if (ImGui::Selectable(MONTH_NAME[i].data(), is_selected)) {
                            current_month = MONTH_NAME[i];
                            temp_month    = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::InputScalar("##eu4 year", ImGuiDataType_U16, &temp_year, &U16_1STEP);

                eu4date.set_date({temp_day, temp_month, temp_year});
                if (running) {
                    ImGui::Text("Working on finding the date in the game");
                }
                if (!running && ImGui::Button("Submit", {200, 20})) {
                    if (worker_thread.joinable()) {
                        worker_thread.join();
                    }

                    if (!mem) {
                        try {
                            mem.reset(new CrossMemory(eu4exe));
                        }
                        catch (const std::exception& e) {
                            if (mem) {
                                mem.reset(nullptr);
                            }
                            spdlog::error(e.what());
                        }
                    }
                    else {
                        worker_thread = std::thread(
                            [&mem, &running, &eu4_date_address, &foundEu4Memloc, &increase_day, &nr_threads, eu4date]() {
                                running = true;
                                if (mem && !eu4_date_address.empty()) {
                                    eu4_date_address = mem->find_byte_from_vector(eu4date.get_days(), eu4_date_address);
                                }
                                else if (mem) {
                                    spdlog::debug("Scanning memory for {} with {} thereds", eu4date.get_days(), nr_threads);
                                    eu4_date_address = mem->scan_memory(eu4date.get_days(), nr_threads);
                                }
                                spdlog::debug("Memory scan returned for {}", eu4_date_address.size());
                                if (mem && eu4_date_address.size() == 1) {
                                    foundEu4Memloc = true;
                                }
                                else if (mem) {
                                    increase_day = true;
                                }

                                running = false;
                            });
                    }
                }
                current_date = eu4date;
            }
            else {
                uint32_t eu4_date_read{};
                mem->bytes_to_T(eu4_date_address.front(), eu4_date_read);
                current_date  = Eu4Date(eu4_date_read);
                current_month = MONTH_NAME[current_date.get_date().month - 1];

                if (!running) {
                    if (worker_thread.joinable()) {
                        worker_thread.join();
                    }
                    running       = true;
                    worker_thread = std::thread([&sound, &play_sound, &running, &exit_worker]() {
                        while (!exit_worker) {
                            if (play_sound) {
                                sound.play();
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                play_sound = false;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        running = false;
                    });
                }
            }

            // Displays the current date
            // If anyone read this: help me not use readonly input and a fake combo list, but keep the theme/aesthetic the the
            // input does

            ImGui::Text("Current Eu4 Date"); // Display some text (you can use a format strings too)
            ImGui::PushItemWidth(100);

            auto [temp_day, temp_month, temp_year] = eu4date.get_date();
            ImGui::InputScalar("##eu4 current day", ImGuiDataType_U8, &temp_day, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::BeginCombo("##eu4 current month", current_month.c_str(), ImGuiComboFlags_NoArrowButton)) {
                for (int i = 0; i < MONTH_NAME.size(); i++) {
                    if (temp_month == i + 1) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::InputScalar("##eu4 current year", ImGuiDataType_U16, &temp_year, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
            // Convert it back in
            current_date.set_date({temp_day, temp_month, temp_year});

            if (foundEu4Memloc) {
                ImGui::Text("Add a notification for a specific date"); // Display some text (you can use a format strings too)
                ImGui::PushItemWidth(100);
                auto [temp_day, temp_month, temp_year] = eu4date.get_date();
                ImGui::InputScalar("##eu4 notification day", ImGuiDataType_U8, &temp_day);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 notification month", MONTH_NAME[temp_month - 1].data())) {
                    for (int i = 0; i < MONTH_NAME.size(); i++) {
                        bool is_selected = (temp_month == i + 1);
                        if (ImGui::Selectable(MONTH_NAME[i].data(), is_selected)) {
                            temp_month = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::InputScalar("##eu4 notification year", ImGuiDataType_U16, &temp_year);
                eu4date.set_date({temp_day, temp_month, temp_year});

                static std::string notify_msg{};
                static uint32_t notify_repeat      = 1;
                static uint32_t notify_repeat_days = 0;
                ImGui::PushItemWidth(200);
                ImGui::InputText("Notification Message", &notify_msg);
                if (notify_msg.size() > MAX_NOTIFY_MSG) {
                    notify_msg.resize(MAX_NOTIFY_MSG);
                }
                ImGui::InputScalar("repeat (0 = infinite)", ImGuiDataType_U32, &notify_repeat);
                // TODO: Make it possible to repeat every x monnth or x years
                ImGui::InputScalar("repeat every x days", ImGuiDataType_U32, &notify_repeat_days);

                // Ensure that the date is not before the current date
                if (eu4date.get_days() < current_date.get_days()) {
                    eu4date = current_date;
                }

                if (ImGui::Button("Submit Notification", {200, 20})) {
                    notify_dates.push_back({eu4date, notify_msg, notify_repeat, notify_repeat_days});
                    notify_msg = "";
                }
            }

            manage_notifications(notify_dates, popup_msg, current_date, play_sound);
            notify_popup(popup_msg);
            imgui_setting(window, images[0], nr_threads);

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
    exit_worker = true;

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
    return 0;
}
