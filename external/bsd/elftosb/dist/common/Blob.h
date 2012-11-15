/*
 * File:	Blob.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_Blob_h_)
#define _Blob_h_

#include "stdafx.h"

/*!
 * \brief Manages a binary object of arbitrary length.
 *
 * The data block is allocated with malloc() instead of the new
 * operator so that we can use realloc() to resize it.
 */
class Blob
{
public:
	//! \brief Default constructor.
	Blob();
	
	//! \brief Constructor.
	Blob(const uint8_t * data, unsigned length);
	
	//! \brief Copy constructor.
	Blob(const Blob & other);
	
	//! \brief Destructor.
	virtual ~Blob();
	
	//! \name Operations
	//@{
	//! \brief Replaces the blob's data.
	void setData(const uint8_t * data, unsigned length);
	
	//! \brief Change the size of the blob's data.
	void setLength(unsigned length);
	
	//! \brief Adds data to the end of the blob.
	void append(const uint8_t * newData, unsigned newDataLength);
	
	//! \brief Disposes of the data.
	void clear();
	
	//! \brief Tell the blob that it no longer owns its data.
	void relinquish();
	//@}
	
	//! \name Accessors
	//@{
	uint8_t * getData() { return m_data; }
	const uint8_t * getData() const { return m_data; }
	unsigned getLength() const { return m_length; }
	//@}
	
	//! \name Operators
	//@{
	operator uint8_t * () { return m_data; }
	operator const uint8_t * () const { return m_data; }
	//@}

protected:
	uint8_t * m_data;	//!< The binary data held by this object.
	unsigned m_length;	//!< Number of bytes pointed to by #m_data.
};

#endif // _Blob_h_

