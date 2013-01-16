/*
 * File:	StSRecordFile.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "stdafx.h"
#include "StSRecordFile.h"
#include "string.h"

StSRecordFile::StSRecordFile(std::istream & inStream)
:	m_stream(inStream)
{
}

//! Frees any data allocated as part of an S-record.
StSRecordFile::~StSRecordFile()
{
	const_iterator it;
	for (it = m_records.begin(); it != m_records.end(); it++)
	{
		SRecord & theRecord = (SRecord &)*it;
		if (theRecord.m_data)
		{
			delete [] theRecord.m_data;
			theRecord.m_data = NULL;
		}
	}
}

//! Just looks for "S[0-9]" as the first two characters of the file.
bool StSRecordFile::isSRecordFile()
{
	int savePosition = m_stream.tellg();
	m_stream.seekg(0, std::ios_base::beg);
	
	char buffer[2];
	m_stream.read(buffer, 2);
	bool isSRecord = (buffer[0] == 'S' && isdigit(buffer[1]));
	
	m_stream.seekg(savePosition, std::ios_base::beg);
	
	return isSRecord;
}

//! Extract records one line at a time and hand them to the parseLine()
//! method. Either CR, LF, or CRLF line endings are supported. The input
//! stream is read until EOF.
//! The parse() method must be called after the object has been constructed
//! before any of the records will become accessible.
//! \exception StSRecordParseException will be thrown if any error occurs while
//!		parsing the input.
void StSRecordFile::parse()
{
	// back to start of stream
	m_stream.seekg(0, std::ios_base::beg);
	
	std::string thisLine;
	
	do {
		char thisChar;
		m_stream.get(thisChar);
		
		if (thisChar == '\r' || thisChar == '\n')
		{
			// skip the LF in a CRLF
			if (thisChar == '\r' && m_stream.peek() == '\n')
				m_stream.ignore();
			
			// parse line if it's not empty
			if (!thisLine.empty())
			{
				parseLine(thisLine);
			
				// reset line
				thisLine.clear();
			}
		}
		else
		{
			thisLine += thisChar;
		}
	} while (!m_stream.eof());
}

bool StSRecordFile::isHexDigit(char c)
{
	return (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

int StSRecordFile::hexDigitToInt(char digit)
{
	if (isdigit(digit))
		return digit - '0';
	else if (digit >= 'a' && digit <= 'f')
		return 10 + digit - 'a';
	else if (digit >= 'A' && digit <= 'F')
		return 10 + digit - 'A';
	
	// unknow char
	return 0;
}

//! \exception StSRecordParseException is thrown if either of the nibble characters
//!		is not a valid hex digit.
int StSRecordFile::readHexByte(std::string & inString, int inIndex)
{
	char nibbleCharHi= inString[inIndex];
	char nibbleCharLo = inString[inIndex + 1];
	
	// must be hex digits
	if (!(isHexDigit(nibbleCharHi) && isHexDigit(nibbleCharLo)))
    {
		throw StSRecordParseException("invalid hex digit");
    }
	
	return (hexDigitToInt(nibbleCharHi) << 4) | hexDigitToInt(nibbleCharLo);
}

//! \brief Parses individual S-records.
//!
//! Takes a single S-record line as input and appends a new SRecord struct
//! to the m_records vector.
//! \exception StSRecordParseException will be thrown if any error occurs while
//!		parsing \a inLine.
void StSRecordFile::parseLine(std::string & inLine)
{
	int checksum = 0;
	SRecord newRecord;
	memset(&newRecord, 0, sizeof(newRecord));
	
	// must start with "S" and be at least a certain length
	if (inLine[0] != SRECORD_START_CHAR && inLine.length() >= SRECORD_MIN_LENGTH)
    {
        throw StSRecordParseException("invalid record length");
    }

	// parse type field
	char typeChar = inLine[1];
	if (!isdigit(typeChar))
    {
		throw StSRecordParseException("invalid S-record type");
    }
	newRecord.m_type = typeChar - '0';
	
	// parse count field
	newRecord.m_count = readHexByte(inLine, 2);
	checksum += newRecord.m_count;
	
	// verify the record length now that we know the count
	if (inLine.length() != 4 + newRecord.m_count * 2)
    {
		throw StSRecordParseException("invalid record length");
    }
	
	// get address length
	int addressLength;	// len in bytes
	bool hasData = false;
	switch (newRecord.m_type)
	{
		case 0:     // contains header information
			addressLength = 2;
			hasData = true;
			break;
		case 1:     // data record with 2-byte address
			addressLength = 2;
			hasData = true;
			break;
		case 2:     // data record with 3-byte address
			addressLength = 3;
			hasData = true;
			break;
		case 3:     // data record with 4-byte address
			addressLength = 4;
			hasData = true;
			break;
		case 5:     // the 2-byte address field contains a count of all prior S1, S2, and S3 records
			addressLength = 2;
			break;
		case 7:     // entry point record with 4-byte address
			addressLength = 4;
			break;
		case 8:     // entry point record with 3-byte address
			addressLength = 3;
			break;
		case 9:     // entry point record with 2-byte address
			addressLength = 2;
			break;
		default:
			// unrecognized type
			//throw StSRecordParseException("unknown S-record type");
            break;
	}
	
	// read address
	int address = 0;
	int i;
	for (i=0; i < addressLength; ++i)
	{
		int addressByte = readHexByte(inLine, SRECORD_ADDRESS_START_CHAR_INDEX + i * 2);
		address = (address << 8) | addressByte;
		checksum += addressByte;
	}
	newRecord.m_address = address;
		
	// read data
	if (hasData)
	{
		int dataStartCharIndex = 4 + addressLength * 2;
		int dataLength = newRecord.m_count - addressLength - 1; // total rem - addr - cksum (in bytes)
		uint8_t * data = new uint8_t[dataLength];
		
		for (i=0; i < dataLength; ++i)
		{
			int dataByte = readHexByte(inLine, dataStartCharIndex + i * 2);
			data[i] = dataByte;
			checksum += dataByte;
		}
		
		newRecord.m_data = data;
		newRecord.m_dataCount = dataLength;
	}
	
	// read and compare checksum byte
	checksum = (~checksum) & 0xff;	// low byte of one's complement of sum of other bytes
	newRecord.m_checksum = readHexByte(inLine, (int)inLine.length() - 2);
	if (checksum != newRecord.m_checksum)
    {
		throw StSRecordParseException("invalid checksum");
    }
	
	// now save the new S-record
	m_records.push_back(newRecord);
}
