#include <spdlog/spdlog.h>

#include <atomic>
#include <iostream>
#include <thread>

int main() {
    int date = 1994;
    std::thread dateUpdate([&date]() {
        while (true) {
            spdlog::log(spdlog::level::info, "Enter date:");
            int tmp_date{};
            std::cin >> tmp_date;
            date = tmp_date;
        }
    });
    while (true) {
        /* code */
    }
}