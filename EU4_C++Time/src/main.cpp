

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
#include <AL/alc.h>


#include <Eu4Date.hpp>
#include <WindowsMem.hpp>
#include <NotificationSound.hpp>

int main() {
    NotificationSound sound("Resources/Audio/quest_complete.wav");
    

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
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
    */
}