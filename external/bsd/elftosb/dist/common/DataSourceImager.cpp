/*
 * File:	DataSourceImager.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "DataSourceImager.h"
#include <stdlib.h>
#include <string.h>

using namespace elftosb;

DataSourceImager::DataSourceImager()
:	Blob(),
	m_fill(0),
	m_baseAddress(0),
	m_isBaseAddressSet(false)
{
}

void DataSourceImager::setBaseAddress(uint32_t address)
{
	m_baseAddress = address;
	m_isBaseAddressSet = true;
}

void DataSourceImager::setFillPattern(uint8_t pattern)
{
	m_fill = pattern;
}

void DataSourceImager::reset()
{
	clear();
	
	m_fill = 0;
	m_baseAddress = 0;
	m_isBaseAddressSet = false;
}

//! \param dataSource Pointer to an instance of a concrete data source subclass.
//!
void DataSourceImager::addDataSource(DataSource * source)
{
	unsigned segmentCount = source->getSegmentCount();
	unsigned index = 0;
	for (; index < segmentCount; ++index)
	{
		addDataSegment(source->getSegmentAt(index));
	}
}

//! \param segment The segment to add. May be any type of data segment, including
//!		a pattern segment.
void DataSourceImager::addDataSegment(DataSource::Segment * segment)
{
	DataSource::PatternSegment * patternSegment = dynamic_cast<DataSource::PatternSegment*>(segment);
	
	unsigned segmentLength = segment->getLength();
	bool segmentHasLocation = segment->hasNaturalLocation();
	uint32_t segmentAddress = segment->getBaseAddress();
	
	uint8_t * toPtr = NULL;
	unsigned addressDelta;
	unsigned newLength;
	
	// If a pattern segment's length is 0 then make it as big as the fill pattern.
	// This needs to be done before the buffer is adjusted.
	if (patternSegment && segmentLength == 0)
	{
		SizedIntegerValue & pattern = patternSegment->getPattern();
		segmentLength = pattern.getSize();
	}
	
	if (segmentLength)
	{
		if (segmentHasLocation)
		{
			// Make sure a base address is set.
			if (!m_isBaseAddressSet)
			{
				m_baseAddress = segmentAddress;
				m_isBaseAddressSet = true;
			}
			
			// The segment is located before our buffer's first address.
			// toPtr is not set in this if, but in the else branch of the next if.
			// Unless the segment completely overwrite the current data.
			if (segmentAddress < m_baseAddress)
			{
				addressDelta = m_baseAddress - segmentAddress;
				
				uint8_t * newData = (uint8_t *)malloc(m_length + addressDelta);
				memcpy(&newData[addressDelta], m_data, m_length);
				free(m_data);
				
				m_data = newData;
				m_length += addressDelta;
				m_baseAddress = segmentAddress;
			}
			
			// This segment is located or extends outside of our buffer.
			if (segmentAddress + segmentLength > m_baseAddress + m_length)
			{
				newLength = segmentAddress + segmentLength - m_baseAddress;
				
				m_data = (uint8_t *)realloc(m_data, newLength);
				
				// Clear any bytes between the old data and the new segment.
				addressDelta = segmentAddress - (m_baseAddress + m_length);
				if (addressDelta)
				{
					memset(m_data + m_length, 0, addressDelta);
				}
				
				toPtr = m_data + (segmentAddress - m_baseAddress);
				m_length = newLength;
			}
			else
			{
				toPtr = m_data + (segmentAddress - m_baseAddress);
			}
		}
		// Segment has no natural location, so just append it to the end of our buffer.
		else
		{
			newLength = m_length + segmentLength;
			m_data = (uint8_t *)realloc(m_data, newLength);
			toPtr = m_data + m_length;
			m_length = newLength;
		}
	}

	// A loop is used because getData() may fill in less than the requested
	// number of bytes per call.
	unsigned offset = 0;
	while (offset < segmentLength)
	{
		offset += segment->getData(offset, segmentLength - offset, toPtr + offset);
	}
}

