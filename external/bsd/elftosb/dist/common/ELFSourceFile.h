/*
 * File:	ELFSourceFile.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_ELFSourceFile_h_)
#define _ELFSourceFile_h_

#include "SourceFile.h"
#include "StELFFile.h"
#include "smart_ptr.h"
#include "DataSource.h"
#include "DataTarget.h"
#include "ELF.h"

namespace elftosb
{

//! Set of supported compiler toolsets.
enum elf_toolset_t
{
	kUnknownToolset,	//!< Unknown.
	kGHSToolset,		//!< Green Hills Software MULTI
	kGCCToolset,		//!< GNU GCC
	kADSToolset			//!< ARM UK RealView
};

//! Options for handling the .secinfo section in GHS-produced ELF files.
enum secinfo_clear_t
{
	// Default value for the .secinfo action.
	kSecinfoDefault,

	//! Ignore the .secinfo section if present. The standard ELF loading
	//! rules are followed.
	kSecinfoIgnore,

	//! The boot ROM clears only those SHT_NOBITS sections present in .secinfo.
	kSecinfoROMClear,
	
	//! The C startup is responsible for clearing sections. No fill commands
	//! are generated for any SHT_NOBITS sections.
	kSecinfoCStartupClear
};

/*!
 * \brief Executable and Loading Format (ELF) source file.
 */
class ELFSourceFile : public SourceFile
{
public:
	//! \brief Default constructor.
	ELFSourceFile(const std::string & path);
	
	//! \brief Destructor.
	virtual ~ELFSourceFile();
	
	//! \brief Identifies whether the stream contains an ELF file.
	static bool isELFFile(std::istream & stream);
	
	//! \name Opening and closing
	//@{
	//! \brief Opens the file.
	virtual void open();
	
	//! \brief Closes the file.
	virtual void close();
	//@}
	
	//! \name Format capabilities
	//@{
	virtual bool supportsNamedSections() const { return true; }
	virtual bool supportsNamedSymbols() const { return true; }
	//@}
	
	//! \name Data source
	//@{
	//! \brief Creates a data source from the entire file.
	virtual DataSource * createDataSource();
	
	//! \brief Creates a data source from one or more sections of the file.
	virtual DataSource * createDataSource(StringMatcher & matcher);
	//@}
	
	//! \name Entry point
	//@{
	//! \brief Returns true if an entry point was set in the file.
	virtual bool hasEntryPoint();
	
	//! \brief Returns the entry point address.
	virtual uint32_t getEntryPointAddress();
	//@}
	
	//! \name Data target
	//@{
	virtual DataTarget * createDataTargetForSection(const std::string & section);
	virtual DataTarget * createDataTargetForSymbol(const std::string & symbol);
	//@}
	
	//! \name Symbols
	//@{
	//! \brief Returns whether a symbol exists in the source file.
	virtual bool hasSymbol(const std::string & name);
	
	//! \brief Returns the value of a symbol.
	virtual uint32_t getSymbolValue(const std::string & name);
	
	//! \brief Returns the size of a symbol.
	virtual unsigned getSymbolSize(const std::string & name);
	//@}
	
	//! \name Direct ELF format access
	//@{
	//! \brief Returns the underlying StELFFile object.
	StELFFile * getELFFile() { return m_file; }
	
	//! \brief Gets information about a symbol in the ELF file.
	bool lookupSymbol(const std::string & name, Elf32_Sym & info);
	//@}

protected:
	smart_ptr<StELFFile> m_file;	//!< Parser for the ELF file.
	elf_toolset_t m_toolset;	//!< Toolset that produced the ELF file.
	secinfo_clear_t m_secinfoOption;	//!< How to deal with the .secinfo section. Ignored if the toolset is not GHS.

protected:
	//! \brief Parses the toolset option value.
	elf_toolset_t readToolsetOption();

	//! \brief Reads the secinfoClear option.
	secinfo_clear_t readSecinfoClearOption();
	
protected:
	/*!
	 * \brief A data source with ELF file sections as the contents.
	 *
	 * Each segment of this data source corresponds directly with a named section
	 * of the ELF file it represents. When the data source is created, it contains
	 * no segments. Segments are created with the addSection() method, which takes
	 * the index of an ELF section and creates a corresponding segment.
	 *
	 * Two segment subclasses are used with this data source. The first, ProgBitsSegment,
	 * is used to represent sections whose type is #SHT_PROGBITS. These sections have
	 * binary data stored in the ELF file. The second segment type is NoBitsSegment.
	 * It is used to represent sections whose type is #SHT_NOBITS. These sections have
	 * no data, but simply allocate a region of memory to be filled with zeroes.
	 * As such, the NoBitsSegment class is a subclass of DataSource::PatternSegment.
	 */
	class ELFDataSource : public DataSource
	{
	public:
		/*!
		 * \brief Represents one named #SHT_PROGBITS section within the ELF file.
		 */
		class ProgBitsSegment : public DataSource::Segment
		{
		public:
			ProgBitsSegment(ELFDataSource & source, StELFFile * elf, unsigned index);
			
			virtual unsigned getData(unsigned offset, unsigned maxBytes, uint8_t * buffer);
			virtual unsigned getLength();
		
			virtual bool hasNaturalLocation() { return true; }
			virtual uint32_t getBaseAddress();
		
		protected:
			StELFFile * m_elf;	//!< The format parser instance for this ELF file.
			unsigned m_sectionIndex;	//!< The index of the section this segment represents.
		};
		
		/*!
		 * \brief Represents one named #SHT_NOBITS section within the ELF file.
		 *
		 * This segment class is a subclass of DataSource::PatternSegment since it
		 * represents a region of memory to be filled with zeroes.
		 */
		class NoBitsSegment : public DataSource::PatternSegment
		{
		public:
			NoBitsSegment(ELFDataSource & source, StELFFile * elf, unsigned index);
			
			virtual unsigned getLength();
		
			virtual bool hasNaturalLocation() { return true; }
			virtual uint32_t getBaseAddress();
		
		protected:
			StELFFile * m_elf;	//!< The format parser instance for this ELF file.
			unsigned m_sectionIndex;	//!< The index of the section this segment represents.
		};
		
	public:
		//! \brief Default constructor.
		ELFDataSource(StELFFile * elf) : DataSource(), m_elf(elf) {}
		
		//! \brief Destructor.
		virtual ~ELFDataSource();

		//! Set the option to control .secinfo usage.
		inline void setSecinfoOption(secinfo_clear_t option) { m_secinfoOption = option; }
		
		//! \brief Adds the ELF section at position \a sectionIndex to the data source.
		void addSection(unsigned sectionIndex);
		
		//! \brief Returns the number of segments in the source.
		virtual unsigned getSegmentCount() { return (unsigned)m_segments.size(); }
		
		//! \brief Returns the segment at position \a index.
		virtual DataSource::Segment * getSegmentAt(unsigned index) { return m_segments[index]; }
		
	protected:
		StELFFile * m_elf;	//!< The ELF file parser.
		secinfo_clear_t m_secinfoOption;	//!< How to deal with the .secinfo section. Ignored if the toolset is not GHS.
		
		typedef std::vector<DataSource::Segment*> segment_vector_t;	//!< A list of segment instances.
		segment_vector_t m_segments;	//!< The segments of this data source.
	};

};

}; // namespace elftosb

#endif // _ELFSourceFile_h_
