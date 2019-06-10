/* Cache of styled source file text
   Copyright (C) 2018-2019 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef SOURCE_CACHE_H
#define SOURCE_CACHE_H

/* This caches highlighted source text, keyed by the source file's
   full name.  A size-limited LRU cache is used.

   Highlighting depends on the GNU Source Highlight library.  When not
   available, this cache will fall back on reading plain text from the
   appropriate file.  */
class source_cache
{
public:

  source_cache ()
  {
  }

  /* Get the source text for the source file in symtab S.  FIRST_LINE
     and LAST_LINE are the first and last lines to return; line
     numbers are 1-based.  If the file cannot be read, false is
     returned.  Otherwise, LINES_OUT is set to the desired text.  The
     returned text may include ANSI terminal escapes.  */
  bool get_source_lines (struct symtab *s, int first_line,
			 int last_line, std::string *lines_out);

  /* Remove all the items from the source cache.  */
  void clear ()
  {
    m_source_map.clear ();
  }

private:

  /* One element in the cache.  */
  struct source_text
  {
    /* The full name of the file.  */
    std::string fullname;
    /* The contents of the file.  */
    std::string contents;
  };

  /* A helper function for get_source_lines that is used when the
     source lines are not highlighted.  The arguments and return value
     are as for get_source_lines.  */
  bool get_plain_source_lines (struct symtab *s, int first_line,
			       int last_line, std::string *lines_out);
  /* A helper function for get_plain_source_lines that extracts the
     desired source lines from TEXT, putting them into LINES_OUT.  The
     arguments are as for get_source_lines.  The return value is the
     desired lines.  */
  std::string extract_lines (const struct source_text &text, int first_line,
			     int last_line);

  /* The contents of the cache.  */
  std::vector<source_text> m_source_map;
};

/* The global source cache.  */
extern source_cache g_source_cache;

#endif /* SOURCE_CACHE_H */
