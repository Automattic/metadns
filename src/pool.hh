
#ifndef __POOL_H__
#define __POOL_H__

#include <cstdlib>
#include <cstdint>
#include <string>
#include <ctime>
#include <chrono>
#include <vector>

#include "defines.h"

class Pool {

public:
	Pool();
	~Pool() { }

	void add_records( const std::string& p_content, const std::string& p_priority );
	void clear_records() { m_content.clear(); }
	uint32_t get_record_count() { return m_content.size(); }

	void set_ttl( std::string p_ttl ) { m_ttl = p_ttl; }
	const std::string get_ttl() { return m_ttl; }

	void set_num_results( std::uint8_t p_num_results ) { m_num_results = p_num_results; }
	const std::uint8_t get_num_results() { return m_num_results; }

	void set_random_results( bool p_random_results ) { m_random_results = p_random_results; }
	const bool get_random_results() { return m_random_results; }

	const std::string get_content( std::uint32_t record, std::uint32_t column ) const;

	const std::chrono::steady_clock::time_point get_last_modified() const;
	void touch();

private:
	std::vector<std::vector<std::string> > m_content;
	std::string m_ttl;
	uint8_t m_num_results;
	bool m_random_results;
	std::chrono::steady_clock::time_point last_refresh;
};

#endif // __POOL_H__

