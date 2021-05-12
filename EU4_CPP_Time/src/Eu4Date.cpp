#include "Eu4Date.hpp"

void Eu4Date::eu4days_to_eu4date(uint32_t days) {
    m_year = days / 365;
    days -= m_year * 365;

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

Eu4Date::Eu4Date(uint32_t days) {
    m_days = days;
    eu4days_to_eu4date(m_days);
}

Eu4Date::Eu4Date(uint8_t day, uint8_t month, uint16_t year) {
    m_days = year * 365;
    for (size_t i = 1; i <= month - 1; i++) {
        m_days += months_days[i - 1];
    }
    m_days += day;
    // Remove one ? cus it works
    m_days--;

    eu4days_to_eu4date(m_days);
}

Eu4Date::~Eu4Date() {}

std::string Eu4Date::get_date_as_string() {
    std::stringstream ss;
    ss << static_cast<uint16_t>(m_day) << "/" << static_cast<uint16_t>(m_month) << "/" << m_year;
    return ss.str();
}