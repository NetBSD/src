/*
 * File:	EncoreBootImage.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "EncoreBootImage.h"
#include <stdexcept>
#include <algorithm>
#include <time.h>
#include "crc.h"
#include "SHA1.h"
#include "Random.h"
#include "rijndael.h"
#include "RijndaelCBCMAC.h"
#include "Logging.h"
#include "EndianUtilities.h"

using namespace elftosb;

EncoreBootImage::EncoreBootImage()
:	m_headerFlags(0),
	m_productVersion(),
	m_componentVersion(),
	m_driveTag(0)
{
}

EncoreBootImage::~EncoreBootImage()
{
	// dispose of all sections
	section_iterator_t it = beginSection();
	for (; it != endSection(); ++it)
	{
		delete *it;
	}
}

//! \exception std::runtime_error Raised if \a newSection has the same tag as a previously
//!		added section.
void EncoreBootImage::addSection(Section * newSection)
{
	// check for another section with this tag
	section_iterator_t it = beginSection();
	for (; it != endSection(); ++it)
	{
		if ((*it)->getIdentifier() == newSection->getIdentifier())
		{
			throw std::runtime_error("new section with non-unique tag");
		}
	}
	
	// no conflicting section tags, so add it
	m_sections.push_back(newSection);
	
	// tell the image who owns it now
	newSection->setImage(this);
}

EncoreBootImage::section_iterator_t EncoreBootImage::findSection(Section * section)
{
	return std::find(beginSection(), endSection(), section);
}

void EncoreBootImage::setProductVersion(const version_t & version)
{
	m_productVersion = version;
}

void EncoreBootImage::setComponentVersion(const version_t & version)
{
	m_componentVersion = version;
}

//! \todo Optimize writing section data. Right now it only writes one block at a
//!		time, which is of course quite slow (in relative terms).
//!	\todo Refactor this into several different methods for writing each region
//!		of the image. Use a context structure to keep track of shared data between
//!		each of the methods.
//! \todo Refactor the section and boot tag writing code to only have a single
//!		copy of the block writing and encryption loop.
void EncoreBootImage::writeToStream(std::ostream & stream)
{
	// always generate the session key or DEK even if image is unencrypted
	m_sessionKey.randomize();
	
	// prepare to compute CBC-MACs with each KEK
	unsigned i;
	smart_array_ptr<RijndaelCBCMAC> macs(0);
	if (isEncrypted())
	{
		macs = new RijndaelCBCMAC[m_keys.size()];
		for (i=0; i < m_keys.size(); ++i)
		{
			RijndaelCBCMAC mac(m_keys[i]);
			(macs.get())[i] = mac;
		}
	}
	
	// prepare to compute SHA-1 digest over entire image
	CSHA1 hash;
	hash.Reset();
	
	// count of total blocks written to the file
	unsigned fileBlocksWritten = 0;

	// we need some pieces of the header down below
	boot_image_header_t imageHeader;
	prepareImageHeader(imageHeader);
	
	// write plaintext header
	{
		// write header
		assert(sizeOfPaddingForCipherBlocks(sizeof(boot_image_header_t)) == 0);
		stream.write(reinterpret_cast<char *>(&imageHeader), sizeof(imageHeader));
		fileBlocksWritten += numberOfCipherBlocks(sizeof(imageHeader));
		
		// update CBC-MAC over image header
		if (isEncrypted())
		{
			for (i=0; i < m_keys.size(); ++i)
			{
				(macs.get())[i].update(reinterpret_cast<uint8_t *>(&imageHeader), sizeof(imageHeader));
			}
		}
		
		// update SHA-1
		hash.Update(reinterpret_cast<uint8_t *>(&imageHeader), sizeof(imageHeader));
	}
	
	// write plaintext section table
	{
		section_iterator_t it = beginSection();
		for (; it != endSection(); ++it)
		{
			Section * section = *it;
			
			// write header for this section
			assert(sizeOfPaddingForCipherBlocks(sizeof(section_header_t)) == 0);
			section_header_t sectionHeader;
			section->fillSectionHeader(sectionHeader);
			stream.write(reinterpret_cast<char *>(&sectionHeader), sizeof(sectionHeader));
			fileBlocksWritten += numberOfCipherBlocks(sizeof(sectionHeader));
			
			// update CBC-MAC over this entry
			if (isEncrypted())
			{
				for (i=0; i < m_keys.size(); ++i)
				{
					(macs.get())[i].update(reinterpret_cast<uint8_t *>(&sectionHeader), sizeof(sectionHeader));
				}
			}
			
			// update SHA-1
			hash.Update(reinterpret_cast<uint8_t *>(&sectionHeader), sizeof(sectionHeader));
		}
	}
	
	// finished with the CBC-MAC
	if (isEncrypted())
	{
		for (i=0; i < m_keys.size(); ++i)
		{
			(macs.get())[i].finalize();
		}
	}
	
	// write key dictionary
	if (isEncrypted())
	{
		key_iterator_t it = beginKeys();
		for (i=0; it != endKeys(); ++it, ++i)
		{
			// write CBC-MAC result for this key, then update SHA-1
			RijndaelCBCMAC & mac = (macs.get())[i];
			const RijndaelCBCMAC::block_t & macResult = mac.getMAC();
			stream.write(reinterpret_cast<const char *>(&macResult), sizeof(RijndaelCBCMAC::block_t));
			hash.Update(reinterpret_cast<const uint8_t *>(&macResult), sizeof(RijndaelCBCMAC::block_t));
			fileBlocksWritten++;
			
			// encrypt DEK with this key, write it out, and update image digest
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Encrypt, *it, Rijndael::Key16Bytes, imageHeader.m_iv);
			AESKey<128>::key_t wrappedSessionKey;
			cipher.blockEncrypt(m_sessionKey, sizeof(AESKey<128>::key_t) * 8, wrappedSessionKey);
			stream.write(reinterpret_cast<char *>(&wrappedSessionKey), sizeof(wrappedSessionKey));
			hash.Update(reinterpret_cast<uint8_t *>(&wrappedSessionKey), sizeof(wrappedSessionKey));
			fileBlocksWritten++;
		}
	}
	
	// write sections and boot tags
	{
		section_iterator_t it = beginSection();
		for (; it != endSection(); ++it)
		{
			section_iterator_t itCopy = it;
			bool isLastSection = (++itCopy == endSection());
			
			Section * section = *it;
			cipher_block_t block;
			unsigned blockCount = section->getBlockCount();
			unsigned blocksWritten = 0;
			
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, imageHeader.m_iv);
			
			// Compute the number of padding blocks needed to align the section. This first
			// call to getPadBlockCountForOffset() passes an offset that excludes
			// the boot tag for this section.
			unsigned paddingBlocks = getPadBlockCountForSection(section, fileBlocksWritten);
			
			// Insert nop commands as padding to align the start of the section, if
			// the section has special alignment requirements.
			NopCommand nop;
			while (paddingBlocks--)
			{
				blockCount = nop.getBlockCount();
				blocksWritten = 0;
				while (blocksWritten < blockCount)
				{
					nop.getBlocks(blocksWritten, 1, &block);
					
					if (isEncrypted())
					{
						// re-init after encrypt to update IV
						cipher.blockEncrypt(block, sizeof(cipher_block_t) * 8, block);
						cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, block);
					}
					
					stream.write(reinterpret_cast<char *>(&block), sizeof(cipher_block_t));
					hash.Update(reinterpret_cast<uint8_t *>(&block), sizeof(cipher_block_t));
					
					blocksWritten++;
					fileBlocksWritten++;
				}
			}
			
			// reinit cipher for boot tag
			cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, imageHeader.m_iv);
			
			// write boot tag
			TagCommand tag(*section);
			tag.setLast(isLastSection);
			if (!isLastSection)
			{
				// If this isn't the last section, the tag needs to include any
				// padding for the next section in its length, otherwise the ROM
				// won't be able to find the next section's boot tag.
				unsigned nextSectionOffset = fileBlocksWritten + section->getBlockCount() + 1;
				tag.setSectionLength(section->getBlockCount() + getPadBlockCountForSection(*itCopy, nextSectionOffset));
			}
			blockCount = tag.getBlockCount();
			blocksWritten = 0;
			while (blocksWritten < blockCount)
			{
				tag.getBlocks(blocksWritten, 1, &block);
				
				if (isEncrypted())
				{
					// re-init after encrypt to update IV
					cipher.blockEncrypt(block, sizeof(cipher_block_t) * 8, block);
					cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, block);
				}
				
				stream.write(reinterpret_cast<char *>(&block), sizeof(cipher_block_t));
				hash.Update(reinterpret_cast<uint8_t *>(&block), sizeof(cipher_block_t));
				
				blocksWritten++;
				fileBlocksWritten++;
			}
			
			// reinit cipher for section data
			cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, imageHeader.m_iv);
			
			// write section data
			blockCount = section->getBlockCount();
			blocksWritten = 0;
			while (blocksWritten < blockCount)
			{
				section->getBlocks(blocksWritten, 1, &block);
				
				// Only encrypt the section contents if the entire boot image is encrypted
				// and the section doesn't have the "leave unencrypted" flag set. Even if the
				// section is unencrypted the boot tag will remain encrypted.
				if (isEncrypted() && !section->getLeaveUnencrypted())
				{
					// re-init after encrypt to update IV
					cipher.blockEncrypt(block, sizeof(cipher_block_t) * 8, block);
					cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, block);
				}
				
				stream.write(reinterpret_cast<char *>(&block), sizeof(cipher_block_t));
				hash.Update(reinterpret_cast<uint8_t *>(&block), sizeof(cipher_block_t));
				
				blocksWritten++;
				fileBlocksWritten++;
			}
		}
	}
	
	// write SHA-1 digest over entire image
	{
		// allocate enough room for digest and bytes to pad out to the next cipher block
		const unsigned padBytes = sizeOfPaddingForCipherBlocks(sizeof(sha1_digest_t));
		unsigned digestBlocksSize = sizeof(sha1_digest_t) + padBytes;
		smart_array_ptr<uint8_t> digestBlocks = new uint8_t[digestBlocksSize];
		hash.Final();
		hash.GetHash(digestBlocks.get());
		
		// set the pad bytes to random values
		RandomNumberGenerator rng;
		rng.generateBlock(&(digestBlocks.get())[sizeof(sha1_digest_t)], padBytes);
		
		// encrypt with session key
		if (isEncrypted())
		{
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Encrypt, m_sessionKey, Rijndael::Key16Bytes, imageHeader.m_iv);
			cipher.blockEncrypt(digestBlocks.get(), digestBlocksSize * 8, digestBlocks.get());
		}
		
		// write to the stream
		stream.write(reinterpret_cast<char *>(digestBlocks.get()), digestBlocksSize);
	}
}

void EncoreBootImage::prepareImageHeader(boot_image_header_t & header)
{
	// get identifier for the first bootable section
	Section * firstBootSection = findFirstBootableSection();
	section_id_t firstBootSectionID = 0;
	if (firstBootSection)
	{
		firstBootSectionID = firstBootSection->getIdentifier();
	}
	
	// fill in header fields
	header.m_signature[0] = 'S';
	header.m_signature[1] = 'T';
	header.m_signature[2] = 'M';
	header.m_signature[3] = 'P';
	header.m_majorVersion = ROM_BOOT_IMAGE_MAJOR_VERSION;
	header.m_minorVersion = ROM_BOOT_IMAGE_MINOR_VERSION;
	header.m_flags = ENDIAN_HOST_TO_LITTLE_U16(m_headerFlags);
	header.m_imageBlocks = ENDIAN_HOST_TO_LITTLE_U32(getImageSize());
	header.m_firstBootableSectionID = ENDIAN_HOST_TO_LITTLE_U32(firstBootSectionID);
	header.m_keyCount = ENDIAN_HOST_TO_LITTLE_U16((uint16_t)m_keys.size());
	header.m_headerBlocks = ENDIAN_HOST_TO_LITTLE_U16((uint16_t)numberOfCipherBlocks(sizeof(header)));
	header.m_sectionCount = ENDIAN_HOST_TO_LITTLE_U16((uint16_t)m_sections.size());
	header.m_sectionHeaderSize = ENDIAN_HOST_TO_LITTLE_U16((uint16_t)numberOfCipherBlocks(sizeof(section_header_t)));
	header.m_signature2[0] = 's';
	header.m_signature2[1] = 'g';
	header.m_signature2[2] = 't';
	header.m_signature2[3] = 'l';
	header.m_timestamp = ENDIAN_HOST_TO_LITTLE_U64(getTimestamp());
	header.m_driveTag = m_driveTag;

	// Prepare version fields by converting them to the correct byte order.
	header.m_productVersion = m_productVersion;
	header.m_componentVersion = m_componentVersion;
	header.m_productVersion.fixByteOrder();
	header.m_componentVersion.fixByteOrder();

	// the fields are dependant on others
	header.m_keyDictionaryBlock = ENDIAN_HOST_TO_LITTLE_U16(header.m_headerBlocks + header.m_sectionCount * header.m_sectionHeaderSize);
	header.m_firstBootTagBlock = ENDIAN_HOST_TO_LITTLE_U32(header.m_keyDictionaryBlock + header.m_keyCount * 2);
	
	// generate random pad bytes
	RandomNumberGenerator rng;
	rng.generateBlock(header.m_padding0, sizeof(header.m_padding0));
	rng.generateBlock(header.m_padding1, sizeof(header.m_padding1));
	
	// compute SHA-1 digest over the image header
	uint8_t * message = reinterpret_cast<uint8_t *>(&header.m_signature);
	uint32_t length = static_cast<uint32_t>(sizeof(header) - sizeof(header.m_digest)); // include padding
	
	CSHA1 hash;
	hash.Reset();
	hash.Update(message, length);
	hash.Final();
	hash.GetHash(header.m_digest);
}

//! Returns the number of microseconds since 00:00 1-1-2000. In actuality, the timestamp
//! is only accurate to seconds, and is simply extended out to microseconds.
//!
//! \todo Use the operating system's low-level functions to get a true microsecond
//!		timestamp, instead of faking it like we do now.
//! \bug The timestamp might be off an hour.
uint64_t EncoreBootImage::getTimestamp()
{
#if WIN32
	struct tm epoch = { 0, 0, 0, 1, 0, 100, 0, 0 }; // 00:00 1-1-2000
#else
	struct tm epoch = { 0, 0, 0, 1, 0, 100, 0, 0, 1, 0, NULL }; // 00:00 1-1-2000
#endif
	time_t epochTime = mktime(&epoch);
	time_t now = time(NULL);
	now -= epochTime;
	uint64_t microNow = uint64_t(now) * 1000000;	// convert to microseconds
	return microNow;
}

//! Scans the section list looking for the first section which has
//! the #ROM_SECTION_BOOTABLE flag set on it.
EncoreBootImage::Section * EncoreBootImage::findFirstBootableSection()
{
	section_iterator_t it = beginSection();
	for (; it != endSection(); ++it)
	{
		if ((*it)->getFlags() & ROM_SECTION_BOOTABLE)
		{
			return *it;
		}
	}
	
	// no bootable sections were found
	return NULL;
}

//! The boot tag for \a section is taken into account, thus making the
//! result offset point to the first block of the actual section data.
//!
//! \note The offset will only be valid if all encryption keys and all
//! sections have already been added to the image.
uint32_t EncoreBootImage::getSectionOffset(Section * section)
{
	// start with boot image headers 
	uint32_t offset = numberOfCipherBlocks(sizeof(boot_image_header_t));	// header
	offset += numberOfCipherBlocks(sizeof(section_header_t)) * sectionCount();	// section table
	offset += 2 * keyCount();	// key dictiontary
	
	// add up sections before this one
	section_iterator_t it = beginSection();
	for (; it != endSection() && *it != section; ++it)
	{
		Section * thisSection = *it;
		
		// insert padding for section alignment
		offset += getPadBlockCountForSection(thisSection, offset);
		
		// add one for boot tag associated with this section
		offset++;
		
		// now add the section's contents
		offset += thisSection->getBlockCount();
	}
	
	// and add padding for this section
	offset += getPadBlockCountForSection(section, offset);
	
	// skip over this section's boot tag
	offset++;
	
	return offset;
}

//! Computes the number of blocks of padding required to align \a section while
//! taking into account the boot tag that gets inserted before the section contents.
unsigned EncoreBootImage::getPadBlockCountForSection(Section * section, unsigned offset)
{
	// Compute the number of padding blocks needed to align the section. This first
	// call to getPadBlockCountForOffset() passes an offset that excludes
	// the boot tag for this section.
	unsigned paddingBlocks = section->getPadBlockCountForOffset(offset);
	
	// If the pad count comes back as 0 then we need to try again with an offset that
	// includes the boot tag. This is all because we're aligning the section contents
	// start and not the section's boot tag.
	if (paddingBlocks == 0)
	{
		paddingBlocks = section->getPadBlockCountForOffset(offset + 1);
	}
	// Otherwise if we get a nonzero pad amount then we need to subtract the block
	// for the section's boot tag from the pad count.
	else
	{
		paddingBlocks--;
	}
	
	return paddingBlocks;
}

uint32_t EncoreBootImage::getImageSize()
{
	// determine to total size of the image
	const uint32_t headerBlocks = numberOfCipherBlocks(sizeof(boot_image_header_t));
	const uint32_t sectionHeaderSize = numberOfCipherBlocks(sizeof(section_header_t));
	uint32_t imageBlocks = headerBlocks;
	imageBlocks += sectionHeaderSize * m_sections.size();	// section table
	imageBlocks += 2 * m_keys.size();	// key dict
	
	// add in each section's size
	section_iterator_t it = beginSection();
	for (; it != endSection(); ++it)
	{
		// add in this section's size, padding to align it, and its boot tag
		imageBlocks += getPadBlockCountForSection(*it, imageBlocks);
		imageBlocks += (*it)->getBlockCount();
		imageBlocks++;
	}
	
	// image MAC
	imageBlocks += 2;
	
	return imageBlocks;
}

void EncoreBootImage::debugPrint() const
{
	const_section_iterator_t it = beginSection();
	for (; it != endSection(); ++it)
	{
		const Section * section = *it;
		section->debugPrint();
	}
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \return A new boot command instance.
//! \retval NULL The boot command pointed to by \a blocks was not recognized as a known
//!     command type.
//!
//! \exception std::runtime_error This exception indicates that a command was recognized
//!     but contained invalid data. Compare this to a NULL result which indicates that
//!     no command was recognized at all.
EncoreBootImage::BootCommand * EncoreBootImage::BootCommand::createFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
    BootCommand * command = NULL;
	
    switch (header->m_tag)
    {
        case ROM_NOP_CMD:
            command = new NopCommand();
            break;
        case ROM_TAG_CMD:
            command = new TagCommand();
            break;
        case ROM_LOAD_CMD:
            command = new LoadCommand();
            break;
        case ROM_FILL_CMD:
            command = new FillCommand();
            break;
        case ROM_MODE_CMD:
            command = new ModeCommand();
            break;
        case ROM_JUMP_CMD:
            command = new JumpCommand();
            break;
        case ROM_CALL_CMD:
            command = new CallCommand();
            break;
    }
    
    if (command)
    {
        command->initFromData(blocks, count, consumed);
    }
    return command;
}

//! The checksum algorithm is totally straightforward, except that the
//! initial checksum byte value is set to 0x5a instead of 0.
uint8_t EncoreBootImage::BootCommand::calculateChecksum(const boot_command_t & header)
{
	const uint8_t * bytes = reinterpret_cast<const uint8_t *>(&header);
	uint8_t checksum = 0x5a;
	int i;
	
	// start at one to skip checksum field
	for (i = 1; i < sizeof(header); ++i)
	{
		checksum += bytes[i];
	}
	
	return checksum;
}

//! The default implementation returns 0, indicating that no blocks are
//! available.
unsigned EncoreBootImage::BootCommand::getBlockCount() const
{
	return 1 + getDataBlockCount();
}

//! Up to \a maxCount cipher blocks are copied into the buffer pointed to by
//! the \a data argument. The index of the first block to copy is
//! held in the \a offset argument.
//!
//! \param offset Starting block number to copy. Zero means the first available block.
//! \param maxCount Up to this number of blocks may be copied into \a data. Must be 1 or greater.
//! \param data Buffer for outgoing cipher blocks. Must have enough room to hold
//!		\a maxCount blocks.
//!
//! \return The number of cipher blocks copied into \a data.
//! \retval 0 No more blocks are available and nothing was written to \a data.
//!
//! \exception std::out_of_range If \a offset is invalid.
unsigned EncoreBootImage::BootCommand::getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data)
{
	assert(data);
	assert(maxCount >= 1);
	
	// check for valid offset
	if (offset >= getBlockCount())
	{
		throw std::out_of_range("invalid offset");
	}
	
	// handle the command header block separately
	if (offset == 0)
	{
		assert(sizeof(boot_command_t) == sizeof(cipher_block_t));
		
		boot_command_t header;
		fillCommandHeader(header);
		memcpy(data, &header, sizeof(header));
		
		return 1;
	}
	
	// handle any data blocks
	return getDataBlocks(offset - 1, maxCount, data);
}

//! The checksum field of \a testHeader is always computed and checked against itself.
//! All other fields are compared to the corresponding value set in \a modelHeader
//! if the appropriate flag is set in \a whichFields. For example, the m_address fields
//! in \a testHeader and \a modelHeader are compared when the CMD_ADDRESS_FIELD bit
//! is set in \a whichFields. An exception is thrown if any comparison fails.
//!
//! \param modelHeader The baseline header to compare against. Only those fields that
//!		have corresponding bits set in \a whichFields need to be set.
//! \param testHeader The actual command header which is being validated.
//! \param whichFields A bitfield used to determine which fields of the boot command
//!		header are compared. Possible values are:
//!			- CMD_TAG_FIELD
//!			- CMD_FLAGS_FIELD
//!			- CMD_ADDRESS_FIELD
//!			- CMD_COUNT_FIELD
//!			- CMD_DATA_FIELD
//!
//! \exception std::runtime_error Thrown if any requested validation fails.
void EncoreBootImage::BootCommand::validateHeader(const boot_command_t * modelHeader, const boot_command_t * testHeader, unsigned whichFields)
{
	// compare all the fields that were requested
	if ((whichFields & CMD_TAG_FIELD) && (testHeader->m_tag != modelHeader->m_tag))
	{
		throw std::runtime_error("invalid tag field");
	}
	
	if ((whichFields & CMD_FLAGS_FIELD) && (testHeader->m_flags != modelHeader->m_flags))
	{
		throw std::runtime_error("invalid flags field");
	}
	
	if ((whichFields & CMD_ADDRESS_FIELD) && (testHeader->m_address != modelHeader->m_address))
	{
		throw std::runtime_error("invalid address field");
	}
	
	if ((whichFields & CMD_COUNT_FIELD) && (testHeader->m_count != modelHeader->m_count))
	{
		throw std::runtime_error("invalid count field");
	}
	
	if ((whichFields & CMD_DATA_FIELD) && (testHeader->m_data != modelHeader->m_data))
	{
		throw std::runtime_error("invalid data field");
	}
	
	// calculate checksum
	uint8_t testChecksum = calculateChecksum(*testHeader);
	if (testChecksum != testHeader->m_checksum)
	{
		throw std::runtime_error("invalid checksum");
	}
}

//! Since the NOP command has no data, this method just validates the command header.
//! All fields except the checksum are expected to be set to 0.
//!
//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error Thrown if header fields are invalid.
void EncoreBootImage::NopCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
	const boot_command_t model = { 0, ROM_NOP_CMD, 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD | CMD_FLAGS_FIELD | CMD_ADDRESS_FIELD | CMD_COUNT_FIELD | CMD_DATA_FIELD);
	
	*consumed = 1;
}

//! All fields of the boot command header structure are set to 0, except
//! for the checksum. This includes the tag field since the tag value for
//! the #ROM_NOP_CMD is zero. And since all fields are zeroes the checksum
//! remains the initial checksum value of 0x5a.
void EncoreBootImage::NopCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = 0;
	header.m_address = 0;
	header.m_count = 0;
	header.m_data = 0;
	header.m_checksum = calculateChecksum(header);	// do this last
}

void EncoreBootImage::NopCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "\tNOOP\n");
}

//! The identifier, length, and flags fields are taken from \a section.
//!
//! \todo How does length get set correctly if the length is supposed to include
//!		this command?
EncoreBootImage::TagCommand::TagCommand(const Section & section)
{
	m_sectionIdentifier = section.getIdentifier();
	m_sectionLength = section.getBlockCount();
	m_sectionFlags = section.getFlags();
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error Thrown if header fields are invalid.
void EncoreBootImage::TagCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
	const boot_command_t model = { 0, ROM_TAG_CMD, 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD);
	
    // read fields from header
	m_isLast = (ENDIAN_LITTLE_TO_HOST_U16(header->m_flags) & ROM_LAST_TAG) != 0;
	m_sectionIdentifier = ENDIAN_LITTLE_TO_HOST_U32(header->m_address);
	m_sectionLength = ENDIAN_LITTLE_TO_HOST_U32(header->m_count);
	m_sectionFlags = ENDIAN_LITTLE_TO_HOST_U32(header->m_data);
	
	*consumed = 1;
}

//! This method currently assumes that the next tag command will come immediately
//! after the data for this section.
void EncoreBootImage::TagCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = ENDIAN_HOST_TO_LITTLE_U16(m_isLast ? ROM_LAST_TAG : 0);
	header.m_address = ENDIAN_HOST_TO_LITTLE_U32(m_sectionIdentifier);
	header.m_count = ENDIAN_HOST_TO_LITTLE_U32(m_sectionLength);
	header.m_data = ENDIAN_HOST_TO_LITTLE_U32(m_sectionFlags);
	header.m_checksum = calculateChecksum(header);	// do this last
}

void EncoreBootImage::TagCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  BTAG | sec=0x%08x | cnt=0x%08x | flg=0x%08x\n", m_sectionIdentifier, m_sectionLength, m_sectionFlags);
}

//! All fields are set to zero.
//!
EncoreBootImage::LoadCommand::LoadCommand()
:	BootCommand(), m_data(), m_padCount(0), m_length(0), m_address(0), m_loadDCD(false)
{
	fillPadding();
}

EncoreBootImage::LoadCommand::LoadCommand(uint32_t address, const uint8_t * data, uint32_t length)
:	BootCommand(), m_data(), m_padCount(0), m_length(0), m_address(address), m_loadDCD(false)
{
	fillPadding();
	setData(data, length);
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error This exception is thrown if the actual CRC of the load
//!     data does not match the CRC stored in the command header. Also thrown if the
//!     \a count parameter is less than the number of data blocks needed for the length
//!     specified in the command header or if header fields are invalid.
void EncoreBootImage::LoadCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
    // check static fields
	const boot_command_t model = { 0, ROM_LOAD_CMD, 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD);
	
    // read fields from header
	m_address = ENDIAN_LITTLE_TO_HOST_U32(header->m_address);
	m_length = ENDIAN_LITTLE_TO_HOST_U32(header->m_count);
    unsigned crc = ENDIAN_LITTLE_TO_HOST_U32(header->m_data);
    unsigned dataBlockCount = numberOfCipherBlocks(m_length);
    m_padCount = sizeOfPaddingForCipherBlocks(dataBlockCount);
	m_loadDCD = (ENDIAN_LITTLE_TO_HOST_U16(header->m_flags) & ROM_LOAD_DCD) != 0;
	
    // make sure there are enough blocks
    if (count - 1 < dataBlockCount)
    {
        throw std::runtime_error("not enough cipher blocks for load data");
    }
    
    // copy data
    setData(reinterpret_cast<const uint8_t *>(blocks + 1), m_length);
    
    // copy padding
    if (m_padCount)
    {
        const uint8_t * firstPadByte = reinterpret_cast<const uint8_t *> (blocks + (1 + dataBlockCount)) - m_padCount;
        memcpy(m_padding, firstPadByte, m_padCount);
    }
    
    // check CRC
    uint32_t actualCRC = calculateCRC();
    if (actualCRC != crc)
    {
        throw std::runtime_error("load data failed CRC check");
    }
    
	*consumed = 1 + dataBlockCount;
}

//! The only thing unique in the load command header is the
//! #elftosb::EncoreBootImage::boot_command_t::m_data. It contains a CRC-32 over the
//! load data, plus any bytes of padding in the last data cipher block.
void EncoreBootImage::LoadCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = ENDIAN_HOST_TO_LITTLE_U16(m_loadDCD ? ROM_LOAD_DCD : 0);
	header.m_address = ENDIAN_HOST_TO_LITTLE_U32(m_address);
	header.m_count = ENDIAN_HOST_TO_LITTLE_U32(m_length);
	header.m_data = ENDIAN_HOST_TO_LITTLE_U32(calculateCRC());
	
	// do this last
	header.m_checksum = calculateChecksum(header);
}

//! A CRC-32 is calculated over the load data, including any pad bytes
//! that are required in the last data cipher block. Including the
//! pad bytes in the CRC makes it vastly easier for the ROM to calculate
//! the CRC for validation.
uint32_t EncoreBootImage::LoadCommand::calculateCRC() const
{
	uint32_t result;
	CRC32 crc;
	crc.update(m_data, m_length);
	if (m_padCount)
	{
		// include random padding in the CRC
		crc.update(m_padding, m_padCount);
	}
	crc.truncatedFinal(reinterpret_cast<uint8_t*>(&result), sizeof(result));
	
	return result;
}

//! A local copy of the load data is made. This copy will be disposed of when this object
//! is destroyed. This means the caller is free to deallocate \a data after this call
//! returns. It also means the caller can pass a pointer into the middle of a buffer for
//! \a data and not worry about ownership issues. 
void EncoreBootImage::LoadCommand::setData(const uint8_t * data, uint32_t length)
{
	assert(data);
	assert(length);
	
	uint8_t * dataCopy = new uint8_t[length];
	memcpy(dataCopy, data, length);
	
	m_data = dataCopy;
	m_length = length;
	
	m_padCount = sizeOfPaddingForCipherBlocks(m_length);
}

//! \return The number of cipher blocks required to hold the load data,
//!		rounded up as necessary.
unsigned EncoreBootImage::LoadCommand::getDataBlockCount() const
{
	// round up to the next cipher block
	return numberOfCipherBlocks(m_length);
}

//! Up to \a maxCount data blocks are copied into the buffer pointed to by
//! the \a data argument. This is only a request for \a maxCount blocks.
//! A return value of 0 indicates that no more blocks are available. The
//! index of the first block to copy is held in the \a offset argument.
//! If there are pad bytes needed to fill out the last data block, they
//! will be filled with random data in order to add to the "whiteness" of
//! the data on both sides of encryption.
//!
//! \param offset Starting block number to copy. Zero means the first available block.
//! \param maxCount Up to this number of blocks may be copied into \a data. Must be 1 or greater.
//! \param data Buffer for outgoing data blocks. Must have enough room to hold
//!		\a maxCount blocks.
//!
//! \return The number of data blocks copied into \a data.
//! \retval 0 No more blocks are available and nothing was written to \a data.
//!
//! \exception std::out_of_range Thrown when offset is invalid.
//!
//! \todo fill pad bytes with random bytes
unsigned EncoreBootImage::LoadCommand::getDataBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data)
{
	assert(data);
	assert(maxCount != 0);
	
	uint32_t blockCount = getDataBlockCount();
	
	// check offset
	if (offset >= blockCount)
	{
		throw std::out_of_range("invalid offset");
	}
	
	// figure out how many blocks to return
	unsigned resultBlocks = blockCount - offset;
	if (resultBlocks > maxCount)
	{
		resultBlocks = maxCount;
		
		// exclude last block if there is padding
		if (m_padCount && (offset != blockCount - 1) && (offset + resultBlocks == blockCount))
		{
			resultBlocks--;
		}
	}
	
	// if there are pad bytes, handle the last block specially
	if (m_padCount && offset == blockCount - 1)
	{
		// copy the remainder of the load data into the first part of the result block
		unsigned remainderLength = sizeof(cipher_block_t) - m_padCount;
		memcpy(data, &m_data[sizeof(cipher_block_t) * offset], remainderLength);
		
		// copy pad bytes we previously generated into the last part of the result block
		// data is a cipher block pointer, so indexing is done on cipher block
		// boundaries, thus we need a byte pointer to index properly
		uint8_t * bytePtr = reinterpret_cast<uint8_t*>(data);
		memcpy(bytePtr + remainderLength, &m_padding, m_padCount);
	}
	else
	{
		memcpy(data, &m_data[sizeof(cipher_block_t) * offset], sizeof(cipher_block_t) * resultBlocks);
	}
	
	return resultBlocks;
}

//! Fills #m_padding with random bytes that may be used to fill up the last data
//! cipher block.
void EncoreBootImage::LoadCommand::fillPadding()
{
	RandomNumberGenerator rng;
	rng.generateBlock(m_padding, sizeof(m_padding));
}

void EncoreBootImage::LoadCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  LOAD | adr=0x%08x | len=0x%08x | crc=0x%08x | flg=0x%08x\n", m_address, m_length, calculateCRC(), m_loadDCD ? ROM_LOAD_DCD : 0);
}

//! The pattern, address, and count are all initialized to zero, and the pattern
//! size is set to a word.
EncoreBootImage::FillCommand::FillCommand()
:	BootCommand(), m_address(0), m_count(0), m_pattern(0)
{
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error Thrown if header fields are invalid.
void EncoreBootImage::FillCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
    // check static fields
	const boot_command_t model = { 0, ROM_FILL_CMD, 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD | CMD_FLAGS_FIELD);
	
    // read fields from header
	m_address = ENDIAN_LITTLE_TO_HOST_U32(header->m_address);
	m_count = ENDIAN_LITTLE_TO_HOST_U32(header->m_count);
    m_pattern = ENDIAN_LITTLE_TO_HOST_U32(header->m_data);
    
	*consumed = 1;
}

void EncoreBootImage::FillCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = 0;
	header.m_address = ENDIAN_HOST_TO_LITTLE_U32(m_address);
	header.m_count = ENDIAN_HOST_TO_LITTLE_U32(m_count);
	header.m_data = ENDIAN_HOST_TO_LITTLE_U32(m_pattern);
	header.m_checksum = calculateChecksum(header);	// do this last
}

//! Extends the pattern across 32 bits.
//!
void EncoreBootImage::FillCommand::setPattern(uint8_t pattern)
{
	m_pattern = (pattern << 24) | (pattern << 16) | (pattern << 8) | pattern;
}

//! Extends the pattern across 32 bits.
//!
void EncoreBootImage::FillCommand::setPattern(uint16_t pattern)
{
	m_pattern = (pattern << 16) | pattern;
}

void EncoreBootImage::FillCommand::setPattern(uint32_t pattern)
{
	m_pattern = pattern;
}

void EncoreBootImage::FillCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  FILL | adr=0x%08x | len=0x%08x | ptn=0x%08x\n", m_address, m_count, m_pattern);
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error Thrown if header fields are invalid.
void EncoreBootImage::ModeCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
    // check static fields
	const boot_command_t model = { 0, ROM_MODE_CMD, 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD | CMD_FLAGS_FIELD | CMD_ADDRESS_FIELD | CMD_COUNT_FIELD);
	
    // read fields from header
    m_mode = ENDIAN_LITTLE_TO_HOST_U32(header->m_data);
    
	*consumed = 1;
}

void EncoreBootImage::ModeCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = 0;
	header.m_address = 0;
	header.m_count = 0;
	header.m_data = ENDIAN_HOST_TO_LITTLE_U32(m_mode);
	header.m_checksum = calculateChecksum(header);	// do this last
}

void EncoreBootImage::ModeCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  MODE | mod=0x%08x\n", m_mode);
}

//! \param blocks Pointer to the raw data blocks.
//! \param count Number of blocks pointed to by \a blocks.
//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
//!		by the command. Should be at least 1 for every command. This must not be NULL
//!		on entry!
//!
//! \exception std::runtime_error Thrown if header fields are invalid.
void EncoreBootImage::JumpCommand::initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)
{
    // check static fields
	const boot_command_t model = { 0, getTag(), 0, 0, 0, 0 };
	const boot_command_t * header = reinterpret_cast<const boot_command_t *>(blocks);
	validateHeader(&model, header, CMD_TAG_FIELD | CMD_COUNT_FIELD);
	
    // read fields from header
    m_address = ENDIAN_LITTLE_TO_HOST_U32(header->m_address);
    m_argument = ENDIAN_LITTLE_TO_HOST_U32(header->m_data);
	m_isHAB = (ENDIAN_LITTLE_TO_HOST_U16(header->m_flags) & ROM_HAB_EXEC) != 0;
    
	*consumed = 1;
}

void EncoreBootImage::JumpCommand::fillCommandHeader(boot_command_t & header)
{
	header.m_tag = getTag();
	header.m_flags = ENDIAN_HOST_TO_LITTLE_U16(m_isHAB ? ROM_HAB_EXEC : 0);
	header.m_address = ENDIAN_HOST_TO_LITTLE_U32(m_address);
	header.m_count = 0;
	header.m_data = ENDIAN_HOST_TO_LITTLE_U32(m_argument);
	header.m_checksum = calculateChecksum(header);	// do this last
}

void EncoreBootImage::JumpCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  JUMP | adr=0x%08x | arg=0x%08x | flg=0x%08x\n", m_address, m_argument, m_isHAB ? ROM_HAB_EXEC : 0);
}

void EncoreBootImage::CallCommand::debugPrint() const
{
	Log::log(Logger::INFO2, "  CALL | adr=0x%08x | arg=0x%08x | flg=0x%08x\n", m_address, m_argument, m_isHAB ? ROM_HAB_EXEC : 0);
}

//! Only if the section has been assigned a boot image owner object will this
//! method be able to fill in the #section_header_t::m_offset field. If no
//! boot image has been set the offset will be set to 0.
void EncoreBootImage::Section::fillSectionHeader(section_header_t & header)
{
	header.m_tag = getIdentifier();
	header.m_offset = 0;
	header.m_length = ENDIAN_HOST_TO_LITTLE_U32(getBlockCount());
	header.m_flags = ENDIAN_HOST_TO_LITTLE_U32(getFlags());
	
	// if we're attached to an image, we can compute our real offset
	if (m_image)
	{
		header.m_offset = ENDIAN_HOST_TO_LITTLE_U32(m_image->getSectionOffset(this));
	}
}

//! The alignment will never be less than 16, since that is the size of the
//! cipher block which is the basic unit of the boot image format. If an
//! alignment less than 16 is set it will be ignored.
//!
//! \param alignment Alignment in bytes for this section. Must be a power of two.
//!		Ignored if less than 16.
void EncoreBootImage::Section::setAlignment(unsigned alignment)
{
	if (alignment > BOOT_IMAGE_MINIMUM_SECTION_ALIGNMENT)
	{
		m_alignment = alignment;
	}
}

//! This method calculates the number of padding blocks that need to be inserted
//! from a given offset for the section to be properly aligned. The value returned
//! is the number of padding blocks that should be inserted starting just after
//! \a offset to align the first cipher block of the section contents. The section's
//! boot tag is \i not taken into account by this method, so the caller must
//! deal with that herself.
//!
//! \param offset Start offset in cipher blocks (not bytes).
//!
//! \return A number of cipher blocks of padding to insert.
unsigned EncoreBootImage::Section::getPadBlockCountForOffset(unsigned offset)
{
	// convert alignment from byte to block alignment
	unsigned blockAlignment = m_alignment >> 4;
	
	unsigned nextAlignmentOffset = (offset + blockAlignment - 1) / blockAlignment * blockAlignment;
	
	return nextAlignmentOffset - offset;
}

EncoreBootImage::BootSection::~BootSection()
{
	deleteCommands();
}

void EncoreBootImage::BootSection::deleteCommands()
{
	// dispose of all sections
	iterator_t it = begin();
	for (; it != end(); ++it)
	{
		delete *it;
	}
}

//! Always returns at least 1 for the required tag command.
//!
unsigned EncoreBootImage::BootSection::getBlockCount() const
{
	unsigned count = 0;
	
	const_iterator_t it = begin();
	for (; it != end(); ++it)
	{
		count += (*it)->getBlockCount();
	}
	
	return count;
}

//! Up to \a maxCount cipher blocks are copied into the buffer pointed to by
//! the \a data argument. A return value of 0 indicates that
//! no more blocks are available. The index of the first block to copy is
//! held in the \a offset argument.
//!
//! \param offset Starting block number to copy. Zero means the first available block.
//! \param maxCount Up to this number of blocks may be copied into \a data.
//! \param data Buffer for outgoing cipher blocks. Must have enough room to hold
//!		\a maxCount blocks.
//!
//! \return The number of cipher blocks copied into \a data.
//! \retval 0 No more blocks are available and nothing was written to \a data.
unsigned EncoreBootImage::BootSection::getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data)
{
	assert(data);
	assert(maxCount >= 1);
	
	unsigned currentOffset = 0;
	unsigned readCount = maxCount;
	
	iterator_t it = begin();
	for (; it != end(); ++it)
	{
		BootCommand * command = *it;
		unsigned commandBlocks = command->getBlockCount();
		
		// this should never be false!
		assert(offset >= currentOffset);
		
		// skip forward until we hit the requested offset
		if (offset >= currentOffset + commandBlocks)
		{
			currentOffset += commandBlocks;
			continue;
		}
		
		// read from this command
		unsigned commandOffset = offset - currentOffset;
		unsigned commandRemaining = commandBlocks - commandOffset;
		if (readCount > commandRemaining)
		{
			readCount = commandRemaining;
		}
		return command->getBlocks(commandOffset, readCount, data);
	}
	
	return 0;
}

//! The entire contents of the section must be in memory, pointed to by \a blocks.
//! Any commands that had previously been added to the section are disposed of.
//!
//! \param blocks Pointer to the section contents.
//! \param count Number of blocks pointed to by \a blocks.
//!
//! \exception std::runtime_error Thrown if a boot command cannot be created from
//!		the cipher block stream.
void EncoreBootImage::BootSection::fillFromData(const cipher_block_t * blocks, unsigned count)
{
	// start with an empty slate
	deleteCommands();
	
	const cipher_block_t * currentBlock = blocks;
	unsigned remaining = count;
	while (remaining)
	{
		// try to create a command from the next cipher block. the number of
		// blocks the command used up is returned in consumed.
		unsigned consumed;
		BootCommand * command = BootCommand::createFromData(currentBlock, remaining, &consumed);
		if (!command)
		{
			throw std::runtime_error("invalid boot section data");
		}
		
		addCommand(command);
		
		// update loop counters
		remaining -= consumed;
		currentBlock += consumed;
	}
}

void EncoreBootImage::BootSection::debugPrint() const
{
	Log::log(Logger::INFO2, "Boot Section 0x%08x:\n", m_identifier);
	
	const_iterator_t it = begin();
	for (; it != end(); ++it)
	{
		const BootCommand * command = *it;
		command->debugPrint();
	}
}

//! A copy is made of \a data. Any previously assigned data is disposed of.
//!
void EncoreBootImage::DataSection::setData(const uint8_t * data, unsigned length)
{
	m_data = new uint8_t[length];
	memcpy(m_data.get(), data, length);
	m_length = length;
}

//! The section takes ownership of \a data and will dispose of it using the
//! array delete operator upon its destruction.
void EncoreBootImage::DataSection::setDataNoCopy(const uint8_t * data, unsigned length)
{
	m_data = data;
	m_length = length;
}

unsigned EncoreBootImage::DataSection::getBlockCount() const
{
	return numberOfCipherBlocks(m_length);
}

unsigned EncoreBootImage::DataSection::getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data)
{
	assert(data);
	assert(maxCount != 0);
	
	unsigned blockCount = getBlockCount();
	unsigned padCount = sizeOfPaddingForCipherBlocks(m_length);
	
	// check offset
	if (offset >= blockCount)
	{
		throw std::out_of_range("invalid offset");
	}
	
	// figure out how many blocks to return
	unsigned resultBlocks = blockCount - offset;
	if (resultBlocks > maxCount)
	{
		resultBlocks = maxCount;
		
		// exclude last block if there is padding
		if (padCount && (offset != blockCount - 1) && (offset + resultBlocks == blockCount))
		{
			resultBlocks--;
		}
	}
	
	// if there are pad bytes, handle the last block specially
	if (padCount && offset == blockCount - 1)
	{
		// copy the remainder of the load data into the first part of the result block
		unsigned remainderLength = sizeof(cipher_block_t) - padCount;
		memcpy(data, &m_data[sizeOfCipherBlocks(offset)], remainderLength);
		
		// set pad bytes to zeroes.
		// data is a cipher block pointer, so indexing is done on cipher block
		// boundaries, thus we need a byte pointer to index properly
		uint8_t * bytePtr = reinterpret_cast<uint8_t*>(data);
		memset(bytePtr + remainderLength, 0, padCount);
	}
	else
	{
		memcpy(data, &m_data[sizeOfCipherBlocks(offset)], sizeOfCipherBlocks(resultBlocks));
	}
	
	return resultBlocks;
}

void EncoreBootImage::DataSection::debugPrint() const
{
	Log::log(Logger::INFO2, "Data Section 0x%08x: (%d bytes, %d blocks)\n", m_identifier, m_length, getBlockCount());
}

