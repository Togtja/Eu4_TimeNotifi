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

    void eu4days_to_eu4date();
    void eu4date_to_eu4days();
    // Always call this after a set_x to fix the date

public:
    bool operator==(const Eu4Date& rhs) {
        return m_days == rhs.m_days && m_day == rhs.m_day && m_month == rhs.m_month && m_year == rhs.m_year;
    }
    bool operator<=(const Eu4Date& rhs) {
        return m_days <= rhs.m_days && m_day <= rhs.m_day && m_month <= rhs.m_month && m_year <= rhs.m_year;
    }
    Eu4Date(uint32_t days);
    Eu4Date(const Eu4DateStruct& date);
    Eu4Date(uint8_t day, uint8_t month, uint16_t year);
    ~Eu4Date();

    std::string get_date_as_string();

    uint32_t get_days() const { return m_days; }
    Eu4DateStruct get_date() const { return {m_day, m_month, m_year}; }

    void set_days(uint32_t days) {
        m_days = days;
        eu4days_to_eu4date();
    }

    void set_day(uint8_t day) {
        m_day = day;
        eu4date_to_eu4days();
    }
    void set_month(uint8_t month) {
        m_month = month;
        eu4date_to_eu4days();
    }
    void set_year(uint16_t year) {
        m_year = year;
        eu4date_to_eu4days();
    }
    void set_date(const Eu4DateStruct& date) {
        m_day   = date.day;
        m_month = date.month;
        m_year  = date.year;
        eu4date_to_eu4days();
    }
};
