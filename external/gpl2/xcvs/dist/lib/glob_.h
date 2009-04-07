/* glob_.h -- Find a path matching a pattern.

   Copyright (C) 2005 Free Software Foundation, Inc.

   Written by Derek Price <derek@ximbiot.com> & Paul Eggert <eggert@CS.UCLA.EDU>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef GLOB_H
#define GLOB_H 1

#ifdef HAVE_SYS_CDEFS_H
# include <sys/cdefs.h>
#endif

#include <stddef.h>

#ifndef __BEGIN_DECLS
# define __BEGIN_DECLS
# define __END_DECLS
#endif
#ifndef __THROW
# define __THROW
#endif

#ifndef __size_t
# define __size_t	size_t
#endif
#ifndef __const
# define __const	const
#endif
#ifndef __restrict
# define __restrict	restrict
#endif
#ifndef __USE_GNU
# define __USE_GNU    1
#endif


#ifndef HAVE_DIRENT_H
# define dirent direct
#endif

#define glob rpl_glob
#define globfree rpl_globfree
#define glob_pattern_p rpl_glob_pattern_p

#define __GLOB_GNULIB 1

/* Now the standard GNU C Library header should work.  */
#include "glob-libc.h"

#endif /* GLOB_H */
