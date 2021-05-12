#pragma once

#include <sstream>
#include <string>
#include <vector>

struct Eu4DateStruct {
    uint8_t day{};
    uint8_t month{};
    uint16_t year{};
};

class Eu4Date {
private:
    inline const static std::vector<uint16_t> months_days{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t m_days{};
    uint8_t m_day{};
    uint8_t m_month{};
    uint16_t m_year{};

    void eu4days_to_eu4date(uint32_t days);

public:
    bool operator==(const Eu4Date& rhs) {
        return m_days == rhs.m_days && m_day == rhs.m_day && m_month == rhs.m_month && m_year == rhs.m_year;
    }
    bool operator<=(const Eu4Date& rhs) {
        return m_days <= rhs.m_days && m_day <= rhs.m_day && m_month <= rhs.m_month && m_year <= rhs.m_year;
    }
    Eu4Date(uint32_t days);
    Eu4Date(uint8_t day, uint8_t month, uint16_t year);
    ~Eu4Date();

    std::string get_date_as_string();

    uint32_t get_days() const { return m_days; }
    Eu4DateStruct get_date() const { return {m_day, m_month, m_year}; }
};
