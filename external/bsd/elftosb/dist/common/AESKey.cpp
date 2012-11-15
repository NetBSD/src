/*
 * File:	AESKey.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "AESKey.h"
#include <stdexcept>
#include "smart_ptr.h"
#include "HexValues.h"
#include <ctype.h>

//! The data from the stream is expected to be hex encoded. Each two characters
//! from the stream encode a single result byte. All non-hexadecimal characters
//! are ignored, including newlines. Every two hexadecimal characters form
//! an encoded byte. This is true even if the two characters representing the
//! upper and lower nibbles are separated by whitespace or other characters.
//!
//! \post The stream read head is left pointing just after the last encoded byte.
//!
//! \param stream Input stream to read from.
//! \param bytes Number of encoded bytes to read. This is the number of \e
//!		result bytes, not the count of bytes to read from the stream.
//! \param[out] buffer Pointer to the buffer where decoded data is written.
//!
//! \exception std::runtime_error This exception will be thrown if less
//!		data than required is available from \a stream, or if some other
//!		error occurs while reading from \a stream.
void AESKeyBase::_readFromStream(std::istream & stream, unsigned bytes, uint8_t * buffer)
{
	char temp[2];
	char c;
	char n = 0;
	
	while (bytes)
	{
		if (stream.get(c).fail())
		{
			throw std::runtime_error("not enough data in stream");
		}
		
		if (isHexDigit(c))
		{
			temp[n++] = c;
			if (n == 2)
			{
				*buffer++ = hexByteToInt(temp);
				bytes--;
				n = 0;
			}
		}
	}
}

//! Key data is written to \a stream as a sequence of hex encoded octets, each two
//! characters long. No spaces or newlines are inserted between the encoded octets
//! or at the end of the sequence.
//!
//! \exception std::runtime_error Thrown if the \a stream reports an error while
//!		writing the key data.
void AESKeyBase::_writeToStream(std::ostream & stream, unsigned bytes, const uint8_t * buffer)
{
	const char hexChars[] = "0123456789ABCDEF";
	while (bytes--)
	{
		uint8_t thisByte = *buffer++;
		char byteString[2];
		byteString[0] = hexChars[(thisByte & 0xf0) >> 4];
		byteString[1] = hexChars[thisByte & 0x0f];
		if (stream.write(byteString, 2).bad())
		{
			throw std::runtime_error("error while writing to stream");
		}
	}
}


