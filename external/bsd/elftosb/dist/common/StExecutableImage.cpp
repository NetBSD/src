/*
 * File:	StExecutableImage.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "StExecutableImage.h"
#include <stdexcept>
#include <algorithm>
#include <string.h>
#include <stdio.h>

StExecutableImage::StExecutableImage(int inAlignment)
:	m_alignment(inAlignment),
	m_hasEntry(false),
	m_entry(0)
{
}

//! Makes a duplicate of each memory region.
StExecutableImage::StExecutableImage(const StExecutableImage & inOther)
:	m_name(inOther.m_name),
    m_alignment(inOther.m_alignment),
	m_hasEntry(inOther.m_hasEntry),
	m_entry(inOther.m_entry),
    m_filters(inOther.m_filters)
{
	const_iterator it = inOther.getRegionBegin();
	for (; it != inOther.getRegionEnd(); ++it)
	{
		const MemoryRegion & region = *it;
		
		MemoryRegion regionCopy(region);
		if (region.m_type == FILL_REGION && region.m_data != NULL)
		{
			regionCopy.m_data = new uint8_t[region.m_length];
			memcpy(regionCopy.m_data, region.m_data, region.m_length);
		}
		
		m_image.push_back(regionCopy);
	}
}

//! Disposes of memory allocated for each region.
StExecutableImage::~StExecutableImage()
{
	MemoryRegionList::iterator it;
	for (it = m_image.begin(); it != m_image.end(); ++it)
	{
		if (it->m_data)
		{
			delete [] it->m_data;
			it->m_data = NULL;
		}
	}
}

//! A copy of \a inName is made, so the original may be disposed of by the caller
//! after this method returns.
void StExecutableImage::setName(const std::string & inName)
{
	m_name = inName;
}

std::string StExecutableImage::getName() const
{
	return m_name;
}

// The region is added with read and write flags set.
//! \exception std::runtime_error will be thrown if the new overlaps an
//!		existing region.
void StExecutableImage::addFillRegion(uint32_t inAddress, unsigned inLength)
{
	MemoryRegion region;
	region.m_type = FILL_REGION;
	region.m_address = inAddress;
	region.m_data = NULL;
	region.m_length = inLength;
	region.m_flags = REGION_RW_FLAG;
	
	insertOrMergeRegion(region);
}

//! A copy of \a inData is made before returning. The copy will be deleted when 
//! the executable image is destructed. Currently, the text region is created with
//! read, write, and executable flags set.
//! \exception std::runtime_error will be thrown if the new overlaps an
//!		existing region.
//! \exception std::bad_alloc is thrown if memory for the copy of \a inData
//!		cannot be allocated.
void StExecutableImage::addTextRegion(uint32_t inAddress, const uint8_t * inData, unsigned inLength)
{
	MemoryRegion region;
	region.m_type = TEXT_REGION;
	region.m_address = inAddress;
	region.m_flags = REGION_RW_FLAG | REGION_EXEC_FLAG;
	
	// copy the data
	region.m_data = new uint8_t[inLength];
	region.m_length = inLength;
	memcpy(region.m_data, inData, inLength);
	
	insertOrMergeRegion(region);
}

//! \exception std::out_of_range is thrown if \a inIndex is out of range.
//!
const StExecutableImage::MemoryRegion & StExecutableImage::getRegionAtIndex(unsigned inIndex) const
{
	// check bounds
	if (inIndex >= m_image.size())
		throw std::out_of_range("inIndex");
	
	// find region by index
	MemoryRegionList::const_iterator it = m_image.begin();
	unsigned i = 0;
	for (; it != m_image.end(); ++it, ++i)
	{
		if (i == inIndex)
			break;
	}
	return *it;
}

//! The list of address filters is kept sorted as filters are added.
//!
void StExecutableImage::addAddressFilter(const AddressFilter & filter)
{
    m_filters.push_back(filter);
    m_filters.sort();
}

//!
void StExecutableImage::clearAddressFilters()
{
    m_filters.clear();
}

//! \exception StExecutableImage::address_filter_exception Raised when a filter
//!     with the type #ADDR_FILTER_ERROR or #ADDR_FILTER_WARNING is matched.
//!
//! \todo Build a list of all matching filters and then execute them at once.
//!     For the warning and error filters, a single exception should be raised
//!     that identifies all the overlapping errors. Currently the user will only
//!     see the first (lowest address) overlap.
void StExecutableImage::applyAddressFilters()
{
restart_loops:
    // Iterate over filters.
    AddressFilterList::const_iterator fit = m_filters.begin();
    for (; fit != m_filters.end(); ++fit)
    {
        const AddressFilter & filter = *fit;
        
        // Iterator over regions.
        MemoryRegionList::iterator rit = m_image.begin();
        for (; rit != m_image.end(); ++rit)
        {
            MemoryRegion & region = *rit;
            
            if (filter.matchesMemoryRegion(region))
            {
                switch (filter.m_action)
                {
                    case ADDR_FILTER_NONE:
                        // Do nothing.
                        break;
                        
                    case ADDR_FILTER_ERROR:
                        // throw error exception
                        throw address_filter_exception(true, m_name, filter);
                        break;
                        
                    case ADDR_FILTER_WARNING:
                        // throw warning exception
                        throw address_filter_exception(false, m_name, filter);
                        break;
                        
                    case ADDR_FILTER_CROP:
                        // Delete the offending portion of the region and restart
                        // the iteration loops.
                        cropRegionToFilter(region, filter);
                        goto restart_loops;
                        break;
                }
            }
        }
    }
}

//! There are several possible cases here:
//!     - No overlap at all. Nothing is done.
//!
//!     - All of the memory region is matched by the \a filter. The region is
//!         removed from #StExecutableImage::m_image and its data memory freed.
//!
//!     - The remaining portion of the region is one contiguous chunk. In this
//!         case, \a region is simply modified. 
//!
//!     - The region is split in the middle by the filter. The original \a region
//!         is modified to match the first remaining chunk. And a new #StExecutableImage::MemoryRegion
//!         instance is created to hold the other leftover piece.
void StExecutableImage::cropRegionToFilter(MemoryRegion & region, const AddressFilter & filter)
{
    uint32_t firstByte = region.m_address;      // first byte occupied by this region
    uint32_t lastByte = region.endAddress();    // last used byte in this region
    
    // compute new address range
    uint32_t cropFrom = filter.m_fromAddress;
    if (cropFrom < firstByte)
    {
        cropFrom = firstByte;
    }
    
    uint32_t cropTo = filter.m_toAddress;
    if (cropTo > lastByte)
    {
        cropTo = lastByte;
    }
    
    // is there actually a match?
    if (cropFrom > filter.m_toAddress || cropTo < filter.m_fromAddress)
    {
        // nothing to do, so bail
        return;
    }
    
    printf("Deleting region 0x%08x-0x%08x\n", cropFrom, cropTo);
    
    // handle if the entire region is to be deleted
    if (cropFrom == firstByte && cropTo == lastByte)
    {
        delete [] region.m_data;
        region.m_data = NULL;
        m_image.remove(region);
    }
    
    // there is at least a little of the original region remaining
    uint32_t newLength = cropTo - cropFrom + 1;
    uint32_t leftoverLength = lastByte - cropTo;
    uint8_t * oldData = region.m_data;
    
    // update the region
    region.m_address = cropFrom;
    region.m_length = newLength;
    
    // crop data buffer for text regions
    if (region.m_type == TEXT_REGION && oldData)
    {
        region.m_data = new uint8_t[newLength];
        memcpy(region.m_data, &oldData[cropFrom - firstByte], newLength);
        
        // dispose of old data
        delete [] oldData;
    }
    
    // create a new region for any part of the original region that was past
    // the crop to address. this will happen if the filter range falls in the
    // middle of the region.
    if (leftoverLength)
    {
        MemoryRegion newRegion;
        newRegion.m_type = region.m_type;
        newRegion.m_flags = region.m_flags;
        newRegion.m_address = cropTo + 1;
        newRegion.m_length = leftoverLength;
        
        if (region.m_type == TEXT_REGION && oldData)
        {
            newRegion.m_data = new uint8_t[leftoverLength];
            memcpy(newRegion.m_data, &oldData[cropTo - firstByte + 1], leftoverLength);
        }
        
        insertOrMergeRegion(newRegion);
    }
}

//! \exception std::runtime_error will be thrown if \a inRegion overlaps an
//!		existing region.
//!
//! \todo Need to investigate if we can use the STL sort algorithm at all. Even
//!     though we're doing merges too, we could sort first then examine the list
//!     for merges.
void StExecutableImage::insertOrMergeRegion(MemoryRegion & inRegion)
{
	uint32_t newStart = inRegion.m_address;
	uint32_t newEnd = newStart + inRegion.m_length;
	
	MemoryRegionList::iterator it = m_image.begin();
	MemoryRegionList::iterator sortedPosition = m_image.begin();
	for (; it != m_image.end(); ++it)
	{
		MemoryRegion & region = *it;
		uint32_t thisStart = region.m_address;
		uint32_t thisEnd = thisStart + region.m_length;
		
		// keep track of where to insert it to retain sort order
		if (thisStart >= newEnd)
		{
			break;
		}
			
		// region types and flags must match in order to merge
		if (region.m_type == inRegion.m_type && region.m_flags == inRegion.m_flags)
		{
			if (newStart == thisEnd || newEnd == thisStart)
			{
				mergeRegions(region, inRegion);
				return;
			}
			else if ((newStart >= thisStart && newStart < thisEnd) || (newEnd >= thisStart && newEnd < thisEnd))
			{
				throw std::runtime_error("new region overlaps existing region");
			}
		}
	}
	
	// not merged, so just insert it in the sorted position
	m_image.insert(it, inRegion);
}

//! Extends \a inNewRegion to include the data in \a inOldRegion. It is
//! assumed that the two regions are compatible. The new region may come either
//! before or after the old region in memory. Note that while the two regions
//! don't necessarily have to be touching, it's probably a good idea. That's
//! because any data between the regions will be set to 0.
//!
//! For TEXT_REGION types, the two original regions will have their data deleted
//! during the merge. Thus, this method is not safe if any outside callers may
//! be accessing the region's data.
void StExecutableImage::mergeRegions(MemoryRegion & inOldRegion, MemoryRegion & inNewRegion)
{
	bool isOldBefore = inOldRegion.m_address < inNewRegion.m_address;
	uint32_t oldEnd = inOldRegion.m_address + inOldRegion.m_length;
	uint32_t newEnd = inNewRegion.m_address + inNewRegion.m_length;
	
	switch (inOldRegion.m_type)
	{
		case TEXT_REGION:
		{
			// calculate new length
			unsigned newLength;
			if (isOldBefore)
			{
				newLength = newEnd - inOldRegion.m_address;
			}
			else
			{
				newLength = oldEnd - inNewRegion.m_address;
			}
			
			// alloc memory
			uint8_t * newData = new uint8_t[newLength];
			memset(newData, 0, newLength);
			
			// copy data from the two regions into new block
			if (isOldBefore)
			{
				memcpy(newData, inOldRegion.m_data, inOldRegion.m_length);
				memcpy(&newData[newLength - inNewRegion.m_length], inNewRegion.m_data, inNewRegion.m_length);
			}
			else
			{
				memcpy(newData, inNewRegion.m_data, inNewRegion.m_length);
				memcpy(&newData[newLength - inOldRegion.m_length], inOldRegion.m_data, inOldRegion.m_length);
				
				inOldRegion.m_address = inNewRegion.m_address;
			}
			
			// replace old region's data
			delete [] inOldRegion.m_data;
			inOldRegion.m_data = newData;
			inOldRegion.m_length = newLength;
			
			// delete new region's data
			delete [] inNewRegion.m_data;
			inNewRegion.m_data = NULL;
			break;
		}
			
		case FILL_REGION:
		{
			if (isOldBefore)
			{
				inOldRegion.m_length = newEnd - inOldRegion.m_address;
			}
			else
			{
				inOldRegion.m_length = oldEnd - inNewRegion.m_address;
				inOldRegion.m_address = inNewRegion.m_address;
			}
			break;
		}
	}
}

//! Used when we remove a region from the region list by value. Because this
//! operator compares the #m_data member, it will only return true for either an
//! exact copy or a reference to the original.
bool StExecutableImage::MemoryRegion::operator == (const MemoryRegion & other) const
{
   return (m_type == other.m_type) && (m_address == other.m_address) && (m_length == other.m_length) && (m_flags == other.m_flags) && (m_data == other.m_data);
}

//! Returns true if the address filter overlaps \a region.
bool StExecutableImage::AddressFilter::matchesMemoryRegion(const MemoryRegion & region) const
{
    uint32_t firstByte = region.m_address;      // first byte occupied by this region
    uint32_t lastByte = region.endAddress();    // last used byte in this region
    return (firstByte >= m_fromAddress && firstByte <= m_toAddress) || (lastByte >= m_fromAddress && lastByte <= m_toAddress);
}

//! The comparison does \em not take the action into account. It only looks at the
//! priority and address ranges of each filter. Priority is considered only if the two
//! filters overlap. Lower priority filters will come after higher priority ones.
//!
//! \retval -1 This filter is less than filter \a b.
//! \retval 0 This filter is equal to filter \a b.
//! \retval 1 This filter is greater than filter \a b.
int StExecutableImage::AddressFilter::compare(const AddressFilter & other) const
{
    if (m_priority != other.m_priority && ((m_fromAddress >= other.m_fromAddress && m_fromAddress <= other.m_toAddress) || (m_toAddress >= other.m_fromAddress && m_toAddress <= other.m_toAddress)))
    {
        // we know the priorities are not equal
        if (m_priority > other.m_priority)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    
    if (m_fromAddress == other.m_fromAddress)
    {
        if (m_toAddress == other.m_toAddress)
        {
            return 0;
        }
        else if (m_toAddress < other.m_toAddress)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    else if (m_fromAddress < other.m_fromAddress)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}



