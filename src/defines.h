
// Cache Defines
#define CACHE_HASH_TIMEOUT      60		// seconds to cache hash database look ups to a particular poolhouse ID
#define CACHE_POOL_TIMEOUT      60		// time out on pool cache database look ups
#define CACHE_SOA_TIMEOUT       300		// seconds to cache the results for SOA, these hardly ever change
#define CACHE_LOOKUP_TIMEOUT	300		// time to cache the results for individual lookups, these don't change often
#define CACHE_RECORDS_TIMEOUT	60		// seconds to cache the record results for deterministic SQL.
#define CACHE_DEFAULT_TIMEOUT	60		// default timeout in the template class
#define CACHE_PURGE_THRESHHOLD	100000	// number of lookups to perform before running cache purge on outdated cache items
#define NUM_BUCKETS             17		// low prime factor of UINT32_MAX, used to create v-nodes/buckets to ensure
										// similar sub domain names are more evenly distributed amongst the server nodes
#define BUCKET_SIZE				( UINT32_MAX / NUM_BUCKETS )

// MySQL Constants
#define CONNECTION_TIMEOUT		10
#define MINIMUM_RECORD_TTL		0

// Result Set Defines
#define RECORD_COLS				8

#define COL_domain_id			0
#define COL_record_name			1
#define COL_record_type			2
#define COL_poolhouse_id		3

#define COL_hash_results		0
#define COL_random_results		1
#define COL_num_results			2
#define COL_ttl					3
#define COL_content				4
#define COL_priority			5
#define COL_hashring_weight		6

#define VEC_domain_id			0
#define VEC_record_name			1
#define VEC_record_type			2
#define VEC_ttl					3
#define VEC_content				4
#define VEC_priority			5
#define VEC_random_value		6

#define POOL_content			0
#define POOL_priority			1

// Debug Message Defines
#define _DEBUG_LOOKUP			0
#define _DEBUG_CACHE			0

