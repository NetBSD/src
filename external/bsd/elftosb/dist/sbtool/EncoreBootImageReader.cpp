/*
 * File:	EncoreBootImageReader.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "EncoreBootImageReader.h"
#include "SHA1.h"
#include "rijndael.h"
#include "RijndaelCBCMAC.h"
#include <assert.h>
#include "EndianUtilities.h"
#include "Logging.h"

using namespace elftosb;

//! \post Stream head points to just after the image header.
//! \exception read_error Thrown if the image header is invalid.
void EncoreBootImageReader::readImageHeader()
{
	// seek to beginning of the stream/file and read the plaintext header
	m_stream.seekg(0, std::ios_base::beg);
	if (m_stream.read((char *)&m_header, sizeof(m_header)).bad())
	{
		throw read_error("failed to read image header");
	}
	
	m_header.m_flags = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_flags);
	m_header.m_imageBlocks = ENDIAN_LITTLE_TO_HOST_U32(m_header.m_imageBlocks);
	m_header.m_firstBootTagBlock = ENDIAN_LITTLE_TO_HOST_U32(m_header.m_firstBootTagBlock);
	m_header.m_firstBootableSectionID = ENDIAN_LITTLE_TO_HOST_U32(m_header.m_firstBootableSectionID);
	m_header.m_keyCount = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_keyCount);
	m_header.m_keyDictionaryBlock = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_keyDictionaryBlock);
	m_header.m_headerBlocks = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_headerBlocks);
	m_header.m_sectionCount = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_sectionCount);
	m_header.m_sectionHeaderSize = ENDIAN_LITTLE_TO_HOST_U16(m_header.m_sectionHeaderSize);
	m_header.m_timestamp = ENDIAN_LITTLE_TO_HOST_U64(m_header.m_timestamp);

//	m_header.m_componentVersion.m_major = ENDIAN_BIG_TO_HOST_U16(m_header.m_componentVersion.m_major);
//	m_header.m_componentVersion.m_minor = ENDIAN_BIG_TO_HOST_U16(m_header.m_componentVersion.m_minor);
//	m_header.m_componentVersion.m_revision = ENDIAN_BIG_TO_HOST_U16(m_header.m_componentVersion.m_revision);

//	m_header.m_productVersion.m_major = ENDIAN_BIG_TO_HOST_U16(m_header.m_productVersion.m_major);
//	m_header.m_productVersion.m_minor = ENDIAN_BIG_TO_HOST_U16(m_header.m_productVersion.m_minor);
//	m_header.m_productVersion.m_revision = ENDIAN_BIG_TO_HOST_U16(m_header.m_productVersion.m_revision);
	
	// check header signature 1
	if (m_header.m_signature[0] != 'S' || m_header.m_signature[1] != 'T' || m_header.m_signature[2] != 'M' || m_header.m_signature[3] != 'P')
	{
		throw read_error("invalid signature 1");
	}

	// check header signature 2 for version 1.1 and greater
	if ((m_header.m_majorVersion > 1 || (m_header.m_majorVersion == 1 && m_header.m_minorVersion >= 1)) && (m_header.m_signature2[0] != 's' || m_header.m_signature2[1] != 'g' || m_header.m_signature2[2] != 't' || m_header.m_signature2[3] != 'l'))
	{
// 		throw read_error("invalid signature 2");
		Log::log(Logger::WARNING, "warning: invalid signature 2\n");
	}
}

//! \pre The image header must have already been read with a call to readImageHeader().
//!
void EncoreBootImageReader::computeHeaderDigest(sha1_digest_t & digest)
{
	CSHA1 hash;
	hash.Reset();
	hash.Update((uint8_t *)&m_header.m_signature, sizeof(m_header) - sizeof(sha1_digest_t));
	hash.Final();
	hash.GetHash(digest);
}

//! \pre The image header must have already been read.
//! \pre The DEK must have been found already.
//! \post The stream head is at the end of the digest.
void EncoreBootImageReader::readImageDigest()
{
	unsigned digestPosition = sizeOfCipherBlocks(m_header.m_imageBlocks - 2);
	m_stream.seekg(digestPosition, std::ios_base::beg);
	
	// read the two cipher blocks containing the digest, including padding
	cipher_block_t digestBlocks[2];
	if (m_stream.read((char *)&digestBlocks, sizeof(digestBlocks)).bad())
	{
		throw read_error("failed to read image digest");
	}
	
	// decrypt the digest
	if (isEncrypted())
	{
		Rijndael cipher;
		cipher.init(Rijndael::CBC, Rijndael::Decrypt, m_dek, Rijndael::Key16Bytes, m_header.m_iv);
		cipher.blockDecrypt((uint8_t *)&digestBlocks, sizeof(digestBlocks) * 8, (uint8_t *)&digestBlocks);
	}
	
	// copy the digest out of the padded blocks
	memcpy(m_digest, &digestBlocks, sizeof(m_digest));
}

//! \pre The image header must have already been read with a call to readImageHeader().
//! \post The stream head is at the end of the image minus the last two cipher blocks.
//! \param digest Where to store the resulting digest.
//! \exception read_error Thrown if the image header is invalid.
void EncoreBootImageReader::computeImageDigest(sha1_digest_t & digest)
{
	m_stream.seekg(0, std::ios_base::beg);
	
	CSHA1 hash;
	hash.Reset();
	
	unsigned blockCount = m_header.m_imageBlocks - 2; // exclude digest at end of image
	while (blockCount--)
	{
		cipher_block_t block;
		if (m_stream.read((char *)&block, sizeof(block)).bad())
		{
			throw read_error("failed to read block while computing image digest");
		}
		hash.Update(block, sizeof(block));
	}
	
	hash.Final();
	hash.GetHash(digest);
}

//! \pre Image header must have been read before this method is called.
//!
void EncoreBootImageReader::readSectionTable()
{
	// seek to the table
	m_stream.seekg(sizeOfCipherBlocks(m_header.m_headerBlocks), std::ios_base::beg);
	
	unsigned sectionCount = m_header.m_sectionCount;
	while (sectionCount--)
	{
		EncoreBootImage::section_header_t header;
		if (m_stream.read((char *)&header, sizeof(header)).bad())
		{
			throw read_error("failed to read section header");
		}
		
		// swizzle section header
		header.m_tag = ENDIAN_LITTLE_TO_HOST_U32(header.m_tag);
		header.m_offset = ENDIAN_LITTLE_TO_HOST_U32(header.m_offset);
		header.m_length = ENDIAN_LITTLE_TO_HOST_U32(header.m_length);
		header.m_flags = ENDIAN_LITTLE_TO_HOST_U32(header.m_flags);
		
		m_sections.push_back(header);
	}
}

//! Requires that an OTP key has been provided as the sole argument. Passing the
//! key into this method lets the caller search the key dictionary for any number
//! of keys and determine which are valid. If \a kek is found in the dictionary,
//! the decrypted DEK is saved and true is returned. A result of false means
//! that \a kek was not found.
//!
//! \pre The image header and section table must have been read already.
//! \post The stream head points somewhere inside the key dictionary, or just after it.
//! \post If the search was successful, the #m_dek member will contain the decrypted
//!		session key. Otherwise #m_dek is not modified.
//! \param kek Search for this KEK in the dictionary.
//! \retval true The DEK was found and decrypted. True is also returned when the
//!		image is not encrypted at all.
//! \retval false No matching key entry was found. The image cannot be decrypted.
bool EncoreBootImageReader::readKeyDictionary(const AESKey<128> & kek)
{
	// do nothing if the image is not encrypted
	if (!isEncrypted())
	{
		return true;
	}
	
	// first compute a CBC-MAC over the image header with our KEK
	RijndaelCBCMAC mac(kek);
	mac.update((const uint8_t *)&m_header, sizeof(m_header));
	
	// run the CBC-MAC over each entry in the section table too
	section_array_t::iterator it = m_sections.begin();
	for (; it != m_sections.end(); ++it)
	{
		mac.update((const uint8_t *)&(*it), sizeof(EncoreBootImage::section_header_t));
	}
	
	// get the CBC-MAC result
	mac.finalize();
	const RijndaelCBCMAC::block_t & macResult = mac.getMAC();
	
	// seek to the key dictionary
	m_stream.seekg(sizeOfCipherBlocks(m_header.m_keyDictionaryBlock), std::ios_base::beg);
	
	// decipher each key entry
	unsigned entries = m_header.m_keyCount;
	while (entries--)
	{
		// read the entry
		EncoreBootImage::dek_dictionary_entry_t entry;
		if (m_stream.read((char *)&entry, sizeof(entry)).bad())
		{
			throw read_error("failed to read key dictionary entry");
		}
		
		// compare the CBC-MAC we computed with the one in this entry
		if (memcmp(macResult, entry.m_mac, sizeof(cipher_block_t)) == 0)
		{
			// it's a match! now decrypt this entry's key in place
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Decrypt, kek, Rijndael::Key16Bytes, m_header.m_iv);
			cipher.blockDecrypt(entry.m_dek, sizeof(entry.m_dek) * 8, entry.m_dek);
			
			m_dek = entry.m_dek;
			memset(entry.m_dek, 0, sizeof(entry.m_dek)); // wipe the key value from memory
			return true;
		}
	}
	
	// if we exit the loop normally then no matching MAC was found
	return false;
}

//! Before the boot tag is added to the #m_bootTags member, some basic checks are performed.
//! The command tag field is checked to make sure it matches #ROM_TAG_CMD. And
//! the checksum field is verified to be sure it's correct.
//!
//! After the call to this method returns, the array of boot tags is accessible
//! with the getBootTags() method. The array is sorted in the order in which
//! the boot tags appeared in the image.
//!
//! \pre Image header must have been read.
//! \pre Key dictionary must have been read and a valid DEK found.
//! \post The stream head is left pointing just after the last boot tag.
//! \exception read_error A failure to read the boot tag, or a failure on one
//!		of the consistency checks will cause this exception to be thrown.
void EncoreBootImageReader::readBootTags()
{
	assert(m_header.m_firstBootTagBlock != 0);
	
	unsigned bootTagOffset = m_header.m_firstBootTagBlock;
	
	while (1)
	{
		// seek to this boot tag and read it into a temporary buffer
		EncoreBootImage::boot_command_t header;
		m_stream.seekg(sizeOfCipherBlocks(bootTagOffset), std::ios_base::beg);
		if (m_stream.read((char *)&header, sizeof(header)).bad())
		{
			throw read_error("failed to read boot tag");
		}
		
		// swizzle to command header
		header.m_flags = ENDIAN_LITTLE_TO_HOST_U16(header.m_flags);
		header.m_address = ENDIAN_LITTLE_TO_HOST_U32(header.m_address);
		header.m_count = ENDIAN_LITTLE_TO_HOST_U32(header.m_count);
		header.m_data = ENDIAN_LITTLE_TO_HOST_U32(header.m_data);
		
		// decrypt in place
		if (isEncrypted())
		{
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Decrypt, m_dek, Rijndael::Key16Bytes, m_header.m_iv);
			cipher.blockDecrypt((uint8_t *)&header, sizeof(header) * 8, (uint8_t *)&header);
		}
		
		// perform some basic checks
		if (header.m_tag != EncoreBootImage::ROM_TAG_CMD)
		{
			throw read_error("boot tag is wrong command type");
		}
		
		uint8_t checksum = calculateCommandChecksum(header);
		if (checksum != header.m_checksum)
		{
			throw read_error("boot tag checksum is invalid");
		}
		
		// save this boot tag
		m_bootTags.push_back(header);
		
		// and finally, update offset and break out of loop
		bootTagOffset += header.m_count + 1; // include this boot tag in offset
		if (header.m_flags & EncoreBootImage::ROM_LAST_TAG || bootTagOffset >= m_header.m_imageBlocks - 2)
		{
			break;
		}
	}
}

uint8_t EncoreBootImageReader::calculateCommandChecksum(EncoreBootImage::boot_command_t & header)
{
	uint8_t * bytes = reinterpret_cast<uint8_t *>(&header);
	uint8_t checksum = 0x5a;
	int i;
	
	// start at one to skip checksum field
	for (i = 1; i < sizeof(header); ++i)
	{
		checksum += bytes[i];
	}
	
	return checksum;
}

//! \param index The index of the section to read.
//!
//! \pre Both the image header and section table must have been read already before
//!		calling this method.
//! \exception read_error This exception is raised if the stream reports an error while
//!		trying to read from the section.
EncoreBootImage::Section * EncoreBootImageReader::readSection(unsigned index)
{
	// look up section header
	assert(index < m_sections.size());
	EncoreBootImage::section_header_t & header = m_sections[index];
	
	// seek to the section
	m_stream.seekg(sizeOfCipherBlocks(header.m_offset), std::ios_base::beg);
	
	uint8_t * contents = NULL;
	try
	{
		// allocate memory for the section contents and read the whole thing
		unsigned contentLength = sizeOfCipherBlocks(header.m_length);
		contents = new uint8_t[contentLength];
		if (m_stream.read((char *)contents, contentLength).bad())
		{
			throw read_error("failed to read section");
		}
		
		// decrypt the entire section at once, if the image is encrypted and
		// the cleartext flag is not set
		if (isEncrypted() && (header.m_flags & EncoreBootImage::ROM_SECTION_CLEARTEXT) == 0)
		{
			Rijndael cipher;
			cipher.init(Rijndael::CBC, Rijndael::Decrypt, m_dek, Rijndael::Key16Bytes, m_header.m_iv);
			cipher.blockDecrypt(contents, contentLength * 8, contents);
		}
		
		// create section object
		EncoreBootImage::Section * resultSection = NULL;
		if (header.m_flags & EncoreBootImage::ROM_SECTION_BOOTABLE)
		{
			// a boot command section.
			EncoreBootImage::BootSection * bootSection = new EncoreBootImage::BootSection(header.m_tag);
			
			bootSection->fillFromData((cipher_block_t *)contents, header.m_length);
			
			resultSection = bootSection;
		}
		else
		{
			// this is a raw data section
			EncoreBootImage::DataSection * dataSection = new EncoreBootImage::DataSection(header.m_tag);
			dataSection->setDataNoCopy(contents, contentLength);
			contents = NULL;
			resultSection = dataSection;
		}
		
		return resultSection;
	}
	catch (...)
	{
		if (contents)
		{
			delete [] contents;
		}
		throw;
	}
}


