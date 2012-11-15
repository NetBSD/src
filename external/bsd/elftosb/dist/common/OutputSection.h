/*
 * File:	OutputSection.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_OutputSection_h_)
#define _OutputSection_h_

#include "Operation.h"
#include "smart_ptr.h"
#include "Blob.h"
#include "OptionContext.h"

namespace elftosb
{

/*!
 * @brief Base class for data model of sections of the output file.
 */
class OutputSection
{
public:
	OutputSection() : m_id(0), m_options(0) {}
	OutputSection(uint32_t identifier) : m_id(identifier), m_options(0) {}
	virtual ~OutputSection() {}
	
	void setIdentifier(uint32_t identifier) { m_id = identifier; }
	uint32_t getIdentifier() const { return m_id; }
	
	//! \brief Set the option context.
	//!
	//! The output section object will assume ownership of the option context
	//! and delete it when the section is deleted.
	inline void setOptions(OptionContext * context) { m_options = context; }
	
	//! \brief Return the option context.
	inline const OptionContext * getOptions() const { return m_options; }
	
protected:
	uint32_t m_id;	//!< Unique identifier.
	smart_ptr<OptionContext> m_options;	//!< Options associated with just this section.
};

/*!
 * @brief A section of the output that contains boot operations.
 */
class OperationSequenceSection : public OutputSection
{
public:
	OperationSequenceSection() : OutputSection() {}
	OperationSequenceSection(uint32_t identifier) : OutputSection(identifier) {}
	
	OperationSequence & getSequence() { return m_sequence; }

protected:
	OperationSequence m_sequence;
};

/*!
 * @brief A section of the output file that contains arbitrary binary data.
 */
class BinaryDataSection : public OutputSection, public Blob
{
public:
	BinaryDataSection() : OutputSection(), Blob() {}
	BinaryDataSection(uint32_t identifier) : OutputSection(identifier), Blob() {}
};

}; // namespace elftosb

#endif // _OutputSection_h_
