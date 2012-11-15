/*
 * File:	EncoreBootImageGenerator.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "EncoreBootImageGenerator.h"
#include "Logging.h"

#define kFlagsOption "flags"
#define kSectionFlagsOption "sectionFlags"
#define kProductVersionOption "productVersion"
#define kComponentVersionOption "componentVersion"
#define kAlignmentOption "alignment"
#define kCleartextOption "cleartext"

using namespace elftosb;

BootImage * EncoreBootImageGenerator::generate()
{
	EncoreBootImage * image = new EncoreBootImage();
	
	// process each output section
	section_vector_t::iterator it = m_sections.begin();
	for (; it != m_sections.end(); ++it)
	{
		OutputSection * section = *it;
		
		OperationSequenceSection * opSection = dynamic_cast<OperationSequenceSection*>(section);
		if (opSection)
		{
			processOperationSection(opSection, image);
			continue;
		}
		
		BinaryDataSection * dataSection = dynamic_cast<BinaryDataSection*>(section);
		if (dataSection)
		{
			processDataSection(dataSection, image);
			continue;
		}
		
		Log::log(Logger::WARNING, "warning: unexpected output section type\n");
	}
	
	// handle global options that affect the image
	processOptions(image);
	
	return image;
}

void EncoreBootImageGenerator::processOptions(EncoreBootImage * image)
{
	// bail if no option context was set
	if (!m_options)
	{
		return;
	}
	
	if (m_options->hasOption(kFlagsOption))
	{
        const IntegerValue * intValue = dynamic_cast<const IntegerValue *>(m_options->getOption(kFlagsOption));
		if (intValue)
		{
			image->setFlags(intValue->getValue());
		}
        else
        {
            Log::log(Logger::WARNING, "warning: flags option is an unexpected type\n");
        }
	}
	
    // handle common options
	processVersionOptions(image);
	processDriveTagOption(image);
}

void EncoreBootImageGenerator::processSectionOptions(EncoreBootImage::Section * imageSection, OutputSection * modelSection)
{
	// Get options context for this output section.
	const OptionContext * context = modelSection->getOptions();
	if (!context)
	{
		return;
	}
	
	// Check for and handle "sectionFlags" option.
	if (context->hasOption(kSectionFlagsOption))
	{
		const Value * value = context->getOption(kSectionFlagsOption);
        const IntegerValue * intValue = dynamic_cast<const IntegerValue *>(value);
		if (intValue)
		{
			// set explicit flags for this section
			imageSection->setFlags(intValue->getValue());
		}
        else
        {
            Log::log(Logger::WARNING, "warning: sectionFlags option is an unexpected type\n");
        }
	}
	
	// Check for and handle "alignment" option.
	if (context->hasOption(kAlignmentOption))
	{
		const Value * value = context->getOption(kAlignmentOption);
        const IntegerValue * intValue = dynamic_cast<const IntegerValue *>(value);
		if (intValue)
		{
			// verify alignment value
			if (intValue->getValue() < EncoreBootImage::BOOT_IMAGE_MINIMUM_SECTION_ALIGNMENT)
			{
				Log::log(Logger::WARNING, "warning: alignment option value must be 16 or greater\n");
			}
			
			imageSection->setAlignment(intValue->getValue());
		}
        else
        {
            Log::log(Logger::WARNING, "warning: alignment option is an unexpected type\n");
        }
	}
	
	// Check for and handle "cleartext" option.
	if (context->hasOption(kCleartextOption))
	{
		const Value * value = context->getOption(kCleartextOption);
        const IntegerValue * intValue = dynamic_cast<const IntegerValue *>(value);
		if (intValue)
		{
			bool leaveUnencrypted = intValue->getValue() != 0;
			imageSection->setLeaveUnencrypted(leaveUnencrypted);
		}
        else
        {
            Log::log(Logger::WARNING, "warning: cleartext option is an unexpected type\n");
        }
	}
}

void EncoreBootImageGenerator::processOperationSection(OperationSequenceSection * section, EncoreBootImage * image)
{
	EncoreBootImage::BootSection * newSection = new EncoreBootImage::BootSection(section->getIdentifier());
	
	OperationSequence & sequence = section->getSequence();
	OperationSequence::iterator_t it = sequence.begin();
	for (; it != sequence.end(); ++it)
	{
		Operation * op = *it;
		
		LoadOperation * loadOp = dynamic_cast<LoadOperation*>(op);
		if (loadOp)
		{
			processLoadOperation(loadOp, newSection);
			continue;
		}
		
		ExecuteOperation * execOp = dynamic_cast<ExecuteOperation*>(op);
		if (execOp)
		{
			processExecuteOperation(execOp, newSection);
			continue;
		}
		
		BootModeOperation * modeOp = dynamic_cast<BootModeOperation*>(op);
		if (modeOp)
		{
			processBootModeOperation(modeOp, newSection);
			continue;
		}
		
		Log::log(Logger::WARNING, "warning: unexpected operation type\n");
	}
	
	// Deal with options that apply to sections.
	processSectionOptions(newSection, section);
	
	// add the boot section to the image
	image->addSection(newSection);
}

void EncoreBootImageGenerator::processLoadOperation(LoadOperation * op, EncoreBootImage::BootSection * section)
{
	DataSource * source = op->getSource();
	DataTarget * target = op->getTarget();
	
	// other sources get handled the same way
	unsigned segmentCount = source->getSegmentCount();
	unsigned index = 0;
	for (; index < segmentCount; ++index)
	{
		DataSource::Segment * segment = source->getSegmentAt(index);
		DataTarget::AddressRange range = target->getRangeForSegment(*source, *segment);
		unsigned rangeLength = range.m_end - range.m_begin;
		
		// handle a pattern segment as a special case to create a fill command
		DataSource::PatternSegment * patternSegment = dynamic_cast<DataSource::PatternSegment*>(segment);
		if (patternSegment)
		{
			SizedIntegerValue & pattern = patternSegment->getPattern();
			
			EncoreBootImage::FillCommand * command = new EncoreBootImage::FillCommand();
			command->setAddress(range.m_begin);
			command->setFillCount(rangeLength);
			setFillPatternFromValue(*command, pattern);
			
			section->addCommand(command);
			continue;
		}
		
		// get the data from the segment
		uint8_t * data = new uint8_t[rangeLength];
		segment->getData(0, rangeLength, data);
		
		// create the boot command
		EncoreBootImage::LoadCommand * command = new EncoreBootImage::LoadCommand();
		command->setData(data, rangeLength); // Makes a copy of the data buffer.
		command->setLoadAddress(range.m_begin);
		command->setDCD(op->isDCDLoad());
		
		section->addCommand(command);
        
        // Free the segment buffer.
        delete [] data;
	}
}

void EncoreBootImageGenerator::setFillPatternFromValue(EncoreBootImage::FillCommand & command, SizedIntegerValue & pattern)
{
	uint32_t u32PatternValue = pattern.getValue() & pattern.getWordSizeMask();
	switch (pattern.getWordSize())
	{
		case kWordSize:
		{
			command.setPattern(u32PatternValue);
			break;
		}
		
		case kHalfWordSize:
		{
			uint16_t u16PatternValue = static_cast<uint16_t>(u32PatternValue);
			command.setPattern(u16PatternValue);
			break;
		}
		
		case kByteSize:
		{
			uint8_t u8PatternValue = static_cast<uint8_t>(u32PatternValue);
			command.setPattern(u8PatternValue);
		}
	}
}

void EncoreBootImageGenerator::processExecuteOperation(ExecuteOperation * op, EncoreBootImage::BootSection * section)
{
	DataTarget * target = op->getTarget();
	uint32_t arg = static_cast<uint32_t>(op->getArgument());
	
	EncoreBootImage::JumpCommand * command;
	switch (op->getExecuteType())
	{
		case ExecuteOperation::kJump:
			command = new EncoreBootImage::JumpCommand();
			break;
		
		case ExecuteOperation::kCall:
			command = new EncoreBootImage::CallCommand();
			break;
	}
	
	command->setAddress(target->getBeginAddress());
	command->setArgument(arg);
	command->setIsHAB(op->isHAB());
	
	section->addCommand(command);
}

void EncoreBootImageGenerator::processBootModeOperation(BootModeOperation * op, EncoreBootImage::BootSection * section)
{
	EncoreBootImage::ModeCommand * command = new EncoreBootImage::ModeCommand();
	command->setBootMode(op->getBootMode());
	
	section->addCommand(command);
}

void EncoreBootImageGenerator::processDataSection(BinaryDataSection * section, EncoreBootImage * image)
{
	EncoreBootImage::DataSection * dataSection = new EncoreBootImage::DataSection(section->getIdentifier());
	dataSection->setData(section->getData(), section->getLength());
	
	// Handle alignment option.
	processSectionOptions(dataSection, section);
	
	image->addSection(dataSection);
}

