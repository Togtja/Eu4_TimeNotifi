#pragma once

#include <memory>
#include <string>

// External
#include <glad/glad.h>
#include <spdlog/spdlog.h>

// Need to include glfw3 after glad
#include <GLFW/glfw3.h>

// Own Libraries
#include "CrossMemory.hpp"
#include "Eu4Date.hpp"
#include "NotificationSound.hpp"

struct EU4_Image {
    GLuint texture;
    int width;
    int height;
};

struct Eu4_Notification {
    Eu4Date date;
    std::string message{};
    uint32_t repeat = 1;
    Eu4DateStruct repeat_date;
    bool operator==(const Eu4_Notification& rhs) { return date == rhs.date && message == rhs.message && repeat == rhs.repeat; }
};

inline static void glfw_error_callback(int error, const char* description) {
    spdlog::log(spdlog::level::critical, "GLFW Error {}: {}", error, description);
}
void create_memory_class_for_eu4(std::unique_ptr<CrossMemory>& mem, const std::string& excecutable);

void start_find_eu4date_in_memory(std::thread& thread,
                                  uint32_t& nr_threads,
                                  std::unique_ptr<CrossMemory>& mem,
                                  std::atomic_bool& running,
                                  const Eu4Date& find_date,
                                  std::vector<const uint8_t*>& eu4_date_address,
                                  bool& foundEu4Memloc,
                                  bool& increase_day);

void sound_player_thread(NotificationSound& sound, std::atomic_bool& play_sound, std::atomic_bool& exit_thread);

void update_current_date(std::unique_ptr<CrossMemory>& mem,
                         const std::vector<const uint8_t*>& eu4_date_address,
                         Eu4Date& current_date);

void get_user_date(Eu4Date& user_date);

void display_current_date(const Eu4Date& current_date);

void get_repeating_date(Eu4DateStruct& user_date);

auto get_reapeating(bool clear_repeat = false);

void add_notification(std::vector<Eu4_Notification>& notify_dates, Eu4Date& notify_date, const Eu4Date& current_date);

void manage_notifications(std::vector<Eu4_Notification>& notify_dates,
                          std::vector<std::string>& popup_msg,
                          const Eu4Date& currDate,
                          std::atomic_bool& play_sound);

void notify_popup(std::vector<std::string>& popup_msg);

void imgui_setting(GLFWwindow* window, const EU4_Image& setting_icon, uint32_t& nr_threads);

bool load_texture(const std::string& image_file, EU4_Image& image);

void select_sound_player_widget(NotificationSound& sound, std::vector<std::string>& sound_devices, int& current_device);

void save_notification_widget(const std::vector<Eu4_Notification>& notify_dates, std::string& current_loaded_file);

void load_notification_widget(std::vector<Eu4_Notification>& notify_dates, int current_device);