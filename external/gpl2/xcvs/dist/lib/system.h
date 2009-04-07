/* system-dependent definitions for CVS.
   Copyright (C) 1989-1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

/***
 *** Begin the default set of autoconf includes.
 ***/

/* Headers assumed for C89 freestanding compilers.  See HACKING for more.  */
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

/* C89 hosted headers assumed since they were included in UNIX version 7.
 * See HACKING for more.
 */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>

/* C89 hosted headers we _think_ GCC supplies even on freestanding systems.
 * If we find any systems which do not have them, a replacement header should
 * be discussed with the GNULIB folks.
 *
 * For more information, please see the `Portability' section of the `HACKING'
 * file.
 */
#include <stdlib.h>
#include <string.h>

/* We assume this because it has been around forever despite not being a part
 * of any of the other standards we assume conformance to.  So far this hasn't
 * been a problem.
 *
 * For more information, please see the `Portability' section of the `HACKING'
 * file.
 */
#include <sys/types.h>

/* A GNULIB replacement for this C99 header is supplied when it is missing.
 * See the comments in stdbool_.h for its limitations.
 */
#include <stdbool.h>

/* Ditto for these POSIX.2 headers.  */
#include <fnmatch.h>
#include <getopt.h>	/* Has GNU extensions,  */

/* We assume <sys/stat.h> because GNULIB does.  */
#include <sys/stat.h>

#if !STDC_HEADERS && HAVE_MEMORY_H
# include <memory.h>
#endif /* !STDC_HEADERS && HAVE_MEMORY_H */
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else /* ! HAVE_INTTYPES_H */
# if HAVE_STDINT_H
#  include <stdint.h>
# endif /* HAVE_STDINT_H */
#endif /* HAVE_INTTYPES_H */
#if HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
/* End the default set of autoconf includes */

/* Assume these headers. */
#include <pwd.h>

/* There is a replacement stub for gettext provided by GNULIB when gettext is
 * not available.
 */
#include <gettext.h>

#ifndef DEVNULL
# define	DEVNULL		"/dev/null"
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

/* The NeXT (without _POSIX_SOURCE, which we don't want) has a utime.h
   which doesn't define anything.  It would be cleaner to have configure
   check for struct utimbuf, but for now I'm checking NeXT here (so I don't
   have to debug the configure check across all the machines).  */
#if defined (HAVE_UTIME_H) && !defined (NeXT)
#  include <utime.h>
#else
#  if defined (HAVE_SYS_UTIME_H)
#    include <sys/utime.h>
#  else
#    ifndef ALTOS
struct utimbuf
{
  long actime;
  long modtime;
};
#    endif
int utime ();
#  endif
#endif

/* errno.h variations:
 *
 * Not all systems set the same error code on a non-existent-file
 * error.  This tries to ask the question somewhat portably.
 * On systems that don't have ENOTEXIST, this should behave just like
 * x == ENOENT.  "x" is probably errno, of course.
 */
#ifdef ENOTEXIST
#  ifdef EOS2ERR
#    define existence_error(x) \
     (((x) == ENOTEXIST) || ((x) == ENOENT) || ((x) == EOS2ERR))
#  else
#    define existence_error(x) \
     (((x) == ENOTEXIST) || ((x) == ENOENT))
#  endif
#else
#  ifdef EVMSERR
#     define existence_error(x) \
((x) == ENOENT || (x) == EINVAL || (x) == EVMSERR)
#  else
#    define existence_error(x) ((x) == ENOENT)
#  endif
#endif

/* check for POSIX signals */
#if defined(HAVE_SIGACTION) && defined(HAVE_SIGPROCMASK)
# define POSIX_SIGNALS
#endif

/* MINIX 1.6 doesn't properly support sigaction */
#if defined(_MINIX)
# undef POSIX_SIGNALS
#endif

/* If !POSIX, try for BSD.. Reason: 4.4BSD implements these as wrappers */
#if !defined(POSIX_SIGNALS)
# if defined(HAVE_SIGVEC) && defined(HAVE_SIGSETMASK) && defined(HAVE_SIGBLOCK)
#  define BSD_SIGNALS
# endif
#endif

/* Under OS/2, this must be included _after_ stdio.h; that's why we do
   it here. */
#ifdef USE_OWN_TCPIP_H
# include "tcpip.h"
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# include <sys/file.h>
#endif

#ifndef SEEK_SET
# define SEEK_SET 0
# define SEEK_CUR 1
# define SEEK_END 2
#endif

#ifndef F_OK
# define F_OK 0
# define X_OK 1
# define W_OK 2
# define R_OK 4
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

/* Convert B 512-byte blocks to kilobytes if K is nonzero,
   otherwise return it unchanged. */
#define convert_blocks(b, k) ((k) ? ((b) + 1) / 2 : (b))

/* Under non-UNIX operating systems (MS-DOS, WinNT, MacOS), many filesystem
   calls take  only one argument; permission is handled very differently on
   those systems than in Unix.  So we leave such systems a hook on which they
   can hang their own definitions.  */

#ifndef CVS_ACCESS
# define CVS_ACCESS access
#endif

#ifndef CVS_CHDIR
# define CVS_CHDIR chdir
#endif

#ifndef CVS_CREAT
# define CVS_CREAT creat
#endif

#ifndef CVS_FOPEN
# define CVS_FOPEN fopen
#endif

#ifndef CVS_FDOPEN
# define CVS_FDOPEN fdopen
#endif

#ifndef CVS_MKDIR
# define CVS_MKDIR mkdir
#endif

#ifndef CVS_OPEN
# define CVS_OPEN open
#endif

#ifndef CVS_READDIR
# define CVS_READDIR readdir
#endif

#ifndef CVS_CLOSEDIR
# define CVS_CLOSEDIR closedir
#endif

#ifndef CVS_OPENDIR
# define CVS_OPENDIR opendir
#endif

#ifndef CVS_RENAME
# define CVS_RENAME rename
#endif

#ifndef CVS_RMDIR
# define CVS_RMDIR rmdir
#endif

#ifndef CVS_UNLINK
# define CVS_UNLINK unlink
#endif

/* Wildcard matcher.  Should be case-insensitive if the system is.  */
#ifndef CVS_FNMATCH
# define CVS_FNMATCH fnmatch
#endif

#ifndef HAVE_FSEEKO
off_t ftello (FILE *);
int fseeko (FILE *, off_t, int);
#endif /* HAVE_FSEEKO */

#ifdef FILENAMES_CASE_INSENSITIVE

# if defined (__CYGWIN32__) || defined (WOE32)
    /* Under Windows, filenames are case-insensitive, and both / and \
       are path component separators.  */
#   define FOLD_FN_CHAR(c) (WNT_filename_classes[(unsigned char) (c)])
extern unsigned char WNT_filename_classes[];
# else /* !__CYGWIN32__ && !WOE32 */
  /* As far as I know, only Macintosh OS X & VMS make it here, but any
   * platform defining FILENAMES_CASE_INSENSITIVE which isn't WOE32 or
   * piggy-backing the same could, in theory.  Since the OS X fold just folds
   * A-Z into a-z, I'm just allowing it to be used for any case insensitive
   * system which we aren't yet making other specific folds or exceptions for.
   * WOE32 needs its own class since \ and C:\ style absolute paths also need
   * to be accounted for.
   */
#   define FOLD_FN_CHAR(c) (OSX_filename_classes[(unsigned char) (c)])
extern unsigned char OSX_filename_classes[];
# endif /* __CYGWIN32__ || WOE32 */

/* The following need to be declared for all case insensitive filesystems.
 * When not FOLD_FN_CHAR is not #defined, a default definition for these
 * functions is provided later in this header file.  */

/* Like strcmp, but with the appropriate tweaks for file names.  */
extern int fncmp (const char *n1, const char *n2);

/* Fold characters in FILENAME to their canonical forms.  */
extern void fnfold (char *FILENAME);

#endif /* FILENAMES_CASE_INSENSITIVE */



/* Some file systems are case-insensitive.  If FOLD_FN_CHAR is
   #defined, it maps the character C onto its "canonical" form.  In a
   case-insensitive system, it would map all alphanumeric characters
   to lower case.  Under Windows NT, / and \ are both path component
   separators, so FOLD_FN_CHAR would map them both to /.  */
#ifndef FOLD_FN_CHAR
# define FOLD_FN_CHAR(c) (c)
# define fnfold(filename) (filename)
# define fncmp strcmp
#endif

/* Different file systems can have different naming patterns which designate
 * a path as absolute.
 */
#ifndef ISABSOLUTE
# define ISABSOLUTE(s) ISSLASH(s[FILE_SYSTEM_PREFIX_LEN(s)])
#endif


/* On some systems, we have to be careful about writing/reading files
   in text or binary mode (so in text mode the system can handle CRLF
   vs. LF, VMS text file conventions, &c).  We decide to just always
   be careful.  That way we don't have to worry about whether text and
   binary differ on this system.  We just have to worry about whether
   the system has O_BINARY and "rb".  The latter is easy; all ANSI C
   libraries have it, SunOS4 has it, and CVS has used it unguarded
   some places for a while now without complaints (e.g. "rb" in
   server.c (server_updated), since CVS 1.8).  The former is just an
   #ifdef.  */

#define FOPEN_BINARY_READ ("rb")
#define FOPEN_BINARY_WRITE ("wb")
#define FOPEN_BINARY_READWRITE ("r+b")

#ifdef O_BINARY
#define OPEN_BINARY (O_BINARY)
#else
#define OPEN_BINARY (0)
#endif

#ifndef fd_select
# define fd_select select
#endif
