/* Shared general utility routines for GDB, the GNU debugger.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <string>
#include <vector>
#include <sstream>

/* If possible, define FUNCTION_NAME, a macro containing the name of
   the function being defined.  Since this macro may not always be
   defined, all uses must be protected by appropriate macro definition
   checks (Eg: "#ifdef FUNCTION_NAME").

   Version 2.4 and later of GCC define a magical variable `__PRETTY_FUNCTION__'
   which contains the name of the function currently being defined.
   This is broken in G++ before version 2.6.
   C9x has a similar variable called __func__, but prefer the GCC one since
   it demangles C++ function names.  */
#if (GCC_VERSION >= 2004)
#define FUNCTION_NAME		__PRETTY_FUNCTION__
#else
#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#define FUNCTION_NAME		__func__  /* ARI: func */
#endif
#endif

/* xmalloc(), xrealloc() and xcalloc() have already been declared in
   "libiberty.h". */

/* Like xmalloc, but zero the memory.  */
void *xzalloc (size_t);

void xfree (void *);

/* Like asprintf and vasprintf, but return the string, throw an error
   if no memory.  */
char *xstrprintf (const char *format, ...) ATTRIBUTE_PRINTF (1, 2);
char *xstrvprintf (const char *format, va_list ap)
     ATTRIBUTE_PRINTF (1, 0);

/* Like snprintf, but throw an error if the output buffer is too small.  */
int xsnprintf (char *str, size_t size, const char *format, ...)
     ATTRIBUTE_PRINTF (3, 4);

/* Returns a std::string built from a printf-style format string.  */
std::string string_printf (const char* fmt, ...)
  ATTRIBUTE_PRINTF (1, 2);

/* Returns a string representation of VAL.  Replacement for C++11
   std::to_string for hosts that lack it.  */

namespace gdb {

#define REPLACE_TO_STRING 0

#ifdef __MINGW32__
/* mingw.org's MinGW runtime before version 5.0 needs the replacement
   below.  Tested with mingwrt-3.22.2 and mingwrt-5.0, and with
   libstdc++ included with MinGW GCC 5.3.0.  */
# include <_mingw.h>
/* We test __MINGW64_VERSION_MAJOR to exclude MinGW64, which doesn't
   need this replacement.  */
# if !defined __MINGW64_VERSION_MAJOR && !defined _GLIBCXX_USE_C99
#  undef REPLACE_TO_STRING
#  define REPLACE_TO_STRING 1
# endif
#endif

#if REPLACE_TO_STRING

template <class T>
inline std::string
to_string (const T &val)
{
  std::stringstream ss;

  ss << val;
  return ss.str ();
}

#else /* !REPLACE_TO_STRING */

using std::to_string;

#endif

#undef REPLACE_TO_STRING
}

/* Make a copy of the string at PTR with LEN characters
   (and add a null character at the end in the copy).
   Uses malloc to get the space.  Returns the address of the copy.  */

char *savestring (const char *ptr, size_t len);

/* The strerror() function can return NULL for errno values that are
   out of range.  Provide a "safe" version that always returns a
   printable string.  */

extern char *safe_strerror (int);

/* Return non-zero if the start of STRING matches PATTERN, zero
   otherwise.  */

static inline int
startswith (const char *string, const char *pattern)
{
  return strncmp (string, pattern, strlen (pattern)) == 0;
}

ULONGEST strtoulst (const char *num, const char **trailer, int base);

/* Skip leading whitespace characters in INP, returning an updated
   pointer.  If INP is NULL, return NULL.  */

extern char *skip_spaces (char *inp);

/* A const-correct version of the above.  */

extern const char *skip_spaces_const (const char *inp);

/* Skip leading non-whitespace characters in INP, returning an updated
   pointer.  If INP is NULL, return NULL.  */

#define skip_to_space(INP) ((char *) skip_to_space_const (INP))

/* A const-correct version of the above.  */

extern const char *skip_to_space_const (const char *inp);

/* Assumes that V is an argv for a program, and iterates through
   freeing all the elements.  */
extern void free_vector_argv (std::vector<char *> &v);

#endif
