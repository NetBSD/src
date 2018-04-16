/* sysdep.h -- handle host dependencies for binutils
   Copyright (C) 1991-2016 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _BIN_SYSDEP_H
#define _BIN_SYSDEP_H

#include "alloca-conf.h"
#include "ansidecl.h"
#include <stdio.h>
#include <sys/types.h>

#include "bfdver.h"

#include <stdarg.h>

#ifdef USE_BINARY_FOPEN
#include "fopen-bin.h"
#else
#include "fopen-same.h"
#endif

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "binary-io.h"

#if !HAVE_DECL_STPCPY
extern char *stpcpy (char *, const char *);
#endif

#if !HAVE_DECL_STRSTR
extern char *strstr ();
#endif

#ifdef HAVE_SBRK
#if !HAVE_DECL_SBRK
extern char *sbrk ();
#endif
#endif

#if !HAVE_DECL_GETENV
extern char *getenv ();
#endif

#if !HAVE_DECL_ENVIRON
extern char **environ;
#endif

#if !HAVE_DECL_FPRINTF
extern int fprintf (FILE *, const char *, ...);
#endif

#if !HAVE_DECL_SNPRINTF
extern int snprintf(char *, size_t, const char *, ...);
#endif

#if !HAVE_DECL_VSNPRINTF
extern int vsnprintf(char *, size_t, const char *, va_list);
#endif

#if !HAVE_DECL_STRNLEN
size_t strnlen (const char *, size_t);
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#endif

#ifndef O_RDWR
#define O_RDWR 2
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

#ifdef HAVE_LOCALE_H
# ifndef ENABLE_NLS
   /* The Solaris version of locale.h always includes libintl.h.  If we have
      been configured with --disable-nls then ENABLE_NLS will not be defined
      and the dummy definitions of bindtextdomain (et al) below will conflict
      with the defintions in libintl.h.  So we define these values to prevent
      the bogus inclusion of libintl.h.  */
#  define _LIBINTL_H
#  define _LIBGETTEXT_H
# endif
# include <locale.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(String) gettext (String)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
#else
# define gettext(Msgid) (Msgid)
# define dgettext(Domainname, Msgid) (Msgid)
# define dcgettext(Domainname, Msgid, Category) (Msgid)
# define textdomain(Domainname) while (0) /* nothing */
# define bindtextdomain(Domainname, Dirname) while (0) /* nothing */
# define _(String) (String)
# define N_(String) (String)
#endif

/* Used by ar.c and objcopy.c.  */
#define BUFSIZE 8192

/* For PATH_MAX.  */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifndef PATH_MAX
/* For MAXPATHLEN.  */
# ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif
# ifndef PATH_MAX
#  ifdef MAXPATHLEN
#   define PATH_MAX MAXPATHLEN
#  else
#   define PATH_MAX 1024
#  endif
# endif
#endif

#endif /* _BIN_SYSDEP_H */
