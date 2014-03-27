
#ifndef __CRC32_H__
#define __CRC32_H__

#include <cstdlib>
#include <cstdint>

const std::uint32_t CRC32_NEGL = 0xffffffffL;

class CRC32 {

public:
	CRC32() { m_crc = CRC32_NEGL; };
	~CRC32() { };

	inline CRC32& operator=( const CRC32& rhs );

	std::uint32_t generate_crc( const char *startPtr, size_t len);
	inline void reset() { m_crc = CRC32_NEGL; }

private:
	static const std::uint32_t crc_32_tab[256];
	std::uint32_t m_crc;
};

#endif	//__CRC32_H__
