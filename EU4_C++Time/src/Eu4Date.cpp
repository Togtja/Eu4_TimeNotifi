#include "Eu4Date.hpp"


Eu4Date::Eu4Date(uint32_t days) {
    m_days = days;

    m_year = days / 365;
	days -= m_year * 365;

	uint8_t month{0};
	for(; month < months_days.size() && days > months_days[month]; month++){
		days -= months_days[month];
	}
	month++; //Add 1 month as it stats on 0
    m_month = month;
	if(days+1 < std::numeric_limits<uint8_t>::max()){
		m_day = static_cast<uint8_t>(days+1);
	}
    else{
        //TODO: log error
    }
}

Eu4Date::Eu4Date(uint8_t day, uint8_t month, uint16_t year){
    m_day = day;
    m_month = month;
    m_year = year;

    m_days = year*365;
    for (size_t i = 0; i < month; i++) {
        m_days += months_days[i]; 
    }
    m_days += day;
    
}

Eu4Date::~Eu4Date() {
}

std::string Eu4Date::get_date_as_string(){
    std::stringstream ss;
    ss << m_day << "/" << m_month << "/" + m_year;
    return ss.str();
}