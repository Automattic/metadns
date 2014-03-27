
#include "hashcache.hh"

using namespace std;

string Hash_Cache::get_hash_value( const uint64_t& p_pool_house_id, string p_qdomain,
									map<uint8_t, string>& ignoreIPs, uint8_t depth ) const {
	Cache::const_iterator iter = m_items.find( p_pool_house_id );
	string retVal = "";
	std::transform(p_qdomain.begin(), p_qdomain.end(), p_qdomain.begin(), ::tolower);

	if ( iter != m_items.end() ) {
		retVal = iter->second->get_node( p_qdomain, ignoreIPs, depth );
	}

	return retVal;
}

void Hash_Cache::get_hash_ring_datafields( const std::uint64_t& p_pool_house_id, string& ttl, uint8_t& num_results) const {
	Cache::const_iterator it = m_items.find( p_pool_house_id );

	if ( it != m_items.end() ) {
		ttl = it->second->get_ttl();
		num_results = it->second->get_num_results();
	}
}

