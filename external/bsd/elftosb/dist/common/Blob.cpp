/*
 * File:	Blob.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "Blob.h"
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

Blob::Blob()
:	m_data(0),
	m_length(0)
{
}

//! Makes a local copy of the \a data argument.
//!
Blob::Blob(const uint8_t * data, unsigned length)
:	m_data(0),
	m_length(length)
{
	m_data = reinterpret_cast<uint8_t*>(malloc(length));
	memcpy(m_data, data, length);
}

//! Makes a local copy of the data owned by \a other.
//!
Blob::Blob(const Blob & other)
:	m_data(0),
	m_length(other.m_length)
{
	m_data = reinterpret_cast<uint8_t *>(malloc(m_length));
	memcpy(m_data, other.m_data, m_length);
}

//! Disposes of the binary data associated with this object.
Blob::~Blob()
{
	if (m_data)
	{
		free(m_data);
	}
}

//! Copies \a data onto the blob's data. The blob does not assume ownership
//! of \a data.
//!
//! \param data Pointer to a buffer containing the data which will be copied
//!		into the blob.
//! \param length Number of bytes pointed to by \a data.
void Blob::setData(const uint8_t * data, unsigned length)
{
	setLength(length);
	memcpy(m_data, data, length);
}

//! Sets the #m_length member variable to \a length and resizes #m_data to
//! the new length. The contents of #m_data past any previous contents are undefined.
//! If the new \a length is 0 then the data will be freed and a subsequent call
//! to getData() will return NULL.
//!
//! \param length New length of the blob's data in bytes.
void Blob::setLength(unsigned length)
{
	if (length == 0)
	{
		clear();
		return;
	}
	
	// Allocate new block.
	if (!m_data)
	{
		m_data = reinterpret_cast<uint8_t*>(malloc(length));
		if (!m_data)
		{
			throw std::runtime_error("failed to allocate memory");
		}
	}
	// Reallocate previous block.
	else
	{
		void * newBlob = realloc(m_data, length);
		if (!newBlob)
		{
			throw std::runtime_error("failed to reallocate memory");
		}
		m_data = reinterpret_cast<uint8_t*>(newBlob);
	}
	
	// Set length.
	m_length = length;
}

void Blob::append(const uint8_t * newData, unsigned newDataLength)
{
	unsigned oldLength = m_length;
	
	setLength(m_length + newDataLength);
	
	memcpy(m_data + oldLength, newData, newDataLength);
}

void Blob::clear()
{
	if (m_data)
	{
		free(m_data);
		m_data = NULL;
	}
	
	m_length = 0;
}

void Blob::relinquish()
{
	m_data = NULL;
	m_length = 0;
}

