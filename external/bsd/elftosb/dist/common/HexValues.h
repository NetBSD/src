/*
 * File:	HexValues.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_HexValues_h_)
#define _HexValues_h_

#include "stdafx.h"

//! \brief Determines whether \a c is a hex digit character.
bool isHexDigit(char c);

//! \brief Converts a hexidecimal character to the integer equivalent.
uint8_t hexCharToInt(char c);

//! \brief Converts a hex-encoded byte to the integer equivalent.
uint8_t hexByteToInt(const char * encodedByte);

#endif // _HexValues_h_
