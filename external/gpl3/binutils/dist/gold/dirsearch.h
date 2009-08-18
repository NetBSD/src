// dirsearch.h -- directory searching for gold  -*- C++ -*-

// Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#ifndef GOLD_DIRSEARCH_H
#define GOLD_DIRSEARCH_H

#include <string>
#include <list>

#include "options.h"
#include "token.h"

namespace gold
{

class General_options;
class Workqueue;

// A simple interface to manage directories to be searched for
// libraries.

class Dirsearch
{
 public:
  Dirsearch()
    : directories_(NULL), token_(true)
  { }

  // Set the list of directories to search.
  void
  initialize(Workqueue*, const General_options::Dir_list*);

  // Search for a file, giving one or two names to search for (the
  // second one may be empty).  Return a full path name for the file,
  // or the empty string if it could not be found.  This may only be
  // called if the token is not blocked.  Set *IS_IN_SYSROOT if the
  // file was found in a directory which is in the sysroot.
  std::string
  find(const std::string&, const std::string& n2, bool *is_in_sysroot) const;

  // Return the blocker token which controls access.
  Task_token*
  token()
  { return &this->token_; }

 private:
  // We can not copy this class.
  Dirsearch(const Dirsearch&);
  Dirsearch& operator=(const Dirsearch&);

  // Directories to search.
  const General_options::Dir_list* directories_;
  // Blocker token to control access from tasks.
  Task_token token_;
};

} // End namespace gold.

#endif // !defined(GOLD_DIRSEARCH_H)
