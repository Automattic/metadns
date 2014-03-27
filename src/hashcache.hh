
#ifndef __HASH_CACHE_H__
#define __HASH_CACHE_H__

#include <cstdlib>
#include <cstdint>
#include <string>
#include <map>

#include "defines.h"
#include "hashring.hh"
#include "cache.hh"

class Hash_Cache : public Cache<std::uint64_t, Hash_Ring> {

public:
	std::string get_hash_value( const std::uint64_t& p_pool_house_id, std::string p_qdomain,
								std::map<std::uint8_t, std::string>& ignoreIPs, std::uint8_t depth ) const;

	void get_hash_ring_datafields( const std::uint64_t& p_pool_house_id, std::string& ttl,
									std::uint8_t& num_results) const;

private:

};

#endif	//__HASH_CACHE_H__

