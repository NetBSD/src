/*
 * File:	SourceFile.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_SourceFile_h_)
#define _SourceFile_h_

#include <string>
#include <iostream>
#include <fstream>
#include "smart_ptr.h"
#include "DataSource.h"
#include "DataTarget.h"
#include "StringMatcher.h"
#include "OptionContext.h"

namespace elftosb
{

/*!
 * \brief Abstract base class for a source file containing executable code.
 *
 * The purpose of this class cluster is to provide a common interface for
 * accessing the contents of different file formats. This is accomplished
 * through several small sets of methods along with the DataSource and
 * DataTarget classes.
 *
 * The primary interface for creating instances of SourceFile is the static
 * SourceFile::openFile() function. It will create the correct subclass of
 * SourceFile by inspecting the file to determine its type.
 */
class SourceFile
{
public:
	// \brief Factory function that creates the correct subclass of SourceFile.
	static SourceFile * openFile(const std::string & path);
	
public:
	//! \brief Default constructor.
	SourceFile(const std::string & path);
	
	//! \brief Destructor.
	virtual ~SourceFile();
	
	//! \brief Set the option context.
	//!
	//! The source file will take ownership of the @a context and delete it
	//! when the source file is itself deleted.
	inline void setOptions(OptionContext * context) { m_options = context; }
	
	//! \brief Return the option context.
	inline const OptionContext * getOptions() const { return m_options; }
	
	//! \brief Returns the path to the file.
	inline const std::string & getPath() const { return m_path; }
	
	//! \brief Get the size in bytes of the file.
	unsigned getSize();
	
	//! \name Opening and closing
	//@{
	//! \brief Opens the file.
	virtual void open();
	
	//! \brief Closes the file.
	virtual void close();
	
	//! \brief Returns whether the file is already open.
	virtual bool isOpen() const { return (bool)m_stream && const_cast<std::ifstream*>(m_stream.get())->is_open(); }
	//@}
	
	//! \name Format capabilities
	//@{
	virtual bool supportsNamedSections() const=0;
	virtual bool supportsNamedSymbols() const=0;
	//@}
	
	//! \name Data source creation
	//@{
	//! \brief Creates a data source from the entire file.
	virtual DataSource * createDataSource()=0;
	
	//! \brief Creates a data source out of one or more sections of the file.
	//!
	//! The \a selector object is used to perform the section name comparison.
	//! If the file does not support named sections, or if there is not a
	//! section with the given name, this method may return NULL.
	virtual DataSource * createDataSource(StringMatcher & matcher) { return NULL; }
	
	//! \brief Creates a data source out of one section of the file.
	virtual DataSource * createDataSource(const std::string & section);
	//@}
	
	//! \name Entry point
	//@{
	//! \brief Returns true if an entry point was set in the file.
	virtual bool hasEntryPoint()=0;
	
	//! \brief Returns the entry point address.
	virtual uint32_t getEntryPointAddress() { return 0; }
	//@}
	
	//! \name Data target creation
	//@{
	virtual DataTarget * createDataTargetForSection(const std::string & section) { return NULL; }
	virtual DataTarget * createDataTargetForSymbol(const std::string & symbol) { return NULL; }
	virtual DataTarget * createDataTargetForEntryPoint();
	//@}
	
	//! \name Symbols
	//@{
	//! \brief Returns whether a symbol exists in the source file.
	virtual bool hasSymbol(const std::string & name) { return false; }
	
	//! \brief Returns the value of a symbol.
	virtual uint32_t getSymbolValue(const std::string & name) { return 0; }
	
	//! \brief Returns the size of a symbol.
	virtual unsigned getSymbolSize(const std::string & name) { return 0; }
	//@}
	
protected:
	std::string m_path;	//!< Path to the file.
	smart_ptr<std::ifstream> m_stream;	//!< File stream, or NULL if file is closed.
	smart_ptr<OptionContext> m_options;	//!< Table of option values.
	
	//! \brief Internal access to the input stream object.
	inline std::ifstream * getStream() { return m_stream; }
};

/*!
 * \brief Binary data file.
 */
class BinarySourceFile : public SourceFile
{
public:
	//! \brief Default constructor.
	BinarySourceFile(const std::string & path) : SourceFile(path) {}
	
	//! \name Format capabilities
	//@{
	virtual bool supportsNamedSections() const { return false; }
	virtual bool supportsNamedSymbols() const { return false; }
	//@}
	
	//! \brief Creates an unmapped data source from the entire file.
	virtual DataSource * createDataSource();
	
	virtual bool hasEntryPoint() { return false; }
};

}; // namespace elftosb

#endif // _SourceFile_h_
