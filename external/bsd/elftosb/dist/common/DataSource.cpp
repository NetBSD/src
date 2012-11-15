/*
 * File:	DataSource.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "DataSource.h"
#include "DataTarget.h"
#include <assert.h>
#include <string.h>
using namespace elftosb;

#pragma mark *** DataSource::PatternSegment ***

DataSource::PatternSegment::PatternSegment(DataSource & source)
:	DataSource::Segment(source), m_pattern()
{
}

DataSource::PatternSegment::PatternSegment(DataSource & source, const SizedIntegerValue & pattern)
:	DataSource::Segment(source), m_pattern(pattern)
{
}

DataSource::PatternSegment::PatternSegment(DataSource & source, uint8_t pattern)
:	DataSource::Segment(source), m_pattern(static_cast<uint8_t>(pattern))
{
}

DataSource::PatternSegment::PatternSegment(DataSource & source, uint16_t pattern)
:	DataSource::Segment(source), m_pattern(static_cast<uint16_t>(pattern))
{
}

DataSource::PatternSegment::PatternSegment(DataSource & source, uint32_t pattern)
:	DataSource::Segment(source), m_pattern(static_cast<uint32_t>(pattern))
{
}

unsigned DataSource::PatternSegment::getData(unsigned offset, unsigned maxBytes, uint8_t * buffer)
{
	memset(buffer, 0, maxBytes);
	
	return maxBytes;
}

//! The pattern segment's length is a function of the data target. If the
//! target is bounded, then the segment's length is simply the target's
//! length. Otherwise, if no target has been set or the target is unbounded,
//! then the length returned is 0.
unsigned DataSource::PatternSegment::getLength()
{
	DataTarget * target = m_source.getTarget();
	if (!target)
	{
		return 0;
	}
	
	uint32_t length;
	if (target->isBounded())
	{
		length = target->getEndAddress() - target->getBeginAddress();
	}
	else
	{
		length = m_pattern.getSize();
	}
	return length;
}

#pragma mark *** PatternSource ***

PatternSource::PatternSource()
:	DataSource(), DataSource::PatternSegment((DataSource&)*this)
{
}

PatternSource::PatternSource(const SizedIntegerValue & value)
:	DataSource(), DataSource::PatternSegment((DataSource&)*this, value)
{
}

#pragma mark *** UnmappedDataSource ***

UnmappedDataSource::UnmappedDataSource()
:	DataSource(), DataSource::Segment((DataSource&)*this), m_data(), m_length(0)
{
}

UnmappedDataSource::UnmappedDataSource(const uint8_t * data, unsigned length)
:	DataSource(), DataSource::Segment((DataSource&)*this), m_data(), m_length(0)
{
	setData(data, length);
}

//! Makes a copy of \a data that is freed when the data source is
//! destroyed. The caller does not have to maintain \a data after this call
//! returns.
void UnmappedDataSource::setData(const uint8_t * data, unsigned length)
{
	m_data.safe_delete();
	
	uint8_t * dataCopy = new uint8_t[length];
	memcpy(dataCopy, data, length);
	m_data = dataCopy;
	m_length = length;
}

unsigned UnmappedDataSource::getData(unsigned offset, unsigned maxBytes, uint8_t * buffer)
{
	assert(offset < m_length);
	unsigned copyBytes = std::min<unsigned>(m_length - offset, maxBytes);
	memcpy(buffer, m_data.get(), copyBytes);
	return copyBytes;
}

#pragma mark *** MemoryImageDataSource ***

MemoryImageDataSource::MemoryImageDataSource(StExecutableImage * image)
:	DataSource(), m_image(image)
{
	// reserve enough room for all segments
	m_segments.reserve(m_image->getRegionCount());
}

MemoryImageDataSource::~MemoryImageDataSource()
{
	segment_array_t::iterator it = m_segments.begin();
	for (; it != m_segments.end(); ++it)
	{
		// delete this segment if it has been created
		if (*it)
		{
			delete *it;
		}
	}
}

unsigned MemoryImageDataSource::getSegmentCount()
{
	return m_image->getRegionCount();
}

DataSource::Segment * MemoryImageDataSource::getSegmentAt(unsigned index)
{
	// return previously created segment
	if (index < m_segments.size() && m_segments[index])
	{
		return m_segments[index];
	}
	
	// extend array out to this index
	if (index >= m_segments.size() && index < m_image->getRegionCount())
	{
		m_segments.resize(index + 1, NULL);
	}
	
	// create the new segment object
	DataSource::Segment * newSegment;
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(index);
	if (region.m_type == StExecutableImage::TEXT_REGION)
	{
		newSegment = new TextSegment(*this, m_image, index);
	}
	else if (region.m_type == StExecutableImage::FILL_REGION)
	{
		newSegment = new FillSegment(*this, m_image, index);
	}
	
	m_segments[index] = newSegment;
	return newSegment;
}

#pragma mark *** MemoryImageDataSource::TextSegment ***

MemoryImageDataSource::TextSegment::TextSegment(MemoryImageDataSource & source, StExecutableImage * image, unsigned index)
:	DataSource::Segment(source), m_image(image), m_index(index)
{
}

unsigned MemoryImageDataSource::TextSegment::getData(unsigned offset, unsigned maxBytes, uint8_t * buffer)
{
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(m_index);
	assert(region.m_type == StExecutableImage::TEXT_REGION);
	
	unsigned copyBytes = std::min<unsigned>(region.m_length - offset, maxBytes);	
	memcpy(buffer, &region.m_data[offset], copyBytes);
	
	return copyBytes;
}

unsigned MemoryImageDataSource::TextSegment::getLength()
{
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(m_index);
	return region.m_length;
}

uint32_t MemoryImageDataSource::TextSegment::getBaseAddress()
{
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(m_index);
	return region.m_address;
}

#pragma mark *** MemoryImageDataSource::FillSegment ***

MemoryImageDataSource::FillSegment::FillSegment(MemoryImageDataSource & source, StExecutableImage * image, unsigned index)
:	DataSource::PatternSegment(source), m_image(image), m_index(index)
{
	SizedIntegerValue zero(0, kWordSize);
	setPattern(zero);
}

unsigned MemoryImageDataSource::FillSegment::getLength()
{
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(m_index);
	return region.m_length;
}

uint32_t MemoryImageDataSource::FillSegment::getBaseAddress()
{
	const StExecutableImage::MemoryRegion & region = m_image->getRegionAtIndex(m_index);
	return region.m_address;
}
