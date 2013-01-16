/*
 * File:	RijndaelCBCMAC.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "RijndaelCBCMAC.h"
#include "rijndael.h"
#include <assert.h>
#include "Logging.h"

void logHexArray(Logger::log_level_t level, const uint8_t * bytes, unsigned count);

//! \param key The key to use as the CBC-MAC secret.
//! \param iv Initialization vector. Defaults to zero if not provided.
RijndaelCBCMAC::RijndaelCBCMAC(const AESKey<128> & key, const uint8_t * iv)
:	m_key(key)
{
	if (iv)
	{
		memcpy(m_mac, iv, sizeof(m_mac));
	}
	else
	{
		memset(m_mac, 0, sizeof(m_mac));
	}
}

//! \param data Pointer to data to process.
//! \param length Number of bytes to process. Must be evenly divisible by #BLOCK_SIZE.
void RijndaelCBCMAC::update(const uint8_t * data, unsigned length)
{
	assert(length % BLOCK_SIZE == 0);
	unsigned blocks = length / BLOCK_SIZE;
	while (blocks--)
	{
		updateOneBlock(data);
		data += BLOCK_SIZE;
	}
}

//! It appears that some forms of CBC-MAC encrypt the final output block again in
//! order to protect against a plaintext attack. This method is a placeholder for
//! such an operation, but it currently does nothing.
void RijndaelCBCMAC::finalize()
{
}

//! On entry the current value of m_mac becomes the initialization vector
//! for the CBC encryption of this block. The output of the encryption then
//! becomes the new MAC, which is stored in m_mac.
void RijndaelCBCMAC::updateOneBlock(const uint8_t * data)
{
	Rijndael cipher;
	cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_key, Rijndael::Key16Bytes, m_mac);
	cipher.blockEncrypt(data, BLOCK_SIZE * 8, m_mac);	// size is in bits
	
//	Log::log(Logger::DEBUG2, "CBC-MAC output block:\n");
//	logHexArray(Logger::DEBUG2, (const uint8_t *)&m_mac, sizeof(m_mac));
}

/*!
 * \brief Log an array of bytes as hex.
 */
void logHexArray(Logger::log_level_t level, const uint8_t * bytes, unsigned count)
{
	Log::SetOutputLevel leveler(level);
//		Log::log("    ");
	unsigned i;
	for (i = 0; i < count; ++i, ++bytes)
	{
		if ((i % 16 == 0) && (i < count - 1))
		{
			if (i != 0)
			{
				Log::log("\n");
			}
			Log::log("    0x%04x: ", i);
		}
		Log::log("%02x ", *bytes & 0xff);
	}
	
	Log::log("\n");
}

