/*
 * File:	Random.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Random.h"
#include <stdexcept>

#ifdef WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0400
	#endif
	#include <windows.h>
	#include <wincrypt.h>
#else	// WIN32
	#include <errno.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif	// WIN32



#ifdef WIN32

MicrosoftCryptoProvider::MicrosoftCryptoProvider()
{
	if(!CryptAcquireContext(&m_hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		throw std::runtime_error("CryptAcquireContext");
	}
}

MicrosoftCryptoProvider::~MicrosoftCryptoProvider()
{
	CryptReleaseContext(m_hProvider, 0);
}

#endif	// WIN32

RandomNumberGenerator::RandomNumberGenerator()
{
#ifndef WIN32
	m_fd = open("/dev/urandom",O_RDONLY);
	if (m_fd == -1)
	{
		throw std::runtime_error("open /dev/urandom");
	}
#endif	// WIN32
}

RandomNumberGenerator::~RandomNumberGenerator()
{
#ifndef WIN32
	close(m_fd);
#endif	// WIN32
}

uint8_t RandomNumberGenerator::generateByte()
{
	uint8_t result;
	generateBlock(&result, 1);
	return result;
}

void RandomNumberGenerator::generateBlock(uint8_t * output, unsigned count)
{
#ifdef WIN32
#	ifdef WORKAROUND_MS_BUG_Q258000
		static MicrosoftCryptoProvider m_provider;
#	endif
	if (!CryptGenRandom(m_provider.GetProviderHandle(), count, output))
	{
		throw std::runtime_error("CryptGenRandom");
	}
#else	// WIN32
	if (read(m_fd, output, count) != count)
	{
		throw std::runtime_error("read /dev/urandom");
	}
#endif	// WIN32
}


