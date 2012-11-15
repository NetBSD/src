/*
 * File:	Logging.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_Logging_h_)
#define _Logging_h_

#include <string>
#include <assert.h>
#include <stdarg.h>

/*!
 * \brief Base logger class.
 *
 * There are two types of logging levels that are used by this class. First
 * there is the filter level. Any log message that is assigned a level
 * higher than the current filter level is discarded. Secondly there is the
 * current output level. Log messages that do not have their own level
 * use the current output level to determine if they should be shown or
 * not.
 *
 * The two methods setFilterLevel() and setOutputLevel() set the filter
 * and default output logging levels, respectively. There are corresponding
 * getter methods as well. Both the filter and output levels are
 * initialized to #INFO during object construction.
 *
 * Most use of the logger classes is expected to be through the Log
 * class. It provides static logging methods that call through to a global
 * singleton logger instance. There is also a Log::SetOutputLevel utility
 * class that makes it extremely easiy to temporarily change the default
 * output logging level.
 *
 * Of all the overloaded log() methods in this class, none of them are
 * really expected to be reimplemented by subclasses. Instead, there is
 * the single protected _log() method that takes a simple string pointer.
 * The other log methods all wind up calling _log(), so it provides a
 * single point to override. In fact, _log() is pure virtual, so subclasses
 * must implement it.
 *
 * \see Log
 */
class Logger
{
public:
	//! \brief Logging levels.
	enum log_level_t
	{
		URGENT = 0,	//!< The lowest level, for messages that must always be logged.
		ERROR,		//!< For fatal error messages.
		WARNING,	//!< For non-fatal warning messages.
		INFO,		//!< The normal log level, for status messages.
		INFO2,		//!< For verbose status messages.
		DEBUG,		//!< For internal reporting.
		DEBUG2		//!< Highest log level; verbose debug logging.
	};
	
public:
	//! \brief Default constructor.
	Logger() : m_filter(INFO), m_level(INFO) {}
	
	//! \brief Destructor.
	virtual ~Logger() {}
	
	//! \name Logging levels
	//@{
	//! \brief Changes the logging level to \a level.
	inline void setFilterLevel(log_level_t level) { m_filter = level; }
	
	//! \brief Returns the current logging filter level.
	inline log_level_t getFilterLevel() const { return m_filter; }
	
	//! \brief Changes the logging output level to \a level.
	inline void setOutputLevel(log_level_t level) { m_level = level; }
	
	//! \brief Returns the current logging output level.
	inline log_level_t getOutputLevel() const { return m_level; }
	//@}
	
	//! \name Logging
	//@{
	//! \brief Log with format.
	virtual void log(const char * fmt, ...);
	
	//! \brief Log a string object.
	virtual void log(const std::string & msg) { log(msg.c_str()); }
	
	//! \brief Log with format at a specific output level.
	virtual void log(log_level_t level, const char * fmt, ...);
	
	//! \brief Log a string output at a specific output level.
	virtual void log(log_level_t level, const std::string & msg) { log(level, msg.c_str()); }
	
	//! \brief Log with format using an argument list.
	virtual void log(const char * fmt, va_list args);
	
	//! \brief Log with format using an argument with a specific output level.
	virtual void log(log_level_t level, const char * fmt, va_list args);
	//@}
		
protected:
	log_level_t m_filter;	//!< The current logging filter level.
	log_level_t m_level;	//!< The current log output level.
	
protected:
	//! \brief The base pure virtual logging function implemented by subclasses.
	virtual void _log(const char * msg)=0;
};

/*!
 * \brief Wraps a set of static functions for easy global logging access.
 *
 * This class has a set of static methods that make it easy to access a global
 * logger instance without having to worry about extern symbols. It does this
 * by keeping a static member variable pointing at the singleton logger instance,
 * which is set with the setLogger() static method.
 *
 * There is also an inner utility class called SetOutputLevel that uses
 * C++ scoping rules to temporarily change the output logging level. When the
 * SetOutputLevel instance falls out of scope the output level is restored
 * to the previous value.
 */
class Log
{
public:
	//! \name Singleton logger access
	//@{
	//! \brief Returns the current global logger singleton.
	static inline Logger * getLogger() { return s_logger; }
	
	//! \brief Sets the global logger singleton instance.
	static inline void setLogger(Logger * logger) { s_logger = logger; }
	//@}
	
	//! \name Logging
	//@{
	//! \brief Log with format.
	static void log(const char * fmt, ...);
	
	//! \brief Log a string object.
	static void log(const std::string & msg);
	
	//! \brief Log with format at a specific output level.
	static void log(Logger::log_level_t level, const char * fmt, ...);
	
	//! \brief Log a string output at a specific output level.
	static void log(Logger::log_level_t level, const std::string & msg);
	//@}
	
protected:
	static Logger * s_logger;	//!< The single global logger instance.
	
public:
	/*!
	 * \brief Utility class to temporarily change the logging output level.
	 *
	 * This class will change the current logging output level of a given
	 * logger instance. Then when it falls out of scope it will set the
	 * level back to what it was originally.
	 *
	 * Use like this:
	 * \code
	 *		// output level is some value here
	 *		{
	 *			Log::SetOutputLevel leveler(Logger::DEBUG);
	 *			// now output level is DEBUG
	 *			Log::log("my debug message 1");
	 *			Log::log("my debug message 2");
	 *		}
	 *		// output level is restored to previous value
	 * \endcode
	 */
	class SetOutputLevel
	{
	public:
		//! \brief Default constructor.
		//!
		//! Saves the current logging output level of the global logger,
		//! as managed by the Log class, and sets the new level to \a level.
		SetOutputLevel(Logger::log_level_t level)
		:	m_logger(Log::getLogger()), m_saved(Logger::INFO)
		{
			assert(m_logger);
			m_saved = m_logger->getOutputLevel();
			m_logger->setOutputLevel(level);
		}
		
		//! \brief Constructor.
		//!
		//! Saves the current logging output level of \a logger and sets
		//! the new level to \a level.
		SetOutputLevel(Logger * logger, Logger::log_level_t level)
		:	m_logger(logger), m_saved(logger->getOutputLevel())
		{
			assert(m_logger);
			m_logger->setOutputLevel(level);
		}
		
		//! \brief Destructor.
		//!
		//! Restores the saved logging output level.
		~SetOutputLevel()
		{
			m_logger->setOutputLevel(m_saved);
		}
		
	protected:
		Logger * m_logger;	//!< The logger instance we're controlling.
		Logger::log_level_t m_saved;	//!< Original logging output level.
	};

};


/*!
 * \brief Simple logger that writes to stdout.
 */
class StdoutLogger : public Logger
{
protected:
	//! \brief Logs the message to stdout.
	virtual void _log(const char * msg);
};

#endif // _Logging_h_
