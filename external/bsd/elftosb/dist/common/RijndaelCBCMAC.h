/*
 * File:	RijndaelCBCMAC.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_RijndaelCBCMAC_h_)
#define _RijndaelCBCMAC_h_

#include "AESKey.h"
#include <string.h>

/*!
 * \brief Class to compute CBC-MAC using the AES/Rijndael cipher.
 *
 * Currently only supports 128-bit keys and block sizes.
 */
class RijndaelCBCMAC
{
public:
	enum
	{
		BLOCK_SIZE = 16	//!< Number of bytes in one cipher block.
	};
	
	//! The cipher block data type.
	typedef uint8_t block_t[BLOCK_SIZE];
	
public:
	//! \brief Default constructor.
	//!
	//! The key and IV are both set to zero.
	RijndaelCBCMAC() {}
	
	//! \brief Constructor.
	RijndaelCBCMAC(const AESKey<128> & key, const uint8_t * iv=0);
	
	//! \brief Process data.
	void update(const uint8_t * data, unsigned length);
	
	//! \brief Signal that all data has been processed.
	void finalize();
	
	//! \brief Returns a reference to the current MAC value.
	const block_t & getMAC() const { return m_mac; }
	
	//! \brief Assignment operator.
	RijndaelCBCMAC & operator = (const RijndaelCBCMAC & other)
	{
		m_key = other.m_key;
		memcpy(m_mac, other.m_mac, sizeof(m_mac));
		return *this;
	}
	
protected:
	AESKey<128> m_key;	//!< 128-bit key to use for the CBC-MAC.
	block_t m_mac;	//!< Current message authentication code value.
	
	void updateOneBlock(const uint8_t * data);
};

#endif // _RijndaelCBCMAC_h_
