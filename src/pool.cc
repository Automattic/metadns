
#include "pool.hh"

using namespace std;

Pool::Pool() : m_ttl ( "-1" ), m_num_results( 0 ), m_random_results( false ) {
	last_refresh = std::chrono::steady_clock::now();
}

const std::chrono::steady_clock::time_point Pool::get_last_modified() const
{
	return last_refresh;
}

void Pool::touch() {
	last_refresh = std::chrono::steady_clock::now();
}

void Pool::add_records( const std::string& p_content, const std::string& p_priority ) {

	m_content.resize( (unsigned int)( m_content.size() ) + 1 );

	m_content[m_content.size() - 1].push_back( p_content );
	m_content[m_content.size() - 1].push_back( p_priority );
}

const string Pool::get_content( uint32_t record, uint32_t column ) const {
	string retVal = "";

	if ( ( record < m_content.size() ) && ( column < m_content.at(record).size() ) ) {
		retVal = m_content[record][column];
	}

	return retVal;
}

