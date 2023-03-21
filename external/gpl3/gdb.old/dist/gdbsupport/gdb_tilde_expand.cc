/* Perform tilde expansion on paths for GDB and gdbserver.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "gdb_tilde_expand.h"
#include <glob.h>

/* RAII-style class wrapping "glob".  */

class gdb_glob
{
public:
  /* Construct a "gdb_glob" object by calling "glob" with the provided
     parameters.  This function can throw if "glob" fails.  */
  gdb_glob (const char *pattern, int flags,
	    int (*errfunc) (const char *epath, int eerrno))
  {
    int ret = glob (pattern, flags, errfunc, &m_glob);

    if (ret != 0)
      {
	if (ret == GLOB_NOMATCH)
	  error (_("Could not find a match for '%s'."), pattern);
	else
	  error (_("glob could not process pattern '%s'."),
		 pattern);
      }
  }

  /* Destroy the object and free M_GLOB.  */
  ~gdb_glob ()
  {
    globfree (&m_glob);
  }

  /* Return the GL_PATHC component of M_GLOB.  */
  int pathc () const
  {
    return m_glob.gl_pathc;
  }

  /* Return the GL_PATHV component of M_GLOB.  */
  char **pathv () const
  {
    return m_glob.gl_pathv;
  }

private:
  /* The actual glob object we're dealing with.  */
  glob_t m_glob;
};

/* See gdbsupport/gdb_tilde_expand.h.  */

std::string
gdb_tilde_expand (const char *dir)
{
  gdb_glob glob (dir, GLOB_TILDE_CHECK, NULL);

  gdb_assert (glob.pathc () > 0);
  /* "glob" may return more than one match to the path provided by the
     user, but we are only interested in the first match.  */
  std::string expanded_dir = glob.pathv ()[0];

  return expanded_dir;
}

/* See gdbsupport/gdb_tilde_expand.h.  */

gdb::unique_xmalloc_ptr<char>
gdb_tilde_expand_up (const char *dir)
{
  gdb_glob glob (dir, GLOB_TILDE_CHECK, NULL);

  gdb_assert (glob.pathc () > 0);
  /* "glob" may return more than one match to the path provided by the
     user, but we are only interested in the first match.  */
  return make_unique_xstrdup (glob.pathv ()[0]);
}
