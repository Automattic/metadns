/*
 * WordPress.com MetaDNS
 */

#ifndef __H_WPMETABACKEND_HH__
#define __H_WPMETABACKEND_HH__

#include <cstring>
#include <vector>
#include <random>
#include <map>
#include <chrono>

#include <pdns/namespaces.hh>
#include <pdns/dnsbackend.hh>
#include <pdns/dnspacket.hh>

#include "defines.h"
#include "database.hh"
#include "hashring.hh"
#include "hashcache.hh"
#include "poolcache.hh"
#include "cache.hh"

typedef std::mt19937 oRandom;
std::uniform_int_distribution<uint32_t> uint_dist;

struct Vec_Sorter {
	bool operator()( const std::vector<string>& lhs, const std::vector<string>& rhs )
	{
		return ( lhs[VEC_random_value] < rhs[VEC_random_value] );
	}
};

class WPMetaBackend : public DNSBackend {

public:
	WPMetaBackend( const string &suffix = "" );
	~WPMetaBackend();
	WPMetaBackend *parent;

	void lookup( const QType &qtype, const string &domain_name, DNSPacket *packet = NULL, int domain_id = -1 );
	bool list( const string &target, int domain_id );
	bool get( DNSResourceRecord &r );
	bool getSOA( const string &domain_name, SOAData &soadata, DNSPacket* packet );

private:
	Database *m_db;

	Hash_Cache m_HC;
	Pool_Cache m_PC;

	Cache<std::string, SOA> m_SC;
	Cache<std::string, Records> m_RC;
	Cache<std::string, Lookup> m_LC;

	vector<vector<std::string> > m_results;
	oRandom m_random;

	string m_qname;
	string m_origin;
	uint32_t m_cache_purge_counter;

	int query_db( const string& p_sql );
	int query_db( const string& p_sql, Database::result_t& result );
	bool prepare_records( const string& query_domain, const Database::result_t& p_results  );
	void send_from_hash_cache( const uint64_t& pool_house_id, const std::string& query_domain, const Database::row_t& rrow );
	void send_from_pool_cache( const uint64_t& pool_house_id, const Database::row_t& rrow );
	bool serve_from_cache( const string& lcase_domain_name, const string& lcase_org_query );
	string build_sql_from_lookup_cache( string sqlfields );

};

#endif  // __H_WPMETABACKEND_HH__

