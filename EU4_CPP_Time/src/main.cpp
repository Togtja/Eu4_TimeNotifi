// C++ Libraries
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

// External Libraries
#include <glad/glad.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <spdlog/spdlog.h>

// Need to include glfe3 after glad + imgui
#include <GLFW/glfw3.h>

// Own Libaries
#include "CrossMemory.hpp"
#include "Eu4Date.hpp"
#include "NotificationSound.hpp"

static void glfw_error_callback(int error, const char* description) {
    spdlog::log(spdlog::level::critical, "GLFW Error {}: {}", error, description);
}

struct Eu4_Notification {
    Eu4Date date;
    std::string message{};
    uint32_t repeat      = 1;
    uint32_t repeat_days = 0;
    bool operator==(const Eu4_Notification& rhs) { return date == rhs.date && message == rhs.message && repeat == rhs.repeat; }
};

// TODO: Move a lot into functions / classes
int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const std::string glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only

    // Create window with graphics context
    ImVec2 window_size{700, 700};
    GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "EU4 Time notification", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    NotificationSound sound("Resources/Audio/quest_complete.wav");

    bool err = gladLoadGL() == 0;
    if (err) {
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
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());

    // Our state
    bool show_demo_window = true;
    // Background color
    const ImVec4 clear_color                              = ImVec4(0.65f, 0.55f, 0.60f, 1.00f);
    constexpr std::array<std::string_view, 12> month_name = {"1:January",
                                                             "2:February",
                                                             "3:March",
                                                             "4:April",
                                                             "5:May",
                                                             "6:June",
                                                             "7:July",
                                                             "8:August",
                                                             "9:September",
                                                             "10:October",
                                                             "11:November",
                                                             "12:December"};
    std::string current_month{month_name[10]};
    bool foundEu4Memloc = false;
    bool increase_day   = false;

    std::atomic_bool play_sound  = false;
    std::atomic_bool exit_worker = false;

    // Pointer to a memory class, that include some nice memory functions
    std::unique_ptr<CrossMemory> mem = nullptr;

    std::vector<const uint8_t*> eu4_date_address{};
    std::vector<Eu4_Notification> notify_dates{};

    // Max concurrent thread -2 because (worker thread + current thread)
    const uint32_t MAX_THREADS = std::thread::hardware_concurrency() * 2 - 2;
    uint32_t nr_threads        = MAX_THREADS;
    std::thread worker_thread;
    std::atomic_bool running = false;

    // Display notification popup
    bool display_notification_popup = false;
    std::vector<std::string> popup_msg{};

    static int n_day{11}, n_month{11}, n_year{1444};

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f     = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize(window_size);
            ImGui::Begin("Hello, world!",
                         0,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_NoDecoration); // Create a window called "Hello, world!" and append into it.

            int c_day{11}, c_month{11}, c_year{1444};

            Eu4Date eu4date(n_day, n_month, n_year);
            auto notiDateStruct = eu4date.get_date();
            n_day               = notiDateStruct.day;
            n_month             = notiDateStruct.month;
            current_month       = month_name[n_month - 1];
            n_year              = notiDateStruct.year;

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
                ImGui::InputInt("##eu4 day", &n_day);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 month", current_month.c_str(), 0)) {
                    for (int i = 0; i < month_name.size(); i++) {
                        bool is_selected = (n_month == i + 1);
                        if (ImGui::Selectable(month_name[i].data(), is_selected)) {
                            current_month = month_name[i];
                            n_month       = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::InputInt("##eu4 year", &n_year);
                if (running) {
                    ImGui::Text("Working on finding the date in the game");
                }
                else {
                    // TODO: Make this apart ofa debug menu
                    ImGui::Text("How many threads to use for the search:");
                    ImGui::SameLine();
                    ImGui::InputScalar("##thread count", ImGuiDataType_U32, &nr_threads);
                    if (nr_threads > MAX_THREADS) {
                        nr_threads = MAX_THREADS;
                    }
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
                c_day   = n_day;
                c_month = n_month;
                c_year  = n_year;
            }
            else {
                uint32_t eu4_date_read{};
                mem->bytes_to_T(eu4_date_address.front(), eu4_date_read);
                Eu4Date cur_date(eu4_date_read);
                const auto& [_day, _month, _year] = cur_date.get_date();
                c_day                             = _day;
                c_month                           = _month;
                current_month                     = month_name[_month - 1];
                c_year                            = _year;

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
            // If anyone read this: help me not use readonly inoput and a fake combo list, but keep the theme/aesthetic the the
            // input does

            ImGui::Text("Current Eu4 Date"); // Display some text (you can use a format strings too)
            ImGui::PushItemWidth(100);

            ImGui::InputInt("##eu4 current day", &c_day, 0, ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            if (ImGui::BeginCombo("##eu4 current month", current_month.c_str(), ImGuiComboFlags_NoArrowButton)) {
                for (int i = 0; i < month_name.size(); i++) {
                    if (c_month == i + 1) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            ImGui::InputInt("##eu4 current year", &c_year, 0, ImGuiInputTextFlags_ReadOnly);

            if (foundEu4Memloc) {
                ImGui::Text("Add a notification for a specific date"); // Display some text (you can use a format strings too)
                ImGui::PushItemWidth(100);
                ImGui::InputInt("##eu4 notification day", &n_day, 1);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 notification month", month_name[n_month - 1].data())) {
                    for (int i = 0; i < month_name.size(); i++) {
                        bool is_selected = (n_month == i + 1);
                        if (ImGui::Selectable(month_name[i].data(), is_selected)) {
                            n_month = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                static std::string notify_msg{};
                static uint32_t notify_repeat      = 1;
                static uint32_t notify_repeat_days = 0;
                ImGui::InputInt("##eu4 notification year", &n_year, 1);
                ImGui::PushItemWidth(200);
                ImGui::InputText("Notification Message", &notify_msg);
                // TODO: Ensure size of message is not to big
                ImGui::InputScalar("repeat (0 = infinite)", ImGuiDataType_U32, &notify_repeat);
                // TODO: Make it possible to repeat every x monnth or x years
                ImGui::InputScalar("repeat every x days", ImGuiDataType_U32, &notify_repeat_days);

                // Ensure that the date is not before the current date
                Eu4Date currDate(c_day, c_month, c_year);
                if (eu4date.get_days() < currDate.get_days()) {
                    n_day   = c_day;
                    n_month = c_month;
                    n_year  = c_year;
                }

                if (ImGui::Button("Submit Notification", {200, 20})) {
                    notify_dates.push_back({eu4date, notify_msg, notify_repeat, notify_repeat_days});
                    notify_msg = "";
                }
            }
            if (!notify_dates.empty()) {
                // Sort the dates
                std::sort(notify_dates.begin(), notify_dates.end(), [](const auto& lhs, const auto& rhs) {
                    return lhs.date.get_days() < rhs.date.get_days();
                });
                // Display and purge the gone-by dates
                auto notify_dates_cpy = notify_dates;
                for (auto& eu4date : notify_dates_cpy) {
                    Eu4Date currDate(c_day, c_month, c_year);
                    if (eu4date.date.get_days() <= currDate.get_days()) {
                        play_sound = true;
                        // Remove if is not set to infinite
                        notify_dates.erase(std::remove(notify_dates.begin(), notify_dates.end(), eu4date), notify_dates.end());

                        // If 0 always repeat
                        if (eu4date.repeat == 0) {
                            eu4date.date = Eu4Date(eu4date.date.get_days() + eu4date.repeat_days);
                            notify_dates.push_back(eu4date);
                        }
                        else {
                            eu4date.repeat--;
                            // If not 0 after repeat subtraction, then repeat
                            if (eu4date.repeat > 0) {
                                eu4date.date = Eu4Date(eu4date.date.get_days() + eu4date.repeat_days);
                                notify_dates.push_back(eu4date);
                            }
                        }

                        if (!eu4date.message.empty()) {
                            popup_msg.push_back(eu4date.message + " (" + eu4date.date.get_date_as_string() + ")");
                        }
                    }
                    // Display the dates here
                    ImGui::Text(
                        fmt::format("Notification for: {} ({})", eu4date.message, eu4date.date.get_date_as_string()).c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("X", {15, 15})) {
                        notify_dates.erase(std::remove(notify_dates.begin(), notify_dates.end(), eu4date), notify_dates.end());
                    }
                }
            }

            if (ImGui::IsAnyItemFocused() && !popup_msg.empty()) {
                ImGui::OpenPopup("Notification Popup");
            }
            if (ImGui::BeginPopup("Notification Popup")) {
                for (auto&& msg : popup_msg) {
                    ImGui::Text(msg.c_str());
                }
                if (ImGui::Button("Ok", {200, 20})) {
                    ImGui::CloseCurrentPopup();
                    popup_msg.clear();
                }

                ImGui::EndPopup();
            }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
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
