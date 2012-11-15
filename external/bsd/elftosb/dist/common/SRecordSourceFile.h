/*
 * File:	SRecordSourceFile.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_SRecordSourceFile_h_)
#define _SRecordSourceFile_h_

#include "SourceFile.h"
#include "StSRecordFile.h"
#include "StExecutableImage.h"

namespace elftosb
{

/*!
 * \brief Executable file in the Motorola S-record format.
 *
 * Instead of presenting each S-record in the file separately, this class
 * builds up a memory image of all of the records. Records next to each other
 * in memory are coalesced into a single memory region. The data source that
 * is returned from createDataSource() exposes these regions as its segments.
 *
 * Because the S-record format does not support the concepts, no support is
 * provided for named sections or symbols.
 */
class SRecordSourceFile : public SourceFile
{
public:
	//! \brief Default constructor.
	SRecordSourceFile(const std::string & path);
	
	//! \brief Destructor.
	virtual ~SRecordSourceFile() {}
	
	//! \brief Test whether the \a stream contains a valid S-record file.
	static bool isSRecordFile(std::istream & stream);
	
	//! \name Opening and closing
	//@{
	//! \brief Opens the file.
	virtual void open();
	
	//! \brief Closes the file.
	virtual void close();
	//@}
	
	//! \name Format capabilities
	//@{
	virtual bool supportsNamedSections() const { return false; }
	virtual bool supportsNamedSymbols() const { return false; }
	//@}
	
	//! \name Data sources
	//@{
	//! \brief Returns data source for the entire file.
	virtual DataSource * createDataSource();
	//@}
	
	//! \name Entry point
	//@{
	//! \brief Returns true if an entry point was set in the file.
	virtual bool hasEntryPoint();
	
	//! \brief Returns the entry point address.
	virtual uint32_t getEntryPointAddress();
	//@}

protected:
	StSRecordFile * m_file;	//!< S-record parser instance.
	StExecutableImage * m_image;	//!< Memory image of the S-record file.
	bool m_hasEntryRecord;	//!< Whether an S7,8,9 record was found.
	StSRecordFile::SRecord m_entryRecord;	//!< Record for the entry point.
	
protected:
	//! \brief Build memory image of the S-record file.
	void buildMemoryImage();
};

}; // namespace elftosb

#endif // _SRecordSourceFile_h_
