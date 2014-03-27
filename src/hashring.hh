
#ifndef __HASHRING_H__
#define __HASHRING_H__

#include <cstdlib>
#include <cstdint>
#include <string>
#include <map>
#include <ctime>
#include <chrono>

#include "defines.h"
#include "crc32.hh"

class Hash_Ring {

public:
	typedef std::map<std::uint32_t, std::string> NodeMap;

	Hash_Ring();
	~Hash_Ring();

	std::uint32_t add_node( const std::string& node, const int& num_copies );
	const std::string& get_node( const std::string& query_data, std::map<std::uint8_t,
									std::string>& ignore_IPs, std::uint8_t depth );

	void set_ttl( std::string p_ttl ) { m_ttl = p_ttl; }
	const std::string get_ttl() { return m_ttl; }
	void set_num_results( std::uint8_t p_numresults ) { m_numresults = p_numresults; }
	const std::uint8_t get_num_results() { return m_numresults; }

	const std::chrono::steady_clock::time_point get_last_modified() const;
	void touch();

private:
	NodeMap m_hashring;
	std::string m_ttl;
	uint8_t m_numresults;
	std::chrono::steady_clock::time_point last_refresh;

	static CRC32 m_hasher;
};

#endif // __HASHRING_H__

