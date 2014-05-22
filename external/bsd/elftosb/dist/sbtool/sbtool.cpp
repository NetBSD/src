/*
 * File:	sbtool.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <stdio.h>
#include "options.h"
#include "EncoreBootImage.h"
#include "smart_ptr.h"
#include "Logging.h"
#include "EncoreBootImageReader.h"
#include "format_string.h"

using namespace elftosb;

//! The tool's name.
const char k_toolName[] = "sbtool";

//! Current version number for the tool.
const char k_version[] = "1.1.4";

//! Copyright string.
const char k_copyright[] = "Copyright (c) 2006-2010 Freescale Semiconductor, Inc.\nAll rights reserved.";

//! Definition of command line options.
static const char * k_optionsDefinition[] = {
	"?|help",
	"v|version",
	"k:key <file>",
	"z|zero-key",
	"x:extract",
	"b|binary",
	"d|debug",
	"q|quiet",
	"V|verbose",
	NULL
};

//! Help string.
const char k_usageText[] = "\nOptions:\n\
  -?/--help                    Show this help\n\
  -v/--version                 Display tool version\n\
  -k/--key <file>              Add OTP key used for decryption\n\
  -z/--zero-key                Add default key of all zeroes\n\
  -x/--extract <index>         Extract section number <index>\n\
  -b/--binary                  Extract section data as binary\n\
  -d/--debug                   Enable debug output\n\
  -q/--quiet                   Output only warnings and errors\n\
  -V/--verbose                 Print extra detailed log information\n\n";

//! An array of strings.
typedef std::vector<std::string> string_vector_t;

// prototypes
int main(int argc, char* argv[], char* envp[]);

/*!
 * \brief Class that encapsulates the sbtool interface.
 *
 * A single global logger instance is created during object construction. It is
 * never freed because we need it up to the last possible minute, when an
 * exception could be thrown.
 */
class sbtool
{
protected:
	int m_argc;							//!< Number of command line arguments.
	char ** m_argv;						//!< String value for each command line argument.
	StdoutLogger * m_logger;			//!< Singleton logger instance.
	string_vector_t m_keyFilePaths;		//!< Paths to OTP key files.
	string_vector_t m_positionalArgs;	//!< Arguments coming after explicit options.
	bool m_isVerbose;					//!< Whether the verbose flag was turned on.
	bool m_useDefaultKey;					//!< Include a default (zero) crypto key.
	bool m_doExtract;					//!< True if extract mode is on.
	unsigned m_sectionIndex;				//!< Index of section to extract.
	bool m_extractBinary;				//!< True if extraction output is binary, false for hex.
	smart_ptr<EncoreBootImageReader> m_reader;	//!< Boot image reader object.
	
public:
	/*!
	 * Constructor.
	 *
	 * Creates the singleton logger instance.
	 */
	sbtool(int argc, char * argv[])
	:	m_argc(argc),
		m_argv(argv),
		m_logger(0),
		m_keyFilePaths(),
		m_positionalArgs(),
		m_isVerbose(false),
		m_useDefaultKey(false),
		m_doExtract(false),
		m_sectionIndex(0),
		m_extractBinary(false),
		m_reader()
	{
		// create logger instance
		m_logger = new StdoutLogger();
		m_logger->setFilterLevel(Logger::INFO);
		Log::setLogger(m_logger);
	}
	
	/*!
	 * Destructor.
	 */
	~sbtool()
	{
	}
	
	/*!
	 * Reads the command line options passed into the constructor.
	 *
	 * This method can return a return code to its caller, which will cause the
	 * tool to exit immediately with that return code value. Normally, though, it
	 * will return -1 to signal that the tool should continue to execute and
	 * all options were processed successfully.
	 *
	 * The Options class is used to parse command line options. See
	 * #k_optionsDefinition for the list of options and #k_usageText for the
	 * descriptive help for each option.
	 *
	 * \retval -1 The options were processed successfully. Let the tool run normally.
	 * \return A zero or positive result is a return code value that should be
	 *		returned from the tool as it exits immediately.
	 */
	int processOptions()
	{
		Options options(*m_argv, k_optionsDefinition);
		OptArgvIter iter(--m_argc, ++m_argv);
		
		// process command line options
		int optchar;
		const char * optarg;
		while (optchar = options(iter, optarg))
		{
			switch (optchar)
			{
				case '?':
					printUsage(options);
					return 0;
				
				case 'v':
					printf("%s %s\n%s\n", k_toolName, k_version, k_copyright);
					return 0;
					
				case 'k':
					m_keyFilePaths.push_back(optarg);
					break;
				
				case 'z':
					m_useDefaultKey = true;
					break;
				
				case 'x':
					m_doExtract = true;
					m_sectionIndex = strtoul(optarg, NULL, 0);
					break;
				
				case 'b':
					m_extractBinary = true;
					Log::getLogger()->setFilterLevel(Logger::WARNING);
					break;
					
				case 'd':
					Log::getLogger()->setFilterLevel(Logger::DEBUG);
					break;
					
				case 'q':
					Log::getLogger()->setFilterLevel(Logger::WARNING);
					break;
					
				case 'V':
					m_isVerbose = true;
					break;
				
				default:
					Log::log(Logger::ERROR, "error: unrecognized option\n\n");
					printUsage(options);
					return 1;
			}
		}
		
		// handle positional args
		if (iter.index() < m_argc)
		{
//			Log::SetOutputLevel leveler(Logger::DEBUG);
//			Log::log("positional args:\n");
			int i;
			for (i = iter.index(); i < m_argc; ++i)
			{
//				Log::log("%d: %s\n", i - iter.index(), m_argv[i]);
				m_positionalArgs.push_back(m_argv[i]);
			}
		}
		
		// all is well
		return -1;
	}

	/*!
	 * Prints help for the tool.
	 */
	void printUsage(Options & options)
	{
		options.usage(std::cout, "sb-file");
		printf("%s", k_usageText);
	}
	
	/*!
	 * Core of the tool. Calls processOptions() to handle command line options
	 * before performing the real work the tool does.
	 */
	int run()
	{
		try
		{
			// read command line options
			int result;
			if ((result = processOptions()) != -1)
			{
				return result;
			}
			
			// set verbose logging
			setVerboseLogging();
			
			// make sure a file was provided
			if (m_positionalArgs.size() < 1)
			{
				throw std::runtime_error("no sb file path was provided");
			}
			
			// read the boot image
			readBootImage();
		}
		catch (std::exception & e)
		{
			Log::log(Logger::ERROR, "error: %s\n", e.what());
			return 1;
		}
		catch (...)
		{
			Log::log(Logger::ERROR, "error: unexpected exception\n");
			return 1;
		}
		
		return 0;
	}
	
	/*!
	 * \brief Turns on verbose logging.
	 */
	void setVerboseLogging()
	{
		if (m_isVerbose)
		{
			// verbose only affects the INFO and DEBUG filter levels
			// if the user has selected quiet mode, it overrides verbose
			switch (Log::getLogger()->getFilterLevel())
			{
				case Logger::INFO:
					Log::getLogger()->setFilterLevel(Logger::INFO2);
					break;
				case Logger::DEBUG:
					Log::getLogger()->setFilterLevel(Logger::DEBUG2);
					break;
			}
		}
	}
	
	/*!
	 * \brief Opens and reads the boot image identified on the command line.
	 * \pre At least one position argument must be present.
	 */
	void readBootImage()
	{
		Log::SetOutputLevel infoLevel(Logger::INFO);
		
		// open the sb file stream
		std::ifstream sbStream(m_positionalArgs[0].c_str(), std::ios_base::binary | std::ios_base::in);
		if (!sbStream.is_open())
		{
			throw std::runtime_error("failed to open input file");
		}
		
		// create the boot image reader
		m_reader = new EncoreBootImageReader(sbStream);
		
		// read image header
		m_reader->readImageHeader();
		const EncoreBootImage::boot_image_header_t & header = m_reader->getHeader();
		if (header.m_majorVersion > 1)
		{
			throw std::runtime_error(format_string("boot image format version is too new (format version %d.%d)\n", header.m_majorVersion, header.m_minorVersion));
		}
		Log::log("---- Boot image header ----\n");
		dumpImageHeader(header);
		
		// compute SHA-1 over image header and test against the digest stored in the header
		sha1_digest_t computedDigest;
		m_reader->computeHeaderDigest(computedDigest);
		if (compareDigests(computedDigest, m_reader->getHeader().m_digest))
		{
			Log::log("Header digest is correct.\n");
		}
		else
		{
			Log::log(Logger::WARNING, "warning: stored SHA-1 header digest does not match the actual header digest\n");
			Log::log(Logger::WARNING, "\n---- Actual SHA-1 digest of image header ----\n");
			logHexArray(Logger::WARNING, (uint8_t *)&computedDigest, sizeof(computedDigest));
		}
		
		// read the section table
		m_reader->readSectionTable();
		const EncoreBootImageReader::section_array_t & sectionTable = m_reader->getSections();
		EncoreBootImageReader::section_array_t::const_iterator it = sectionTable.begin();
		Log::log("\n---- Section table ----\n");
		unsigned n = 0;
		for (; it != sectionTable.end(); ++it, ++n)
		{
			const EncoreBootImage::section_header_t & sectionHeader = *it;
			Log::log("Section %d:\n", n);
			dumpSectionHeader(sectionHeader);
		}
		
		// read the key dictionary
		// XXX need to support multiple keys, not just the first!
		if (m_reader->isEncrypted())
		{
			Log::log("\n---- Key dictionary ----\n");
			if (m_keyFilePaths.size() > 0 || m_useDefaultKey)
			{
				if (m_keyFilePaths.size() > 0)
				{
					std::string & keyPath = m_keyFilePaths[0];
					std::ifstream keyStream(keyPath.c_str(), std::ios_base::binary | std::ios_base::in);
					if (!keyStream.is_open())
					{
						Log::log(Logger::WARNING, "warning: unable to read key %s\n", keyPath.c_str());
					}
					AESKey<128> kek(keyStream);
				
					// search for this key in the key dictionary
					if (!m_reader->readKeyDictionary(kek))
					{
						throw std::runtime_error("the provided key is not valid for this encrypted boot image");
					}
					
					Log::log("\nKey %s was found in key dictionary.\n", keyPath.c_str());
				}
				else
				{
					// default key of zero, overriden if -k was used
					AESKey<128> defaultKek;
				
					// search for this key in the key dictionary
					if (!m_reader->readKeyDictionary(defaultKek))
					{
						throw std::runtime_error("the default key is not valid for this encrypted boot image");
					}
					
					Log::log("\nDefault key was found in key dictionary.\n");
				}
				
				// print out the DEK
				AESKey<128> dek = m_reader->getKey();
				std::stringstream dekStringStream(std::ios_base::in | std::ios_base::out);
				dek.writeToStream(dekStringStream);
				std::string dekString = dekStringStream.str();
// 				Log::log("\nData encryption key: %s\n", dekString.c_str());
				Log::log("\nData encryption key:\n");
				logHexArray(Logger::INFO, (const uint8_t *)&dek.getKey(), sizeof(AESKey<128>::key_t));
			}
			else
			{
				throw std::runtime_error("the image is encrypted but no key was provided");
			}
		}
		
		// read the SHA-1 digest over the entire image. this is done after
		// reading the key dictionary because the digest is encrypted in
		// encrypted boot images.
		m_reader->readImageDigest();
		const sha1_digest_t & embeddedDigest = m_reader->getDigest();
		Log::log("\n---- SHA-1 digest of entire image ----\n");
		logHexArray(Logger::INFO, (const uint8_t *)&embeddedDigest, sizeof(embeddedDigest));
		
		// compute the digest over the entire image and compare
		m_reader->computeImageDigest(computedDigest);
		if (compareDigests(computedDigest, embeddedDigest))
		{
			Log::log("Image digest is correct.\n");
		}
		else
		{
			Log::log(Logger::WARNING, "warning: stored SHA-1 digest does not match the actual digest\n");
			Log::log(Logger::WARNING, "\n---- Actual SHA-1 digest of entire image ----\n");
			logHexArray(Logger::WARNING, (uint8_t *)&computedDigest, sizeof(computedDigest));
		}
		
		// read the boot tags
		m_reader->readBootTags();
		Log::log("\n---- Boot tags ----\n");
		unsigned block = header.m_firstBootTagBlock;
		const EncoreBootImageReader::boot_tag_array_t & tags = m_reader->getBootTags();
		EncoreBootImageReader::boot_tag_array_t::const_iterator tagIt = tags.begin();
		for (n = 0; tagIt != tags.end(); ++tagIt, ++n)
		{
			const EncoreBootImage::boot_command_t & command = *tagIt;
			Log::log("%04u: @ block %06u | id=0x%08x | length=%06u | flags=0x%08x\n", n, block, command.m_address, command.m_count, command.m_data);
					
			if (command.m_data & EncoreBootImage::ROM_SECTION_BOOTABLE)
			{
				Log::log("        0x1 = ROM_SECTION_BOOTABLE\n");
			}
			
			if (command.m_data & EncoreBootImage::ROM_SECTION_CLEARTEXT)
			{
				Log::log("        0x2 = ROM_SECTION_CLEARTEXT\n");
			}
			
			block += command.m_count + 1;
		}
        
        // now read all of the sections
		Log::log(Logger::INFO2, "\n---- Sections ----\n");
        for (n = 0; n < header.m_sectionCount; ++n)
        {
            EncoreBootImage::Section * section = m_reader->readSection(n);
            section->debugPrint();
			
			// Check if this is the section the user wants to extract.
			if (m_doExtract && n == m_sectionIndex)
			{
				extractSection(section);
			}
        }
	}
	
	//! \brief Dumps the contents of a section to stdout.
	//!
	//! If #m_extractBinary is true then the contents are written as
	//! raw binary to stdout. Otherwise the data is formatted using
	//! logHexArray().
	void extractSection(EncoreBootImage::Section * section)
	{
		// Allocate buffer to hold section data.
		unsigned blockCount = section->getBlockCount();
		unsigned dataLength = sizeOfCipherBlocks(blockCount);
		smart_array_ptr<uint8_t> buffer = new uint8_t[dataLength];
		cipher_block_t * data = reinterpret_cast<cipher_block_t *>(buffer.get());
		
		// Read section data into the buffer one block at a time.
		unsigned offset;
		for (offset = 0; offset < blockCount;)
		{
			unsigned blocksRead = section->getBlocks(offset, 1, data);
			offset += blocksRead;
			data += blocksRead;
		}
		
		// Print header.
		Log::log(Logger::INFO, "\nSection %d contents:\n", m_sectionIndex);
		
		// Now dump the extracted data to stdout.
		if (m_extractBinary)
		{
			if (fwrite(buffer.get(), 1, dataLength, stdout) != dataLength)
			{
				throw std::runtime_error(format_string("failed to write data to stdout (%d)", ferror(stdout)));
			}
		}
		else
		{
			// Use the warning log level so the data will be visible even in quiet mode.
			logHexArray(Logger::WARNING, buffer, dataLength);
		}
	}
	
	//! \brief Compares two SHA-1 digests and returns whether they are equal.
	//! \retval true The two digests are equal.
	//! \retval false The \a a and \a b digests are different from each other.
	bool compareDigests(const sha1_digest_t & a, const sha1_digest_t & b)
	{
		return memcmp(a, b, sizeof(sha1_digest_t)) == 0;
	}
	
	/*
	struct boot_image_header_t
	{
		union
		{
			sha1_digest_t m_digest;		//!< SHA-1 digest of image header. Also used as the crypto IV.
			struct
			{
				cipher_block_t m_iv;	//!< The first four bytes of the digest form the initialization vector.
				uint8_t m_extra[4];		//!< The leftover top four bytes of the SHA-1 digest.
			};
		};
		uint8_t m_signature[4];			//!< 'STMP', see #ROM_IMAGE_HEADER_SIGNATURE.
		uint16_t m_version;				//!< Version of the boot image format, see #ROM_BOOT_IMAGE_VERSION.
		uint16_t m_flags;				//!< Flags or options associated with the entire image.
		uint32_t m_imageBlocks;			//!< Size of entire image in blocks.
		uint32_t m_firstBootTagBlock;	//!< Offset from start of file to the first boot tag, in blocks.
		section_id_t m_firstBootableSectionID;	//!< ID of section to start booting from.
		uint16_t m_keyCount;			//!< Number of entries in DEK dictionary.
		uint16_t m_keyDictionaryBlock;	//!< Starting block number for the key dictionary.
		uint16_t m_headerBlocks;		//!< Size of this header, including this size word, in blocks.
		uint16_t m_sectionCount;		//!< Number of section headers in this table.
		uint16_t m_sectionHeaderSize;	//!< Size in blocks of a section header.
		uint8_t m_padding0[6];			//!< Padding to align #m_timestamp to long word.
		uint64_t m_timestamp;			//!< Timestamp when image was generated in microseconds since 1-1-2000.
		version_t m_productVersion;		//!< Product version.
		version_t m_componentVersion;	//!< Component version.
		uint16_t m_driveTag;
		uint8_t m_padding1[6];          //!< Padding to round up to next cipher block.
	};
	*/
	void dumpImageHeader(const EncoreBootImage::boot_image_header_t & header)
	{
		version_t vers;

		Log::SetOutputLevel infoLevel(Logger::INFO);
		Log::log("Signature 1:           %c%c%c%c\n", header.m_signature[0], header.m_signature[1], header.m_signature[2], header.m_signature[3]);
		Log::log("Signature 2:           %c%c%c%c\n", header.m_signature2[0], header.m_signature2[1], header.m_signature2[2], header.m_signature2[3]);
		Log::log("Format version:        %d.%d\n", header.m_majorVersion, header.m_minorVersion);
		Log::log("Flags:                 0x%04x\n", header.m_flags);
		Log::log("Image blocks:          %u\n", header.m_imageBlocks);
		Log::log("First boot tag block:  %u\n", header.m_firstBootTagBlock);
		Log::log("First boot section ID: 0x%08x\n", header.m_firstBootableSectionID);
		Log::log("Key count:             %u\n", header.m_keyCount);
		Log::log("Key dictionary block:  %u\n", header.m_keyDictionaryBlock);
		Log::log("Header blocks:         %u\n", header.m_headerBlocks);
		Log::log("Section count:         %u\n", header.m_sectionCount);
		Log::log("Section header size:   %u\n", header.m_sectionHeaderSize);
		Log::log("Timestamp:             %llu\n", header.m_timestamp);
		vers = header.m_productVersion;
		vers.fixByteOrder();
		Log::log("Product version:       %x.%x.%x\n", vers.m_major, vers.m_minor, vers.m_revision);
		vers = header.m_componentVersion;
		vers.fixByteOrder();
		Log::log("Component version:     %x.%x.%x\n", vers.m_major, vers.m_minor, vers.m_revision);
		if (header.m_majorVersion == 1 && header.m_minorVersion >= 1)
		{
			Log::log("Drive tag:             0x%04x\n", header.m_driveTag);
		}
		Log::log("SHA-1 digest of header:\n");
		logHexArray(Logger::INFO, (uint8_t *)&header.m_digest, sizeof(header.m_digest));
	}
	
	void dumpSectionHeader(const EncoreBootImage::section_header_t & header)
	{
		Log::SetOutputLevel infoLevel(Logger::INFO);
		Log::log("    Identifier: 0x%x\n", header.m_tag);
		Log::log("    Offset:     %d block%s (%d bytes)\n", header.m_offset, header.m_offset!=1?"s":"", sizeOfCipherBlocks(header.m_offset));
		Log::log("    Length:     %d block%s (%d bytes)\n", header.m_length, header.m_length!=1?"s":"", sizeOfCipherBlocks(header.m_length));
		Log::log("    Flags:      0x%08x\n", header.m_flags);
		
		if (header.m_flags & EncoreBootImage::ROM_SECTION_BOOTABLE)
		{
			Log::log("                0x1 = ROM_SECTION_BOOTABLE\n");
		}
		
		if (header.m_flags & EncoreBootImage::ROM_SECTION_CLEARTEXT)
		{
			Log::log("                0x2 = ROM_SECTION_CLEARTEXT\n");
		}
	}
	
	/*!
	 * \brief Log an array of bytes as hex.
	 */
	void logHexArray(Logger::log_level_t level, const uint8_t * bytes, unsigned count)
	{
		Log::SetOutputLevel leveler(level);

		unsigned i;
		for (i = 0; i < count; ++i, ++bytes)
		{
			if ((i % 16 == 0) && (i < count - 1))
			{
				if (i != 0)
				{
					Log::log("\n");
				}
				Log::log("    0x%08x: ", i);
			}
			Log::log("%02x ", *bytes & 0xff);
		}
		
		Log::log("\n");
	}

};

/*!
 * Main application entry point. Creates an sbtool instance and lets it take over.
 */
int main(int argc, char* argv[], char* envp[])
{
	try
	{
		return sbtool(argc, argv).run();
	}
	catch (...)
	{
		Log::log(Logger::ERROR, "error: unexpected exception\n");
		return 1;
	}

	return 0;
}





