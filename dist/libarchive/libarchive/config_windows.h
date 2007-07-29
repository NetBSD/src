/*-
 * Copyright (c) 2003-2006 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/* Start of configuration for native Win32 with Visual Studio. */
/* TODO: Fix this. */
#undef	HAVE_ACL_CREATE_ENTRY
#undef	HAVE_ACL_INIT
#undef	HAVE_ACL_SET_FD
#undef	HAVE_ACL_SET_FD_NP
#undef	HAVE_ACL_SET_FILE
#undef	HAVE_ACL_USER
#undef	HAVE_BZLIB_H
#undef	HAVE_CHFLAGS
#define	HAVE_DECL_INT64_MAX 1
#define	HAVE_DECL_INT64_MIN 1
#define	HAVE_DECL_SIZE_MAX 1
#define	HAVE_DECL_STRERROR_R 1
#define	HAVE_DECL_UINT32_MAX 1
#define	HAVE_DECL_UINT64_MAX 1
#define	HAVE_EFTYPE 1
#define	HAVE_EILSEQ 1
#define	HAVE_ERRNO_H 1
#undef	HAVE_FCHDIR
#undef	HAVE_FCHFLAGS
#undef	HAVE_FCHMOD
#undef	HAVE_FCHOWN
#define	HAVE_FCNTL_H 1
#undef	HAVE_FSEEKO
#undef	HAVE_FUTIMES
#undef	HAVE_GRP_H
#undef	HAVE_INTTYPES_H
#undef	HAVE_LCHFLAGS
#undef	HAVE_LCHMOD
#undef	HAVE_LCHOWN
#define	HAVE_LIMITS_H 1
#undef	HAVE_LUTIMES
#define	HAVE_MALLOC 1
#define	HAVE_MEMMOVE 1
#define	HAVE_MEMORY_H 1
#define	HAVE_MEMSET 1
#define	HAVE_MKDIR 1
#undef	HAVE_MKFIFO
#undef	HAVE_PATHS_H
#undef	HAVE_POLL
#undef	HAVE_POLL_H
#undef	HAVE_PWD_H
#undef	HAVE_SELECT
#undef	HAVE_STDINT_H
#define	HAVE_STDLIB_H 1
#define	HAVE_STRCHR 1
#define	HAVE_STRDUP 1
#define	HAVE_STRERROR 1
#undef	HAVE_STRERROR_R
#define	HAVE_STRINGS_H 1
#define	HAVE_STRING_H 1
#define	HAVE_STRRCHR 1
#undef	HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC
#undef	HAVE_STRUCT_STAT_ST_RDEV
#undef	HAVE_STRUCT_TM_TM_GMTOFF
#undef	HAVE_SYS_ACL_H
#undef	HAVE_SYS_IOCTL_H
#undef	HAVE_SYS_SELECT_H
#define	HAVE_SYS_STAT_H 1
#undef	HAVE_SYS_TIME_H
#define	HAVE_SYS_TYPES_H 1
#undef	HAVE_SYS_WAIT_H
#undef	HAVE_TIMEGM
#undef	HAVE_UNISTD_H
#undef	HAVE_UTIME
#undef	HAVE_UTIMES
#undef	HAVE_UTIME_H
#define	HAVE_WCHAR_H 1
#define	HAVE_WCSCPY 1
#define	HAVE_WCSLEN 1
#define	HAVE_WMEMCMP 1
#define	HAVE_WMEMCPY 1
#undef	HAVE_ZLIB_H
#define	STDC_HEADERS 1
#define	TIME_WITH_SYS_TIME 1

/*
 * TODO: libarchive relies heavily on having the file type
 * encoded as part of the mode value.  Windows kind-of, sort-of
 * supports this, but the following needs to be carefully compared
 * to Windows conventions and quite possibly changed extensively.
 */
#define	S_IFIFO	 0010000		/* named pipe (fifo) */
//#define	S_IFCHR	 0020000		/* character special */
//#define	S_IFDIR	 0040000		/* directory */
#define	S_IFBLK	 0060000		/* block special */
//#define	S_IFREG	 0100000		/* regular */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */
#define	S_ISVTX	 0001000		/* save swapped text even after use */
//#define	S_ISUID
//#define	S_ISGID
//#define	PATH_MAX
#define	S_IRWXU 0700
#define	S_IRWXG 0070
#define	S_IRWXO 0007
#define	S_ISDIR(m)	(((m) & 0170000) == S_IFDIR)	/* directory */
#define	S_ISCHR(m)	(((m) & 0170000) == S_IFCHR)	/* char special */
#define	S_ISBLK(m)	(((m) & 0170000) == S_IFBLK)	/* block special */
#define	S_ISREG(m)	(((m) & 0170000) == S_IFREG)	/* regular file */
#define	S_ISFIFO(m)	(((m) & 0170000) == S_IFIFO)	/* fifo or socket */
#define	S_ISLNK(m)	(((m) & 0170000) == S_IFLNK)	/* symbolic link */
#define	S_ISSOCK(m)	(((m) & 0170000) == S_IFSOCK)	/* socket */

/* Basic definitions for system and integer types. */
typedef int uid_t;
typedef int gid_t;
typedef int id_t;
typedef unsigned short mode_t;
typedef unsigned _int64 uint64_t;
typedef unsigned _int16 uint16_t;
typedef uint64_t uintmax_t;
typedef _int64 intmax_t;

/* Replacement for major/minor/makedev. */
#define	major(x) ((int)(0x00ff & ((x) >> 8)))
#define	minor(x) ((int)(0xffff00ff & (x)))
#define	makedev(maj,min) ((0xff00 & ((maj)<<8))|(0xffff00ff & (min)))

#define	EFTYPE 7
#define	STDERR_FILENO 2

/* Alias the Windows _function to the POSIX equivalent. */
#include <io.h>
#define	write _write
#define	read _read
#define	lseek _lseek
#define	open _open
#define	chdir _chdir
#define	mkdir _mkdir
#define	close _close

#define	PACKAGE_NAME "libarchive"
#define	PACKAGE_VERSION "2.0experimental"

/* TODO: Fix the code, don't suppress the warnings. */
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#pragma warning(disable:4305)
#pragma warning(disable:4267)

/* End of Win32/Visual Studio definitions. */
