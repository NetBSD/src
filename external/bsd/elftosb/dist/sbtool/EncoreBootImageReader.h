/*
 * File:	EncoreBootImageReader.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_EncoreBootImageReader_h_)
#define _EncoreBootImageReader_h_

#include "EncoreBootImage.h"

namespace elftosb
{

/*!
 * \brief Reads a Piano/Encore boot image from an input stream.
 */
class EncoreBootImageReader
{
public:
	/*!
	 * \brief Exception class used for error found while reading a boot image.
	 */
	class read_error : public std::runtime_error
	{
	public:
		//! \brief Constructor.
		read_error(const std::string & msg) : std::runtime_error(msg) {}
	};
	
	//! \brief An array of section headers.
	typedef std::vector<EncoreBootImage::section_header_t> section_array_t;
	
	//! \brief An array of boot tags.
	typedef std::vector<EncoreBootImage::boot_command_t> boot_tag_array_t;
	
public:
	//! \brief Default constructor.
	EncoreBootImageReader(std::istream & stream) : m_stream(stream) {}
	
	//! \brief Destructor.
	virtual ~EncoreBootImageReader() {}
	
	//! \name Decryption key
	//! These methods provide access to the Data Encryption Key (DEK). Normally
	//! the DEK is discovered using the readKeyDictionary() method.
	//@{
	inline void setKey(const AESKey<128> & key) { m_dek = key; }
	inline const AESKey<128> & getKey() const { return m_dek; }
	//@}
	
	//! \name Readers
	//! This group of methods is responsible for reading and parsing different
	//! pieces and parts of the boot image file.
	//@{
	//! \brief Reads the header from the image.
	void readImageHeader();
	
	//! \brief Computes the actual SHA-1 digest of the image header.
	void computeHeaderDigest(sha1_digest_t & digest);
	
	//! \brief Reads the digest at the end of the image.
	void readImageDigest();
	
	//! \brief Run a SHA-1 digest over the entire image.
	void computeImageDigest(sha1_digest_t & digest);
	
	//! \brief Read the plaintext section table entries.
	void readSectionTable();
	
	//! \brief Reads the key dictionary, if the image is encrypted.
	bool readKeyDictionary(const AESKey<128> & kek);
	
	//! \brief
	void readBootTags();
	
	//! \brief
	EncoreBootImage::Section * readSection(unsigned index);
	//@}
	
	//! \name Accessors
	//! Information retrieved with reader methods is accessible through
	//! these methods.
	//@{
	//! \brief Returns whether the image is encrypted or not.
	//! \pre The header must have been read already.
	inline bool isEncrypted() const { return m_header.m_keyCount > 0; }
	
	//! \brief Returns a reference to the image's header.
	const EncoreBootImage::boot_image_header_t & getHeader() const { return m_header; }
	
	//! \brief Returns a reference to the SHA-1 digest read from the image.
	const sha1_digest_t & getDigest() const { return m_digest; }
	
	//! \brief Returns a reference to the STL container holding the section headers.
	inline const section_array_t & getSections() const { return m_sections; }
	
	//! \brief Returns a reference to the STL container holding the boot tags.
	inline const boot_tag_array_t & getBootTags() const { return m_bootTags; }
	//@}
	
protected:
	std::istream & m_stream;	//!< The input stream to read the image from.
	AESKey<128> m_dek;	//!< DEK (data encryption key) read from the key dictionary.
	EncoreBootImage::boot_image_header_t m_header;	//!< Header from the boot image.
	sha1_digest_t m_digest;	//!< SHA-1 digest as read from the image.
	section_array_t m_sections;	//!< The section table.
	boot_tag_array_t m_bootTags;	//!< The array of boot tags read from the image.
	
protected:
	//! \brief Calculates the 8-bit checksum on a boot command header.
	uint8_t calculateCommandChecksum(EncoreBootImage::boot_command_t & header);
};

}; // namespace elftosb

#endif // _EncoreBootImageReader_h_
