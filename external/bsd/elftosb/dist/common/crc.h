/*
 * File:	crc.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_crc_h_)
#define CRYPTOPP_CRC32_H

#include "stdafx.h"

const uint32_t CRC32_NEGL = 0xffffffffL;

#ifdef __LITTLE_ENDIAN__
	#define CRC32_INDEX(c) (c & 0xff)
	#define CRC32_SHIFTED(c) (c >> 8)
#else
	#define CRC32_INDEX(c) (c >> 24)
	#define CRC32_SHIFTED(c) (c << 8)
#endif

//! CRC Checksum Calculation
class CRC32
{
public:
	enum
	{
		DIGESTSIZE = 4
	};
	
	CRC32();
	
	void update(const uint8_t * input, unsigned length);
	
	void truncatedFinal(uint8_t * hash, unsigned size);

	void updateByte(uint8_t b) { m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc); }
	uint8_t getCrcByte(unsigned i) const { return ((uint8_t *)&(m_crc))[i]; }

private:
	void reset() { m_crc = CRC32_NEGL; m_count = 0; }
	
	static const uint32_t m_tab[256];
	uint32_t m_crc;
    unsigned m_count;
};

#endif // _crc_h_
