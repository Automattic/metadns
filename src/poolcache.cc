
#include "poolcache.hh"

using namespace std;

void Pool_Cache::get_pool_cache_datafields( const uint64_t& p_pool_house_id, std::string& ttl,
								std::uint8_t& num_results, bool& random ) const {
	Cache::const_iterator it = m_items.find( p_pool_house_id );

	if ( it != m_items.end() ) {
		ttl = it->second->get_ttl();
		num_results = it->second->get_num_results();
		random = it->second->get_random_results();
	}
}

