/*
 * File:	StELFFile.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_StELFFile_h_)
#define _StELFFile_h_

#include "stdafx.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdexcept>
#include "ELF.h"

//! Variations of the ARM ELF format.
typedef enum {
	eARMVariant = 1,	//!< Standard ARM ELF specification.
	eGHSVariant,		//!< Green Hills Software variant.
	eGCCVariant		//!< GNU Compiler Collection variant.
} ELFVariant_t;

//! Possible ARM ELF symbol types.
typedef enum {
	eUnknownSymbol,
	eARMSymbol,
	eThumbSymbol,
	eDataSymbol
} ARMSymbolType_t;

/*!
 * \brief Parser for Executable and Linking Format (ELF) files.
 *
 * The stream passed into the constructor needs to stay open for the life
 * of the object. This is because calls to getSectionDataAtIndex() and
 * getSegmentDataAtIndex() read the data directly from the input stream.
 */
class StELFFile
{
public:
	typedef std::vector<Elf32_Shdr>::const_iterator const_section_iterator;
	typedef std::vector<Elf32_Phdr>::const_iterator const_segment_iterator;
	
public:
	//! \brief Constructor.
	StELFFile(std::istream & inStream);
	
	//! \brief Destructor.
	virtual ~StELFFile();
	
	//! \name File format variant
	//@{
	//! \brief Return the ELF format variant to which this file is set.
	virtual ELFVariant_t ELFVariant() { return m_elfVariant; }
	
	//! \brief Set the ELF format variation to either #eARMVariant or #eGHSVariant.
	virtual void setELFVariant(ELFVariant_t variant) { m_elfVariant = variant; }
	//@}
	
	//! \name File name
	//@{
	virtual void setName(const std::string & inName) { m_name = inName; }
	virtual std::string getName() const { return m_name; }
	//@}
	
	//! \name ELF header
	//@{
	//! \brief Returns the ELF file header.
	inline const Elf32_Ehdr & getFileHeader() const { return m_header; }
	//@}
	
	//! \name Sections
	//! Methods pertaining to the object file's sections.
	//@{
	//! \brief Returns the number of sections in the file.
	inline unsigned getSectionCount() const { return static_cast<unsigned>(m_sectionHeaders.size()); }
	
	//! \brief Returns a reference to section number \a inIndex.
	const Elf32_Shdr & getSectionAtIndex(unsigned inIndex) const;
	
	inline const_section_iterator getSectionBegin() const { return m_sectionHeaders.begin(); }
	inline const_section_iterator getSectionEnd() const { return m_sectionHeaders.end(); }
	
	//! \brief Returns the index of the section with the name \a inName.
	unsigned getIndexOfSectionWithName(const std::string & inName);
	
	//! \brief Returns the data for the section.
	uint8_t * getSectionDataAtIndex(unsigned inIndex);
	
	//! \brief Returns the data for the section.
	uint8_t * getSectionData(const_section_iterator inSection);
	//@}
	
	//! \name Segments
	//! Methods for accessing the file's program headers for segments.
	//@{
	//! \brief Returns the number of segments, or program headers, in the file.
	inline unsigned getSegmentCount() const { return static_cast<unsigned>(m_programHeaders.size()); }
	
	//! \brief Returns a reference to the given segment.
	const Elf32_Phdr & getSegmentAtIndex(unsigned inIndex) const;

	inline const_segment_iterator getSegmentBegin() const { return m_programHeaders.begin(); }
	inline const_segment_iterator getSegmentEnd() const { return m_programHeaders.end(); }
	
	//! \brief Returns the data of the specified segment.
	uint8_t * getSegmentDataAtIndex(unsigned inIndex);
	
	//! \brief Returns the data of the specified segment.
	uint8_t * getSegmentData(const_segment_iterator inSegment);
	//@}
	
	//! \name String table
	//! Methods for accessing the string tables.
	//@{
	//! \brief Returns a string from the file's section name string table.
	std::string getSectionNameAtIndex(unsigned inIndex);
	
	//! \brief Returns a string from any string table in the object file.
	std::string getStringAtIndex(unsigned inStringTableSectionIndex, unsigned inStringIndex);
	//@}
	
	//! \name Symbol table
	//! Methods for accessing the object file's symbol table. Currently only
	//! a single symbol table with the section name ".symtab" is supported.
	//@{
	//! \brief Returns the number of symbols in the default ".symtab" symbol table.
	unsigned getSymbolCount();
	
	//! \brief Returns the symbol with index \a inIndex.
	const Elf32_Sym & getSymbolAtIndex(unsigned inIndex);
	
	//! \brief Returns the section index of the string table containing symbol names.
	unsigned getSymbolNameStringTableIndex() const;
	
	//! \brief Returns the name of the symbol described by \a inSymbol.
	std::string getSymbolName(const Elf32_Sym & inSymbol);
	
	unsigned getIndexOfSymbolAtAddress(uint32_t symbolAddress, bool strict=true);
	
	ARMSymbolType_t getTypeOfSymbolAtIndex(unsigned symbolIndex);
	//@}
	
	//! \name Debugging
	//@{
	void dumpSections();
	void dumpSymbolTable();
	//@}

protected:
	std::istream & m_stream;	//!< The source stream for the ELF file.
	ELFVariant_t m_elfVariant;	//!< Variant of the ARM ELF format specification.
	std::string m_name;			//!< File name. (optional)
	Elf32_Ehdr m_header;	//!< The ELF file header.
	std::vector<Elf32_Shdr> m_sectionHeaders;	//!< All of the section headers.
	std::vector<Elf32_Phdr> m_programHeaders;	//!< All of the program headers.
	unsigned m_symbolTableIndex;	//!< Index of ".symtab" section, or #SHN_UNDEF if not present.
	
	/*!
	 * Little structure containing information about cached section data.
	 */
	struct SectionDataInfo
	{
		uint8_t * m_data;	//!< Pointer to section data.
		unsigned m_size;	//!< Section data size in bytes.
		bool m_swapped;	//!< Has this section been byte swapped yet? Used for symbol table.
	};
	typedef std::map<unsigned, SectionDataInfo> SectionDataMap;
	SectionDataMap m_sectionDataCache;	//!< Cached data of sections.
	
	//! \brief Reads a section's data either from cache or from disk.
	SectionDataInfo & getCachedSectionData(unsigned inSectionIndex);
	
	//! \brief Reads the file, section, and program headers into memory.
	void readFileHeaders();
	
	uint8_t * readSectionData(const Elf32_Shdr & inHeader);
	uint8_t * readSegmentData(const Elf32_Phdr & inHeader);
	
	//! \brief Byte swaps the symbol table data into host endianness.
	void byteSwapSymbolTable(const Elf32_Shdr & header, SectionDataInfo & info);
};

/*!
 * \brief Simple exception thrown to indicate an error in the input ELF file format.
 */
class StELFFileException : public std::runtime_error
{
public:
	//! \brief Default constructor.
	StELFFileException(const std::string & inMessage) : std::runtime_error(inMessage) {}
};

#endif // _StELFFile_h_
