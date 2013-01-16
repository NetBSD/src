/*
 * File:	ExcludesListMatcher.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "ExcludesListMatcher.h"

using namespace elftosb;

ExcludesListMatcher::ExcludesListMatcher()
:	GlobMatcher("")
{
}

ExcludesListMatcher::~ExcludesListMatcher()
{
}

//! \param isInclude True if this pattern is an include, false if it is an exclude.
//! \param pattern String containing the glob pattern.
void ExcludesListMatcher::addPattern(bool isInclude, const std::string & pattern)
{
	glob_list_item_t item;
	item.m_isInclude = isInclude;
	item.m_glob = pattern;
	
	// add to end of list
	m_patterns.push_back(item);
}

//! If there are no entries in the match list, the match fails.
//!
//! \param testValue The string to match against the pattern list.
//! \retval true The \a testValue argument matches.
//! \retval false No match was made against the argument.
bool ExcludesListMatcher::match(const std::string & testValue)
{
	if (!m_patterns.size())
	{
		return false;
	}
	
	// Iterate over the match list. Includes act as an OR operator, while
	// excludes act as an AND operator.
	bool didMatch = false;
	bool isFirstItem = true;
	glob_list_t::iterator it = m_patterns.begin();
	for (; it != m_patterns.end(); ++it)
	{
		glob_list_item_t & item = *it;
		
		// if this pattern is an include and it doesn't match, or
		// if this pattern is an exclude and it does match, then the match fails
		bool didItemMatch = globMatch(testValue.c_str(), item.m_glob.c_str());
		
		if (item.m_isInclude)
		{
			// Include
			if (isFirstItem)
			{
				didMatch = didItemMatch;
			}
			else
			{
				didMatch = didMatch || didItemMatch;
			}
		}
		else
		{
			// Exclude
			if (isFirstItem)
			{
				didMatch = !didItemMatch;
			}
			else
			{
				didMatch = didMatch && !didItemMatch;
			}
		}
		
		isFirstItem = false;
	}
	
	return didMatch;
}

