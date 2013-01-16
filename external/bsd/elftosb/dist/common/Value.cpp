/*
 * File:	Value.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Value.h"

using namespace elftosb;

//! Returns a varying size depending on the word size attribute.
//!
size_t SizedIntegerValue::getSize() const
{
	switch (m_size)
	{
		case kWordSize:
			return sizeof(uint32_t);
		case kHalfWordSize:
			return sizeof(uint16_t);
		case kByteSize:
			return sizeof(uint8_t);
	}
	return kWordSize;
}

//! The resulting mask can be used to truncate the integer value to be
//! certain it doesn't extend beyond the associated word size.
uint32_t SizedIntegerValue::getWordSizeMask() const
{
	switch (m_size)
	{
		case kWordSize:
			return 0xffffffff;
		case kHalfWordSize:
			return 0x0000ffff;
		case kByteSize:
			return 0x000000ff;
	}
	return 0;
}

