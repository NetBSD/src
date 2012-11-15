/*
 * File:	Random.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_Random_h_)
#define _Random_h_

#include "stdafx.h"

#ifdef WIN32
/*!
 * This class is from the crypto++ library.
 */
class MicrosoftCryptoProvider
{
public:
	MicrosoftCryptoProvider();
	~MicrosoftCryptoProvider();
#if defined(_WIN64)
	typedef unsigned __int64 ProviderHandle;	// type HCRYPTPROV, avoid #include <windows.h>
#else
	typedef unsigned long ProviderHandle;
#endif
	ProviderHandle GetProviderHandle() const {return m_hProvider;}
private:
	ProviderHandle m_hProvider;
};

#pragma comment(lib, "advapi32.lib")
#endif	// WIN32

/*!
 * Encapsulates the Windows CryptoAPI's CryptGenRandom or /dev/urandom on Unix systems.
 */
class RandomNumberGenerator
{
public:
	RandomNumberGenerator();
	~RandomNumberGenerator();
	
	uint8_t generateByte();
	void generateBlock(uint8_t * output, unsigned count);

protected:
#ifdef WIN32
#	ifndef WORKAROUND_MS_BUG_Q258000
		MicrosoftCryptoProvider m_provider;
#	endif
#else	// WIN32
	int m_fd;
#endif	// WIN32
};


#endif // _Random_h_
