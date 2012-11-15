/*
 * File:	DataTarget.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "DataTarget.h"
#include "DataSource.h"
#include "ElftosbErrors.h"

using namespace elftosb;

//! \exception elftosb::semantic_error Thrown if the source has multiple segments.
DataTarget::AddressRange ConstantDataTarget::getRangeForSegment(DataSource & source, DataSource::Segment & segment)
{
	// can't handle multi-segment data sources
	if (source.getSegmentCount() > 1)
	{
		throw semantic_error("constant targets only support single-segment sources");
	}
	
	// always relocate the segment to our begin address
	AddressRange range;
	range.m_begin = m_begin;
	
	if (isBounded())
	{
		// we have an end address. trim the result range to the segment size
		// or let the end address crop the segment.
		range.m_end = std::min<uint32_t>(m_end, m_begin + segment.getLength());
	}
	else
	{
		// we have no end address, so the segment size determines it.
		range.m_end = m_begin + segment.getLength();
	}
	
	return range;
}

//! If the \a segment has a natural location, the returned address range extends
//! from the segment's base address to its base address plus its length.
//!
//! \exception elftosb::semantic_error This exception is thrown if the \a segment
//!		does not have a natural location associated with it.
DataTarget::AddressRange NaturalDataTarget::getRangeForSegment(DataSource & source, DataSource::Segment & segment)
{
	if (!segment.hasNaturalLocation())
	{
		throw semantic_error("source has no natural location");
	}
	
	AddressRange range;
	range.m_begin = segment.getBaseAddress();
	range.m_end = segment.getBaseAddress() + segment.getLength();
	return range;
}

