/*
 * File:	BootImageGenerator.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_BootImageGenerator_h_)
#define _BootImageGenerator_h_

#include "OutputSection.h"
#include "BootImage.h"
#include "OptionContext.h"

namespace elftosb
{

/*!
 * \brief Abstract base class for generators of specific boot image formats.
 *
 * Subclasses implement a concrete generator for a certain boot image format, but
 * they all have the same interface.
 *
 * After creating an instance of a subclass the user adds OutputSection objects
 * to the generator. These objects describe discrete sections within the resulting
 * boot image file. If the format does not support multiple sections then only
 * the first will be used.
 *
 * Options that are common to all boot image formats are handled by methods
 * defined in this class. These are the current common options:
 *	- productVersion
 *	- componentVersion
 *	- driveTag
 */
class BootImageGenerator
{
public:
	//! \brief Constructor.
	BootImageGenerator() {}
	
	//! \brief Destructor.
	virtual ~BootImageGenerator() {}
	
	//! \brief Add another section to the output.
	void addOutputSection(OutputSection * section) { m_sections.push_back(section); }
	
	//! \brief Set the global option context.
	void setOptionContext(OptionContext * context) { m_options = context; }
	
	//! \brief Pure virtual method to generate the output BootImage from input sections.
	virtual BootImage * generate()=0;
	
protected:
	//! Type for a list of model output sections.
	typedef std::vector<OutputSection*> section_vector_t;
	
	section_vector_t m_sections;	//!< Requested output sections.
	OptionContext * m_options;	//!< Global option context.
    
    //! \brief Handle common product and component version options.
    void processVersionOptions(BootImage * image);
	
	//! \brief Handle the common option which sets the system drive tag.
	void processDriveTagOption(BootImage * image);
};

}; // namespace elftosb

#endif // _BootImageGenerator_h_

