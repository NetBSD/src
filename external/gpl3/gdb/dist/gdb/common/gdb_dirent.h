/* Portable <dirent.h>.
   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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

#ifndef GDB_DIRENT_H
#define GDB_DIRENT_H 1

/* See description of `AC_HEADER_DIRENT' in the Autoconf manual.  */
#ifdef HAVE_DIRENT_H
# include <dirent.h>		/* ARI: dirent.h */
# define NAMELEN(dirent) strlen ((dirent)->d_name)    /* ARI: strlen d_name */
#else
# define dirent direct
# define NAMELEN(dirent) (dirent)->d_namelen	/* ARI: d_namelen */
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#endif /* not GDB_DIRENT_H */
