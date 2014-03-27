
#ifndef __POOLCACHE_H__
#define __POOLCACHE_H__

#include <cstdlib>
#include <cstdint>
#include <string>

#include "defines.h"
#include "pool.hh"
#include "cache.hh"

class Pool_Cache : public Cache<std::uint64_t, Pool> {

public:
	void get_pool_cache_datafields( const uint64_t& p_pool_house_id, std::string& ttl,
									std::uint8_t& num_results, bool& random ) const;

private:

};

#endif	//__POOLCACHE_H__

