// C++ Libraries

// External Libraries
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
// Need to include glfe3 after glad + imgui
#include <GLFW/glfw3.h>

// Own Libaries

static void glfw_error_callback(int error, const char* description) {
    spdlog::log(spdlog::level::critical, "GLFW Error {}: {}", error, description);
}

int main(int, char**) {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "EU4 Time notification", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    bool err = gladLoadGL() == 0;
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window    = true;
    bool show_another_window = false;
    ImVec4 clear_color       = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn
        // more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f     = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a
                                                                  // closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
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