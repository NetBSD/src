/*
 * File:	GlobMatcher.cpp
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */

#include "GlobMatcher.h"

#ifndef NEGATE
#define NEGATE	'^'			// std cset negation char
#endif

using namespace elftosb;

//! The glob pattern must match the \e entire test value argument in order
//! for the match to be considered successful. Thus, even if, for example,
//! the pattern matches all but the last character the result will be false.
//!
//! \retval true The test value does match the glob pattern.
//! \retval false The test value does not match the glob pattern.
bool GlobMatcher::match(const std::string & testValue)
{
	return globMatch(testValue.c_str(), m_pattern.c_str());
}

//! \note This glob implementation was originally written by ozan s. yigit in
//!		December 1994. This is public domain source code.
bool GlobMatcher::globMatch(const char *str, const char *p)
{
	int negate;
	int match;
	int c;

	while (*p) {
		if (!*str && *p != '*')
			return false;

		switch (c = *p++) {

		case '*':
			while (*p == '*')
				p++;

			if (!*p)
				return true;

			if (*p != '?' && *p != '[' && *p != '\\')
				while (*str && *p != *str)
					str++;

			while (*str) {
				if (globMatch(str, p))
					return true;
				str++;
			}
			return false;

		case '?':
			if (*str)
				break;
			return false;
		
		// set specification is inclusive, that is [a-z] is a, z and
		// everything in between. this means [z-a] may be interpreted
		// as a set that contains z, a and nothing in between.
		case '[':
			if (*p != NEGATE)
				negate = false;
			else {
				negate = true;
				p++;
			}

			match = false;

			while (!match && (c = *p++)) {
				if (!*p)
					return false;
				if (*p == '-') {	// c-c
					if (!*++p)
						return false;
					if (*p != ']') {
						if (*str == c || *str == *p ||
						    (*str > c && *str < *p))
							match = true;
					}
					else {		// c-]
						if (*str >= c)
							match = true;
						break;
					}
				}
				else {			// cc or c]
					if (c == *str)
						match = true;
					if (*p != ']') {
						if (*p == *str)
							match = true;
					}
					else
						break;
				}
			}

			if (negate == match)
				return false;
			// if there is a match, skip past the cset and continue on
			while (*p && *p != ']')
				p++;
			if (!*p++)	// oops!
				return false;
			break;

		case '\\':
			if (*p)
				c = *p++;
		default:
			if (c != *str)
				return false;
			break;

		}
		str++;
	}

	return !*str;
}

