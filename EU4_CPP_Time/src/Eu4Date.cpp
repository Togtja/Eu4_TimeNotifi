#include "Eu4Date.hpp"

#include <limits>

void Eu4Date::eu4days_to_eu4date() {
    m_year    = m_days / 365;
    auto days = m_days - m_year * 365;

    uint8_t month{0};
    for (; month < months_days.size() && days >= months_days[month]; month++) {
        days -= months_days[month];
    }
    month++; // Add 1 month as it stats on 0
    m_month = month;
    if (days + 1 < std::numeric_limits<uint8_t>::max()) {
        m_day = static_cast<uint8_t>(days + 1);
    }
    else {
        // TODO: log error
    }
}

void Eu4Date::eu4date_to_eu4days() {
    m_days = m_year * 365;
    for (size_t i = 1; i <= m_month - 1; i++) {
        m_days += months_days[i - 1];
    }
    m_days += m_day;
    // Remove one ? cus it works
    m_days--;
    // Correct dates such as 32/01/2000 to 01/02/2000
    eu4days_to_eu4date();
}

Eu4Date::Eu4Date(uint32_t days) : m_days(days) {
    eu4days_to_eu4date();
}

Eu4Date::Eu4Date(const Eu4DateStruct& date) : m_day(date.day), m_month(date.month), m_year(date.year) {
    eu4date_to_eu4days();
}

Eu4Date::Eu4Date(uint8_t day, uint8_t month, uint16_t year) : m_day(day), m_month(month), m_year(year) {
    eu4date_to_eu4days();
}

Eu4Date::~Eu4Date() {}

std::string Eu4Date::get_date_as_string() {
    std::stringstream ss;
    ss << static_cast<uint16_t>(m_day) << "/" << static_cast<uint16_t>(m_month) << "/" << m_year;
    return ss.str();
}