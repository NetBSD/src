/* Portable <regex.h>.
   Copyright (C) 2000-2014 Free Software Foundation, Inc.

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

#ifndef GDB_REGEX_H
#define GDB_REGEX_H 1

#ifdef USE_INCLUDED_REGEX
# include "xregex.h"
#else
/* Request 4.2 BSD regex functions.  */
# define _REGEX_RE_COMP
# include <regex.h>
#endif

/* From utils.c.  */
struct cleanup *make_regfree_cleanup (regex_t *);
char *get_regcomp_error (int, regex_t *);
struct cleanup *compile_rx_or_error (regex_t *pattern, const char *rx,
				     const char *message);

#endif /* not GDB_REGEX_H */
