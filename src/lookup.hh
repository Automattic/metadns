
#ifndef __LOOKUP_H__
#define __LOOKUP_H__

#include <string>
#include <ctime>
#include <chrono>

#include "defines.h"

class Lookup {
public:
	std::string sql_fields;
	time_t timeout;

	const std::chrono::steady_clock::time_point get_last_modified() const { return last_refresh; }
	void touch() { last_refresh = std::chrono::steady_clock::now(); }

private:
	std::chrono::steady_clock::time_point last_refresh;
};

#endif //__LOOKUP_H__

