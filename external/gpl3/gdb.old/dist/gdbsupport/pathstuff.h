/* Path manipulation routines for GDB and gdbserver.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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

#ifndef COMMON_PATHSTUFF_H
#define COMMON_PATHSTUFF_H

#include "gdbsupport/byte-vector.h"

/* Path utilities.  */

/* Return the real path of FILENAME, expanding all the symbolic links.

   Contrary to "gdb_abspath", this function does not use
   CURRENT_DIRECTORY for path expansion.  Instead, it relies on the
   current working directory (CWD) of GDB or gdbserver.  */

extern gdb::unique_xmalloc_ptr<char> gdb_realpath (const char *filename);

/* Return a copy of FILENAME, with its directory prefix canonicalized
   by gdb_realpath.  */

extern gdb::unique_xmalloc_ptr<char>
  gdb_realpath_keepfile (const char *filename);

/* Return PATH in absolute form, performing tilde-expansion if necessary.
   PATH cannot be NULL or the empty string.
   This does not resolve symlinks however, use gdb_realpath for that.

   Contrary to "gdb_realpath", this function uses CURRENT_DIRECTORY
   for the path expansion.  This may lead to scenarios the current
   working directory (CWD) is different than CURRENT_DIRECTORY.

   If CURRENT_DIRECTORY is NULL, this function returns a copy of
   PATH.  */

extern gdb::unique_xmalloc_ptr<char> gdb_abspath (const char *path);

/* If the path in CHILD is a child of the path in PARENT, return a
   pointer to the first component in the CHILD's pathname below the
   PARENT.  Otherwise, return NULL.  */

extern const char *child_path (const char *parent, const char *child);

/* Return whether PATH contains a directory separator character.  */

extern bool contains_dir_separator (const char *path);

/* Get the usual user cache directory for the current platform.

   On Linux, it follows the XDG Base Directory specification: use
   $XDG_CACHE_HOME/gdb if the XDG_CACHE_HOME environment variable is
   defined, otherwise $HOME/.cache.

   On macOS, it follows the local convention and uses
   ~/Library/Caches/gdb.

  The return value is absolute and tilde-expanded.  Return an empty
  string if neither XDG_CACHE_HOME (on Linux) or HOME are defined.  */

extern std::string get_standard_cache_dir ();

/* Get the usual temporary directory for the current platform.

   On Windows, this is the TMP or TEMP environment variable.

   On the rest, this is the TMPDIR environment variable, if defined, else /tmp.

   Throw an exception on error.  */

extern std::string get_standard_temp_dir ();

/* Return the file name of the user's shell.  Normally this comes from
   the SHELL environment variable.  */

extern const char *get_shell ();

/* Make a filename suitable to pass to mkstemp based on F (e.g.
   /tmp/foo -> /tmp/foo-XXXXXX).  */

extern gdb::char_vector make_temp_filename (const std::string &f);

#endif /* COMMON_PATHSTUFF_H */
