
#include "hashring.hh"

using namespace std;

CRC32 Hash_Ring::m_hasher = CRC32();

Hash_Ring::Hash_Ring() {
	last_refresh = std::chrono::steady_clock::now();
}

Hash_Ring::~Hash_Ring() {
	;
}

const std::chrono::steady_clock::time_point Hash_Ring::get_last_modified() const {
	return last_refresh;
}

uint32_t Hash_Ring::add_node( const string& node, const int& num_copies ) {
	uint32_t int_hash = 0;
	string s_HashIt;
	uint32_t bucket_value;

	for ( int icopies = 0; icopies < num_copies; icopies++ ) {

		s_HashIt = node + ":" + std::to_string( icopies );
		int_hash = m_hasher.generate_crc( s_HashIt.c_str(), s_HashIt.length() );
		int_hash = int_hash % BUCKET_SIZE;

		for ( int i_loop = 0; i_loop < NUM_BUCKETS; i_loop++) {
			bucket_value = int_hash + ( i_loop * BUCKET_SIZE );
			m_hashring[ bucket_value ] = node;
		}
	}
	return int_hash;
}

const string& Hash_Ring::get_node( const string& query_data, map<uint8_t, string>& ignore_IPs, uint8_t depth ) {

	static string emptyString = "";

	if ( m_hashring.empty() )
		return emptyString;

	uint32_t int_hash = m_hasher.generate_crc( query_data.c_str() , query_data.length() );
	NodeMap::const_iterator it;

	while ( depth > 0 ) {
		it = m_hashring.lower_bound( int_hash );

		if ( it == m_hashring.end() )
			it = m_hashring.begin();

		int_hash = it->first;
		int_hash++;
		bool found = false;

		for ( auto itr = ignore_IPs.begin(), end = ignore_IPs.end(); itr != end; itr++  ) {
			if ( ( itr->first >= depth ) &&
				( itr->second == it->second ) ) {
				found = true;
				break;
			}
		}

		if ( ! found )
			depth--;
	}
	return it->second;
}

void Hash_Ring::touch() {
	last_refresh = std::chrono::steady_clock::now();
}

