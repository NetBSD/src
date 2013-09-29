/* sysdep.h -- handle host dependencies for the GNU linker
   Copyright 1995, 1996, 1997, 1999, 2002, 2003, 2005, 2007, 2012
   Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef LD_SYSDEP_H
#define LD_SYSDEP_H

#ifdef PACKAGE
#error sysdep.h must be included in lieu of config.h
#endif

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#ifdef STRING_WITH_STRINGS
#include <string.h>
#include <strings.h>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
extern char *strchr ();
extern char *strrchr ();
#endif
#endif
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* for PATH_MAX */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
/* for MAXPATHLEN */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef PATH_MAX
# define LD_PATHMAX PATH_MAX
#else
# ifdef MAXPATHLEN
#  define LD_PATHMAX MAXPATHLEN
# else
#  define LD_PATHMAX 1024
# endif
#endif

#ifdef HAVE_REALPATH
# define REALPATH(a,b) realpath (a, b)
#else
# define REALPATH(a,b) NULL
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_BINARY_FOPEN
#include "fopen-bin.h"
#else
#include "fopen-same.h"
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_RDWR
#define O_RDWR 2
#endif
#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif
/* Systems that don't already define this, don't need it.  */
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#if !HAVE_DECL_STRSTR
extern char *strstr ();
#endif

#if !HAVE_DECL_FREE
extern void free ();
#endif

#if !HAVE_DECL_GETENV
extern char *getenv ();
#endif

#if !HAVE_DECL_ENVIRON
extern char **environ;
#endif

#endif /* ! defined (LD_SYSDEP_H) */
