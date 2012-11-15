/*
 * File:	DataSourceImager.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_DataSourceImager_h_)
#define _DataSourceImager_h_

#include "Blob.h"
#include "DataSource.h"

namespace elftosb {

/*!
 * \brief Converts a DataSource into a single binary buffer.
 */
class DataSourceImager : public Blob
{
public:
	//! \brief Constructor.
	DataSourceImager();
	
	//! \name Setup
	//@{
	void setBaseAddress(uint32_t address);
	void setFillPattern(uint8_t pattern);
	//@}
	
	void reset();
	
	//! \name Accessors
	//@{
	uint32_t getBaseAddress() { return m_baseAddress; }
	//@}
	
	//! \name Operations
	//@{
	//! \brief Adds all of the segments of which \a dataSource is composed.
	void addDataSource(DataSource * source);
	
	//! \brief Adds the data from one data segment.
	void addDataSegment(DataSource::Segment * segment);
	//@}

protected:
	uint8_t m_fill;
	uint32_t m_baseAddress;
	bool m_isBaseAddressSet;
};

};

#endif // _DataSourceImager_h_
