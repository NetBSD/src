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

#include "defs.h"
#include "source-cache.h"
#include "common/scoped_fd.h"
#include "source.h"
#include "cli/cli-style.h"

#ifdef HAVE_SOURCE_HIGHLIGHT
/* If Gnulib redirects 'open' and 'close' to its replacements
   'rpl_open' and 'rpl_close' via cpp macros, including <fstream>
   below with those macros in effect will cause unresolved externals
   when GDB is linked.  Happens, e.g., in the MinGW build.  */
#undef open
#undef close
#include <fstream>
#include <sstream>
#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>
#endif

/* The number of source files we'll cache.  */

#define MAX_ENTRIES 5

/* See source-cache.h.  */

source_cache g_source_cache;

/* See source-cache.h.  */

bool
source_cache::get_plain_source_lines (struct symtab *s, int first_line,
				      int last_line, std::string *lines)
{
  scoped_fd desc (open_source_file (s));
  if (desc.get () < 0)
    return false;

  if (s->line_charpos == 0)
    find_source_lines (s, desc.get ());

  if (first_line < 1 || first_line > s->nlines || last_line < 1)
    return false;

  if (lseek (desc.get (), s->line_charpos[first_line - 1], SEEK_SET) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  int last_charpos;
  if (last_line >= s->nlines)
    {
      struct stat st;

      if (fstat (desc.get (), &st) < 0)
	perror_with_name (symtab_to_filename_for_display (s));
      /* We could cache this in line_charpos... */
      last_charpos = st.st_size;
    }
  else
    last_charpos = s->line_charpos[last_line];

  lines->resize (last_charpos - s->line_charpos[first_line - 1]);
  if (myread (desc.get (), &(*lines)[0], lines->size ()) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  return true;
}

/* See source-cache.h.  */

std::string
source_cache::extract_lines (const struct source_text &text, int first_line,
			     int last_line)
{
  int lineno = 1;
  std::string::size_type pos = 0;
  std::string::size_type first_pos = std::string::npos;

  while (pos != std::string::npos && lineno <= last_line)
    {
      std::string::size_type new_pos = text.contents.find ('\n', pos);

      if (lineno == first_line)
	first_pos = pos;

      pos = new_pos;
      if (lineno == last_line || pos == std::string::npos)
	{
	  if (first_pos == std::string::npos)
	    return {};
	  if (pos == std::string::npos)
	    pos = text.contents.size ();
	  return text.contents.substr (first_pos, pos - first_pos);
	}
      ++lineno;
      ++pos;
    }

  return {};
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
      /* Not handled by Source Highlight.  */
      break;

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
source_cache::get_source_lines (struct symtab *s, int first_line,
				int last_line, std::string *lines)
{
  if (first_line < 1 || last_line < 1 || first_line > last_line)
    return false;

#ifdef HAVE_SOURCE_HIGHLIGHT
  if (source_styling && can_emit_style_escape (gdb_stdout))
    {
      const char *fullname = symtab_to_fullname (s);

      for (const auto &item : m_source_map)
	{
	  if (item.fullname == fullname)
	    {
	      *lines = extract_lines (item, first_line, last_line);
	      return true;
	    }
	}

      const char *lang_name = get_language_name (SYMTAB_LANGUAGE (s));
      if (lang_name != nullptr)
	{
	  std::ifstream input (fullname);
	  if (input.is_open ())
	    {
	      if (s->line_charpos == 0)
		{
		  scoped_fd desc = open_source_file (s);
		  if (desc.get () < 0)
		    return false;
		  find_source_lines (s, desc.get ());

		  /* FULLNAME points to a value owned by the symtab
		     (symtab::fullname).  Calling open_source_file reallocates
		     that value, so we must refresh FULLNAME to avoid a
		     use-after-free.  */
		  fullname = symtab_to_fullname (s);
		}
	      srchilite::SourceHighlight highlighter ("esc.outlang");
	      highlighter.setStyleFile("esc.style");

	      std::ostringstream output;
	      highlighter.highlight (input, output, lang_name, fullname);

	      source_text result = { fullname, output.str () };
	      m_source_map.push_back (std::move (result));

	      if (m_source_map.size () > MAX_ENTRIES)
		m_source_map.erase (m_source_map.begin ());

	      *lines = extract_lines (m_source_map.back (), first_line,
				      last_line);
	      return true;
	    }
	}
    }
#endif /* HAVE_SOURCE_HIGHLIGHT */

  return get_plain_source_lines (s, first_line, last_line, lines);
}
