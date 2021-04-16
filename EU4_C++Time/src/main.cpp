// C++ Libraries
#include <algorithm>
#include <iostream>
#include <string>
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

int main(int argc, char* argv[]) {
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
    bool show_demo_window                     = true;
    ImVec4 clear_color                        = ImVec4(0.65f, 0.55f, 0.60f, 1.00f);
    const std::vector<std::string> month_name = {"1:January",
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
    std::string current_month{"11:November"};
    bool foundEu4Memloc = false;
    bool increase_day   = false;

    CrossMemory* mem = nullptr;

    std::vector<uint8_t*> eu4_date_address{};
    std::vector<Eu4Date> notify_dates{};

    std::thread worker_thread;
    bool running = false;

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
            static int day{11}, month{11}, year{1444};
            int c_day{11}, c_month{11}, c_year{1444};
            // Find the eu4 Date:
            if (!foundEu4Memloc) {
                static std::string eu4exe{"eu4.exe"};
                if (!increase_day) {
                    ImGui::Text("Eu4 excecutable name (typically eu4.exe):");
                    ImGui::InputText("##eu4 exe", &eu4exe);
                    ImGui::Text("Enter the current EU4 Date:"); // Display some text (you can use a format strings too)
                }
                else {
                    ImGui::Text("Enter a different EU4 Date:");
                }
                const std::string eu4_insert_date_txt = "##eu4 date";
                ImGui::PushItemWidth(100);
                ImGui::InputInt("##eu4 day", &day);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 month", current_month.c_str(), 0)) {
                    for (int i = 0; i < month_name.size(); i++) {
                        bool is_selected = (month == i + 1);
                        if (ImGui::Selectable(month_name[i].c_str(), is_selected)) {
                            current_month = month_name[i];
                            month         = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::InputInt("##eu4 year", &year);
                if (running) {
                    ImGui::Text("Working on finding the date in the game");
                }
                if (!running && ImGui::Button("Submit", {200, 20})) {
                    if (worker_thread.joinable()) {
                        worker_thread.join();
                    }
                    // Eu4Date eu4date(day, month, year);
                    // Eu4Date eu4loop(eu4date.get_days());
                    // auto datet = eu4loop.get_date();
                    // Eu4Date eu4loop2(datet.day, datet.month, datet.year);
                    // std::cout << "eu4 loop: " << eu4loop.get_date_as_string() << " days: " << eu4loop2.get_days() << "\n";

                    worker_thread = std::thread([&mem, &running, &eu4_date_address, &foundEu4Memloc, &increase_day]() {
                        running = true;
                        Eu4Date eu4date(day, month, year);
                        if (!mem) {
                            try {
                                mem = new CrossMemory(eu4exe);
                            }
                            catch (const std::exception& e) {
                                if (mem) {
                                    delete mem;
                                    mem = nullptr;
                                }
                                std::cout << e.what() << '\n';
                            }
                        }
                        if (!eu4_date_address.empty()) {
                            eu4_date_address = mem->find_byte_from_vector(eu4date.get_days(), eu4_date_address);
                        }
                        else {
                            eu4_date_address = mem->find_value(eu4date.get_days());
                        }
                        std::cout << "return size:" << eu4_date_address.size();
                        if (eu4_date_address.size() == 1) {
                            foundEu4Memloc = true;
                        }
                        else {
                            increase_day = true;
                        }

                        running = false;
                    });
                }
                c_day   = day;
                c_month = month;
                c_year  = year;
            }
            else {
                uint32_t eu4_date_read{};
                mem->bytes_to_T2(eu4_date_address.front(), eu4_date_read);
                Eu4Date cur_date(eu4_date_read);
                const auto& [_day, _month, _year] = cur_date.get_date();
                c_day                             = _day;
                c_month                           = _month;
                current_month                     = month_name[_month - 1];
                c_year                            = _year;
                // TODO: set c_day from day etc...
            }

            // Displays the current date
            // If anyone read this help me not use readonly inout and a fake combo list, but keep the theme from the input

            ImGui::Text("Current Eu4 Date"); // Display some text (you can use a format strings too)
            ImGui::PushItemWidth(100);
            // ImGui::Text(fmt::format("{}", c_day).c_str());
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
                static int n_day{11}, n_month{11}, n_year{1444};
                Eu4Date currDate(c_day, c_month, c_year);
                Eu4Date notiDate(n_day, n_month, n_year);
                if (notiDate.get_days() < currDate.get_days()) {
                    n_day   = c_day;
                    n_month = c_month;
                    n_year  = c_year;
                }
                ImGui::Text("Add a notification for a specific date"); // Display some text (you can use a format strings too)
                ImGui::PushItemWidth(100);
                // ImGui::Text(fmt::format("{}", c_day).c_str());
                ImGui::InputInt("##eu4 notification day", &n_day, 1);
                ImGui::SameLine();
                if (ImGui::BeginCombo("##eu4 notification month", month_name[n_month - 1].c_str())) {
                    for (int i = 0; i < month_name.size(); i++) {
                        bool is_selected = (n_month == i + 1);
                        if (ImGui::Selectable(month_name[i].c_str(), is_selected)) {
                            n_month = i + 1;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::InputInt("##eu4 notification year", &n_year, 1);
                if (ImGui::Button("Submit", {200, 20})) {
                    notify_dates.push_back(Eu4Date(n_day, n_month, n_year));
                }
            }
            if (!notify_dates.empty()) {
                auto notify_dates_cpy = notify_dates;
                for (auto eu4date : notify_dates_cpy) {
                    Eu4Date currDate(c_day, c_month, c_year);
                    if (eu4date.get_days() <= currDate.get_days()) {
                        sound.play();
                        notify_dates.erase(std::remove(notify_dates.begin(), notify_dates.end(), eu4date), notify_dates.end());
                    }
                }
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

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (mem) {
        delete mem;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

/*
//standard C++
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

//External Libraries
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <Eu4Date.hpp>
#include <NotificationSound.hpp>
#include <WindowsMem.hpp>


int main() {
    NotificationSound sound("Resources/Audio/quest_complete.wav");
    sound.play();


    GLFWwindow* window;

    //Initialize the library
    if (!glfwInit())
        return -1;

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    //ImGUI stuff

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
    /**
    DWORD access = PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
    HANDLE proc  = getProcessByName("EU4.exe", access);
    if (proc == NULL) {
        std::cout << "failed to get process";
        return 0;
    }
    if (std::filesystem::exists("addresses.txt")) {
        std::ifstream file("addresses.txt", std::ios::in);
        std::string line;
        std::getline(file, line);
    }

    uint32_t num;
    std::string cmd;
    std::cout << "value to find\n";
    std::cin >> num;
    std::cin.get();

    auto mapping = find_byte(proc, num);
    std::cout << "found " << mapping.size() << " results\n";
    std::cout << "display [y/n]\n";
    std::getline(std::cin, cmd);
    while (cmd == "y") {
        for (const auto [k, v] : mapping) {
            auto bytes = readAddress(proc, k, sizeof(v));
            std::cout << std::string(bytes.begin(), bytes.end()) << "\n";
        }
        std::cout << "display [y/n]\n";
        std::getline(std::cin, cmd);
    }
    while (mapping.size() > 1) {
        std::cout << "Update number to be: ";
        std::cin >> num;
        std::cin.get();
        mapping = find_byte_from_map(proc, num, mapping);
        std::cout << "found " << mapping.size() << " results\n";
        std::cout << "display [y/n]\n";
        std::getline(std::cin, cmd);
        while (cmd == "y") {
            for (const auto [k, v] : mapping) {
                if constexpr(std::is_same_v<decltype(v), std::string>){

                }
                auto cpy = v;
                bytesToT(proc, k, cpy);
                std::cout << v << "vs" << cpy << "\n";
            }
            std::cout << "display [y/n]\n";
            std::getline(std::cin, cmd);
        }
    }
    if (mapping.size() > 0) {
        std::ofstream file("addresses.txt", std::ios::out);
        for (const auto [k, v] : mapping) {
            file << k << ":" << sizeof(v);
        }
        file.close();
    }
    CloseHandle(proc);

    std::cout << "The end";
    std::cin >> num;
}
    */