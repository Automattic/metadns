/*
 * MySQL Database class
 */

#ifndef __DATABASE_HH__
#define __DATABASE_HH__

#include <cstdint>
#include <string>
#include <vector>
#include <pthread.h>

#include <mysql.h>
#include <pdns/dnsbackend.hh>

#include "defines.h"

class Database {

public:
	typedef std::vector<std::string> row_t;
	typedef std::vector<row_t> result_t;

	Database( const std::string &p_database, const std::string &p_host = "", std::uint16_t p_port = 0,
			const std::string &p_socket = "",const std::string &p_user = "", const std::string &p_password = "" );
	~Database();

	int query( const std::string &s_query, result_t &result );
	int query( const std::string &s_query );
	bool get_row( row_t &row );

	bool connect();
	void disconnect();

	std::string escape( const std::string &s_parse );

private:
	MYSQL m_conn;
	MYSQL_RES *m_results;
	static pthread_mutex_t connect_mutex;

	std::string m_database;
	std::string m_host;
	std::uint16_t m_port;
	std::string m_socket;
	std::string m_user;
	std::string m_password;
};

#endif	// __DATABASE_HH__
