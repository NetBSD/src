/*
 * File:	encryptgpk.cpp
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
#include "format_string.h"
#include "Blob.h"
#include "Random.h"
#include "rijndael.h"

using namespace elftosb;

//! Size in bytes of the unencrypted group private key.
#define GPK_LENGTH (40)

//! Size in bytes of the encrypted output data. This size must be modulo 16, the chunk size for the
//! AES-128 crypto algorithm. The group private key is inserted at offset 16.
#define OUTPUT_DATA_LENGTH (64)

//! Position in the output data of the first byte of the group private key.
#define OUTPUT_DATA_GPK_OFFSET (16)

//! The tool's name.
const char k_toolName[] = "encryptgpk";

//! Current version number for the tool.
const char k_version[] = "1.0.2";

//! Copyright string.
const char k_copyright[] = "Copyright (c) 2008 Freescale Semiconductor. All rights reserved.";

//! Default output array name.
const char k_defaultArrayName[] = "_endDisplay";

//! Definition of command line options.
static const char * k_optionsDefinition[] = {
	"?|help",
	"v|version",
	"k:key <file>",
	"z|zero-key",
	"o:output",
	"p:prefix",
	"a:array",
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
  -o/--output <file>           Write output to this file\n\
  -p/--prefix <prefix>         Set the output array prefix\n\
  -a/--array <name>            Specify the output array name\n\
  -d/--debug                   Enable debug output\n\
  -q/--quiet                   Output only warnings and errors\n\
  -V/--verbose                 Print extra detailed log information\n\n";

//! Init vector used for CBC encrypting the output data.
static const uint8_t kInitVector[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

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
class encryptgpk
{
protected:
	int m_argc;							//!< Number of command line arguments.
	char ** m_argv;						//!< String value for each command line argument.
	StdoutLogger * m_logger;			//!< Singleton logger instance.
	string_vector_t m_keyFilePaths;		//!< Paths to OTP key files.
	string_vector_t m_positionalArgs;	//!< Arguments coming after explicit options.
	bool m_isVerbose;					//!< Whether the verbose flag was turned on.
	bool m_useDefaultKey;					//!< Include a default (zero) crypto key.
	std::string m_outputPath;			//!< Path to output file.
	std::string m_gpkPath;				//!< Path to input group private key file.
	std::string m_outputPrefix;			//!< Prefix to the output array.
	std::string m_arrayName;			//!< Output array's name.
	
public:
	/*!
	 * Constructor.
	 *
	 * Creates the singleton logger instance.
	 */
	encryptgpk(int argc, char * argv[])
	:	m_argc(argc),
		m_argv(argv),
		m_logger(0),
		m_keyFilePaths(),
		m_positionalArgs(),
		m_isVerbose(false),
		m_useDefaultKey(false),
		m_outputPath(),
		m_gpkPath(),
		m_outputPrefix(),
		m_arrayName(k_defaultArrayName)
	{
		// create logger instance
		m_logger = new StdoutLogger();
		m_logger->setFilterLevel(Logger::INFO);
		Log::setLogger(m_logger);
	}
	
	/*!
	 * Destructor.
	 */
	~encryptgpk()
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
				
				case 'o':
					m_outputPath = optarg;
					break;

				case 'p':
					m_outputPrefix = optarg;
					break;

				case 'a':
					m_arrayName = optarg;
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
		options.usage(std::cout, "gpk-file");
		printf(k_usageText, k_toolName);
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
				throw std::runtime_error("no input file path was provided");
			}

			// Make sure at least one key was specified.
			if (m_keyFilePaths.size() == 0 && m_useDefaultKey == false)
			{
				throw std::runtime_error("no crypto key was specified");
			}
			
			// Do the work.
			generateOutput();
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
	 * \brief Builds the output data blob, encrypts it, and writes it to the output file.
	 */
	void generateOutput()
	{
		// Create the output data blob and set it to the correct size.
		Blob data;
		data.setLength(OUTPUT_DATA_LENGTH);
		
		// Fill it with random values.
		RandomNumberGenerator rng;
		rng.generateBlock(data.getData(), OUTPUT_DATA_LENGTH);

		// Read the GPK and overlay it into the output data.
		// The first positional arg is the GPK file path.
		Blob gpk = readGPK(m_positionalArgs[0]);
		memcpy(data.getData() + OUTPUT_DATA_GPK_OFFSET, gpk.getData(), GPK_LENGTH);

		// This is the key object for our crypto key.
		AESKey<128> cryptoKey = readKeyFile();

		// Read the key file.
		// Encrypt the output data block.
		Rijndael cipher;
		cipher.init(Rijndael::CBC, Rijndael::Encrypt, cryptoKey, Rijndael::Key16Bytes, (uint8_t *)&kInitVector);
		cipher.blockEncrypt(data.getData(), OUTPUT_DATA_LENGTH * 8, data.getData());

		// Open the output file.
		std::ofstream outputStream(m_outputPath.c_str(), std::ios_base::out | std::ios_base::trunc);
		if (!outputStream.is_open())
		{
			throw std::runtime_error(format_string("could not open output file %s", m_outputPath.c_str()));
		}
		
		writeCArray(outputStream, data);
	}

	/*!
	 * \brief Reads the group private key binary data.
	 */
	Blob readGPK(std::string & path)
	{
		std::ifstream stream(path.c_str(), std::ios_base::in | std::ios_base::binary);
		if (!stream.is_open())
		{
			throw std::runtime_error("could not open group private key file");
		}

		Blob gpk;
		gpk.setLength(GPK_LENGTH);

		stream.read((char *)gpk.getData(), GPK_LENGTH);

		return gpk;
	}

	/*!
	 * \brief Returns a key object based on the user's specified key.
	 */
	AESKey<128> readKeyFile()
	{
		if (m_keyFilePaths.size() > 0)
		{
			// Open the key file.
			std::string & keyPath = m_keyFilePaths[0];
			std::ifstream keyStream(keyPath.c_str(), std::ios_base::in);
			if (!keyStream.is_open())
			{
				throw std::runtime_error(format_string("unable to read key file %s\n", keyPath.c_str()));
			}
			keyStream.seekg(0);
			
			// Read the first key in the file.
			AESKey<128> key(keyStream);
			return key;
		}

		// Otherwise, create a zero key and return it.
		AESKey<128> defaultKey;
		return defaultKey;

	}

	/*!
	 * \brief Writes the given data blob as an array in a C source file.
	 */
	void writeCArray(std::ofstream & stream, const Blob & data)
	{
		const uint8_t * dataPtr = data.getData();
		unsigned length = data.getLength();

		// Write first line.
		std::string text = format_string("%s%sunsigned char %s[%d] = {", m_outputPrefix.c_str(), m_outputPrefix.size() > 0 ? " " : "", m_arrayName.c_str(), length);
		stream.write(text.c_str(), text.size());
		
		// Write each word of the array.
		unsigned i = 0;
		while (i < length)
		{
			// Insert a comma at the end of the previous line unless this is the first word we're outputting.
			text = format_string("%s\n    0x%02x", i == 0 ? "" : ",", (*dataPtr++) & 0xff);
			stream.write(text.c_str(), text.size());

			i++;
		}

		// Write last line, terminating the array.
		text = "\n};\n\n";
		stream.write(text.c_str(), text.size());
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

};

/*!
 * Main application entry point. Creates an sbtool instance and lets it take over.
 */
int main(int argc, char* argv[], char* envp[])
{
	try
	{
		return encryptgpk(argc, argv).run();
	}
	catch (...)
	{
		Log::log(Logger::ERROR, "error: unexpected exception\n");
		return 1;
	}

	return 0;
}





