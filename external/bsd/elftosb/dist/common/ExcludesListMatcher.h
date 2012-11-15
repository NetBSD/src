/*
 * File:	ExcludesListMatcher.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_ExcludesListMatcher_h_)
#define _ExcludesListMatcher_h_

#include "GlobMatcher.h"
#include <vector>
#include <string>

namespace elftosb
{

/*!
 * \brief Matches strings using a series of include and exclude glob patterns.
 *
 * This string matcher class uses a sequential, ordered list of glob patterns to
 * determine whether a string matches.  Attached to each pattern is an include/exclude
 * action. The patterns in the list effectively form a Boolean expression. Includes
 * act as an OR operator while excludes act as an AND NOT operator.
 *
 * Examples (plus prefix is include, minus prefix is exclude):
 * - +foo: foo
 * - -foo: !foo
 * - +foo, +bar: foo || bar
 * - +foo, -bar: foo && !bar
 * - +foo, -bar, +baz: foo && !bar || baz
 *
 * The only reason for inheriting from GlobMatcher is so we can access the protected
 * globMatch() method.
 */
class ExcludesListMatcher : public GlobMatcher
{
public:
	//! \brief Default constructor.
	ExcludesListMatcher();
	
	//! \brief Destructor.
	~ExcludesListMatcher();
	
	//! \name Pattern list
	//@{
	//! \brief Add one include or exclude pattern to the end of the match list.
	void addPattern(bool isInclude, const std::string & pattern);
	//@}
	
	//! \brief Performs a single string match test against testValue.
	virtual bool match(const std::string & testValue);

protected:
	//! \brief Information about one glob pattern entry in a match list.
	struct glob_list_item_t
	{
		bool m_isInclude;	//!< True if include, false if exclude.
		std::string m_glob;	//!< The glob pattern to match.
	};
	
	typedef std::vector<glob_list_item_t> glob_list_t;
	glob_list_t m_patterns;	//!< Ordered list of include and exclude patterns.
};

}; // namespace elftosb

#endif // _ExcludesListMatcher_h_
