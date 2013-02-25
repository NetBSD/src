/*
 * File:	EncoreBootImage.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_EncoreBootImage_h_)
#define _EncoreBootImage_h_

#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <string.h>
#include "BootImage.h"
#include "rijndael.h"
#include "smart_ptr.h"
#include "AESKey.h"
#include "StExecutableImage.h"

namespace elftosb
{

//! An AES-128 cipher block is 16 bytes.
typedef uint8_t cipher_block_t[16];

//! A SHA-1 digest is 160 bits, or 20 bytes.
typedef uint8_t sha1_digest_t[20];

//! Unique identifier type for a section.
typedef uint32_t section_id_t;

//! Utility to return the byte length of a number of cipher blocks.
inline size_t sizeOfCipherBlocks(unsigned count) { return sizeof(cipher_block_t) * count; }

//! Utility to return the number of cipher blocks required to hold an object
//! that is \a s bytes long.
inline size_t numberOfCipherBlocks(size_t s) { return (s + sizeof(cipher_block_t) - 1) / sizeof(cipher_block_t); }

//! Utility to calculate the byte length for the cipher blocks required to hold
//! and object that is \a bytes long.
inline size_t sizeInCipherBlocks(size_t s) { return (unsigned)sizeOfCipherBlocks(numberOfCipherBlocks(s)); }

//! Utility to return the number of bytes of padding required to fill out
//! the last cipher block in a set of cipher blocks large enough to hold
//! an object that is \a s bytes large. The result may be 0 if \a s is
//! an even multiple of the cipher block size.
inline size_t sizeOfPaddingForCipherBlocks(size_t s) { return sizeInCipherBlocks(s) - s; }

/*!
 * \brief Class to manage Encore boot image files.
 *
 * Initially this class will only support generation of boot images, but
 * its design will facilitate the addition of the ability to read an
 * image and examine its contents.
 *
 * A boot image is composed of the following regions:
 * - Header
 * - Section table
 * - Key dictionary
 * - Section data
 * - Authentication
 *
 * Multiple sections are within a boot image are fully supported. Two general types
 * of sections are supported with this class. Bootable sections, represented by the
 * EncoreBootImage::BootSection class, contain a sequence of commands to be
 * interpreted by the boot ROM. Data sections are represented by the
 * EncoreBootImage::DataSection class and can contain any arbitrary data.
 *
 * An image can either be encrypted or unencrypted. The image uses a session key,
 * or DEK (data encryption key), and the key dictionary to support any number of keys
 * using a single image. The header and section table are always unencrypted even
 * in encrypted images. This allows host utilities to access the individual
 * sections without needing to have access to an encryption key.
 *
 * To construct a boot image, first create an instance of EncoreBootImage. Then
 * create instances of the EncoreBootImage::BootSection or EncoreBootImage::DataSection
 * for each of the sections in the image. For bootable sections, create and add
 * the desired boot command objects. These are all subclasses of
 * EncoreBootImage::BootCommand.
 *
 * If the boot image is to be encrypted, you need to add keys, which are instances
 * of the AES128Key class. If no keys are added, the entire boot image will be unencrypted.
 *
 * When the image is fully constructed, it can be written to any std::ostream with
 * a call to writeToStream(). The same image can be written to streams any
 * number of times.
 */
class EncoreBootImage : public BootImage
{
public:
	//! \brief Flag constants for the m_flags field of #elftosb::EncoreBootImage::boot_image_header_t.
	enum
	{
		ROM_DISPLAY_PROGRESS = (1 << 0),		//!< Print progress reports.
		ROM_VERBOSE_PROGRESS = (1 << 1)			//!< Progress reports are verbose.
	};
	
#define ROM_IMAGE_HEADER_SIGNATURE "STMP"	//!< Signature in #elftosb::EncoreBootImage::boot_image_header_t::m_signature.
#define ROM_IMAGE_HEADER_SIGNATURE2 "sgtl"	//!< Value for #elftosb::EncoreBootImage::boot_image_header_t::m_signature2;
#define	ROM_BOOT_IMAGE_MAJOR_VERSION 1		//!< Current boot image major version.
#define	ROM_BOOT_IMAGE_MINOR_VERSION 1		//!< Current boot image minor version.
	
	enum {
		//! Minimum alignment for a section is 16 bytes.
		BOOT_IMAGE_MINIMUM_SECTION_ALIGNMENT = sizeof(cipher_block_t)
	};
	
// All of these structures are packed to byte alignment in order to match
// the structure on disk.
#pragma pack(1)
	
	//! \brief Header for the entire boot image.
	//!
	//! Fields of this header are arranged so that those used by the bootloader ROM all come
	//! first. They are also set up so that all fields are not split across cipher block
	//! boundaries. The fields not used by the bootloader are not subject to this
	//! restraint.
	//!
	//! Image header size is always a round number of cipher blocks. The same also applies to
	//! the boot image itself. The padding, held in #elftosb::EncoreBootImage::boot_image_header_t::m_padding0
	//! and #elftosb::EncoreBootImage::boot_image_header_t::m_padding1 is filled with random bytes.
	//!
	//! The DEK dictionary, section table, and each section data region must all start on
	//! cipher block boundaries.
	//!
	//! This header is not encrypted in the image file.
	//!
	//! The m_digest field contains a SHA-1 digest of the fields of the header that follow it.
	//! It is the first field in the header so it doesn't change position or split the header
	//! in two if fields are added to the header.
	struct boot_image_header_t
	{
		union
		{
			sha1_digest_t m_digest;		//!< SHA-1 digest of image header. Also used as the crypto IV.
			struct
			{
				cipher_block_t m_iv;	//!< The first 16 bytes of the digest form the initialization vector.
				uint8_t m_extra[4];		//!< The leftover top four bytes of the SHA-1 digest.
			};
		};
		uint8_t m_signature[4];			//!< 'STMP', see #ROM_IMAGE_HEADER_SIGNATURE.
		uint8_t m_majorVersion;			//!< Major version for the image format, see #ROM_BOOT_IMAGE_MAJOR_VERSION.
		uint8_t m_minorVersion;		//!< Minor version of the boot image format, see #ROM_BOOT_IMAGE_MINOR_VERSION.
		uint16_t m_flags;				//!< Flags or options associated with the entire image.
		uint32_t m_imageBlocks;			//!< Size of entire image in blocks.
		uint32_t m_firstBootTagBlock;	//!< Offset from start of file to the first boot tag, in blocks.
		section_id_t m_firstBootableSectionID;	//!< ID of section to start booting from.
		uint16_t m_keyCount;			//!< Number of entries in DEK dictionary.
		uint16_t m_keyDictionaryBlock;	//!< Starting block number for the key dictionary.
		uint16_t m_headerBlocks;		//!< Size of this header, including this size word, in blocks.
		uint16_t m_sectionCount;		//!< Number of section headers in this table.
		uint16_t m_sectionHeaderSize;	//!< Size in blocks of a section header.
		uint8_t m_padding0[2];			//!< Padding to align #m_timestamp to long word.
		uint8_t m_signature2[4];		//!< Second signature to distinguish this .sb format from the 36xx format, see #ROM_IMAGE_HEADER_SIGNATURE2.
		uint64_t m_timestamp;			//!< Timestamp when image was generated in microseconds since 1-1-2000.
		version_t m_productVersion;		//!< Product version.
		version_t m_componentVersion;	//!< Component version.
		uint16_t m_driveTag;			//!< Drive tag for the system drive which this boot image belongs to.
		uint8_t m_padding1[6];          //!< Padding to round up to next cipher block.
	};

	//! \brief Entry in #elftosb::EncoreBootImage::dek_dictionary_t.
	//!
	//! The m_dek field in each entry is encrypted using the KEK with the m_iv field from
	//! the image header as the IV.
	struct dek_dictionary_entry_t
	{
		cipher_block_t m_mac;			//!< CBC-MAC of the header.
		aes128_key_t m_dek;				//!< AES-128 key with which the image payload is encrypted.
	};

	//! \brief The DEK dictionary always follows the image header, in the next cipher block.
	struct dek_dictionary_t
	{
		dek_dictionary_entry_t m_entries[1];
	};

	//! \brief Section flags constants for the m_flags field of #elftosb::EncoreBootImage::section_header_t.
	enum
	{
		ROM_SECTION_BOOTABLE = (1 << 0),	//!< The section contains bootloader commands.
		ROM_SECTION_CLEARTEXT = (1 << 1)	//!< The section is unencrypted. Applies only if the rest of the boot image is encrypted.
	};

	//! \brief Information about each section, held in the section table.
	//! \see section_table_t
	struct section_header_t
	{
		uint32_t m_tag;					//!< Unique identifier for this section. High bit must be zero.
		uint32_t m_offset;				//!< Offset to section data from start of image in blocks.
		uint32_t m_length;				//!< Size of section data in blocks.
		uint32_t m_flags;				//!< Section flags.
	};
	
	//! \brief An index of all sections within the boot image.
	//!
	//! The section table will be padded so that its length is divisible by 16 (if necessary).
	//! Actually, each entry is padded to be a round number of cipher blocks, which
	//! automatically makes this true for the entire table.
	//!
	//! Sections are ordered as they appear in this table, but are identified by the
	//! #elftosb::EncoreBootImage::section_header_t::m_tag.
	//!
	//! The data for each section in encrypted separately with the DEK in CBC mode using
	//! m_iv for the IV. This allows the ROM to jump to any given section without needing
	//! to read the previous cipher block. In addition, the data for each section is
	//! prefixed with a "boot tag", which describes the section which follows it. Boot
	//! tags are the same format as a boot command, and are described by the
	//! EncoreBootImage::TagCommand class.
	//!
	//! The section table starts immediately after the image header, coming before the
	//! key dictionary (if present). The section table is not encrypted.
	struct section_table_t
	{
		section_header_t m_sections[1];	//!< The table entries.
	};

	//! \brief Structure for a Piano bootloader command.
	//!
	//! Each command is composed of a fixed length header of 16 bytes. This happens to be
	//! the size of a cipher block. Most commands will only require the header.
	//!
	//! But some commands, i.e. the "load data" command, may require additional arbitrary
	//! amounts of data. This data is packed into the N cipher blocks that immediately
	//! follow the command header. If the length of the data is not divisible by 16, then
	//! random (not zero!) pad bytes will be added.
	struct boot_command_t
	{
		uint8_t m_checksum;				//!< Simple checksum over other command fields.
		uint8_t m_tag;					//!< Tag telling which command this is.
		uint16_t m_flags;				//!< Flags for this command.
		uint32_t m_address;				//!< Target address.
		uint32_t m_count;				//!< Number of bytes on which to operate.
		uint32_t m_data;				//!< Additional data used by certain commands.
	};

#pragma pack()
	
	//! \brief Bootloader command tag constants.
	enum
	{
		ROM_NOP_CMD = 0x00,		//!< A no-op command.
		ROM_TAG_CMD = 0x01,		//!< Section tag command.
		ROM_LOAD_CMD = 0x02,	//!< Load data command.
		ROM_FILL_CMD = 0x03,	//!< Pattern fill command.
		ROM_JUMP_CMD = 0x04,	//!< Jump to address command.
		ROM_CALL_CMD = 0x05,	//!< Call function command.
		ROM_MODE_CMD = 0x06		//!< Change boot mode command.
	};
	
	//! \brief Flag field constants for #ROM_TAG_CMD.
	enum
	{
		ROM_LAST_TAG = (1 << 0)	//!< This tag command is the last one in the image.
	};
	
	//! \brief Flag field constants for #ROM_LOAD_CMD.
	enum
	{
		ROM_LOAD_DCD = (1 << 0)	//!< Execute the DCD after loading completes.
	};
	
	//! \brief Flag field constants for #ROM_FILL_CMD.
	enum
	{
		ROM_FILL_BYTE = 0,		//!< Fill with byte sized pattern.
		ROM_FILL_HALF_WORD = 1,	//!< Fill with half-word sized pattern.
		ROM_FILL_WORD = 2		//!< Fill with word sized pattern.
	};
	
	//! brief Flag field constants for #ROM_JUMP_CMD and #ROM_CALL_CMD.
	enum
	{
		ROM_HAB_EXEC = (1 << 0)	//!< Changes jump or call command to a HAB jump or call.
	};

public:
	// Forward declaration.
	class Section;
	
	/*!
	 * \brief Base class for objects that produce cipher blocks.
	 */
	class CipherBlockGenerator
	{
	public:
	
		//! \name Cipher blocks
		//@{
		//! \brief Returns the total number of cipher blocks.
		//!
		//! The default implementation returns 0, indicating that no blocks are
		//! available.
		virtual unsigned getBlockCount() const { return 0; }
		
		//! \brief Returns the contents of up to \a maxCount cipher blocks.
		//!
		//! Up to \a maxCount cipher blocks are copied into the buffer pointed to by
		//! the \a data argument. This is only a request for \a maxCount blocks,
		//! the subclass implementation of this method is free to return any number
		//! of blocks from 0 up to \a maxCount. A return value of 0 indicates that
		//! no more blocks are available. The index of the first block to copy is
		//! held in the \a offset argument.
		//!
		//! \param offset Starting block number to copy. Zero means the first available block.
		//! \param maxCount Up to this number of blocks may be copied into \a data. Must be 1 or greater.
		//! \param data Buffer for outgoing cipher blocks. Must have enough room to hold
		//!		\a maxCount blocks.
		//!
		//! \return The number of cipher blocks copied into \a data.
		//! \retval 0 No more blocks are available and nothing was written to \a data.
		virtual unsigned getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data) { return 0; }
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const {}
	};
	
	/*!
	 * \brief Abstract base class for all bootloader commands.
	 */
	class BootCommand : public CipherBlockGenerator
	{
	public:
		//! \brief Creates the correct subclass of BootCommand for the given raw data.
		static BootCommand * createFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
	public:
		//! \brief Default constructor.
		BootCommand() : CipherBlockGenerator() {}
		
		//! \brief Destructor.
		virtual ~BootCommand() {}
		
		//! \brief Read the command contents from raw data.
		//!
		//! The subclass implementations should validate the contents of the command, including
		//! the fields of the command header in the first block. It should be assumed that
		//! only the tag field was examined to determine which subclass of BootCommand
		//! should be created.
		//!
		//! \param blocks Pointer to the raw data blocks.
		//! \param count Number of blocks pointed to by \a blocks.
		//! \param[out] consumed On exit, this points to the number of cipher blocks that were occupied
		//!		by the command. Should be at least 1 for every command. This must not be NULL
		//!		on entry!
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed)=0;
		
		//! \name Header
		//@{
		//! \brief Pure virtual method to return the tag value for this command.
		virtual uint8_t getTag() const = 0;
		
		//! \brief Pure virtual method to construct the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header) = 0;
		
		//! \brief Calculates the checksum for the given command header.
		virtual uint8_t calculateChecksum(const boot_command_t & header);
		//@}
		
		//! \name Cipher blocks
		//@{
		//! \brief Returns the total number of cipher blocks.
		virtual unsigned getBlockCount() const;
		
		//! \brief Returns the contents of up to \a maxCount cipher blocks.
		virtual unsigned getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data);
		//@}
		
		//! \name Data blocks
		//@{
		//! \brief Returns the number of data cipher blocks that follow this command.
		//!
		//! The default implementation returns 0, indicating that no data blocks are
		//! available.
		virtual unsigned getDataBlockCount() const { return 0; }
		
		//! \brief Returns the contents of up to \a maxCount data blocks.
		//!
		//! Up to \a maxCount data blocks are copied into the buffer pointed to by
		//! the \a data argument. This is only a request for \a maxCount blocks,
		//! the subclass implementation of this method is free to return any number
		//! of blocks from 0 up to \a maxCount. A return value of 0 indicates that
		//! no more blocks are available. The index of the first block to copy is
		//! held in the \a offset argument.
		//!
		//! \param offset Starting block number to copy. Zero means the first available block.
		//! \param maxCount Up to this number of blocks may be copied into \a data. Must be 1 or greater.
		//! \param data Buffer for outgoing data blocks. Must have enough room to hold
		//!		\a maxCount blocks.
		//!
		//! \return The number of data blocks copied into \a data.
		//! \retval 0 No more blocks are available and nothing was written to \a data.
		virtual unsigned getDataBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data) { return 0; }
		//@}
	
	protected:
		//! The flag bit values for the \a whichFields parameter of validateHeader().
		enum
		{
			CMD_TAG_FIELD = 1,
			CMD_FLAGS_FIELD = 2,
			CMD_ADDRESS_FIELD = 4,
			CMD_COUNT_FIELD = 8,
			CMD_DATA_FIELD = 16
		};

		//! \brief
		void validateHeader(const boot_command_t * modelHeader, const boot_command_t * testHeader, unsigned whichFields);
	};
	
	/*!
	 * \brief No operation bootloader command.
	 */
	class NopCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		NopCommand() : BootCommand() {}
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_NOP_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
	};
	
	/*!
	 * \brief Section tag bootloader command.
	 */
	class TagCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		TagCommand() : BootCommand() {}
		
		//! \brief Constructor taking a section object.
		TagCommand(const Section & section);
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_TAG_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		//@}
		
		//! \name Field accessors
		//@{
		inline void setSectionIdentifier(uint32_t identifier) { m_sectionIdentifier = identifier; }
		inline void setSectionLength(uint32_t length) { m_sectionLength = length; }
		inline void setSectionFlags(uint32_t flags) { m_sectionFlags = flags; }
		inline void setLast(bool isLast) { m_isLast = isLast; }
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
		
	protected:
		uint32_t m_sectionIdentifier;	//!< Unique identifier for the section containing this command.
		uint32_t m_sectionLength;	//!< Number of cipher blocks this section occupies.
		uint32_t m_sectionFlags;	//!< Flags pertaining to this section.
		bool m_isLast;	//!< Is this the last tag command?
	};
	
	/*!
	 * \brief Load data bootloader command.
	 */
	class LoadCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		LoadCommand();
		
		//! \brief Constructor.
		LoadCommand(uint32_t address, const uint8_t * data, uint32_t length);
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_LOAD_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		
		//! \brief Sets the load-dcd flag.
		inline void setDCD(bool isDCD) { m_loadDCD = isDCD; }
		//@}
		
		//! \name Address
		//@{
		inline void setLoadAddress(uint32_t address) { m_address = address; }
		inline uint32_t getLoadAddress() const { return m_address; }
		//@}
		
		//! \name Load data
		//@{
		//! \brief Set the data for the command to load.
		void setData(const uint8_t * data, uint32_t length);
		
		inline uint8_t * getData() { return m_data; }
		inline const uint8_t * getData() const { return m_data; }
		inline uint32_t getLength() const { return m_length; }
		//@}
		
		//! \name Data blocks
		//@{
		//! \brief Returns the number of data cipher blocks that follow this command.
		virtual unsigned getDataBlockCount() const;
		
		//! \brief Returns the contents of up to \a maxCount data blocks.
		virtual unsigned getDataBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data);
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
	
	protected:
		smart_array_ptr<uint8_t> m_data;	//!< Pointer to data to load.
		uint8_t m_padding[15];	//!< Up to 15 pad bytes may be required.
		unsigned m_padCount;	//!< Number of pad bytes.
		uint32_t m_length;	//!< Number of bytes to load.
		uint32_t m_address;	//!< Address to which data will be loaded.
		bool m_loadDCD;	//!< Whether to execute the DCD after loading.
		
		void fillPadding();
		uint32_t calculateCRC() const;
	};
	
	/*!
	 * \brief Pattern fill bootloader command.
	 */
	class FillCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		FillCommand();
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_FILL_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		//@}
		
		//! \name Address range
		//@{
		inline void setAddress(uint32_t address) { m_address = address; };
		inline uint32_t getAddress() const { return m_address; }
		
		inline void setFillCount(uint32_t count) { m_count = count; }
		inline uint32_t getFillCount() const { return m_count; }
		//@}
		
		//! \name Pattern
		//@{
		void setPattern(uint8_t pattern);
		void setPattern(uint16_t pattern);
		void setPattern(uint32_t pattern);
		
		inline uint32_t getPattern() const { return m_pattern; }
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
	
	protected:
		uint32_t m_address;	//!< Fill start address.
		uint32_t m_count;	//!< Number of bytes to fill.
		uint32_t m_pattern;	//!< Fill pattern.
	};
	
	/*!
	 * \brief Change boot mode bootloader command.
	 */
	class ModeCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		ModeCommand() : BootCommand(), m_mode(0) {}
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_MODE_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		//@}
		
		//! \name Boot mode
		//@{
		inline void setBootMode(uint32_t mode) { m_mode = mode; }
		inline uint32_t getBootMode() const { return m_mode; }
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
	
	protected:
		uint32_t m_mode;	//!< New boot mode.
	};
	
	/*!
	 * \brief Jump to address bootloader command.
	 */
	class JumpCommand : public BootCommand
	{
	public:
		//! \brief Default constructor.
		JumpCommand() : BootCommand(), m_address(0), m_argument(0), m_isHAB(false), m_ivtSize(0) {}
		
		//! \brief Read the command contents from raw data.
		virtual void initFromData(const cipher_block_t * blocks, unsigned count, unsigned * consumed);
		
		//! \name Header
		//@{
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_JUMP_CMD; }
		
		//! \brief Constructs the header for this boot command.
		virtual void fillCommandHeader(boot_command_t & header);
		//@}
		
		//! \name Accessors
		//@{
		inline void setAddress(uint32_t address) { m_address = address; }
		inline uint32_t getAddress() const { return m_address; }
		
		inline void setArgument(uint32_t argument) { m_argument = argument; }
		inline uint32_t getArgument() const { return m_argument; }
		
		inline void setIsHAB(bool isHAB) { m_isHAB = isHAB; }
		inline bool isHAB() const { return m_isHAB; }
		
		inline void setIVTSize(uint32_t ivtSize) { m_ivtSize = ivtSize; }
		inline uint32_t getIVTSize() const { return m_ivtSize; }
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
		
	protected:
		uint32_t m_address;	//!< Address of the code to execute.
		uint32_t m_argument;	//!< Sole argument to pass to code.
		bool m_isHAB;		//!< Whether this jump/call is a special HAB jump/call. When this flag is set, m_address becomes the IVT address and m_ivtSize is the IVT size.
		uint32_t m_ivtSize;	//!< Size of the IVT for a HAB jump/call.
	};
	
	/*!
	 * \brief Call function bootloader command.
	 */
	class CallCommand : public JumpCommand
	{
	public:
		//! \brief Default constructor.
		CallCommand() : JumpCommand() {}
		
		//! \brief Returns the tag value for this command.
		virtual uint8_t getTag() const { return ROM_CALL_CMD; }
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
	};
	
	/*!
	 * \brief Base class for a section of an Encore boot image.
	 *
	 * Provides methods to manage the unique identifier that all sections have, and
	 * to set the boot image object which owns the section. There are also virtual
	 * methods to get header flags and fill in the header used in the section
	 * table. Subclasses must implement at least fillSectionHeader().
	 */
	class Section : public CipherBlockGenerator
	{
	public:
		//! \brief Default constructor.
		Section() : CipherBlockGenerator(), m_identifier(0), m_image(0), m_alignment(BOOT_IMAGE_MINIMUM_SECTION_ALIGNMENT), m_flags(0), m_leaveUnencrypted(false) {}
		
		//! \brief Constructor taking the unique identifier for this section.
		Section(uint32_t identifier) : CipherBlockGenerator(), m_identifier(identifier), m_image(0), m_alignment(BOOT_IMAGE_MINIMUM_SECTION_ALIGNMENT), m_flags(0), m_leaveUnencrypted(false) {}
		
		//! \name Identifier
		//@{
		inline void setIdentifier(uint32_t identifier) { m_identifier = identifier; }
		inline uint32_t getIdentifier() const { return m_identifier; }
		//@}
		
		//! \name Header
		//@{
		//! \brief Sets explicit flags for this section.
		virtual void setFlags(uint32_t flags) { m_flags = flags; }
		
		//! \brief Returns the flags for this section.
		//!
		//! The return value consists of the flags set with setFlags() possibly or-ed
		//! with #ROM_SECTION_CLEARTEXT if the section has been set to be left
		//! unencrypted.
		virtual uint32_t getFlags() const { return m_flags | ( m_leaveUnencrypted ? ROM_SECTION_CLEARTEXT : 0); }
		
		//! \brief Pure virtual method to construct the header for this section.
		virtual void fillSectionHeader(section_header_t & header);
		//@}
		
		//! \name Owner image
		//@{
		//! \brief Called when the section is added to an image.
		void setImage(EncoreBootImage * image) { m_image = image; }
		
		//! \brief Returns a pointer to the image that this section belongs to.
		EncoreBootImage * getImage() const { return m_image; }
		//@}
		
		//! \name Alignment
		//@{
		//! \brief Sets the required alignment in the output file for this section.
		void setAlignment(unsigned alignment);
		
		//! \brief Returns the current alignment, the minimum of which will be 16.
		unsigned getAlignment() const { return m_alignment; }
		
		//! \brief Computes padding amount for alignment requirement.
		unsigned getPadBlockCountForOffset(unsigned offset);
		//@}
		
		//! \name Leave unencrypted flag
		//@{
		//! \brief Sets whether the section will be left unencrypted.
		void setLeaveUnencrypted(unsigned flag) { m_leaveUnencrypted = flag; }
		
		//! \brief Returns true if the section will remain unencrypted.
		bool getLeaveUnencrypted() const { return m_leaveUnencrypted; }
		//@}
	
	protected:
		uint32_t m_identifier;	//!< Unique identifier for this section.
		EncoreBootImage * m_image;	//!< The image to which this section belongs.
		unsigned m_alignment;	//!< Alignment requirement for the start of this section.
		uint32_t m_flags;	//!< Section flags set by the user.
		bool m_leaveUnencrypted;	//!< Set to true to prevent this section from being encrypted.
	};
	
	/*!
	 * \brief A bootable section of an Encore boot image.
	 */
	class BootSection : public Section
	{
	public:
		typedef std::list<BootCommand*> command_list_t;
		typedef command_list_t::iterator iterator_t;
		typedef command_list_t::const_iterator const_iterator_t;
		
	public:
		//! \brief Default constructor.
		BootSection() : Section() {}
		
		//! \brief Constructor taking the unique identifier for this section.
		BootSection(uint32_t identifier) : Section(identifier) {}
		
		//! \brief Destructor.
		virtual ~BootSection();
		
		//! \brief Load the section from raw data.
		virtual void fillFromData(const cipher_block_t * blocks, unsigned count);
		
		//! \name Header
		//@{
		//! \brief Returns the flags for this section.
		virtual uint32_t getFlags() const { return Section::getFlags() | ROM_SECTION_BOOTABLE; }
		//@}
		
		//! \name Commands
		//@{
		//! \brief Append a new command to the section.
		//!
		//! The section takes ownership of the command and will delete it when
		//! the section is destroyed.
		void addCommand(BootCommand * command) { m_commands.push_back(command); }
		
		//! \brief Returns the number of commands in this section, excluding the tag command.
		unsigned getCommandCount() const { return (unsigned)m_commands.size(); }
		
		iterator_t begin() { return m_commands.begin(); }
		iterator_t end() { return m_commands.end(); }
		
		const_iterator_t begin() const { return m_commands.begin(); }
		const_iterator_t end() const { return m_commands.end(); }
		//@}
	
		//! \name Cipher blocks
		//@{
		//! \brief Returns the total number of cipher blocks occupied by this section.
		virtual unsigned getBlockCount() const;
		
		//! \brief Returns the contents of up to \a maxCount cipher blocks.
		virtual unsigned getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data);
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
		
	protected:
		command_list_t m_commands;	//!< Commands held in this section.
		
	protected:
		//! \brief Remove all commands from the section.
		void deleteCommands();
	};
	
	/*!
	 * \brief A non-bootable section of an Encore boot image.
	 */
	class DataSection : public Section
	{
	public:
		//! \brief Default constructor.
		DataSection() : Section(), m_data(), m_length(0) {}
		
		//! \brief Constructor taking the unique identifier for this section.
		DataSection(uint32_t identifier) : Section(identifier), m_data(), m_length(0) {}
		
		//! \brief Set the data section's contents.
		void setData(const uint8_t * data, unsigned length);
		
		//! \brief Set the data section's contents without copying \a data.
		void setDataNoCopy(const uint8_t * data, unsigned length);
	
		//! \name Cipher blocks
		//@{
		//! \brief Returns the total number of cipher blocks occupied by this section.
		virtual unsigned getBlockCount() const;
		
		//! \brief Returns the contents of up to \a maxCount cipher blocks.
		virtual unsigned getBlocks(unsigned offset, unsigned maxCount, cipher_block_t * data);
		//@}
		
		//! \brief Print out a string representation of the object.
		virtual void debugPrint() const;
		
	protected:
		smart_array_ptr<uint8_t> m_data;	//!< The section's contents.
		unsigned m_length;	//!< Number of bytes of data.
	};

public:
	typedef std::list<Section*> section_list_t;				//!< List of image sections.
	typedef section_list_t::iterator section_iterator_t;	//!< Iterator over sections.
	typedef section_list_t::const_iterator const_section_iterator_t;	//!< Const iterator over sections.
	
	typedef std::vector<AES128Key> key_list_t;				//!< List of KEKs.
	typedef key_list_t::iterator key_iterator_t;			//!< Iterator over KEKs.
	typedef key_list_t::const_iterator const_key_iterator_t;			//!< Const iterator over KEKs.

public:
	//! \brief Default constructor.
	EncoreBootImage();
	
	//! \brief Destructor.
	virtual ~EncoreBootImage();
	
	//! \name Sections
	//@{
	void addSection(Section * newSection);
	inline unsigned sectionCount() const { return (unsigned)m_sections.size(); }
	
	inline section_iterator_t beginSection() { return m_sections.begin(); }
	inline section_iterator_t endSection() { return m_sections.end(); }
	inline const_section_iterator_t beginSection() const { return m_sections.begin(); }
	inline const_section_iterator_t endSection() const { return m_sections.end(); }
	
	section_iterator_t findSection(Section * section);
	
	//! \brief Calculates the starting block number for the given section.
	uint32_t getSectionOffset(Section * section);
	//@}
	
	//! \name Encryption keys
	//@{
	inline void addKey(const AES128Key & newKey) { m_keys.push_back(newKey); }
	inline unsigned keyCount() const { return (unsigned)m_keys.size(); }

	inline key_iterator_t beginKeys() { return m_keys.begin(); }
	inline key_iterator_t endKeys() { return m_keys.end(); }
	inline const_key_iterator_t beginKeys() const { return m_keys.begin(); }
	inline const_key_iterator_t endKeys() const { return m_keys.end(); }
	
	//! \brief The image is encrypted if there is at least one key.
	inline bool isEncrypted() const { return m_keys.size() != 0; }
	//@}
	
	//! \name Versions
	//@{
	virtual void setProductVersion(const version_t & version);
	virtual void setComponentVersion(const version_t & version);
	//@}
	
	//! \name Flags
	//@{
	inline void setFlags(uint16_t flags) { m_headerFlags = flags; }
	inline uint32_t getFlags() const { return m_headerFlags; }
	//@}
	
	//! \brief Specify the drive tag to be set in the output file header.
	virtual void setDriveTag(uint16_t tag) { m_driveTag = tag; }
	
	//! \brief Calculates the total number of cipher blocks the image consumes.
	uint32_t getImageSize();
	
	//! \brief Returns the preferred ".sb" extension for Encore boot images.
	virtual std::string getFileExtension() const { return ".sb"; }
	
	//! \name Output
	//@{
	//! \brief Write the boot image to an output stream.
	virtual void writeToStream(std::ostream & stream);
	//@}
		
	//! \brief Print out a string representation of the object.
	virtual void debugPrint() const;
	
protected:
	uint16_t m_headerFlags;	//!< Flags field in the boot image header.
	version_t m_productVersion;		//!< Product version.
	version_t m_componentVersion;	//!< Component version.
	uint16_t m_driveTag;	//!< System drive tag for this boot image.
	section_list_t m_sections;	//!< Sections contained in this image.
	key_list_t m_keys;	//!< List of key encryption keys. If empty, the image is unencrypted.
	AES128Key m_sessionKey;	//!< Session key we're using.
	
	void prepareImageHeader(boot_image_header_t & header);
	uint64_t getTimestamp();
	Section * findFirstBootableSection();
	unsigned getPadBlockCountForSection(Section * section, unsigned offset);
};

}; // namespace elftosb

#endif // _EncoreBootImage_h_
