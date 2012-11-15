/*
 * File:	SearchPath.h
 *
 * Copyright (c) Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
 */
#if !defined(_searchpath_h_)
#define _searchpath_h_

#include <string>
#include <list>

/*!
 * \brief Handles searching a list of paths for a file.
 */
class PathSearcher
{
public:
	//!
	enum _target_type
	{
		kFindFile,
		kFindDirectory
	};
	
	//!
	typedef enum _target_type target_type_t;
	
protected:
	//! Global search object singleton.
	static PathSearcher * s_searcher;
	
public:
	//! \brief Access global path searching object.
	static PathSearcher & getGlobalSearcher();
	
public:
	//! \brief Constructor.
	PathSearcher() {}
	
	//! \brief Add a new search path to the end of the list.
	void addSearchPath(std::string & path);
	
	//! \brief Attempts to locate a file by using the search paths.
	bool search(const std::string & base, target_type_t targetType, bool searchCwd, std::string & result);

protected:
	typedef std::list<std::string> string_list_t;	//!< Linked list of strings.
	string_list_t m_paths;	//!< Ordered list of paths to search.
	
	//! \brief Returns whether \a path is absolute.
	bool isAbsolute(const std::string & path);
	
	//! \brief Combines two paths into a single one.
	std::string joinPaths(const std::string & first, const std::string & second);
};

#endif // _searchpath_h_
