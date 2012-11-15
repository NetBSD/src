/*
 * File:	AESKey.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_AESKey_h_)
#define _AESKey_h_

#include "stdafx.h"
#include <string.h>
#include <iostream>
#include "Random.h"

//! An AES-128 key is 128 bits, or 16 bytes.
typedef uint8_t aes128_key_t[16];

/*!
 * \brief Base class for AESKey<S>.
 *
 * This class implements some bigger, non-template methods used in the
 * AESKey<S> templated subclass.
 */
class AESKeyBase
{
public:
	//! \brief Reads hex encoded data from \a stream.
	void _readFromStream(std::istream & stream, unsigned bytes, uint8_t * buffer);
	
	//! \brief Writes hex encoded data to \a stream.
	void _writeToStream(std::ostream & stream, unsigned bytes, const uint8_t * buffer);
};

/*!
 * \brief Generic AES key class.
 *
 * The template parameter \a S is the number of bits in the key.
 *
 * The underlying key type can be accessed like this: AESKey<128>::key_t
 *
 * When a key instance is destroyed, it erases the key data by setting it
 * to all zeroes.
 *
 * \todo Add a way to allow only key sizes of 128, 192, and 256 bits.
 * \todo Find a cross platform way to prevent the key data from being written
 *		to the VM swapfile.
 *
 * AESKey<128> key = AESKey<128>::readFromStream(s);
 */
template <int S>
class AESKey : public AESKeyBase
{
public:
	//! Type for this size of AES key.
	typedef uint8_t key_t[S/8];
	
public:
	//! \brief Default constructor.
	//!
	//! Initializes the key to 0.
	AESKey()
	{
		memset(m_key, 0, sizeof(m_key));
	}
	
	//! \brief Constructor taking a key value reference.
	AESKey(const key_t & key)
	{
		memcpy(m_key, &key, sizeof(m_key));
	}
	
	// \brief Constructor taking a key value pointer.
	AESKey(const key_t * key)
	{
		memcpy(m_key, key, sizeof(m_key));
	}
	
	//! \brief Constructor, reads key from stream in hex format.
	AESKey(std::istream & stream)
	{
		readFromStream(stream);
	}
	
	//! \brief Copy constructor.
	AESKey(const AESKey<S> & other)
	{
		memcpy(m_key, other.m_key, sizeof(m_key));
	}
	
	//! \brief Destructor.
	//!
	//! Sets the key value to zero.
	~AESKey()
	{
		memset(m_key, 0, sizeof(m_key));
	}
	
	//! \brief Set to the key to a randomly generated value.
	void randomize()
	{
		RandomNumberGenerator rng;
		rng.generateBlock(m_key, sizeof(m_key));
	}
	
	//! \brief Reads the key from a hex encoded data stream.
	void readFromStream(std::istream & stream)
	{
		_readFromStream(stream, S/8, reinterpret_cast<uint8_t*>(&m_key));
	}
	
	//! \brief Writes the key to a data stream in hex encoded format.
	void writeToStream(std::ostream & stream)
	{
		_writeToStream(stream, S/8, reinterpret_cast<uint8_t*>(&m_key));
	}
	
	//! \name Key accessors
	//@{
	inline const key_t & getKey() const { return m_key; }
	inline void getKey(key_t * key) const { memcpy(key, m_key, sizeof(m_key)); }
	
	inline void setKey(const key_t & key) { memcpy(m_key, &key, sizeof(m_key)); }
	inline void setKey(const key_t * key) { memcpy(m_key, key, sizeof(m_key)); }
	inline void setKey(const AESKey<S> & key) { memcpy(m_key, key.m_key, sizeof(m_key)); }
	//@}
	
	//! \name Operators
	//@{
	const AESKey<S> & operator = (const AESKey<S> & key) { setKey(key); return *this; }
	const AESKey<S> & operator = (const key_t & key) { setKey(key); return *this; }
	const AESKey<S> & operator = (const key_t * key) { setKey(key); return *this; }
	
	operator const key_t & () const { return m_key; }
	operator const key_t * () const { return m_key; }
	//@}
	
protected:
	key_t m_key;	//!< The key value.
};

//! Standard type definition for an AES-128 key.
typedef AESKey<128> AES128Key;

#endif // _AESKey_h_
