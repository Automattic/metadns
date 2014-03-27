
#ifndef __SOA_H__
#define __SOA_H__

#include <cstdlib>
#include <string>
#include <ctime>
#include <chrono>

#include "defines.h"

class SOA {
public:
	long domain_id;
	std::string host_master;
	long serial;
	std::string name_server;
	int refresh;
	int retry;
	int expire;
	int default_ttl;
	int ttl;

	const std::chrono::steady_clock::time_point get_last_modified() const { return last_refresh; }
	void touch() { last_refresh = std::chrono::steady_clock::now(); }

private:
	std::chrono::steady_clock::time_point last_refresh;
};

#endif	//__SOA_H__

