/*	$NetBSD: dirent.h,v 1.21 2003/08/07 09:44:10 agc Exp $	*/

/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dirent.h	8.2 (Berkeley) 7/28/94
 */

#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <sys/featuretest.h>
#include <sys/types.h>

/*
 * The kernel defines the format of directory entries returned by 
 * the getdirentries(2) system call.
 */
#include <sys/dirent.h>

#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
#define	d_ino		d_fileno	/* backward compatibility */
#endif

typedef struct _dirdesc DIR;

#if defined(_NETBSD_SOURCE)

/* definitions for library routines operating on directories. */
#define	DIRBLKSIZ	1024

/* structure describing an open directory. */
struct _dirdesc {
	int	dd_fd;		/* file descriptor associated with directory */
	long	dd_loc;		/* offset in current buffer */
	long	dd_size;	/* amount of data returned by getdents */
	char	*dd_buf;	/* data buffer */
	int	dd_len;		/* size of data buffer */
	off_t	dd_seek;	/* magic cookie returned by getdents */
	long	dd_rewind;	/* magic cookie for rewinding */
	int	dd_flags;	/* flags for readdir */
	void	*dd_lock;	/* lock for concurrent access */
};

#define	dirfd(dirp)	((dirp)->dd_fd)

/* flags for __opendir2() */
#define DTF_HIDEW	0x0001	/* hide whiteout entries */
#define DTF_NODUP	0x0002	/* don't return duplicate names */
#define DTF_REWIND	0x0004	/* rewind after reading union stack */
#define __DTF_READALL	0x0008	/* everything has been read */

#include <sys/null.h>

#endif

#ifndef _KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int closedir __P((DIR *));
DIR *opendir __P((const char *));
struct dirent *readdir __P((DIR *));
int readdir_r __P((DIR *, struct dirent * __restrict,
    struct dirent ** __restrict));
void rewinddir __P((DIR *));
#if defined(_XOPEN_SOURCE) || defined(_NETBSD_SOURCE)
void seekdir __P((DIR *, long));
long telldir __P((const DIR *));
#endif /* defined(_NETBSD_SOURCE) || defined(_XOPEN_SOURCE) */
#if defined(_NETBSD_SOURCE)
DIR *__opendir2 __P((const char *, int));
void __seekdir __P((DIR *, long));
int scandir __P((const char *, struct dirent ***,
    int (*)(const struct dirent *), int (*)(const void *, const void *)));
int alphasort __P((const void *, const void *));
int getdirentries __P((int, char *, int, long *));
int getdents __P((int, char *, size_t));
#endif /* defined(_NETBSD_SOURCE) */
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_DIRENT_H_ */
