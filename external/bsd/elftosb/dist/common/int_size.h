/*
 * File:	int_size.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_int_size_h_)
#define _int_size_h_

namespace elftosb
{

//! Supported sizes of integers.
typedef enum {
	kWordSize,		//!< 32-bit word.
	kHalfWordSize,	//!< 16-bit half word.
	kByteSize		//!< 8-bit byte.
} int_size_t;

}; // namespace elftosb

#endif // _int_size_h_
