/*
 * File:	ELFSourceFile.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "ELFSourceFile.h"
#include "Logging.h"
#include "GHSSecInfo.h"
#include <ctype.h>
#include <algorithm>
#include "string.h"

//! The name of the toolset option.
#define kToolsetOptionName "toolset"
#define kGHSToolsetName "GHS"
#define kGCCToolsetName "GCC"
#define kGNUToolsetName "GNU"
#define kADSToolsetName "ADS"

//! Name of the option to control .secinfo action.
#define kSecinfoClearOptionName "secinfoClear"
#define kSecinfoDefaultName "DEFAULT"
#define kSecinfoIgnoreName "IGNORE"
#define kSecinfoROMName "ROM"
#define kSecinfoCName "C"

using namespace elftosb;

ELFSourceFile::ELFSourceFile(const std::string & path)
:	SourceFile(path),
	m_toolset(kUnknownToolset),
	m_secinfoOption(kSecinfoDefault)
{
}

ELFSourceFile::~ELFSourceFile()
{
}

bool ELFSourceFile::isELFFile(std::istream & stream)
{
	try
	{
		StELFFile elf(stream);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void ELFSourceFile::open()
{
	// Read toolset option
	m_toolset = readToolsetOption();

	// Read option and select default value
	m_secinfoOption = readSecinfoClearOption();
	if (m_secinfoOption == kSecinfoDefault)
	{
		m_secinfoOption = kSecinfoCStartupClear;
	}
	
	// Open the stream
	SourceFile::open();
	
	m_file = new StELFFile(*m_stream);
//	m_file->dumpSections();
	
	// Set toolset in elf file object
	switch (m_toolset)
	{
        // default toolset is GHS
		case kGHSToolset:
        case kUnknownToolset:
			m_file->setELFVariant(eGHSVariant);
			break;
		case kGCCToolset:
			m_file->setELFVariant(eGCCVariant);
			break;
		case kADSToolset:
			m_file->setELFVariant(eARMVariant);
			break;
	}
}

void ELFSourceFile::close()
{
	SourceFile::close();
	
	m_file.safe_delete();
}

elf_toolset_t ELFSourceFile::readToolsetOption()
{
	do {
		const OptionContext * options = getOptions();
		if (!options || !options->hasOption(kToolsetOptionName))
		{
			break;
		}
		
		const Value * value = options->getOption(kToolsetOptionName);
		const StringValue * stringValue = dynamic_cast<const StringValue*>(value);
		if (!stringValue)
		{
			// Not a string value, warn the user.
			Log::log(Logger::WARNING, "invalid type for 'toolset' option\n");
			break;
		}
		
		std::string toolsetName = *stringValue;
		
		// convert option value to uppercase
		std::transform<std::string::const_iterator, std::string::iterator, int (*)(int)>(toolsetName.begin(), toolsetName.end(), toolsetName.begin(), toupper);
		
		if (toolsetName == kGHSToolsetName)
		{
			return kGHSToolset;
		}
		else if (toolsetName == kGCCToolsetName || toolsetName == kGNUToolsetName)
		{
			return kGCCToolset;
		}
		else if (toolsetName == kADSToolsetName)
		{
			return kADSToolset;
		}

		// Unrecognized option value, log a warning.
		Log::log(Logger::WARNING, "unrecognized value for 'toolset' option\n");
	} while (0);
	
	return kUnknownToolset;
}

//! It is up to the caller to convert from kSecinfoDefault to the actual default
//! value.
secinfo_clear_t ELFSourceFile::readSecinfoClearOption()
{
	do {
		const OptionContext * options = getOptions();
		if (!options || !options->hasOption(kSecinfoClearOptionName))
		{
			break;
		}
		
		const Value * value = options->getOption(kSecinfoClearOptionName);
		const StringValue * stringValue = dynamic_cast<const StringValue*>(value);
		if (!stringValue)
		{
			// Not a string value, warn the user.
			Log::log(Logger::WARNING, "invalid type for 'secinfoClear' option\n");
			break;
		}
		
		std::string secinfoOption = *stringValue;
		
		// convert option value to uppercase
		std::transform<std::string::const_iterator, std::string::iterator, int (*)(int)>(secinfoOption.begin(), secinfoOption.end(), secinfoOption.begin(), toupper);
		
		if (secinfoOption == kSecinfoDefaultName)
		{
			return kSecinfoDefault;
		}
		else if (secinfoOption == kSecinfoIgnoreName)
		{
			return kSecinfoIgnore;
		}
		else if (secinfoOption == kSecinfoROMName)
		{
			return kSecinfoROMClear;
		}
		else if (secinfoOption == kSecinfoCName)
		{
			return kSecinfoCStartupClear;
		}

		// Unrecognized option value, log a warning.
		Log::log(Logger::WARNING, "unrecognized value for 'secinfoClear' option\n");
	} while (0);
	
	return kSecinfoDefault;
}

//! To create a data source for all sections of the ELF file, a WildcardMatcher
//! is instantiated and passed to createDataSource(StringMatcher&).
DataSource * ELFSourceFile::createDataSource()
{
	WildcardMatcher matcher;
	return createDataSource(matcher);
}
	
DataSource * ELFSourceFile::createDataSource(StringMatcher & matcher)
{
	assert(m_file);
	ELFDataSource * source = new ELFDataSource(m_file);
	source->setSecinfoOption(m_secinfoOption);
	
	Log::log(Logger::DEBUG2, "filtering sections of file: %s\n", getPath().c_str());
	
	// We start at section 1 to skip the null section that is always first.
	unsigned index = 1;
	for (; index < m_file->getSectionCount(); ++index)
	{
		const Elf32_Shdr & header = m_file->getSectionAtIndex(index);
		std::string name = m_file->getSectionNameAtIndex(header.sh_name);
		
		// Ignore most section types
		if (!(header.sh_type == SHT_PROGBITS || header.sh_type == SHT_NOBITS))
		{
			continue;
		}

		// Ignore sections that don't have the allocate flag set.
		if ((header.sh_flags & SHF_ALLOC) == 0)
		{
			continue;
		}

		if (matcher.match(name))
		{
			Log::log(Logger::DEBUG2, "creating segment for section %s\n", name.c_str());
			source->addSection(index);
		}
		else
		{
			Log::log(Logger::DEBUG2, "section %s did not match\n", name.c_str());
		}
	}
	
	return source;
}

//! It is assumed that all ELF files have an entry point.
//!
bool ELFSourceFile::hasEntryPoint()
{
	return true;
}

//! The StELFFile::getTypeOfSymbolAtIndex() method uses different methods of determining
//! ARM/Thumb mode depending on the toolset.
uint32_t ELFSourceFile::getEntryPointAddress()
{
	uint32_t entryPoint = 0;
	
	// get entry point address
	const Elf32_Ehdr & header = m_file->getFileHeader();

	// find symbol corresponding to entry point and determine if
	// it is arm or thumb mode
	unsigned symbolIndex = m_file->getIndexOfSymbolAtAddress(header.e_entry);
	if (symbolIndex != 0)
	{
		ARMSymbolType_t symbolType = m_file->getTypeOfSymbolAtIndex(symbolIndex);
		bool entryPointIsThumb = (symbolType == eThumbSymbol);
		const Elf32_Sym & symbol = m_file->getSymbolAtIndex(symbolIndex);
		std::string symbolName = m_file->getSymbolName(symbol);

		Log::log(Logger::DEBUG2, "Entry point is %s@0x%08x (%s)\n", symbolName.c_str(), symbol.st_value, entryPointIsThumb ? "Thumb" : "ARM");

		// set entry point, setting the low bit if it is thumb mode
		entryPoint = header.e_entry + (entryPointIsThumb ? 1 : 0);
	}
	else
	{
		entryPoint = header.e_entry;
	}
	
	return entryPoint;
}

//! \return A DataTarget that describes the named section.
//! \retval NULL There was no section with the requested name.
DataTarget * ELFSourceFile::createDataTargetForSection(const std::string & section)
{
	assert(m_file);
	unsigned index = m_file->getIndexOfSectionWithName(section);
	if (index == SHN_UNDEF)
	{
		return NULL;
	}
	
	const Elf32_Shdr & sectionHeader = m_file->getSectionAtIndex(index);
	uint32_t beginAddress = sectionHeader.sh_addr;
	uint32_t endAddress = beginAddress + sectionHeader.sh_size;
	ConstantDataTarget * target = new ConstantDataTarget(beginAddress, endAddress);
	return target;
}

//! \return A DataTarget instance pointing at the requested symbol.
//! \retval NULL No symbol matching the requested name was found.
DataTarget * ELFSourceFile::createDataTargetForSymbol(const std::string & symbol)
{
	assert(m_file);
	unsigned symbolCount = m_file->getSymbolCount();
	unsigned i;
	
	for (i=0; i < symbolCount; ++i)
	{
		const Elf32_Sym & symbolHeader = m_file->getSymbolAtIndex(i);
		std::string symbolName = m_file->getSymbolName(symbolHeader);
		if (symbolName == symbol)
		{
            ARMSymbolType_t symbolType = m_file->getTypeOfSymbolAtIndex(i);
            bool symbolIsThumb = (symbolType == eThumbSymbol);
            
			uint32_t beginAddress = symbolHeader.st_value + (symbolIsThumb ? 1 : 0);
			uint32_t endAddress = beginAddress + symbolHeader.st_size;
			ConstantDataTarget * target = new ConstantDataTarget(beginAddress, endAddress);
			return target;
		}
	}
	
	// didn't find a matching symbol
	return NULL; 
}

bool ELFSourceFile::hasSymbol(const std::string & name)
{
	Elf32_Sym symbol;
	return lookupSymbol(name, symbol);
}

uint32_t ELFSourceFile::getSymbolValue(const std::string & name)
{
	unsigned symbolCount = m_file->getSymbolCount();
	unsigned i;
	
	for (i=0; i < symbolCount; ++i)
	{
		const Elf32_Sym & symbolHeader = m_file->getSymbolAtIndex(i);
		std::string symbolName = m_file->getSymbolName(symbolHeader);
		if (symbolName == name)
		{
            // If the symbol is a function, then we check to see if it is Thumb code and set bit 0 if so.
            if (ELF32_ST_TYPE(symbolHeader.st_info) == STT_FUNC)
            {
                ARMSymbolType_t symbolType = m_file->getTypeOfSymbolAtIndex(i);
                bool symbolIsThumb = (symbolType == eThumbSymbol);
                return symbolHeader.st_value + (symbolIsThumb ? 1 : 0);
            }
            else
            {
			    return symbolHeader.st_value;
            }
		}
	}
	
    // Couldn't find the symbol, so return 0.
	return 0;
}

unsigned ELFSourceFile::getSymbolSize(const std::string & name)
{
	Elf32_Sym symbol;
	if (!lookupSymbol(name, symbol))
	{
		return 0;
	}
	
	return symbol.st_size;
}

//! \param name The name of the symbol on which info is wanted.
//! \param[out] info Upon succssful return this is filled in with the symbol's information.
//!
//! \retval true The symbol was found and \a info is valid.
//! \retval false No symbol with \a name was found in the file.
bool ELFSourceFile::lookupSymbol(const std::string & name, Elf32_Sym & info)
{
	assert(m_file);
	unsigned symbolCount = m_file->getSymbolCount();
	unsigned i;
	
	for (i=0; i < symbolCount; ++i)
	{
		const Elf32_Sym & symbol = m_file->getSymbolAtIndex(i);
		std::string thisSymbolName = m_file->getSymbolName(symbol);
		
		// Is this the symbol we're looking for?
		if (thisSymbolName == name)
		{
			info = symbol;
			return true;
		}
	}
	
	// Didn't file the symbol.
	return false;
}

ELFSourceFile::ELFDataSource::~ELFDataSource()
{
	segment_vector_t::iterator it = m_segments.begin();
	for (; it != m_segments.end(); ++it)
	{
		delete *it;
	}
}

//! Not all sections will actually result in a new segment being created. Only
//! those sections whose type is #SHT_PROGBITS or #SHT_NOBITS will create
//! a new segment. Also, only sections whose size is non-zero will actually
//! create a segment.
//!
//! In addition to this, ELF files that have been marked as being created by
//! the Green Hills Software toolset have an extra step. #SHT_NOBITS sections
//! are looked up in the .secinfo section to determine if they really
//! should be filled. If not in the .secinfo table, no segment will be
//! created for the section.
void ELFSourceFile::ELFDataSource::addSection(unsigned sectionIndex)
{
	// get section info
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(sectionIndex);
	if (section.sh_size == 0)
	{
		// empty section, so ignore it
		return;
	}
	
	// create the right segment subclass based on the section type
	DataSource::Segment * segment = NULL;
	if (section.sh_type == SHT_PROGBITS)
	{
		segment = new ProgBitsSegment(*this, m_elf, sectionIndex);
	}
	else if (section.sh_type == SHT_NOBITS)
	{
		// Always add NOBITS sections by default.
		bool addNobits = true;

		// For GHS ELF files, we use the secinfoClear option to figure out what to do.
		// If set to ignore, treat like a normal ELF file and always add. If set to
		// ROM, then only clear if the section is listed in .secinfo. Otherwise if set
		// to C startup, then let the C startup do all clearing.
		if (m_elf->ELFVariant() == eGHSVariant)
		{
			GHSSecInfo secinfo(m_elf);

			// If there isn't a .secinfo section present then use the normal ELF rules
			// and always add NOBITS sections.
			if (secinfo.hasSecinfo() && m_secinfoOption != kSecinfoIgnore)
			{
				switch (m_secinfoOption)
				{
					case kSecinfoROMClear:
						addNobits = secinfo.isSectionFilled(section);
						break;

					case kSecinfoCStartupClear:
						addNobits = false;
						break;
				}
			}
		}

		if (addNobits)
		{
			segment = new NoBitsSegment(*this, m_elf, sectionIndex);
		}
		else
		{
			std::string name = m_elf->getSectionNameAtIndex(section.sh_name);
			Log::log(Logger::DEBUG2, "..section %s is not filled\n", name.c_str());
		}
	}
	
	// add segment if one was created
	if (segment)
	{
		m_segments.push_back(segment);
	}
}

ELFSourceFile::ELFDataSource::ProgBitsSegment::ProgBitsSegment(ELFDataSource & source, StELFFile * elf, unsigned index)
:	DataSource::Segment(source), m_elf(elf), m_sectionIndex(index)
{
}

unsigned ELFSourceFile::ELFDataSource::ProgBitsSegment::getData(unsigned offset, unsigned maxBytes, uint8_t * buffer)
{
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(m_sectionIndex);
	uint8_t * data = m_elf->getSectionDataAtIndex(m_sectionIndex);
	
	assert(offset < section.sh_size);
	
	unsigned copyBytes = std::min<unsigned>(section.sh_size - offset, maxBytes);
	if (copyBytes)
	{
		memcpy(buffer, &data[offset], copyBytes);
	}
	
	return copyBytes;
}

unsigned ELFSourceFile::ELFDataSource::ProgBitsSegment::getLength()
{
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(m_sectionIndex);
	return section.sh_size;
}

uint32_t ELFSourceFile::ELFDataSource::ProgBitsSegment::getBaseAddress()
{
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(m_sectionIndex);
	return section.sh_addr;
}

ELFSourceFile::ELFDataSource::NoBitsSegment::NoBitsSegment(ELFDataSource & source, StELFFile * elf, unsigned index)
:	DataSource::PatternSegment(source), m_elf(elf), m_sectionIndex(index)
{
}

unsigned ELFSourceFile::ELFDataSource::NoBitsSegment::getLength()
{
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(m_sectionIndex);
	return section.sh_size;
}

uint32_t ELFSourceFile::ELFDataSource::NoBitsSegment::getBaseAddress()
{
	const Elf32_Shdr & section = m_elf->getSectionAtIndex(m_sectionIndex);
	return section.sh_addr;
}

