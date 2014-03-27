
#include "wpmetabackend.hh"

#include <cstdlib>
#include <cstdint>
#include <thread>
#include <ctime>
#include <chrono>
#include <string>
#include <map>
#include <exception>

#include "pdns/namespaces.hh"

#include <pdns/dns.hh>
#include <pdns/dnsbackend.hh>
#include <pdns/dnspacket.hh>
#include <pdns/ueberbackend.hh>
#include <pdns/ahuexception.hh>
#include <pdns/logger.hh>
#include <pdns/arguments.hh>

static string backend_name = "[WPMetaBackend]";

WPMetaBackend::WPMetaBackend( const string &suffix ) : m_qname( "" ), m_origin( "" ) , m_cache_purge_counter( 0 ) {
	setArgPrefix( "wpmeta" + suffix );
	try {
		m_db = new Database( getArg( "dbname" ),
							getArg( "host" ),
							getArgAsNum( "port" ),
							getArg( "socket" ),
							getArg( "user" ),
							getArg( "password" ) );
		m_db->connect();

		m_random.seed( std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() ) );
		m_random.seed( std::hash<std::thread::id>()( std::this_thread::get_id() ) );
	}
	catch ( std::exception &ex ) {
		L << Logger::Error << backend_name << " Connection failed: " << ex.what() << endl;
		throw AhuException( backend_name + "Failed to create the Database object: " + ex.what() );
	}
	L << Logger::Error << backend_name << ": creation successful." << endl;
}

WPMetaBackend::~WPMetaBackend() {
	m_db->disconnect();
}

int WPMetaBackend::query_db( const string& p_sql ) {
	try {
		return ( m_db->query( p_sql ) );
	} catch ( std::exception &ex ) {
		throw AhuException( ex.what() );
	}
}

int WPMetaBackend::query_db( const string& p_sql, Database::result_t& result ) {
	try {
		return ( m_db->query( p_sql, result ) );
	} catch ( std::exception &ex ) {
		throw AhuException( ex.what() );
	}
}

void WPMetaBackend::send_from_pool_cache( const uint64_t& pool_house_id, const Database::row_t& rrow ) {

	string s_ttl = "300";
	uint8_t num_results = 5;
	bool random_results = true;

	m_PC.get_pool_cache_datafields( pool_house_id, s_ttl, num_results, random_results );
	Pool* m_pool = m_PC.get_object( pool_house_id );
	uint32_t recCount = m_pool->get_record_count();

	vector<vector<string> > tmp_vec;
	tmp_vec.resize( recCount );

	for (uint32_t i_loop = 0; i_loop < recCount; i_loop++) {
		tmp_vec[i_loop].push_back( rrow[COL_domain_id] );
		tmp_vec[i_loop].push_back( rrow[COL_record_name] );
		tmp_vec[i_loop].push_back( rrow[COL_record_type] );
		tmp_vec[i_loop].push_back( s_ttl );
		tmp_vec[i_loop].push_back( m_pool->get_content( i_loop, POOL_content ) );
		tmp_vec[i_loop].push_back( m_pool->get_content( i_loop, POOL_priority ) );

		if ( random_results ) {
			tmp_vec[i_loop].push_back( std::to_string( uint_dist( m_random ) ) );
		}
	}

	if ( random_results )
		sort( tmp_vec.begin(), tmp_vec.end(), Vec_Sorter() );

	if ( num_results > 0 ) {
		int remove = tmp_vec.size() - num_results;
		if ( remove > 0 ) {
			while ( remove ) {
				tmp_vec.pop_back();
				remove--;
			}
		}
	}
	m_results.insert( m_results.end(), tmp_vec.begin(), tmp_vec.end() );
}

void WPMetaBackend::send_from_hash_cache( const uint64_t& pool_house_id, const std::string& query_domain, const Database::row_t& rrow ) {

	string s_ttl = "300";
	uint8_t num_results = 3;

	m_HC.get_hash_ring_datafields( pool_house_id, s_ttl, num_results );

	if ( num_results > 0 ) {
		vector<vector<string> > tmp_vec;
		tmp_vec.resize( num_results );
		map<uint8_t, string> addedIPs;
		string hash_content;

		for (int i_loop = 0; i_loop < num_results; i_loop++) {

			hash_content = m_HC.get_hash_value( pool_house_id, query_domain, addedIPs, (i_loop + 1) );

			tmp_vec[i_loop].push_back( rrow[COL_domain_id] );
			tmp_vec[i_loop].push_back( rrow[COL_record_name] );
			tmp_vec[i_loop].push_back( rrow[COL_record_type] );
			tmp_vec[i_loop].push_back( s_ttl );
			tmp_vec[i_loop].push_back( hash_content );
			tmp_vec[i_loop].push_back( "0" );

			addedIPs[ i_loop + 1 ]= hash_content;
		}
		m_results.insert( m_results.end(), tmp_vec.begin(), tmp_vec.end() );
	}
}

bool WPMetaBackend::prepare_records( const string& query_domain, const Database::result_t& p_results ) {

	m_results.resize( 0 );

	if ( ( p_results.size() > 0 ) && ( 4 != p_results[0].size() ) )
		return false;

	Database::row_t rowPool;
	string sql;

	for ( size_t i_loop = 0; i_loop < p_results.size(); i_loop++ ) {

		uint64_t pool_house_id = 0;
		try {
			pool_house_id = std::stoul( p_results[i_loop][COL_poolhouse_id] );
		}
		catch (const std::exception& ex) {
			L << Logger::Error << "failed to convert poolhouse_id ["
								<< p_results[i_loop][COL_poolhouse_id] << "] : "
								 << ex.what() << endl;
		}

		if ( m_HC.have_valid_cache( pool_house_id ) ) {
#if _DEBUG_CACHE
			L << Logger::Debug << "found poolhouse " << pool_house_id << " in HashCache, serving." << endl;
#endif
			send_from_hash_cache( pool_house_id, query_domain, p_results[i_loop] );
			continue;
		}

		if ( m_PC.have_valid_cache( pool_house_id ) ) {
#if _DEBUG_CACHE
			L << Logger::Debug << "found poolhouse " << pool_house_id << " in PoolCache, serving." << endl;
#endif
			send_from_pool_cache( pool_house_id, p_results[i_loop] );
			continue;
		}

		sql = "SELECT hash_results, random_results, num_results, ttl, content, priority, hashring_weight "
				"FROM poolhouse "
				"INNER JOIN pool USING ( poolhouse_id ) "
				"WHERE ( ( poolhouse_id = " + p_results[i_loop][COL_poolhouse_id] + " ) AND "
				" ( poolhouse.active = 1 ) AND ( pool.active = 1 ) AND ( pool.in_pool = 1 ) );";

		m_db->query( sql );

		if ( ! m_db->get_row( rowPool ) ) {
			L << Logger::Error << "failed to get pool records for poolhouse_id = " << p_results[i_loop][COL_poolhouse_id] << endl;
			continue;
		}

		int hash_results = std::stoi( rowPool[COL_hash_results].c_str() );
		int num_results = std::stoi( rowPool[COL_num_results].c_str() );

		if ( 0 != hash_results ) {
			Hash_Ring *m_HR = new Hash_Ring();
			m_HR->set_ttl( rowPool[COL_ttl] );
			m_HR->set_num_results( num_results );

			do {
				m_HR->add_node( rowPool[COL_content], std::stoi( rowPool[COL_hashring_weight] ) );
			} while ( m_db->get_row( rowPool ) );

			m_HC.freshen_cache( pool_house_id, m_HR );
			send_from_hash_cache( pool_house_id, query_domain, p_results[i_loop] );

		} else {
			int random_results = std::stoi( rowPool[COL_random_results].c_str() );

			Pool *m_IPP = new Pool();
			m_IPP->set_ttl( rowPool[COL_ttl] );
			m_IPP->set_num_results( num_results );
			m_IPP->set_random_results( random_results != 0 );

			do {
				m_IPP->add_records( rowPool[COL_content], rowPool[COL_priority] );
			} while ( m_db->get_row( rowPool ) );

			m_PC.freshen_cache( pool_house_id, m_IPP );
			send_from_pool_cache( pool_house_id, p_results[i_loop] );
		}
	}
	return true;
}

bool WPMetaBackend::list( const string &target, int domain_id ) {

#if _DEBUG_LOOKUP
	L << Logger::Debug << "----- IN LIST -----" << endl;
	L << Logger::Debug << "domain_id  = " << domain_id << endl;
	L << Logger::Debug << "--------------------" << endl;
#endif
	Database::result_t result;
	string sql;

	sql = "SELECT domain_name FROM domain WHERE ( domain_id = " + std::to_string( domain_id ) + " );";

	int i_result = this->query_db( sql, result );

	if( i_result <= 0)
		return false;

	m_origin = result[0][0];

	sql = "SELECT domain_id, record_name, record_type, poolhouse_id "
			"FROM record "
			"WHERE ( ( domain_id = " + std::to_string( domain_id ) + " ) AND "
			"		( record.active = 1 ) );";

	i_result = this->query_db( sql, result );

	if( i_result <= 0)
		return false;
	else
		return ( this->prepare_records( m_origin, result ) );
}

bool WPMetaBackend::getSOA( const string &domain_name, SOAData &soadata, DNSPacket* packet ) {

#if _DEBUG_LOOKUP
	L << Logger::Debug << "----- IN GET SOA -----" << endl;
	L << Logger::Debug << "looking for '" << domain_name << "'" << endl;
	L << Logger::Debug << "----------------------" << endl;
#endif
	string lcase_domain = domain_name;
	std::transform(lcase_domain.begin(), lcase_domain.end(), lcase_domain.begin(), ::tolower);

	if ( m_SC.have_valid_cache( lcase_domain ) ) {
#if _DEBUG_LOOKUP
		L << Logger::Debug << "found in SOACache, serving." << endl;
#endif
		SOA* soa_obj = m_SC.get_object( lcase_domain );

		soadata.domain_id   = soa_obj->domain_id;
		soadata.hostmaster  = soa_obj->host_master;
		soadata.serial      = soa_obj->serial;
		soadata.nameserver  = soa_obj->name_server;
		soadata.refresh     = soa_obj->refresh;
		soadata.retry       = soa_obj->retry;
		soadata.expire      = soa_obj->expire;
		soadata.default_ttl = soa_obj->default_ttl;
		soadata.ttl         = soa_obj->ttl;
		soadata.db          = this;

#if _DEBUG_LOOKUP
		L << Logger::Debug << "soadata.domain_id   = " << soadata.domain_id << endl;
		L << Logger::Debug << "soadata.hostmaster  = " << soadata.hostmaster << endl;
		L << Logger::Debug << "soadata.serial      = " << soadata.serial << endl;
		L << Logger::Debug << "soadata.nameserver  = " << soadata.nameserver << endl;
		L << Logger::Debug << "soadata.refresh     = " << soadata.refresh << endl;
		L << Logger::Debug << "soadata.retry       = " << soadata.retry << endl;
		L << Logger::Debug << "soadata.expire      = " << soadata.expire << endl;
		L << Logger::Debug << "soadata.default_ttl = " << soadata.default_ttl << endl;
		L << Logger::Debug << "soadata.ttl         = " << soadata.ttl << endl;
#endif
		return true;
	}

	Database::result_t result;
	int i_result;
	string sql;

	if ( lcase_domain.find_first_of( "'\\" ) != string::npos )
		lcase_domain = m_db->escape( lcase_domain );

	sql = "SELECT domain_id, host_master, serial_number, name_server,"
			"refresh, retry, expire, default_ttl, ttl "
			"FROM domain "
			"INNER JOIN domainsoa USING ( domainsoa_id ) "
			"WHERE ( ( domain_name = '" + lcase_domain + "') AND "
			"( domain.active = 1 ) AND ( domainsoa.active = 1 ) );";

	i_result = this->query_db( sql, result );

	if( i_result <= 0)
		return false;

	soadata.domain_id   = std::stol( result[0][0].c_str() );
	soadata.hostmaster  = result[0][1].c_str();
	soadata.serial      = std::stol( result[0][2].c_str() );
	soadata.nameserver  = result[0][3].c_str();
	soadata.refresh     = std::stoi( result[0][4].c_str() );
	soadata.retry       = std::stoi( result[0][5].c_str() );
	soadata.expire      = std::stoi( result[0][6].c_str() );
	soadata.default_ttl = std::stoi( result[0][7].c_str() );
	soadata.ttl         = std::stoi( result[0][8].c_str() );

	if ( soadata.ttl < soadata.default_ttl )
		soadata.ttl    = soadata.default_ttl;

	soadata.db         = this;

#if _DEBUG_LOOKUP
	L << Logger::Debug << "soadata.domain_id   = " << soadata.domain_id << endl;
	L << Logger::Debug << "soadata.hostmaster  = " << soadata.hostmaster << endl;
	L << Logger::Debug << "soadata.serial      = " << soadata.serial << endl;
	L << Logger::Debug << "soadata.nameserver  = " << soadata.nameserver << endl;
	L << Logger::Debug << "soadata.refresh     = " << soadata.refresh << endl;
	L << Logger::Debug << "soadata.retry       = " << soadata.retry << endl;
	L << Logger::Debug << "soadata.expire      = " << soadata.expire << endl;
	L << Logger::Debug << "soadata.default_ttl = " << soadata.default_ttl << endl;
	L << Logger::Debug << "soadata.ttl         = " << soadata.ttl << endl;
#endif

	SOA *new_soa = new SOA();
	new_soa->domain_id = soadata.domain_id;
	new_soa->host_master = soadata.hostmaster;
	new_soa->serial = soadata.serial;
	new_soa->name_server = soadata.nameserver;
	new_soa->refresh = soadata.refresh;
	new_soa->retry = soadata.retry;
	new_soa->expire = soadata.expire;
	new_soa->default_ttl = soadata.default_ttl;
	new_soa->ttl = soadata.ttl;

	m_SC.freshen_cache( lcase_domain, new_soa );

	return true;
}

string WPMetaBackend::build_sql_from_lookup_cache( string sql_fields ) {
	vector<string> vec_args;
	string retVal = "";
	size_t pos;
	while( sql_fields.size() )
	{
		pos = sql_fields.find_first_of( "|" );
		if ( pos != string::npos ) {
			vec_args.push_back( sql_fields.substr( 0, pos++ ) );
			sql_fields = sql_fields.substr( pos, sql_fields.size() - 1 );
		} else {
			vec_args.push_back( sql_fields );
			break;
		}
	}
	if ( 4 == vec_args.size() ) {
		if ( "ANY" == vec_args[3] ) {
			retVal = "SELECT domain_id, record_name, record_type, poolhouse_id "
					"FROM record "
					"WHERE ( ( domain_id = " + vec_args[0] + " ) AND "
					"( record_name = '" + vec_args[1] + "' OR record_name = '" + vec_args[2] + "' ) AND "
					"( record.active = 1 ) );";
		} else {
			retVal = "SELECT domain_id, record_name, record_type, poolhouse_id "
					"FROM record "
					"WHERE ( ( domain_id = " + vec_args[0] + " ) AND "
					"( record_name = '" + vec_args[1] + "' OR record_name = '" + vec_args[2] + "' ) AND "
					"( record_type = '" + vec_args[3] + "' ) AND "
					"( record.active = 1 ) );";
		}
	}
	return retVal;
}

bool WPMetaBackend::serve_from_cache( const string& p_search_index, const string& p_org_query ) {

	if ( m_LC.have_valid_cache( p_search_index ) ) {
		string str_sql_fields = m_LC.get_object( p_search_index )->sql_fields;
#if _DEBUG_CACHE
		L << Logger::Debug << "found '" << str_sql_fields << "' in Lookup_Cache, serving." << endl;
#endif
		if ( m_RC.have_valid_cache( str_sql_fields ) ) {
#if _DEBUG_CACHE
			L << Logger::Debug << "found '" << str_sql_fields << "' in Record_Cache, serving." << endl;
#endif
			this->prepare_records( p_org_query, m_RC.get_object( str_sql_fields )->data );
			m_qname = p_org_query;
			return true;
		} else {
#if _DEBUG_CACHE
			L << Logger::Debug << "NOT found in Record_Cache, we have Lookup_Cache : generating results" << endl;
#endif
			string sql = build_sql_from_lookup_cache( str_sql_fields );
			if ( sql.size() > 0 ) {
				Database::result_t result;
				this->query_db( sql, result );

				Records *new_recs = new Records();
				new_recs->data = result;
				m_RC.freshen_cache( str_sql_fields, new_recs );

				this->prepare_records( p_org_query, result );
				m_qname = p_org_query;
				return true;
			} else {
#if _DEBUG_CACHE
				L << Logger::Debug << "something amiss in Lookup_Cache fields, releasing back into the ether." << endl;
#endif
				m_LC.release_cache( p_search_index );
			}
		}
	}
	return false;
}

void WPMetaBackend::lookup( const QType &qtype, const string &domain_name, DNSPacket *p, int domain_id ) {

#if _DEBUG_LOOKUP
	L << Logger::Debug << "----- IN LOOKUP -----" << endl;
	L << Logger::Debug << "record type ='" << qtype.getName() << "'" << endl;
	L << Logger::Debug << "looking for '" << domain_name << "'" << endl;
	L << Logger::Debug << "domain id = " << domain_id << endl;
	L << Logger::Debug << "---------------------" << endl;
#endif
	m_cache_purge_counter++;
	if ( m_cache_purge_counter > CACHE_PURGE_THRESHHOLD ) {
		m_LC.purge_cache();
		m_RC.purge_cache();
		m_cache_purge_counter = 0;
	}

	string lcase_domain = domain_name;
	std::transform(lcase_domain.begin(), lcase_domain.end(), lcase_domain.begin(), ::tolower);

	if ( serve_from_cache( lcase_domain + "|" + qtype.getName(), domain_name ) )
		return;

	string s_domain_id = std::to_string( domain_id );
	Database::row_t rrow;
	Database::result_t result;
	string sql;
	bool found = false;
	m_origin = "";

	if ( lcase_domain.find_first_of( "'\\" ) != string::npos )
		lcase_domain = m_db->escape( lcase_domain );

	if ( domain_id < 0 ) {
		size_t pos = 0;
		string lcase_domain_temp = lcase_domain;

		while ( ( ! lcase_domain_temp.empty() ) && ( string::npos != pos ) ) {
			sql = "SELECT domain_id "
					"FROM domain "
					"WHERE ( ( domain_name = '" + lcase_domain_temp + "' ) AND "
					"( domain.active = 1 ) );";

			this->query_db( sql, result );

			if ( 0 < result.size() ) {
				s_domain_id = result[0][0];
				m_origin = lcase_domain_temp;
				found = true;
				break;
			}
			pos = lcase_domain.find_first_of( ".", pos + 1 );
			lcase_domain_temp = lcase_domain.substr( pos + 1 );
		}
	} else {
		sql = "SELECT domain_name FROM domain WHERE ( domain_id = " + s_domain_id + " );";
		this->query_db( sql, result );

		if ( 0 == result.size() ) {
			throw AhuException( "Unable to find records for domain_id = " + s_domain_id );
		}
		found = true;
		m_origin = result[0][0];
	}

	if ( found ) {
#if _DEBUG_LOOKUP
		L << Logger::Debug << "----- IN FOUND -----" << endl;
		L << Logger::Debug << "with domain '" << domain_name << "'" << endl;
		L << Logger::Debug << "m_origin  '" << m_origin << "'" << endl;
		L << Logger::Debug << "---------------------" << endl;
#endif
		string host;

		if ( domain_name.length() == m_origin.length() ) {
			host = "";
		} else {
			host = domain_name.substr( 0, ( domain_name.length() - m_origin.length() ) - 1 );
			if ( host.find_first_of( "'\\" ) != string::npos )
				host = m_db->escape( host );
		}

		if ( '.' == host[ host.length() - 1 ] )
			host.erase( host.length() - 1 );

		size_t pos = 0;
		string lcase_domain_temp = lcase_domain;
		string host_temp = host;
		found = false;

		while ( ( lcase_domain_temp.length() >= m_origin.length() ) && ( string::npos != pos ) ) {

			if ( "ANY" == qtype.getName() ) {
				sql = "SELECT domain_id, record_name, record_type, poolhouse_id "
						"FROM record "
						"WHERE ( ( domain_id = " + s_domain_id + " ) AND "
						"( record_name = '" + host_temp + "' OR record_name = '" + lcase_domain_temp + "') AND "
						"( record.active = 1 ) );";
			} else {
				sql = "SELECT domain_id, record_name, record_type, poolhouse_id "
						"FROM record "
						"WHERE ( ( domain_id = " + s_domain_id + " ) AND "
						"( record_name = '" + host_temp + "' OR record_name = '" + lcase_domain_temp + "') AND "
						"( record_type = '" + qtype.getName() + "' ) AND "
						"( record.active = 1 ) );";
			}

			this->query_db( sql, result );

			if ( result.size() > 0 ) {
				found = true;
				break;
			}

			pos = lcase_domain.find_first_of( ".", pos + 1 );
			lcase_domain_temp = lcase_domain.substr( pos + 1 );
			if ( ( lcase_domain_temp.length() - m_origin.length() ) > 0 )
				host_temp = lcase_domain_temp.substr( 0, ( lcase_domain_temp.length() - m_origin.length() ) - 1 );
			else
				host_temp = "*";

			host_temp = ( ( "*" == host_temp ) ? "" : "*.")  + host_temp;
			lcase_domain_temp = "*." + lcase_domain_temp;
		}

		Lookup *new_lookup = new Lookup();

		if ( found )
			new_lookup->sql_fields = s_domain_id + "|" + host_temp + "|" + lcase_domain_temp + "|" + qtype.getName();
		else
			new_lookup->sql_fields = s_domain_id + "|" + host + "|" + lcase_domain + "|" + qtype.getName();

		m_LC.freshen_cache( lcase_domain + "|" + qtype.getName(), new_lookup );
#if _DEBUG_LOOKUP
		L << Logger::Debug << "added " << new_lookup->sql_fields << " into Lookup_Cache for '" << lcase_domain + "|" + qtype.getName() << "'" << endl;
#endif
		Records *new_recs = new Records();
		new_recs->data = result;
		m_RC.freshen_cache( new_lookup->sql_fields, new_recs );
#if _DEBUG_LOOKUP
		L << Logger::Debug << "added " << result.size() << " records into Record_Cache for '" << new_lookup->sql_fields << "'" << endl;
#endif
		this->prepare_records( lcase_domain, result );

		m_qname = domain_name;
	}
}

bool WPMetaBackend::get( DNSResourceRecord &rr ) {

#if _DEBUG_LOOKUP
	L << Logger::Debug << "in get() : have " << m_results.size() << " records." << endl;
#endif
	if ( m_results.size() == 0 ) {
		return false;
	}

	rr.content = m_results[0][VEC_content];
	rr.qtype   = m_results[0][VEC_record_type];

	if( ! m_qname.empty() ) {
		rr.qname = m_qname;
	} else {
		rr.qname = m_results[0][VEC_record_name];

		if ( rr.qname.length() < m_origin.length() ) {
			rr.qname += ".";
			rr.qname += m_origin;
		}
	}

	rr.ttl           = std::stoi( m_results[0][VEC_ttl].c_str() );
	rr.priority      = std::stoi( m_results[0][VEC_priority].length() == 0 ? "0" : m_results[0][VEC_priority] );
	rr.domain_id     = std::stoi( m_results[0][VEC_domain_id].c_str() );
	rr.last_modified = 0;

	if ( rr.ttl < MINIMUM_RECORD_TTL )
		rr.ttl       = MINIMUM_RECORD_TTL;

#if _DEBUG_LOOKUP
	L << Logger::Debug << "rr.content       = " << rr.content << endl;
	L << Logger::Debug << "rr.qtype         = " << rr.qtype.getName() << endl;
	L << Logger::Debug << "rr.qname         = " << rr.qname << endl;
	L << Logger::Debug << "rr.ttl           = " << rr.ttl << endl;
	L << Logger::Debug << "rr.priority      = " << rr.priority << endl;
	L << Logger::Debug << "rr.domain_id     = " << rr.domain_id << endl;
#endif

	m_results.erase( m_results.begin() );
	return true;
}

class WPMetaFactory : public BackendFactory {

public:
	WPMetaFactory() : BackendFactory( "wpmeta" ) {}

	void declareArguments(const string &suffix = "") {
		declare(suffix, "dbname", "Database name to connect to", "metadns" );
		declare(suffix, "user",  "DB user to connect as", "metadns" );
		declare(suffix, "host", "Host to connect to", "127.0.0.1" );
		declare(suffix, "port", "Port to connect to", "3306" );
		declare(suffix, "password", "DB password to connect with", "123456" );
		declare(suffix, "socket", "DB socket to connect to", "" );
	}

	WPMetaBackend *make( const string &suffix = "" ) {
		return new WPMetaBackend( suffix );
	}
};

class WPMetaLoader {

public:
	WPMetaLoader() {
		BackendMakers().report( new WPMetaFactory() );
		L << Logger::Info << backend_name << " This is the WordPress Meta DNS bootstrapping ("
			 << __DATE__ << ", " << __TIME__ << ")" << endl;
	}
};

static WPMetaLoader wpMetaloader;

