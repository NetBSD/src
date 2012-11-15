/*
 * File:	Version.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_Version_h_)
#define _Version_h_

#include <string>
#include "stdafx.h"

namespace elftosb
{

//! Same version struct used for 3600 boot image.
struct version_t
{
	uint16_t m_major;
	uint16_t m_pad0;
	uint16_t m_minor;
	uint16_t m_pad1;
	uint16_t m_revision;
	uint16_t m_pad2;
	
	version_t()
	:	m_major(0x999), m_pad0(0), m_minor(0x999), m_pad1(0), m_revision(0x999), m_pad2(0)
	{
	}
	
	version_t(uint16_t maj, uint16_t min, uint16_t rev)
	:	m_major(maj), m_pad0(0), m_minor(min), m_pad1(0), m_revision(rev), m_pad2(0)
	{
	}
	
	version_t(const std::string & versionString)
	:	m_major(0x999), m_pad0(0), m_minor(0x999), m_pad1(0), m_revision(0x999), m_pad2(0)
	{
		set(versionString);
	}
	
	//! \brief Sets the version by parsing a string.
	void set(const std::string & versionString);

	//! \brief
	void fixByteOrder();
};

}; // namespace elftosb

#endif // _Version_h_
