
#ifndef __CACHE_H__
#define __CACHE_H__

#include <cstdint>
#include <map>
#include <ctime>
#include <chrono>

#include <pdns/namespaces.hh>
#include <pdns/dnsbackend.hh>
#include <pdns/dnspacket.hh>

#include "soa.hh"
#include "lookup.hh"
#include "records.hh"
#include "pool.hh"
#include "hashring.hh"

template<class IndexType, class ObjectType>
class Cache {

public:
	typedef std::map<IndexType, ObjectType*> Node_Map;
	typedef typename Node_Map::iterator iterator;
	typedef typename Node_Map::const_iterator const_iterator;

	Cache() : m_current_item( NULL ) { }
	~Cache();

	virtual bool have_valid_cache( const IndexType& p_index );
	void freshen_cache( const IndexType& p_index, ObjectType* p_object );
	ObjectType* get_object( const IndexType& p_index );
	void release_cache( const IndexType& p_index ) { Cache::remove( p_index ); }
	void purge_cache();

protected:
	Node_Map m_items;

private:
	ObjectType* m_current_item;

	void add( const IndexType& p_index, ObjectType* p_object );
	void update( const IndexType& p_index, ObjectType* p_object );
	void remove( const IndexType& p_index );
};

/* Template Functions */

template <class IndexType, class ObjectType>
Cache<IndexType, ObjectType>::~Cache() {
	for ( auto itr = m_items.begin(), end = m_items.end(); itr != end; itr++  ) {
		ObjectType* tmp_HR = itr->second;
		delete tmp_HR;
	}
}

template <class IndexType, class ObjectType>
void Cache<IndexType, ObjectType>::purge_cache() {
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	uint64_t last_mod;

	for ( iterator itr = m_items.begin(); itr != m_items.end(); ) {
		last_mod = std::chrono::duration_cast<std::chrono::seconds>( now - itr->second->get_last_modified() ).count();

		if ( typeid( ObjectType ) == typeid( Lookup& ) ) {
			if ( last_mod > CACHE_LOOKUP_TIMEOUT ) {
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}else if ( typeid( ObjectType ) == typeid( Records& ) ) {
			if ( last_mod > CACHE_RECORDS_TIMEOUT ) {
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}else if ( typeid( ObjectType ) == typeid( SOA& ) ) {
			if ( last_mod > CACHE_SOA_TIMEOUT ) {
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}else if ( typeid( ObjectType ) == typeid( Pool& ) ) {
			if ( last_mod > CACHE_POOL_TIMEOUT ) {
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}else if ( typeid( ObjectType ) == typeid( Hash_Ring& ) ) {
			if ( last_mod > CACHE_HASH_TIMEOUT ){
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}else {
			if ( last_mod > CACHE_DEFAULT_TIMEOUT ) {
				if ( NULL != itr->second ) {
					delete itr->second;
				}
				itr = m_items.erase( itr );
			} else
				itr++;
		}
	}
}

template <class IndexType, class ObjectType>
bool Cache<IndexType, ObjectType>::have_valid_cache( const IndexType& p_index ) {
	m_current_item = NULL;
	m_current_item = get_object( p_index );

	if ( NULL == m_current_item ) {
		return false;
	} else {
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		uint64_t last_mod = std::chrono::duration_cast<std::chrono::seconds>
							( now - m_current_item->get_last_modified() ).count();

		if ( typeid( ObjectType ) == typeid( SOA& ) ) {
			return ( last_mod < CACHE_SOA_TIMEOUT );
		}else if ( typeid( ObjectType ) == typeid( Lookup& ) ) {
			return ( last_mod < CACHE_LOOKUP_TIMEOUT );
		}else if ( typeid( ObjectType ) == typeid( Records& ) ) {
			return ( last_mod < CACHE_RECORDS_TIMEOUT );
		}else if ( typeid( ObjectType ) == typeid( Pool& ) ) {
			return ( last_mod < CACHE_POOL_TIMEOUT );
		}else if ( typeid( ObjectType ) == typeid( Hash_Ring& ) ) {
			return ( last_mod < CACHE_HASH_TIMEOUT );
		}else {
			return ( last_mod < CACHE_DEFAULT_TIMEOUT );
		}
	}
}

template <class IndexType, class ObjectType>
void Cache<IndexType, ObjectType>::freshen_cache( const IndexType& p_index, ObjectType* p_object ) {

	ObjectType* tmp_IPP = get_object( p_index );

	if ( NULL != tmp_IPP )
		update( p_index, p_object );
	else
		add( p_index, p_object );

	p_object->touch();
}

template <class IndexType, class ObjectType>
void Cache<IndexType, ObjectType>::add( const IndexType& p_index, ObjectType* p_object ) {
	m_items[ p_index ] = p_object;
}

template <class IndexType, class ObjectType>
void Cache<IndexType, ObjectType>::update( const IndexType& p_index, ObjectType* p_object ) {

	this->remove( p_index );
	this->add( p_index, p_object );
}

template <class IndexType, class ObjectType>
void Cache<IndexType, ObjectType>::remove( const IndexType& p_index ) {
	iterator it = m_items.find( p_index );

	if ( it != m_items.end() ) {
		ObjectType *tmpPtr = it->second;
		m_items.erase( it );
		delete tmpPtr;
	}

	m_current_item = NULL;
}

template <class IndexType, class ObjectType>
ObjectType* Cache<IndexType, ObjectType>::get_object( const IndexType& p_index ) {

	if ( NULL == m_current_item ) {
		const_iterator it = m_items.find( p_index );

		if ( it != m_items.end() )
			m_current_item = it->second;
	}
	return m_current_item;
}

#endif	//__CACHE_H__
