/*
 *  BootImageGenerator.cpp
 *  elftosb
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "BootImageGenerator.h"
#include "Logging.h"

//! Name of product version option.
#define kProductVersionOption "productVersion"

//! Name of component version option.
#define kComponentVersionOption "componentVersion"

//! Name of option that specifies the drive tag for this .sb file.
#define kDriveTagOption "driveTag"

using namespace elftosb;

void BootImageGenerator::processVersionOptions(BootImage * image)
{
	// bail if no option context was set
	if (!m_options)
	{
		return;
	}
	
	const StringValue * stringValue;
	version_t version;
	
    // productVersion
	if (m_options->hasOption(kProductVersionOption))
	{
		stringValue = dynamic_cast<const StringValue *>(m_options->getOption(kProductVersionOption));
		if (stringValue)
		{
			version.set(*stringValue);
			image->setProductVersion(version);
		}
        else
        {
            Log::log(Logger::WARNING, "warning: productVersion option is an unexpected type\n");
        }
	}
	
    // componentVersion
	if (m_options->hasOption(kComponentVersionOption))
	{
		stringValue = dynamic_cast<const StringValue *>(m_options->getOption(kComponentVersionOption));
		if (stringValue)
		{
			version.set(*stringValue);
			image->setComponentVersion(version);
		}
        else
        {
            Log::log(Logger::WARNING, "warning: componentVersion option is an unexpected type\n");
        }
	}
}

void BootImageGenerator::processDriveTagOption(BootImage * image)
{
	if (m_options->hasOption(kDriveTagOption))
	{
		const IntegerValue * intValue = dynamic_cast<const IntegerValue *>(m_options->getOption(kDriveTagOption));
		if (intValue)
		{
			image->setDriveTag(intValue->getValue());
		}
        else
        {
            Log::log(Logger::WARNING, "warning: driveTag option is an unexpected type\n");
        }
	}
}

