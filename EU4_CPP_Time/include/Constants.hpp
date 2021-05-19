#pragma once
#include <array>
#include <string_view>
#include <thread>

constexpr std::string_view GLSL_VERSION = "#version 130";

constexpr std::array<std::string_view, 1> IMAGES_PATH = {{"Resources/Images/settings.png"}};

// Background color
const ImVec4 CLEAR_COLOR                              = ImVec4(0.65f, 0.55f, 0.60f, 1.00f);
constexpr std::array<std::string_view, 12> MONTH_NAME = {"1:January",
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

// Max concurrent thread -2 because (worker thread + current thread)
const uint32_t MAX_THREADS = std::thread::hardware_concurrency() * 2 - 2;

constexpr size_t MAX_NOTIFY_MSG = 200;

ImVec2 WINDOWS_SIZE{700, 700};

uint8_t U8_1STEP   = 1;
uint16_t U16_1STEP = 1;

#ifdef _WIN32
constexpr std::string_view DEFAULT_EU4_BIN = "eu4.exe";
#else
constexpr std::string_view DEFAULT_EU4_BIN = "eu4";
#endif