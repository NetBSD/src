/*
 * File:	keygen.cpp
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
#include <string>
#include <vector>
#include "options.h"
#include "smart_ptr.h"
#include "Logging.h"
#include "AESKey.h"

//! The tool's name.
const char k_toolName[] = "keygen";

//! Current version number for the tool.
const char k_version[] = "1.0";

//! Copyright string.
const char k_copyright[] = "Copyright (c) 2006-2009 Freescale Semiconductor, Inc.\nAll rights reserved.";

//! Definition of command line options.
static const char * k_optionsDefinition[] = {
	"?|help",
	"v|version",
	"q|quiet",
	"V|verbose",
	"n:number <int>",
	NULL
};

//! Help string.
const char k_usageText[] = "\nOptions:\n\
  -?/--help                    Show this help\n\
  -v/--version                 Display tool version\n\
  -q/--quiet                   Output only warnings and errors\n\
  -V/--verbose                 Print extra detailed log information\n\
  -n/--number <int>            Number of keys to generate per file (default=1)\n\n";

//! An array of strings.
typedef std::vector<std::string> string_vector_t;

// prototypes
int main(int argc, char* argv[], char* envp[]);

/*!
 * \brief Class that encapsulates the keygen interface.
 *
 * A single global logger instance is created during object construction. It is
 * never freed because we need it up to the last possible minute, when an
 * exception could be thrown.
 */
class keygen
{
protected:
	int m_argc;							//!< Number of command line arguments.
	char ** m_argv;						//!< String value for each command line argument.
	StdoutLogger * m_logger;			//!< Singleton logger instance.
	string_vector_t m_positionalArgs;	//!< Arguments coming after explicit options.
	bool m_isVerbose;					//!< Whether the verbose flag was turned on.
	int m_keyCount;						//!< Number of keys to generate.
	
public:
	/*!
	 * Constructor.
	 *
	 * Creates the singleton logger instance.
	 */
	keygen(int argc, char * argv[])
	:	m_argc(argc),
		m_argv(argv),
		m_logger(0),
		m_positionalArgs(),
		m_isVerbose(false),
		m_keyCount(1)
	{
		// create logger instance
		m_logger = new StdoutLogger();
		m_logger->setFilterLevel(Logger::INFO);
		Log::setLogger(m_logger);
	}
	
	/*!
	 * Destructor.
	 */
	~keygen()
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
					
				case 'd':
					Log::getLogger()->setFilterLevel(Logger::DEBUG);
					break;
					
				case 'q':
					Log::getLogger()->setFilterLevel(Logger::WARNING);
					break;
					
				case 'V':
					m_isVerbose = true;
					break;
				
				case 'n':
					m_keyCount = strtol(optarg, NULL, 0);
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
		options.usage(std::cout, "key-files...");
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
				throw std::runtime_error("no output file path was provided");
			}
			
			// generate key files
			string_vector_t::const_iterator it = m_positionalArgs.begin();
			for (; it != m_positionalArgs.end(); ++it)
			{
				generateKeyFile(*it);
			}
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
	 * \brief Opens the file at \a path and writes a random key file.
	 *
	 * Each key file will have #m_keyCount number of keys written into it,
	 * each on a line by itself.
	 */
	void generateKeyFile(const std::string & path)
	{
		std::ofstream outputStream(path.c_str(), std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		if (outputStream.is_open())
		{
			int i;
			for (i = 0; i < m_keyCount; ++i)
			{
				AESKey<128> key;
				key.randomize();
				key.writeToStream(outputStream);
				
				// put a newline after the key
				outputStream.write("\n", 1);
				
				// dump it
				dumpKey(key);
			}
			
			Log::log(Logger::INFO, "wrote key file %s\n", path.c_str());
		}
		else
		{
			throw std::runtime_error("could not open output file");
		}
	}
	
	/*!
	 * \brief Write the value of each byte of the \a key to the log.
	 */
	void dumpKey(const AESKey<128> & key)
	{
		// dump key bytes
		Log::log(Logger::INFO2, "key bytes: ");
		AESKey<128>::key_t the_key;
		key.getKey(&the_key);
		int q;
		for (q=0; q<16; q++)
		{
			Log::log(Logger::INFO2, "%02x ", the_key[q]);
		}
		Log::log(Logger::INFO2, "\n");
	}
	
	/*!
	 * \brief Log an array of bytes as hex.
	 */
	void logHexArray(Logger::log_level_t level, const uint8_t * bytes, unsigned count)
	{
		Log::SetOutputLevel leveler(level);
//		Log::log("    ");
		unsigned i;
		for (i = 0; i < count; ++i, ++bytes)
		{
			if ((i % 16 == 0) && (i < count - 1))
			{
				if (i != 0)
				{
					Log::log("\n");
				}
				Log::log("    0x%04x: ", i);
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
		return keygen(argc, argv).run();
	}
	catch (...)
	{
		Log::log(Logger::ERROR, "error: unexpected exception\n");
		return 1;
	}

	return 0;
}





