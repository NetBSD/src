/*
 * File:	StELFFile.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "StELFFile.h"
#include <ios>
#include <stdexcept>
#include <stdio.h>
#include "EndianUtilities.h"

//! \exception StELFFileException is thrown if there is a problem with the file format.
//!
StELFFile::StELFFile(std::istream & inStream)
:	m_stream(inStream)
{
	readFileHeaders();
}

//! Disposes of the string table data.
StELFFile::~StELFFile()
{
	SectionDataMap::iterator it = m_sectionDataCache.begin();
	for (; it != m_sectionDataCache.end(); ++it)
	{
		SectionDataInfo & info = it->second;
		if (info.m_data != NULL)
		{
			delete [] info.m_data;
		}
	}
}

//! \exception StELFFileException is thrown if the file is not an ELF file.
//!
void StELFFile::readFileHeaders()
{
	// move read head to beginning of stream
	m_stream.seekg(0, std::ios_base::beg);
	
	// read ELF header
	m_stream.read(reinterpret_cast<char *>(&m_header), sizeof(m_header));
	if (m_stream.bad())
	{
		throw StELFFileException("could not read file header");
	}
	
	// convert endianness
	m_header.e_type = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_type);
	m_header.e_machine = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_machine);
	m_header.e_version = ENDIAN_LITTLE_TO_HOST_U32(m_header.e_version);
	m_header.e_entry = ENDIAN_LITTLE_TO_HOST_U32(m_header.e_entry);
	m_header.e_phoff = ENDIAN_LITTLE_TO_HOST_U32(m_header.e_phoff);
	m_header.e_shoff = ENDIAN_LITTLE_TO_HOST_U32(m_header.e_shoff);
	m_header.e_flags = ENDIAN_LITTLE_TO_HOST_U32(m_header.e_flags);
	m_header.e_ehsize = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_ehsize);
	m_header.e_phentsize = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_phentsize);
	m_header.e_phnum = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_phnum);
	m_header.e_shentsize = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_shentsize);
	m_header.e_shnum = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_shnum);
	m_header.e_shstrndx = ENDIAN_LITTLE_TO_HOST_U16(m_header.e_shstrndx);
	
	// check magic number
	if (!(m_header.e_ident[EI_MAG0] == ELFMAG0 && m_header.e_ident[EI_MAG1] == ELFMAG1 && m_header.e_ident[EI_MAG2] == ELFMAG2 && m_header.e_ident[EI_MAG3] == ELFMAG3))
	{
		throw StELFFileException("invalid magic number in ELF header");
	}
	
	try
	{
		int i;
		
		// read section headers
		if (m_header.e_shoff != 0 && m_header.e_shnum > 0)
		{
			Elf32_Shdr sectionHeader;
			for (i=0; i < m_header.e_shnum; ++i)
			{
				m_stream.seekg(m_header.e_shoff + m_header.e_shentsize * i, std::ios::beg);
				m_stream.read(reinterpret_cast<char *>(&sectionHeader), sizeof(sectionHeader));
				if (m_stream.bad())
				{
					throw StELFFileException("could not read section header");
				}
				
				// convert endianness
				sectionHeader.sh_name = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_name);
				sectionHeader.sh_type = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_type);
				sectionHeader.sh_flags = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_flags);
				sectionHeader.sh_addr = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_addr);
				sectionHeader.sh_offset = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_offset);
				sectionHeader.sh_size = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_size);
				sectionHeader.sh_link = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_link);
				sectionHeader.sh_info = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_info);
				sectionHeader.sh_addralign = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_addralign);
				sectionHeader.sh_entsize = ENDIAN_LITTLE_TO_HOST_U32(sectionHeader.sh_entsize);
				
				m_sectionHeaders.push_back(sectionHeader);
			}
		}
		
		// read program headers
		if (m_header.e_phoff != 0 && m_header.e_phnum > 0)
		{
			Elf32_Phdr programHeader;
			for (i=0; i < m_header.e_phnum; ++i)
			{
				m_stream.seekg(m_header.e_phoff + m_header.e_phentsize * i, std::ios::beg);
				m_stream.read(reinterpret_cast<char *>(&programHeader), sizeof(programHeader));
				if (m_stream.bad())
				{
					throw StELFFileException("could not read program header");
				}
				
				// convert endianness
				programHeader.p_type = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_type);
				programHeader.p_offset = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_type);
				programHeader.p_vaddr = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_vaddr);
				programHeader.p_paddr = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_paddr);
				programHeader.p_filesz = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_filesz);
				programHeader.p_memsz = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_memsz);
				programHeader.p_flags = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_flags);
				programHeader.p_align = ENDIAN_LITTLE_TO_HOST_U32(programHeader.p_align);
				
				m_programHeaders.push_back(programHeader);
			}
		}
		
		// look up symbol table section index
		{
		    std::string symtab_section_name(SYMTAB_SECTION_NAME);
		    m_symbolTableIndex = getIndexOfSectionWithName(symtab_section_name);
		}
	}
	catch (...)
	{
		throw StELFFileException("error reading file");
	}
}

const Elf32_Shdr & StELFFile::getSectionAtIndex(unsigned inIndex) const
{
	if (inIndex > m_sectionHeaders.size())
		throw std::invalid_argument("inIndex");
	
	return m_sectionHeaders[inIndex];
}

//! If there is not a matching section, then #SHN_UNDEF is returned instead.
//!
unsigned StELFFile::getIndexOfSectionWithName(const std::string & inName)
{
	unsigned sectionIndex = 0;
	const_section_iterator it = getSectionBegin();
	for (; it != getSectionEnd(); ++it, ++sectionIndex)
	{
		const Elf32_Shdr & header = *it;
		if (header.sh_name != 0)
		{
			std::string sectionName = getSectionNameAtIndex(header.sh_name);
			if (inName == sectionName)
				return sectionIndex;
		}
	}
	
	// no matching section
	return SHN_UNDEF;
}

//! The pointer returned from this method must be freed with the delete array operator (i.e., delete []).
//! If either the section data offset (sh_offset) or the section size (sh_size) are 0, then NULL will
//! be returned instead.
//!
//! The data is read directly from the input stream passed into the constructor. The stream must
//! still be open, or an exception will be thrown.
//!
//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::getSectionDataAtIndex(unsigned inIndex)
{
	return readSectionData(m_sectionHeaders[inIndex]);
}

//! The pointer returned from this method must be freed with the delete array operator (i.e., delete []).
//! If either the section data offset (sh_offset) or the section size (sh_size) are 0, then NULL will
//! be returned instead.
//!
//! The data is read directly from the input stream passed into the constructor. The stream must
//! still be open, or an exception will be thrown.
//!
//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::getSectionData(const_section_iterator inSection)
{
	return readSectionData(*inSection);
}

//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::readSectionData(const Elf32_Shdr & inHeader)
{
	// check for empty data
	if (inHeader.sh_offset == 0 || inHeader.sh_size == 0)
		return NULL;
		
	uint8_t * sectionData = new uint8_t[inHeader.sh_size];
	
	try
	{
		m_stream.seekg(inHeader.sh_offset, std::ios::beg);
		m_stream.read(reinterpret_cast<char *>(sectionData), inHeader.sh_size);
		if (m_stream.bad())
			throw StELFFileException("could not read entire section");
	}
	catch (StELFFileException)
	{
		throw;
	}
	catch (...)
	{
		throw StELFFileException("error reading section data");
	}
		
	return sectionData;
}

const Elf32_Phdr & StELFFile::getSegmentAtIndex(unsigned inIndex) const
{
	if (inIndex > m_programHeaders.size())
		throw std::invalid_argument("inIndex");
	
	return m_programHeaders[inIndex];
}

//! The pointer returned from this method must be freed with the delete array operator (i.e., delete []).
//! If either the segment offset (p_offset) or the segment file size (p_filesz) are 0, then NULL will
//! be returned instead.
//!
//! The data is read directly from the input stream passed into the constructor. The stream must
//! still be open, or an exception will be thrown.
//!
//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::getSegmentDataAtIndex(unsigned inIndex)
{
	return readSegmentData(m_programHeaders[inIndex]);
}

//! The pointer returned from this method must be freed with the delete array operator (i.e., delete []).
//! If either the segment offset (p_offset) or the segment file size (p_filesz) are 0, then NULL will
//! be returned instead.
//!
//! The data is read directly from the input stream passed into the constructor. The stream must
//! still be open, or an exception will be thrown.
//!
//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::getSegmentData(const_segment_iterator inSegment)
{
	return readSegmentData(*inSegment);
}
	
//! \exception StELFFileException is thrown if an error occurs while reading the file.
//! \exception std::bad_alloc is thrown if memory for the data cannot be allocated.
uint8_t * StELFFile::readSegmentData(const Elf32_Phdr & inHeader)
{
	// check for empty data
	if (inHeader.p_offset == 0 || inHeader.p_filesz== 0)
		return NULL;
		
	uint8_t * segmentData = new uint8_t[inHeader.p_filesz];
	
	try
	{
		m_stream.seekg(inHeader.p_offset, std::ios::beg);
		m_stream.read(reinterpret_cast<char *>(segmentData), inHeader.p_filesz);
		if (m_stream.bad())
			throw StELFFileException("could not read entire segment");
	}
	catch (StELFFileException)
	{
		throw;
	}
	catch (...)
	{
		throw StELFFileException("error reading segment data");
	}
	
	return segmentData;
}

//! If the index is out of range, or if there is no string table in the file, then
//! an empty string will be returned instead. This will also happen when the index
//! is either 0 or the last byte in the table, since the table begins and ends with
//! zero bytes.
std::string StELFFile::getSectionNameAtIndex(unsigned inIndex)
{
	// make sure there's a section name string table
	if (m_header.e_shstrndx == SHN_UNDEF)
		return std::string("");
	
	return getStringAtIndex(m_header.e_shstrndx, inIndex);
}

//! \exception std::invalid_argument is thrown if the section identified by \a
//!		inStringTableSectionIndex is not actually a string table, or if \a
//!		inStringIndex is out of range for the string table.
std::string StELFFile::getStringAtIndex(unsigned inStringTableSectionIndex, unsigned inStringIndex)
{
	// check section type
	const Elf32_Shdr & header = getSectionAtIndex(inStringTableSectionIndex);
	if (header.sh_type != SHT_STRTAB)
		throw std::invalid_argument("inStringTableSectionIndex");
	
	if (inStringIndex >= header.sh_size)
		throw std::invalid_argument("inStringTableSectionIndex");
	
	// check cache
	SectionDataInfo & info = getCachedSectionData(inStringTableSectionIndex);
	return std::string(&reinterpret_cast<char *>(info.m_data)[inStringIndex]);
}

StELFFile::SectionDataInfo & StELFFile::getCachedSectionData(unsigned inSectionIndex)
{
	// check cache
	SectionDataMap::iterator it = m_sectionDataCache.find(inSectionIndex);
	if (it != m_sectionDataCache.end())
		return it->second;
	
	// not in cache, add it
	const Elf32_Shdr & header = getSectionAtIndex(inSectionIndex);
	uint8_t * data = getSectionDataAtIndex(inSectionIndex);
	
	SectionDataInfo info;
	info.m_data = data;
	info.m_size = header.sh_size;
	
	m_sectionDataCache[inSectionIndex] = info;
	return m_sectionDataCache[inSectionIndex];
}

//! The number of entries in the symbol table is the symbol table section size
//! divided by the size of each symbol entry (the #Elf32_Shdr::sh_entsize field of the
//! symbol table section header).
unsigned StELFFile::getSymbolCount()
{
	if (m_symbolTableIndex == SHN_UNDEF)
		return 0;
	
	const Elf32_Shdr & header = getSectionAtIndex(m_symbolTableIndex);
	return header.sh_size / header.sh_entsize;
}

//! \exception std::invalid_argument is thrown if \a inIndex is out of range.]
//!
const Elf32_Sym & StELFFile::getSymbolAtIndex(unsigned inIndex)
{
	// get section data
	const Elf32_Shdr & header = getSectionAtIndex(m_symbolTableIndex);
	SectionDataInfo & info = getCachedSectionData(m_symbolTableIndex);
	
	// has the symbol table been byte swapped yet?
	if (!info.m_swapped)
	{
		byteSwapSymbolTable(header, info);
	}
	
	unsigned symbolOffset = header.sh_entsize * inIndex;
	if (symbolOffset >= info.m_size)
	{
		throw std::invalid_argument("inIndex");
	}
	
	Elf32_Sym * symbol = reinterpret_cast<Elf32_Sym *>(&info.m_data[symbolOffset]);
	return *symbol;
}

void StELFFile::byteSwapSymbolTable(const Elf32_Shdr & header, SectionDataInfo & info)
{
	unsigned symbolCount = getSymbolCount();
	unsigned i = 0;
	unsigned symbolOffset = 0;
	
	for (; i < symbolCount; ++i, symbolOffset += header.sh_entsize)
	{
		Elf32_Sym * symbol = reinterpret_cast<Elf32_Sym *>(&info.m_data[symbolOffset]);
		symbol->st_name = ENDIAN_LITTLE_TO_HOST_U32(symbol->st_name);
		symbol->st_value = ENDIAN_LITTLE_TO_HOST_U32(symbol->st_value);
		symbol->st_size = ENDIAN_LITTLE_TO_HOST_U32(symbol->st_size);
		symbol->st_shndx = ENDIAN_LITTLE_TO_HOST_U16(symbol->st_shndx);
	}
	
	// remember that we've byte swapped the symbols
	info.m_swapped = true;
}

unsigned StELFFile::getSymbolNameStringTableIndex() const
{
	const Elf32_Shdr & header = getSectionAtIndex(m_symbolTableIndex);
	return header.sh_link;
}

std::string StELFFile::getSymbolName(const Elf32_Sym & inSymbol)
{
	unsigned symbolStringTableIndex = getSymbolNameStringTableIndex();
	return getStringAtIndex(symbolStringTableIndex, inSymbol.st_name);
}

//! Returns STN_UNDEF if it cannot find a symbol at the given \a symbolAddress.
unsigned StELFFile::getIndexOfSymbolAtAddress(uint32_t symbolAddress, bool strict)
{
	unsigned symbolCount = getSymbolCount();
	unsigned symbolIndex = 0;
	for (; symbolIndex < symbolCount; ++symbolIndex)
	{
		const Elf32_Sym & symbol = getSymbolAtIndex(symbolIndex);
		
		// the GHS toolchain puts in STT_FUNC symbols marking the beginning and ending of each
		// file. if the entry point happens to be at the beginning of the file, the beginning-
		// of-file symbol will have the same value and type. fortunately, the size of these
		// symbols is 0 (or seems to be). we also ignore symbols that start with two dots just
		// in case.
		if (symbol.st_value == symbolAddress && (strict && ELF32_ST_TYPE(symbol.st_info) == STT_FUNC && symbol.st_size != 0))
		{
			std::string symbolName = getSymbolName(symbol);
			
			// ignore symbols that start with two dots
			if (symbolName[0] == '.' && symbolName[1] == '.')
				continue;
			
			// found the symbol!
			return symbolIndex;
		}
	}
	
	return STN_UNDEF;
}

ARMSymbolType_t StELFFile::getTypeOfSymbolAtIndex(unsigned symbolIndex)
{
	ARMSymbolType_t symType = eARMSymbol;
	const Elf32_Sym & symbol = getSymbolAtIndex(symbolIndex);
	
	if (m_elfVariant == eGHSVariant)
	{
		if (symbol.st_other & STO_THUMB)
			symType = eThumbSymbol;
	}
	else
	{
		unsigned mappingSymStart = 1;
		unsigned mappingSymCount = getSymbolCount() - 1;	// don't include first undefined symbol
		bool mapSymsFirst = (m_header.e_flags & EF_ARM_MAPSYMSFIRST) != 0;
		if (mapSymsFirst)
		{
			// first symbol '$m' is number of mapping syms
			const Elf32_Sym & mappingSymCountSym = getSymbolAtIndex(1);
			if (getSymbolName(mappingSymCountSym) == MAPPING_SYMBOL_COUNT_TAGSYM)
			{
				mappingSymCount = mappingSymCountSym.st_value;
				mappingSymStart = 2;
			}

		}
		
		uint32_t lastMappingSymAddress = 0;
		unsigned mappingSymIndex = mappingSymStart;
		for (; mappingSymIndex < mappingSymCount + mappingSymStart; ++mappingSymIndex)
		{
			const Elf32_Sym & mappingSym = getSymbolAtIndex(mappingSymIndex);
			std::string mappingSymName = getSymbolName(mappingSym);
			ARMSymbolType_t nextSymType = eUnknownSymbol;
			
			if (mappingSymName == ARM_SEQUENCE_MAPSYM)
				symType = eARMSymbol;
			else if (mappingSymName == DATA_SEQUENCE_MAPSYM)
				symType = eDataSymbol;
			else if (mappingSymName == THUMB_SEQUENCE_MAPSYM)
				symType = eThumbSymbol;
			
			if (nextSymType != eUnknownSymbol)
			{
				if (symbol.st_value >= lastMappingSymAddress && symbol.st_value < mappingSym.st_value)
					break;
				
				symType = nextSymType;
				lastMappingSymAddress = mappingSym.st_value;
			}
		}
	}
	
	return symType;
}

void StELFFile::dumpSections()
{
	unsigned count = getSectionCount();
	unsigned i = 0;
	
	const char * sectionTypes[12] = { "NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH", "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM" };
	
	for (; i < count; ++i)
	{
		const Elf32_Shdr & header = getSectionAtIndex(i);
		std::string name = getSectionNameAtIndex(header.sh_name);
		
		printf("%s: %s, 0x%08x, 0x%08x, 0x%08x, %d, %d, %d\n", name.c_str(), sectionTypes[header.sh_type], header.sh_addr, header.sh_offset, header.sh_size, header.sh_link, header.sh_info, header.sh_entsize);
	}
}

void StELFFile::dumpSymbolTable()
{
	const char * symbolTypes[5] = { "NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE" };
	const char * symbolBinding[3] = { "LOCAL", "GLOBAL", "WEAK" };
	
	unsigned count = getSymbolCount();
	unsigned i = 0;
	
	for (; i < count; ++i)
	{
		const Elf32_Sym & symbol = getSymbolAtIndex(i);
		std::string name = getSymbolName(symbol);
		
		printf("'%s': %s, %s, 0x%08x, 0x%08x, %d. 0x%08x\n", name.c_str(), symbolTypes[ELF32_ST_TYPE(symbol.st_info)], symbolBinding[ELF32_ST_BIND(symbol.st_info)], symbol.st_value, symbol.st_size, symbol.st_shndx, symbol.st_other);
	}
}



