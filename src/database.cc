
#include "database.hh"

using namespace std;

pthread_mutex_t Database::connect_mutex = PTHREAD_MUTEX_INITIALIZER;

Database::Database( const string &p_database, const string &p_host, uint16_t p_port, const string &p_socket,
					const string &p_user, const string &p_password ) : m_results( NULL ) {
	m_host = p_host;
	m_user = p_user;
	m_password = p_password;
	m_database = p_database;
	m_port = p_port;
	m_socket = p_socket;
}

Database::~Database() {
	this->disconnect();
	mysql_library_end();
}

bool Database::connect() {
	pthread_mutex_lock(&connect_mutex);
	if ( mysql_library_init( 0, NULL, NULL ) ) {
		throw AhuException( "Unable to initialise the MySQL library" );
		return false;
	}

	mysql_init( &m_conn );

	if ( NULL == &m_conn ) {
		throw AhuException( "Unable to initialise the MySQL connection object" );
		return false;
	}

	my_bool reconnect = 1;
	int conn_timeout = CONNECTION_TIMEOUT;

	mysql_options( &m_conn, MYSQL_OPT_RECONNECT, &reconnect );
	mysql_options( &m_conn, MYSQL_OPT_READ_TIMEOUT, &conn_timeout );
	mysql_options( &m_conn, MYSQL_OPT_WRITE_TIMEOUT, &conn_timeout );

	if ( NULL == mysql_real_connect(&m_conn, m_host.empty() ? NULL : m_host.c_str(),
								  m_user.empty() ? NULL : m_user.c_str(),
								  m_password.empty() ? NULL : m_password.c_str(),
								  m_database.empty() ? NULL : m_database.c_str(),
								  m_port,
								  m_socket.empty() ? NULL : m_socket.c_str(),
								  CLIENT_MULTI_RESULTS) ) {
		throw AhuException( "Unable to connect to the database" );
	}
	pthread_mutex_unlock( &connect_mutex );
	return true;
}

void Database::disconnect() {
	mysql_close( &m_conn );
}

int Database::query( const string &s_query ) {
	if ( NULL != m_results )
		throw AhuException( "Unable to start a new query, as there are still un-collected records." );

	int err;
	if ( ( err = mysql_query( &m_conn, s_query.c_str() ) ) )
		throw AhuException( "call to mysql_query failed: " + std::to_string( err ) );

	return 0;
}

int Database::query( const string &s_query, result_t &result ) {
	result.clear();
	this->query( s_query );
	row_t row;

	while ( get_row( row ) )
		result.push_back( row );

	return result.size();
}

bool Database::get_row( row_t &row ) {
	row.clear();
	if ( ! m_results )
		if ( ! ( m_results = mysql_use_result( &m_conn ) ) )
			throw AhuException( "Failed on mysql_use_result" );

	MYSQL_ROW rrow;

	if ( ( rrow = mysql_fetch_row( m_results ) ) ) {
		for ( unsigned int iLoop = 0; iLoop < mysql_num_fields( m_results ); iLoop++)
			row.push_back( rrow[iLoop] ? : "" );
		return true;
	}

	mysql_free_result( m_results );

	while ( 0 == mysql_next_result( &m_conn ) ) {
		if ( ( m_results = mysql_use_result( &m_conn ) ) )
			mysql_free_result( m_results );
	}

	m_results = NULL;
	return false;
}

string Database::escape( const string &s_parse ) {
	string retVal;

	for ( string::const_iterator it = s_parse.begin(); it != s_parse.end(); ++it) {
		if ( ( *it == '\'' ) || ( *it == '\\' ) )
			retVal += '\\';

		retVal += *it;
	}

	return retVal;
}

