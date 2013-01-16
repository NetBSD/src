/*
 * File:	SRecordSourceFile.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "SRecordSourceFile.h"
#include "Logging.h"
#include "smart_ptr.h"
#include <assert.h>
#include <string.h>
enum
{
	//! Size in bytes of the buffer used to collect S-record data records
	//! before adding them to the executable image. Currently 64KB.
	COLLECTION_BUFFER_SIZE = 64 * 1024
};

using namespace elftosb;

SRecordSourceFile::SRecordSourceFile(const std::string & path)
:	SourceFile(path), m_image(0), m_hasEntryRecord(false)
{
}

bool SRecordSourceFile::isSRecordFile(std::istream & stream)
{
	StSRecordFile srec(stream);
	return srec.isSRecordFile();
}

void SRecordSourceFile::open()
{
	SourceFile::open();
	
	// create file parser and examine file
	m_file = new StSRecordFile(*m_stream);
	m_file->parse();
	
	// build an image of the file
	m_image = new StExecutableImage();
	buildMemoryImage();
	
	// dispose of file parser object
	delete m_file;
	m_file = 0;
}

void SRecordSourceFile::close()
{
	assert(m_image);
	
	SourceFile::close();
	
	// dispose of memory image
	delete m_image;
	m_image = 0;
}

//! \pre The file must be open before this method can be called.
//!
DataSource * SRecordSourceFile::createDataSource()
{
	assert(m_image);
	return new MemoryImageDataSource(m_image);
}

//! \retval true The file has an S7, S8, or S9 record.
//! \retval false No entry point is available.
bool SRecordSourceFile::hasEntryPoint()
{
	return m_hasEntryRecord;
}

//! If no entry point is available then 0 is returned instead. The method scans
//! the records in the file looking for S7, S8, or S9 records. Thus, 16-bit,
//! 24-bit, and 32-bit entry point records are supported.
//!
//! \return Entry point address.
//! \retval 0 No entry point is available.
uint32_t SRecordSourceFile::getEntryPointAddress()
{
	if (m_hasEntryRecord)
	{
		// the address in the record is the entry point
		Log::log(Logger::DEBUG2, "entry point address is 0x%08x\n", m_entryRecord.m_address);
		return m_entryRecord.m_address;
	}
	
	return 0;
}

//! Scans the S-records of the file looking for data records. These are S3, S2, or
//! S1 records. The contents of these records are added to an StExecutableImage
//! object, which coalesces the individual records into contiguous regions of
//! memory.
//!
//! Also looks for S7, S8, or S9 records that contain the entry point. The first
//! match of one of these records is saved off into the #m_entryRecord member.
//! 
//! \pre The #m_file member must be valid.
//! \pre The #m_image member variable must have been instantiated.
void SRecordSourceFile::buildMemoryImage()
{
	assert(m_file);
	assert(m_image);
	
	// Clear the entry point related members.
	m_hasEntryRecord = false;
	memset(&m_entryRecord, 0, sizeof(m_entryRecord));
	
	// Allocate buffer to hold data before adding it to the executable image.
	// Contiguous records are added to this buffer. When overflowed or when a
	// non-contiguous record is encountered the buffer is added to the executable
	// image where it will be coalesced further. We don't add records individually
	// to the image because coalescing record by record is very slow.
	smart_array_ptr<uint8_t> buffer = new uint8_t[COLLECTION_BUFFER_SIZE];
	unsigned startAddress;
	unsigned nextAddress;
	unsigned dataLength = 0;
	
	// process SRecords
    StSRecordFile::const_iterator it = m_file->getBegin();
	for (; it != m_file->getEnd(); it++)
	{
        const StSRecordFile::SRecord & theRecord = *it;
        
        // only handle S3,2,1 records
        bool isDataRecord = theRecord.m_type == 3 || theRecord.m_type == 2 || theRecord.m_type == 1;
        bool hasData = theRecord.m_data && theRecord.m_dataCount;
		if (isDataRecord && hasData)
		{
			// If this record's data would overflow the collection buffer, or if the
			// record is not contiguous with the rest of the data in the collection
			// buffer, then flush the buffer to the executable image and restart.
			if (dataLength && ((dataLength + theRecord.m_dataCount > COLLECTION_BUFFER_SIZE) || (theRecord.m_address != nextAddress)))
			{
				m_image->addTextRegion(startAddress, buffer, dataLength);
				
				dataLength = 0;
			}
			
			// Capture addresses when starting an empty buffer.
			if (dataLength == 0)
			{
				startAddress = theRecord.m_address;
				nextAddress = startAddress;
			}
			
			// Copy record data into place in the collection buffer and update
			// size and address.
			memcpy(&buffer[dataLength], theRecord.m_data, theRecord.m_dataCount);
			dataLength += theRecord.m_dataCount;
			nextAddress += theRecord.m_dataCount;
		}
		else if (!m_hasEntryRecord)
		{
			// look for S7,8,9 records
			bool isEntryPointRecord = theRecord.m_type == 7 || theRecord.m_type == 8 || theRecord.m_type == 9;
			if (isEntryPointRecord)
			{
				// save off the entry point record so we don't have to scan again
				memcpy(&m_entryRecord, &theRecord, sizeof(m_entryRecord));
				m_hasEntryRecord = true;
			}
		}
	}
	
	// Add any leftover data in the collection buffer to the executable image.
	if (dataLength)
	{
		m_image->addTextRegion(startAddress, buffer, dataLength);
	}
}

