#pragma once

#include <string>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

// Need to include glfe3 after glad + imgui
#include <GLFW/glfw3.h>

#include "Constants.hpp"
#include "Eu4Date.hpp"

struct EU4_Image {
    GLuint texture;
    int width;
    int height;
};

struct Eu4_Notification {
    Eu4Date date;
    std::string message{};
    uint32_t repeat      = 1;
    uint32_t repeat_days = 0;
    bool operator==(const Eu4_Notification& rhs) { return date == rhs.date && message == rhs.message && repeat == rhs.repeat; }
};

static void glfw_error_callback(int error, const char* description) {
    spdlog::log(spdlog::level::critical, "GLFW Error {}: {}", error, description);
}

void manage_notifications(std::vector<Eu4_Notification>& notify_dates,
                          std::vector<std::string>& popup_msg,
                          const Eu4Date& currDate,
                          std::atomic_bool& play_sound) {
    // Sort the dates
    std::sort(notify_dates.begin(), notify_dates.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.date.get_days() < rhs.date.get_days();
    });
    // Display and purge the gone-by dates
    auto notify_dates_cpy = notify_dates;
    for (auto& eu4date : notify_dates_cpy) {
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
        ImGui::Text(fmt::format("Notification for: {} ({})", eu4date.message, eu4date.date.get_date_as_string()).c_str());
        ImGui::SameLine();
        if (ImGui::Button("X", {15, 15})) {
            notify_dates.erase(std::remove(notify_dates.begin(), notify_dates.end(), eu4date), notify_dates.end());
        }
    }
}

void notify_popup(std::vector<std::string>& popup_msg) {
    if (ImGui::IsWindowFocused() && !popup_msg.empty()) {
        ImGui::OpenPopup("Notification Popup");
        ImVec2 windows_center = {WINDOWS_SIZE.x / 2, WINDOWS_SIZE.y / 2};
        ImGui::SetNextWindowPos(windows_center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
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
}

void imgui_setting(GLFWwindow* window, const EU4_Image& setting_icon, uint32_t& nr_threads) {
    if (ImGui::ImageButton((void*)&setting_icon.texture, ImVec2(setting_icon.width, setting_icon.height))) {
        ImGui::OpenPopup("settings");
    }

    if (ImGui::BeginPopupModal("settings")) {
        ImGui::Text("How many threads to use for the search:");
        ImGui::SameLine();
        ImGui::InputScalar("##thread count", ImGuiDataType_U32, &nr_threads);
        if (nr_threads > MAX_THREADS) {
            nr_threads = MAX_THREADS;
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        // Close the pop up if escape or enter is pressed
        int esc_state   = glfwGetKey(window, GLFW_KEY_ESCAPE);
        int enter_state = glfwGetKey(window, GLFW_KEY_ENTER);
        if (esc_state == GLFW_PRESS || enter_state == GLFW_PRESS) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool load_texture(const std::string& image_file, EU4_Image& image) {
    if (!std::filesystem::exists(image_file)) {
        return false;
    }
    // Load from file
    int image_width  = 0;
    int image_height = 0;
    int comp;
    unsigned char* image_data = stbi_load(image_file.c_str(), &image_width, &image_height, &comp, STBI_rgb_alpha);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    image.texture = image_texture;
    image.width   = image_width;
    image.height  = image_height;

    return true;
}
