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

void create_memory_class_for_eu4(std::unique_ptr<CrossMemory>& mem, const std::string& eu4exe) {
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

void start_find_eu4date_in_memory(std::thread& thread,
                                  uint32_t& nr_threads,
                                  std::unique_ptr<CrossMemory>& mem,
                                  std::atomic_bool& running,
                                  Eu4Date& find_date,
                                  std::vector<const uint8_t*>& eu4_date_address,
                                  bool& foundEu4Memloc,
                                  bool& increase_day) {
    if (thread.joinable()) {
        thread.join();
    }
    thread = std::thread([&mem, &running, &eu4_date_address, &foundEu4Memloc, &increase_day, &nr_threads, find_date]() {
        running = true;
        if (mem && !eu4_date_address.empty()) {
            eu4_date_address = mem->find_byte_from_vector(find_date.get_days(), eu4_date_address);
        }
        else if (mem) {
            spdlog::debug("Scanning memory for {} with {} threads", find_date.get_days(), nr_threads);
            eu4_date_address = mem->scan_memory(find_date.get_days(), nr_threads);
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

void sound_player_thread(NotificationSound& sound, std::atomic_bool& play_sound, std::atomic_bool& exit_thread) {
    while (!exit_thread) {
        if (play_sound) {
            sound.play();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            play_sound = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void update_current_date(std::unique_ptr<CrossMemory>& mem,
                         const std::vector<const uint8_t*>& eu4_date_address,
                         Eu4Date& current_date) {
    uint32_t eu4_date_read{};
    mem->bytes_to_T(eu4_date_address.front(), eu4_date_read);
    current_date = Eu4Date(eu4_date_read);
}

void get_user_date(Eu4Date& user_date) {
    ImGui::PushItemWidth(100);
    auto [temp_day, temp_month, temp_year] = user_date.get_date();

    ImGui::InputScalar("##eu4 day", ImGuiDataType_U8, &temp_day, &U8_1STEP);
    ImGui::SameLine();
    if (ImGui::BeginCombo("##eu4 month", MONTH_NAME[temp_month - 1].data(), 0)) {
        for (int i = 0; i < MONTH_NAME.size(); i++) {
            bool is_selected = (user_date.get_date().month == i + 1);
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
    ImGui::InputScalar("##eu4 year", ImGuiDataType_U16, &temp_year, &U16_1STEP);

    user_date.set_date({temp_day, temp_month, temp_year});
}

void display_current_date(const Eu4Date& current_date) {
    // Displays the current date
    // If anyone read this: help me not use readonly input and a fake combo list, but keep the theme/aesthetic the the
    // input does

    ImGui::Text("Current Eu4 Date"); // Display some text (you can use a format strings too)
    ImGui::PushItemWidth(100);

    auto [temp_day, temp_month, temp_year] = current_date.get_date();
    ImGui::InputScalar("##eu4 current day", ImGuiDataType_U8, &temp_day, 0, 0, 0, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::BeginCombo("##eu4 current month", MONTH_NAME[temp_month - 1].data(), ImGuiComboFlags_NoArrowButton)) {
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
}

void add_notification(std::vector<Eu4_Notification>& notify_dates, Eu4Date& notify_date, const Eu4Date& current_date) {
    ImGui::Text("Add a notification for a specific date"); // Display some text (you can use a format strings too)
    get_user_date(notify_date);

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
    if (notify_date.get_days() < current_date.get_days()) {
        notify_date = current_date;
    }

    if (ImGui::Button("Submit Notification", {200, 20})) {
        notify_dates.push_back({notify_date, notify_msg, notify_repeat, notify_repeat_days});
        notify_msg = "";
    }
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
