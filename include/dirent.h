/*-
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)dirent.h	5.18 (Berkeley) 2/23/91
 *	$Id: dirent.h,v 1.5 1993/12/15 00:50:19 jtc Exp $
 */

#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <sys/dirent.h>

/* structure describing an open directory. */
typedef struct {
	int	dd_fd;		/* file descriptor associated with directory */
	long	dd_loc;		/* offset in current buffer */
	long	dd_size;	/* amount of data returned by getdirentries */
	char	*dd_buf;	/* data buffer */
	int	dd_len;		/* size of data buffer */
	long	dd_seek;	/* magic cookie returned by getdirentries */
	void	*dd_ddloc;	/* Linked list of ddloc structs for telldir/seekdir */
} DIR;


#ifndef _POSIX_SOURCE
#define	d_ino		d_fileno	/* backward compatibility */

/* definitions for library routines operating on directories. */
#define	DIRBLKSIZ	1024

#define	dirfd(dirp)	((dirp)->dd_fd)

#ifndef NULL
#define	NULL	0
#endif
#endif /* _POSIX_SOURCE */


#include <sys/cdefs.h>

__BEGIN_DECLS
DIR *opendir __P((const char *));
struct dirent *readdir __P((DIR *));
void rewinddir __P((DIR *));
int closedir __P((DIR *));
#ifndef _POSIX_SOURCE
long telldir __P((const DIR *));
void seekdir __P((DIR *, long));
int scandir __P((const char *, struct dirent ***,
    int (*)(struct dirent *), int (*)(const void *, const void *)));
int alphasort __P((const void *, const void *));
int getdirentries __P((int, char *, int, long *));
#endif /* not POSIX */
__END_DECLS

#endif /* !_DIRENT_H_ */
