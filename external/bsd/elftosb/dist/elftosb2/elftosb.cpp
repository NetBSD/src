/*
 * File:	elftosb.cpp
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
#include "ConversionController.h"
#include "options.h"
#include "Version.h"
#include "EncoreBootImage.h"
#include "smart_ptr.h"
#include "Logging.h"
#include "EncoreBootImageGenerator.h"
#include "SearchPath.h"
#include "format_string.h"

//! An array of strings.
typedef std::vector<std::string> string_vector_t;

//! The tool's name.
const char k_toolName[] = "elftosb";

//! Current version number for the tool.
const char k_version[] = "2.6.1";

//! Copyright string.
const char k_copyright[] = "Copyright (c) 2004-2010 Freescale Semiconductor, Inc.\nAll rights reserved.";

static const char * k_optionsDefinition[] = {
	"?|help",
	"v|version",
	"f:chip-family <family>",
	"c:command <file>",
	"o:output <file>",
	"P:product <version>",
	"C:component <version>",
	"k:key <file>",
	"z|zero-key",
	"D:define <const>",
	"O:option <option>",
	"d|debug",
	"q|quiet",
	"V|verbose",
	"p:search-path <path>",
	NULL
};

//! Help string.
const char k_usageText[] = "\nOptions:\n\
  -?/--help                    Show this help\n\
  -v/--version                 Display tool version\n\
  -f/--chip-family <family>    Select the chip family (default is 37xx)\n\
  -c/--command <file>          Use this command file\n\
  -o/--output <file>           Write output to this file\n\
  -p/--search-path <path>      Add a search path used to find input files\n\
  -P/--product <version        Set product version\n\
  -C/--component <version>     Set component version\n\
  -k/--key <file>              Add OTP key, enable encryption\n\
  -z/--zero-key                Add default key of all zeroes\n\
  -D/--define <const>=<int>    Define or override a constant value\n\
  -O/--option <name>=<value>   Set or override a processing option\n\
  -d/--debug                   Enable debug output\n\
  -q/--quiet                   Output only warnings and errors\n\
  -V/--verbose                 Print extra detailed log information\n\n";

// prototypes
int main(int argc, char* argv[], char* envp[]);

/*!
 * \brief Class that encapsulates the elftosb tool.
 *
 * A single global logger instance is created during object construction. It is
 * never freed because we need it up to the last possible minute, when an
 * exception could be thrown.
 */
class elftosbTool
{
protected:
	//! Supported chip families.
	enum chip_family_t
	{
		k37xxFamily,	//!< 37xx series.
		kMX28Family,	//!< Catskills series.
	};
	
	/*!
	 * \brief A structure describing an entry in the table of chip family names.
	 */
	struct FamilyNameTableEntry
	{
		const char * const name;
		chip_family_t family;
	};
	
	//! \brief Table that maps from family name strings to chip family constants.
	static const FamilyNameTableEntry kFamilyNameTable[];
	
	int m_argc;							//!< Number of command line arguments.
	char ** m_argv;						//!< String value for each command line argument.
	StdoutLogger * m_logger;			//!< Singleton logger instance.
	string_vector_t m_keyFilePaths;		//!< Paths to OTP key files.
	string_vector_t m_positionalArgs;	//!< Arguments coming after explicit options.
	bool m_isVerbose;					//!< Whether the verbose flag was turned on.
	bool m_useDefaultKey;					//!< Include a default (zero) crypto key.
	const char * m_commandFilePath;		//!< Path to the elftosb command file.
	const char * m_outputFilePath;		//!< Path to the output .sb file.
	const char * m_searchPath;			//!< Optional search path for input files.
	elftosb::version_t m_productVersion;	//!< Product version specified on command line.
	elftosb::version_t m_componentVersion;	//!< Component version specified on command line.
	bool m_productVersionSpecified;		//!< True if the product version was specified on the command line.
	bool m_componentVersionSpecified;		//!< True if the component version was specified on the command line.
	chip_family_t m_family;				//!< Chip family that the output file is formatted for.
	elftosb::ConversionController m_controller;	//!< Our conversion controller instance.
		
public:
	/*!
	 * Constructor.
	 *
	 * Creates the singleton logger instance.
	 */
	elftosbTool(int argc, char * argv[])
	:	m_argc(argc),
		m_argv(argv),
		m_logger(0),
		m_keyFilePaths(),
		m_positionalArgs(),
		m_isVerbose(false),
		m_useDefaultKey(false),
		m_commandFilePath(NULL),
		m_outputFilePath(NULL),
		m_searchPath(NULL),
		m_productVersion(),
		m_componentVersion(),
		m_productVersionSpecified(false),
		m_componentVersionSpecified(false),
		m_family(k37xxFamily),
		m_controller()
	{
		// create logger instance
		m_logger = new StdoutLogger();
		m_logger->setFilterLevel(Logger::INFO);
		Log::setLogger(m_logger);
	}
	
	/*!
	 * Destructor.
	 */
	~elftosbTool()
	{
	}
	
	/*!
	 * \brief Searches the family name table.
	 *
	 * \retval true The \a name was found in the table, and \a family is valid.
	 * \retval false No matching family name was found. The \a family argument is not modified.
	 */
	bool lookupFamilyName(const char * name, chip_family_t * family)
	{
		// Create a local read-write copy of the argument string.
		std::string familyName(name);
		
		// Convert the argument string to lower case for case-insensitive comparison.
		for (int n=0; n < familyName.length(); n++)
		{
			familyName[n] = tolower(familyName[n]);
		}
		
        // Exit the loop if we hit the NULL terminator entry.
		const FamilyNameTableEntry * entry = &kFamilyNameTable[0];
		for (; entry->name; entry++)
		{
			// Compare lowercased name with the table entry.
			if (familyName == entry->name)
			{
				*family = entry->family;
				return true;
			}
		}
		
		// Failed to find a matching name.
		return false;
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
				
				case 'f':
					if (!lookupFamilyName(optarg, &m_family))
					{
						Log::log(Logger::ERROR, "error: unknown chip family '%s'\n", optarg);
						printUsage(options);
						return 0;
					}
					break;
					
				case 'c':
					m_commandFilePath = optarg;
					break;
					
				case 'o':
					m_outputFilePath = optarg;
					break;
					
				case 'P':
					m_productVersion.set(optarg);
					m_productVersionSpecified = true;
					break;
					
				case 'C':
					m_componentVersion.set(optarg);
					m_componentVersionSpecified = true;
					break;
					
				case 'k':
					m_keyFilePaths.push_back(optarg);
					break;
				
				case 'z':
					m_useDefaultKey = true;
					break;
					
				case 'D':
					overrideVariable(optarg);
					break;

				case 'O':
					overrideOption(optarg);
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
				
				case 'p':
				{
					std::string newSearchPath(optarg);
					PathSearcher::getGlobalSearcher().addSearchPath(newSearchPath);
					break;
				}
					
				default:
					Log::log(Logger::ERROR, "error: unrecognized option\n\n");
					printUsage(options);
					return 0;
			}
		}
		
		// handle positional args
		if (iter.index() < m_argc)
		{
			Log::SetOutputLevel leveler(Logger::DEBUG);
			Log::log("positional args:\n");
			int i;
			for (i = iter.index(); i < m_argc; ++i)
			{
				Log::log("%d: %s\n", i - iter.index(), m_argv[i]);
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
		options.usage(std::cout, "files...");
		printf(k_usageText, k_toolName);
	}
	
	/*!
	 * \brief Core of the tool.
	 *
	 * Calls processOptions() to handle command line options before performing the
	 * real work the tool does.
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
			
			// check argument values
			checkArguments();

			// set up the controller
			m_controller.setCommandFilePath(m_commandFilePath);
			
			// add external paths to controller
			string_vector_t::iterator it = m_positionalArgs.begin();
			for (; it != m_positionalArgs.end(); ++it)
			{
				m_controller.addExternalFilePath(*it);
			}
			
			// run conversion
			convert();
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
	 * \brief Validate arguments that can be checked.
	 * \exception std::runtime_error Thrown if an argument value fails to pass validation.
	 */
	void checkArguments()
	{
		if (m_commandFilePath == NULL)
		{
			throw std::runtime_error("no command file was specified");
		}
		if (m_outputFilePath == NULL)
		{
			throw std::runtime_error("no output file was specified");
		}
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
	 * \brief Returns the integer value for a string.
	 *
	 * Metric multiplier prefixes are supported.
	 */
	uint32_t parseIntValue(const char * value)
	{
		// Accept 'true'/'yes' and 'false'/'no' as integer values.
		if ((strcmp(value, "true") == 0) || (strcmp(value, "yes") == 0))
		{
			return 1;
		}
		else if ((strcmp(value, "false") == 0) || (strcmp(value, "no") == 0))
		{
			return 0;
		}
		
		uint32_t intValue = strtoul(value, NULL, 0);
		unsigned multiplier;
		switch (value[strlen(value) - 1])
		{
			case 'G':
				multiplier = 1024 * 1024 * 1024;
				break;
			case 'M':
				multiplier = 1024 * 1024;
				break;
			case 'K':
				multiplier = 1024;
				break;
			default:
				multiplier = 1;
		}
		intValue *= multiplier;
		return intValue;
	}
	
	/*!
	 * \brief Parses the -D option to override a constant value.
	 */
	void overrideVariable(const char * optarg)
	{
		// split optarg into two strings
		std::string constName(optarg);
		int i;
		for (i=0; i < strlen(optarg); ++i)
		{
			if (optarg[i] == '=')
			{
				constName.resize(i++);
				break;
			}
		}
		
		uint32_t constValue = parseIntValue(&optarg[i]);
		
		elftosb::EvalContext & context = m_controller.getEvalContext();
		context.setVariable(constName, constValue);
		context.lockVariable(constName);
	}

	/*!
	 * \brief
	 */
	void overrideOption(const char * optarg)
	{
		// split optarg into two strings
		std::string optionName(optarg);
		int i;
		for (i=0; i < strlen(optarg); ++i)
		{
			if (optarg[i] == '=')
			{
				optionName.resize(i++);
				break;
			}
		}
		
		// handle quotes for option value
		const char * valuePtr = &optarg[i];
		bool isString = false;
		int len;
		if (valuePtr[0] == '"')
		{
			// remember that the value is a string and get rid of the opening quote
			isString = true;
			valuePtr++;

			// remove trailing quote if present
			len = strlen(valuePtr);
			if (valuePtr[len] == '"')
			{
				len--;
			}
		}

		elftosb::Value * value;
		if (isString)
		{
			std::string stringValue(valuePtr);
			stringValue.resize(len);	// remove trailing quote
			value = new elftosb::StringValue(stringValue);
		}
		else
		{
			value = new elftosb::IntegerValue(parseIntValue(valuePtr));
		}

		// Set and lock the option in the controller
		m_controller.setOption(optionName, value);
		m_controller.lockOption(optionName);
	}
	
	/*!
	 * \brief Do the conversion.
	 * \exception std::runtime_error This exception is thrown if the conversion controller does
	 *		not produce a boot image, or if the output file cannot be opened. Other errors
	 *		internal to the conversion controller may also produce this exception.
	 */
	void convert()
	{
		// create a generator for the chosen chip family
		smart_ptr<elftosb::BootImageGenerator> generator;
		switch (m_family)
		{
			case k37xxFamily:
				generator = new elftosb::EncoreBootImageGenerator;
				elftosb::g_enableHABSupport = false;
				break;

			case kMX28Family:
				generator = new elftosb::EncoreBootImageGenerator;
				elftosb::g_enableHABSupport = true;
				break;
		}
		
		// process input and get a boot image
		m_controller.run();
		smart_ptr<elftosb::BootImage> image = m_controller.generateOutput(generator);
		if (!image)
		{
			throw std::runtime_error("failed to produce output!");
		}
		
		// set version numbers if they were provided on the command line
		if (m_productVersionSpecified)
		{
			image->setProductVersion(m_productVersion);
		}
		if (m_componentVersionSpecified)
		{
			image->setComponentVersion(m_componentVersion);
		}
		
		// special handling for each family
		switch (m_family)
		{
			case k37xxFamily:
			case kMX28Family:
			{
				// add OTP keys
				elftosb::EncoreBootImage * encoreImage = dynamic_cast<elftosb::EncoreBootImage*>(image.get());
				if (encoreImage)
				{
					// add keys
					addCryptoKeys(encoreImage);
					
					// print debug image
					encoreImage->debugPrint();
				}
				break;
			}
		}
		
		// write output
		std::ofstream outputStream(m_outputFilePath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		if (outputStream.is_open())
		{
			image->writeToStream(outputStream);
		}
		else
		{
			throw std::runtime_error(format_string("could not open output file %s", m_outputFilePath));
		}
	}
	
	/*!
	 * \brief
	 */
	void addCryptoKeys(elftosb::EncoreBootImage * encoreImage)
	{
		string_vector_t::iterator it = m_keyFilePaths.begin();
		for (; it != m_keyFilePaths.end(); ++it)
		{
			std::string & keyPath = *it;
			
			std::string actualPath;
			bool found = PathSearcher::getGlobalSearcher().search(keyPath, PathSearcher::kFindFile, true, actualPath);
			if (!found)
			{
				throw std::runtime_error(format_string("unable to find key file %s\n", keyPath.c_str()));
			}
			
			std::ifstream keyStream(actualPath.c_str(), std::ios_base::in);
			if (!keyStream.is_open())
			{
				throw std::runtime_error(format_string("unable to read key file %s\n", keyPath.c_str()));
			}
			keyStream.seekg(0);
			
			try
			{
				// read as many keys as possible from the stream
				while (true)
				{
					AESKey<128> key(keyStream);
					encoreImage->addKey(key);
					
					// dump key bytes
					dumpKey(key);
				}
			}
			catch (...)
			{
				// ignore the exception -- there are just no more keys in the stream
			}
		}
		
		// add the default key of all zero bytes if requested
		if (m_useDefaultKey)
		{
			AESKey<128> defaultKey;
			encoreImage->addKey(defaultKey);
		}
	}
	
	/*!
	 * \brief Write the value of each byte of the \a key to the log.
	 */
	void dumpKey(const AESKey<128> & key)
	{
		// dump key bytes
		Log::log(Logger::DEBUG, "key bytes: ");
		AESKey<128>::key_t the_key;
		key.getKey(&the_key);
		int q;
		for (q=0; q<16; q++)
		{
			Log::log(Logger::DEBUG, "%02x ", the_key[q]);
		}
		Log::log(Logger::DEBUG, "\n");
	}

};

const elftosbTool::FamilyNameTableEntry elftosbTool::kFamilyNameTable[] =
	{
		{ "37xx", k37xxFamily },
		{ "377x", k37xxFamily },
		{ "378x", k37xxFamily },
		{ "mx23", k37xxFamily },
		{ "imx23", k37xxFamily },
		{ "i.mx23", k37xxFamily },
		{ "mx28", kMX28Family },
		{ "imx28", kMX28Family },
		{ "i.mx28", kMX28Family },
		
		// Null terminator entry.
		{ NULL, k37xxFamily }
	};

/*!
 * Main application entry point. Creates an sbtool instance and lets it take over.
 */
int main(int argc, char* argv[], char* envp[])
{
	try
	{
		return elftosbTool(argc, argv).run();
	}
	catch (...)
	{
		Log::log(Logger::ERROR, "error: unexpected exception\n");
		return 1;
	}

	return 0;
}



