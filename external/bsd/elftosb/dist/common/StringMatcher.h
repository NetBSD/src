/*
 * File:	GlobSectionSelector.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_StringMatcher_h_)
#define _StringMatcher_h_

#include <string>

namespace elftosb
{

/*!
 * \brief Abstract interface class used to select strings by name.
 */
class StringMatcher
{
public:
	//! \brief Performs a single string match test against testValue.
	//!
	//! \retval true The \a testValue argument matches.
	//! \retval false No match was made against the argument.
	virtual bool match(const std::string & testValue)=0;
};

/*!
 * \brief String matcher subclass that matches all test strings.
 */
class WildcardMatcher : public StringMatcher
{
public:
	//! \brief Always returns true, indicating a positive match.
	virtual bool match(const std::string & testValue) { return true; }
};

/*!
 * \brief Simple string matcher that compares against a fixed value.
 */
class FixedMatcher : public StringMatcher
{
public:
	//! \brief Constructor. Sets the string to compare against to be \a fixedValue.
	FixedMatcher(const std::string & fixedValue) : m_value(fixedValue) {}
	
	//! \brief Returns whether \a testValue is the same as the value passed to the constructor.
	//!
	//! \retval true The \a testValue argument matches the fixed compare value.
	//! \retval false The argument is not the same as the compare value.
	virtual bool match(const std::string & testValue)
	{
		return testValue == m_value;
	}

protected:
	const std::string & m_value;	//!< The section name to look for.
};

}; // namespace elftosb

#endif // _StringMatcher_h_
