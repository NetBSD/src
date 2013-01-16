/*
 * File:	Version.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Version.h"
#include "EndianUtilities.h"

using namespace elftosb;

/*!
 * Parses a string in the form "xxx.xxx.xxx" (where x is a digit) into
 * three version fields for major, minor, and revision. The output is
 * right aligned BCD in host-natural byte order.
 *
 * \param versionString String containing the version.
 */
void version_t::set(const std::string & versionString)
{
	size_t length = versionString.size();
	unsigned version = 0;
	unsigned index = 0;
	
	typedef enum {
		kVersionStateNone,
		kVersionStateMajor,
		kVersionStateMinor,
		kVersionStateRevision
	} VersionParseState;
	
	// set initial versions to 0s
	m_major = 0;
	m_minor = 0;
	m_revision = 0;
	
	VersionParseState parseState = kVersionStateNone;
	bool done = false;
	for (; index < length && !done; ++index)
	{
		char c = versionString[index];
		
		if (isdigit(c))
		{
			switch (parseState)
			{
				case kVersionStateNone:
					parseState = kVersionStateMajor;
					version = c - '0';
					break;
				case kVersionStateMajor:
				case kVersionStateMinor:
				case kVersionStateRevision:
					version = (version << 4) | (c - '0');
					break;
			}
		}
		else if (c == '.')
		{
			switch (parseState)
			{
				case kVersionStateNone:
					parseState = kVersionStateNone;
					break;
				case kVersionStateMajor:
					m_major = version;
					version = 0;
					parseState = kVersionStateMinor;
					break;
				case kVersionStateMinor:
					m_minor = version;
					version = 0;
					parseState = kVersionStateRevision;
					break;
				case kVersionStateRevision:
					m_revision = version;
					version = 0;
					done = true;
					break;
			}
		}
		else
		{
			switch (parseState)
			{
				case kVersionStateNone:
					parseState = kVersionStateNone;
					break;
				case kVersionStateMajor:
					m_major = version;
					done = true;
					break;
				case kVersionStateMinor:
					m_minor = version;
					done = true;
					break;
				case kVersionStateRevision:
					m_revision = version;
					done = true;
					break;
			}
		}
	}
	
	switch (parseState)
	{
		case kVersionStateMajor:
			m_major = version;
			break;
		case kVersionStateMinor:
			m_minor = version;
			break;
		case kVersionStateRevision:
			m_revision = version;
			break;
		default:
			// do nothing
			break;
	}
}

//! \brief Converts host endian BCD version values to the equivalent big-endian BCD values.
//!
//! The output is a half-word. And BCD is inherently big-endian, or byte ordered, if
//! you prefer to think of it that way. So for little endian systems, we need to convert
//! the output half-word in reverse byte order. When it is written to disk or a
//! buffer it will come out big endian.
//!
//! For example:
//!     - The input is BCD in host endian format, so 0x1234. Written to a file, this would
//!       come out as 0x34 0x12, reverse of what we want.
//!     - The desired BCD output is the two bytes 0x12 0x34.
//!     - So the function's uint16_t result must be 0x3412 on a little-endian host.
//!
//! On big endian hosts, we don't have to worry about byte swapping.
void version_t::fixByteOrder()
{
	m_major = ENDIAN_HOST_TO_BIG_U16(m_major);
	m_minor = ENDIAN_HOST_TO_BIG_U16(m_minor);
	m_revision = ENDIAN_HOST_TO_BIG_U16(m_revision);
}

