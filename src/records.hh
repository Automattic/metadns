
#ifndef __RECORDS_H__
#define __RECORDS_H__

#include <cstdlib>
#include <ctime>
#include <chrono>

#include "defines.h"
#include "database.hh"

class Records {
public:
	Database::result_t data;
	time_t timeout;

	const std::chrono::steady_clock::time_point get_last_modified() const { return last_refresh; }
	void touch() { last_refresh = std::chrono::steady_clock::now(); }

private:
	std::chrono::steady_clock::time_point last_refresh;
};

#endif //__RECORDS_H__

