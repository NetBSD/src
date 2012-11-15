/*
 * File:	BootImage.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_BootImage_h_)
#define _BootImage_h_

#include <iostream>
#include "Version.h"

namespace elftosb
{

/*!
 * \brief Abstract base class for all boot image format classes.
 *
 * Provides virtual methods for all of the common features between different
 * boot image formats. These are the product and component version numbers
 * and the drive tag.
 *
 * Also provided is the virtual method writeToStream() that lets the caller
 * stream out the boot image without knowing the underlying format type.
 */
class BootImage
{
public:
	//! \brief Constructor.
	BootImage() {}
	
	//! \brief Destructor.
	virtual ~BootImage() {}
	
	//! \name Versions
	//@{
	virtual void setProductVersion(const version_t & version)=0;
	virtual void setComponentVersion(const version_t & version)=0;
	//@}
	
	//! \brief Specify the drive tag to be set in the output file header.
	virtual void setDriveTag(uint16_t tag)=0;

	//! \brief Returns a string containing the preferred file extension for image format.
	virtual std::string getFileExtension() const=0;
	
	//! \brief Write the boot image to an output stream.
	virtual void writeToStream(std::ostream & stream)=0;
};

}; // namespace elftosb

#endif // _BootImage_h_

