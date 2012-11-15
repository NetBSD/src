/*
 * File:	SourceFile.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "SourceFile.h"
#include "ELFSourceFile.h"
#include "SRecordSourceFile.h"
#include <assert.h>
#include "format_string.h"
#include "SearchPath.h"

using namespace elftosb;

//! The supported file types are currently:
//!		- ELF files
//!		- Motorola S-record files
//!		- Binary files
//!
//! Any file that is not picked up by the other subclasses will result in a
//! an instance of BinaryDataFile.
//!
//! \return An instance of the correct subclass of SourceFile for the given path.
//!
//! \exception std::runtime_error Thrown if the file cannot be opened.
//!
//! \see elftosb::ELFSourceFile
//! \see elftosb::SRecordSourceFile
//! \see elftosb::BinarySourceFile
SourceFile * SourceFile::openFile(const std::string & path)
{
	// Search for file using search paths
	std::string actualPath;
	bool found = PathSearcher::getGlobalSearcher().search(path, PathSearcher::kFindFile, true, actualPath);
	if (!found)
	{
		throw std::runtime_error(format_string("unable to find file %s\n", path.c_str()));
	}

	std::ifstream testStream(actualPath.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!testStream.is_open())
	{
		throw std::runtime_error(format_string("failed to open file: %s", actualPath.c_str()));
	}
	
	// catch exceptions so we can close the file stream
	try
	{
		if (ELFSourceFile::isELFFile(testStream))
		{
			testStream.close();
			return new ELFSourceFile(actualPath);
		}
		else if (SRecordSourceFile::isSRecordFile(testStream))
		{
			testStream.close();
			return new SRecordSourceFile(actualPath);
		}
		
		// treat it as a binary file
		testStream.close();
		return new BinarySourceFile(actualPath);
	}
	catch (...)
	{
		testStream.close();
		throw;
	}
}

SourceFile::SourceFile(const std::string & path)
:	m_path(path), m_stream()
{
}

//! The file is closed if it had been left opened.
//!
SourceFile::~SourceFile()
{
	if (isOpen())
	{
		m_stream->close();
	}
}

//! \exception std::runtime_error Raised if the file could not be opened successfully.
void SourceFile::open()
{
	assert(!isOpen());
	m_stream = new std::ifstream(m_path.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!m_stream->is_open())
	{
		throw std::runtime_error(format_string("failed to open file: %s", m_path.c_str()));
	}
}

void SourceFile::close()
{
	assert(isOpen());
	
	m_stream->close();
	m_stream.safe_delete();
}

unsigned SourceFile::getSize()
{
	bool wasOpen = isOpen();
	std::ifstream::pos_type oldPosition;
	
	if (!wasOpen)
	{
		open();
	}
	
	assert(m_stream);
	oldPosition = m_stream->tellg();
	m_stream->seekg(0, std::ios_base::end);
	unsigned resultSize = m_stream->tellg();
	m_stream->seekg(oldPosition);
	
	if (!wasOpen)
	{
		close();
	}
	
	return resultSize;
}

//! If the file does not support named sections, or if there is not a
//! section with the given name, this method may return NULL.
//!
//! This method is just a small wrapper that creates an
//! FixedMatcher string matcher instance and uses the createDataSource()
//! that takes a reference to a StringMatcher.
DataSource * SourceFile::createDataSource(const std::string & section)
{
	FixedMatcher matcher(section);
	return createDataSource(matcher);
}

DataTarget * SourceFile::createDataTargetForEntryPoint()
{
	if (!hasEntryPoint())
	{
		return NULL;
	}
	
	return new ConstantDataTarget(getEntryPointAddress());
}

DataSource * BinarySourceFile::createDataSource()
{
	std::istream * fileStream = getStream();
	assert(fileStream);
	
	// get stream size
	fileStream->seekg(0, std::ios_base::end);
	int length = fileStream->tellg();
	
	// allocate buffer
	smart_array_ptr<uint8_t> data = new uint8_t[length];
//	if (!data)
//	{
//	    throw std::bad_alloc();
//	}
	
	// read entire file into the buffer
	fileStream->seekg(0, std::ios_base::beg);
	if (fileStream->read((char *)data.get(), length).bad())
	{
		throw std::runtime_error(format_string("unexpected end of file: %s", m_path.c_str()));
	}
	
	// create the data source. the buffer is copied, so we can dispose of it.
	return new UnmappedDataSource(data, length);
}
