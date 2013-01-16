/*
 * File:	GlobMatcher.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_GlobMatcher_h_)
#define _GlobMatcher_h_

#include "StringMatcher.h"

namespace elftosb
{

/*!
 * \brief This class uses glob pattern matching to match strings.
 *
 * Glob patterns:
 *	- *	matches zero or more characters
 *	- ?	matches any single character
 *	- [set]	matches any character in the set
 *	- [^set]	matches any character NOT in the set
 *		where a set is a group of characters or ranges. a range
 *		is written as two characters seperated with a hyphen: a-z denotes
 *		all characters between a to z inclusive.
 *	- [-set]	set matches a literal hypen and any character in the set
 *	- []set]	matches a literal close bracket and any character in the set
 *
 *	- char	matches itself except where char is '*' or '?' or '['
 *	- \\char	matches char, including any pattern character
 *
 * Examples:
 *	- a*c		ac abc abbc ...
 *	- a?c		acc abc aXc ...
 *	- a[a-z]c		aac abc acc ...
 *	- a[-a-z]c	a-c aac abc ...
 */
class GlobMatcher : public StringMatcher
{
public:
	//! \brief Constructor.
	GlobMatcher(const std::string & pattern)
	:	StringMatcher(), m_pattern(pattern)
	{
	}
	
	//! \brief Returns whether \a testValue matches the glob pattern.
	virtual bool match(const std::string & testValue);
	
protected:
	std::string m_pattern;	//!< The glob pattern to match against.
	
	//! \brief Glob implementation.
	bool globMatch(const char * str, const char * p);
};

}; // namespace elftosb

#endif // _GlobMatcher_h_
