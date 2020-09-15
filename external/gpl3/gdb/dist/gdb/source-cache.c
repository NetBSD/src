/* Cache of styled source file text
   Copyright (C) 2018-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "source-cache.h"
#include "gdbsupport/scoped_fd.h"
#include "source.h"
#include "cli/cli-style.h"
#include "symtab.h"
#include "gdbsupport/selftest.h"
#include "objfiles.h"
#include "exec.h"

#ifdef HAVE_SOURCE_HIGHLIGHT
/* If Gnulib redirects 'open' and 'close' to its replacements
   'rpl_open' and 'rpl_close' via cpp macros, including <fstream>
   below with those macros in effect will cause unresolved externals
   when GDB is linked.  Happens, e.g., in the MinGW build.  */
#undef open
#undef close
#include <sstream>
#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>
#endif

/* The number of source files we'll cache.  */

#define MAX_ENTRIES 5

/* See source-cache.h.  */

source_cache g_source_cache;

/* See source-cache.h.  */

std::string
source_cache::get_plain_source_lines (struct symtab *s,
				      const std::string &fullname)
{
  scoped_fd desc (open_source_file (s));
  if (desc.get () < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  struct stat st;
  if (fstat (desc.get (), &st) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  std::string lines;
  lines.resize (st.st_size);
  if (myread (desc.get (), &lines[0], lines.size ()) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  time_t mtime = 0;
  if (SYMTAB_OBJFILE (s) != NULL && SYMTAB_OBJFILE (s)->obfd != NULL)
    mtime = SYMTAB_OBJFILE (s)->mtime;
  else if (exec_bfd)
    mtime = exec_bfd_mtime;

  if (mtime && mtime < st.st_mtime)
    warning (_("Source file is more recent than executable."));

  std::vector<off_t> offsets;
  offsets.push_back (0);
  for (size_t offset = lines.find ('\n');
       offset != std::string::npos;
       offset = lines.find ('\n', offset))
    {
      ++offset;
      /* A newline at the end does not start a new line.  It would
	 seem simpler to just strip the newline in this function, but
	 then "list" won't print the final newline.  */
      if (offset != lines.size ())
	offsets.push_back (offset);
    }

  offsets.shrink_to_fit ();
  m_offset_cache.emplace (fullname, std::move (offsets));

  return lines;
}

#ifdef HAVE_SOURCE_HIGHLIGHT

/* Return the Source Highlight language name, given a gdb language
   LANG.  Returns NULL if the language is not known.  */

static const char *
get_language_name (enum language lang)
{
  switch (lang)
    {
    case language_c:
    case language_objc:
      return "c.lang";

    case language_cplus:
      return "cpp.lang";

    case language_d:
      return "d.lang";

    case language_go:
      return "go.lang";

    case language_fortran:
      return "fortran.lang";

    case language_m2:
      /* Not handled by Source Highlight.  */
      break;

    case language_asm:
      return "asm.lang";

    case language_pascal:
      return "pascal.lang";

    case language_opencl:
      /* Not handled by Source Highlight.  */
      break;

    case language_rust:
      return "rust.lang";

    case language_ada:
      return "ada.lang";

    default:
      break;
    }

  return nullptr;
}

#endif /* HAVE_SOURCE_HIGHLIGHT */

/* See source-cache.h.  */

bool
source_cache::ensure (struct symtab *s)
{
  std::string fullname = symtab_to_fullname (s);

  size_t size = m_source_map.size ();
  for (int i = 0; i < size; ++i)
    {
      if (m_source_map[i].fullname == fullname)
	{
	  /* This should always hold, because we create the file
	     offsets when reading the file, and never free them
	     without also clearing the contents cache.  */
	  gdb_assert (m_offset_cache.find (fullname)
		      != m_offset_cache.end ());
	  /* Not strictly LRU, but at least ensure that the most
	     recently used entry is always the last candidate for
	     deletion.  Note that this property is relied upon by at
	     least one caller.  */
	  if (i != size - 1)
	    std::swap (m_source_map[i], m_source_map[size - 1]);
	  return true;
	}
    }

  std::string contents;
  try
    {
      contents = get_plain_source_lines (s, fullname);
    }
  catch (const gdb_exception_error &e)
    {
      /* If 's' is not found, an exception is thrown.  */
      return false;
    }

  if (source_styling && gdb_stdout->can_emit_style_escape ())
    {
#ifdef HAVE_SOURCE_HIGHLIGHT
      bool already_styled = false;
      const char *lang_name = get_language_name (SYMTAB_LANGUAGE (s));
      if (lang_name != nullptr)
	{
	  /* The global source highlight object, or null if one was
	     never constructed.  This is stored here rather than in
	     the class so that we don't need to include anything or do
	     conditional compilation in source-cache.h.  */
	  static srchilite::SourceHighlight *highlighter;

	  try
	    {
	      if (highlighter == nullptr)
		{
		  highlighter = new srchilite::SourceHighlight ("esc.outlang");
		  highlighter->setStyleFile ("esc.style");
		}

	      std::istringstream input (contents);
	      std::ostringstream output;
	      highlighter->highlight (input, output, lang_name, fullname);
	      contents = output.str ();
	      already_styled = true;
	    }
	  catch (...)
	    {
	      /* Source Highlight will throw an exception if
		 highlighting fails.  One possible reason it can fail
		 is if the language is unknown -- which matters to gdb
		 because Rust support wasn't added until after 3.1.8.
		 Ignore exceptions here and fall back to
		 un-highlighted text. */
	    }
	}

      if (!already_styled)
#endif /* HAVE_SOURCE_HIGHLIGHT */
	{
	  gdb::optional<std::string> ext_contents;
	  ext_contents = ext_lang_colorize (fullname, contents);
	  if (ext_contents.has_value ())
	    contents = std::move (*ext_contents);
	}
    }

  source_text result = { std::move (fullname), std::move (contents) };
  m_source_map.push_back (std::move (result));

  if (m_source_map.size () > MAX_ENTRIES)
    m_source_map.erase (m_source_map.begin ());

  return true;
}

/* See source-cache.h.  */

bool
source_cache::get_line_charpos (struct symtab *s,
				const std::vector<off_t> **offsets)
{
  std::string fullname = symtab_to_fullname (s);

  auto iter = m_offset_cache.find (fullname);
  if (iter == m_offset_cache.end ())
    {
      if (!ensure (s))
	return false;
      iter = m_offset_cache.find (fullname);
      /* cache_source_text ensured this was entered.  */
      gdb_assert (iter != m_offset_cache.end ());
    }

  *offsets = &iter->second;
  return true;
}

/* A helper function that extracts the desired source lines from TEXT,
   putting them into LINES_OUT.  The arguments are as for
   get_source_lines.  Returns true on success, false if the line
   numbers are invalid.  */

static bool
extract_lines (const std::string &text, int first_line, int last_line,
	       std::string *lines_out)
{
  int lineno = 1;
  std::string::size_type pos = 0;
  std::string::size_type first_pos = std::string::npos;

  while (pos != std::string::npos && lineno <= last_line)
    {
      std::string::size_type new_pos = text.find ('\n', pos);

      if (lineno == first_line)
	first_pos = pos;

      pos = new_pos;
      if (lineno == last_line || pos == std::string::npos)
	{
	  /* A newline at the end does not start a new line.  */
	  if (first_pos == std::string::npos
	      || first_pos == text.size ())
	    return false;
	  if (pos == std::string::npos)
	    pos = text.size ();
	  else
	    ++pos;
	  *lines_out = text.substr (first_pos, pos - first_pos);
	  return true;
	}
      ++lineno;
      ++pos;
    }

  return false;
}

/* See source-cache.h.  */

bool
source_cache::get_source_lines (struct symtab *s, int first_line,
				int last_line, std::string *lines)
{
  if (first_line < 1 || last_line < 1 || first_line > last_line)
    return false;

  if (!ensure (s))
    return false;

  return extract_lines (m_source_map.back ().contents,
			first_line, last_line, lines);
}

#if GDB_SELF_TEST
namespace selftests
{
static void extract_lines_test ()
{
  std::string input_text = "abc\ndef\nghi\njkl\n";
  std::string result;

  SELF_CHECK (extract_lines (input_text, 1, 1, &result)
	      && result == "abc\n");
  SELF_CHECK (!extract_lines (input_text, 2, 1, &result));
  SELF_CHECK (extract_lines (input_text, 1, 2, &result)
	      && result == "abc\ndef\n");
  SELF_CHECK (extract_lines ("abc", 1, 1, &result)
	      && result == "abc");
}
}
#endif

void _initialize_source_cache ();
void
_initialize_source_cache ()
{
#if GDB_SELF_TEST
  selftests::register_test ("source-cache", selftests::extract_lines_test);
#endif
}
