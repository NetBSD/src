/*	$NetBSD: stand.h,v 1.33.8.1 1999/12/27 18:36:02 wrstuden Exp $	*/

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
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
 *	@(#)stand.h	8.1 (Berkeley) 6/11/93
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include "saioctl.h"
#include "saerrno.h"

#ifndef NULL
#define	NULL	0
#endif

#ifdef LIBSA_USE_MEMSET
#define	bzero(s, l)	memset(s, 0, l)
#endif
#ifdef LIBSA_USE_MEMCPY
#define	bcopy(s, d, l)	memcpy(d, s, l)	/* For non-overlapping copies only */
#endif

struct open_file;

/*
 * This structure is used to define file system operations in a file system
 * independent way.
 */
#if !defined(LIBSA_SINGLE_FILESYSTEM)
struct fs_ops {
	int	(*open) __P((char *path, struct open_file *f));
	int	(*close) __P((struct open_file *f));
	int	(*read) __P((struct open_file *f, void *buf,
			     size_t size, size_t *resid));
	int	(*write) __P((struct open_file *f, void *buf,
			     size_t size, size_t *resid));
	off_t	(*seek) __P((struct open_file *f, off_t offset, int where));
	int	(*stat) __P((struct open_file *f, struct stat *sb));
};

extern struct fs_ops file_system[];
extern int nfsys;

#define	FS_OPEN(fs)		((fs)->open)
#define	FS_CLOSE(fs)		((fs)->close)
#define	FS_READ(fs)		((fs)->read)
#define	FS_WRITE(fs)		((fs)->write)
#define	FS_SEEK(fs)		((fs)->seek)
#define	FS_STAT(fs)		((fs)->stat)

#else

#define	FS_OPEN(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_open)
#define	FS_CLOSE(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_close)
#define	FS_READ(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_read)
#define	FS_WRITE(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_write)
#define	FS_SEEK(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_seek)
#define	FS_STAT(fs)		___CONCAT(LIBSA_SINGLE_FILESYSTEM,_stat)

int	FS_OPEN(unused) __P((char *path, struct open_file *f));
int	FS_CLOSE(unused) __P((struct open_file *f));
int	FS_READ(unused) __P((struct open_file *f, void *buf,
			     size_t size, size_t *resid));
int	FS_WRITE(unused) __P((struct open_file *f, void *buf,
			      size_t size, size_t *resid));
off_t	FS_SEEK(unused) __P((struct open_file *f, off_t offset, int where));
int	FS_STAT(unused) __P((struct open_file *f, struct stat *sb));

#endif

/* where values for lseek(2) */
#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */

/* Device switch */
#if !defined(LIBSA_SINGLE_DEVICE)

struct devsw {
	char	*dv_name;
	int	(*dv_strategy) __P((void *devdata, int rw,
				    daddr_t blk, size_t size,
				    void *buf, size_t *rsize));
	int	(*dv_open) __P((struct open_file *f, ...));
	int	(*dv_close) __P((struct open_file *f));
	int	(*dv_ioctl) __P((struct open_file *f, u_long cmd, void *data));
};

extern struct devsw devsw[];	/* device array */
extern int ndevs;		/* number of elements in devsw[] */

#define	DEV_NAME(d)		((d)->dv_name)
#define	DEV_STRATEGY(d)		((d)->dv_strategy)
#define	DEV_OPEN(d)		((d)->dv_open)
#define	DEV_CLOSE(d)		((d)->dv_close)
#define	DEV_IOCTL(d)		((d)->dv_ioctl)

#else

#define	DEV_NAME(d)		___STRING(LIBSA_SINGLE_DEVICE)
#define	DEV_STRATEGY(d)		___CONCAT(LIBSA_SINGLE_DEVICE,strategy)
#define	DEV_OPEN(d)		___CONCAT(LIBSA_SINGLE_DEVICE,open)
#define	DEV_CLOSE(d)		___CONCAT(LIBSA_SINGLE_DEVICE,close)
#define	DEV_IOCTL(d)		___CONCAT(LIBSA_SINGLE_DEVICE,ioctl)

int	DEV_STRATEGY(unused) __P((void *devdata, int rw, daddr_t blk,
				  size_t size, void *buf, size_t *rsize));
int	DEV_OPEN(unused) __P((struct open_file *f, ...));
int	DEV_CLOSE(unused) __P((struct open_file *f));
int	DEV_IOCTL(unused) __P((struct open_file *f, u_long cmd, void *data));

#endif

struct open_file {
	int		f_flags;	/* see F_* below */
#if !defined(LIBSA_SINGLE_DEVICE)
	struct devsw	*f_dev;		/* pointer to device operations */
#endif
	void		*f_devdata;	/* device specific data */
#if !defined(LIBSA_SINGLE_FILESYSTEM)
	struct fs_ops	*f_ops;		/* pointer to file system operations */
#endif
	void		*f_fsdata;	/* file system specific data */
#if !defined(LIBSA_NO_RAW_ACCESS)
	off_t		f_offset;	/* current file offset (F_RAW) */
#endif
};

#define	SOPEN_MAX	4
extern struct open_file files[];

/* f_flags values */
#define	F_READ		0x0001	/* file opened for reading */
#define	F_WRITE		0x0002	/* file opened for writing */
#if !defined(LIBSA_NO_RAW_ACCESS)
#define	F_RAW		0x0004	/* raw device open - no file system */
#endif
#define F_NODEV		0x0008	/* network open - no device */

#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define tolower(c)	((c) - 'A' + 'a')
#define isspace(c)	((c) == ' ' || (c) == '\t')
#define isdigit(c)	((c) >= '0' && (c) <= '9')

int	devopen __P((struct open_file *, const char *, char **));
#ifdef HEAP_VARIABLE
void	setheap __P((void *, void *));
#endif
void	*alloc __P((unsigned int));
void	free __P((void *, unsigned int));
struct	disklabel;
char	*getdisklabel __P((const char *, struct disklabel *));
int	dkcksum __P((struct disklabel *));

void	printf __P((const char *, ...));
int	sprintf __P((char *, const char *, ...));
int	snprintf __P((char *, size_t, const char *, ...));
void	vprintf __P((const char *, _BSD_VA_LIST_));
int	vsprintf __P((char *, const char *, _BSD_VA_LIST_));
int	vsnprintf __P((char *, size_t, const char *, _BSD_VA_LIST_));
void	twiddle __P((void));
void	gets __P((char *));
int	getfile __P((char *prompt, int mode));
char	*strerror __P((int));
__dead void	panic __P((const char *, ...)) __attribute__((noreturn));
__dead void	_rtt __P((void)) __attribute__((noreturn));
void	bcopy __P((const void *, void *, size_t));
void	*memcpy __P((void *, const void *, size_t));
int	memcmp __P((const void *, const void *, size_t));
void	exec __P((char *, char *, int));
int	open __P((const char *, int));
int	close __P((int));
void	closeall __P((void));
ssize_t	read __P((int, void *, size_t));
ssize_t	write __P((int, void *, size_t));
off_t	lseek __P((int, off_t, int));
int	ioctl __P((int, u_long, char *));

extern int opterr, optind, optopt, optreset;
extern char *optarg;
int	getopt __P((int, char * const *, const char *));

char	*getpass __P((const char *));
int	checkpasswd __P((void));

int	nodev __P((void));
int	noioctl __P((struct open_file *, u_long, void *));
void	nullsys __P((void));

int	null_open __P((char *path, struct open_file *f));
int	null_close __P((struct open_file *f));
ssize_t	null_read __P((struct open_file *f, void *buf,
			size_t size, size_t *resid));
ssize_t	null_write __P((struct open_file *f, void *buf,
			size_t size, size_t *resid));
off_t	null_seek __P((struct open_file *f, off_t offset, int where));
int	null_stat __P((struct open_file *f, struct stat *sb));

/* Machine dependent functions */
void	machdep_start __P((char *, int, char *, char *, char *));
int	getchar __P((void));
void	putchar __P((int));    

#ifdef __INTERNAL_LIBSA_CREAD
int	oopen __P((const char *, int));
int	oclose __P((int));
ssize_t	oread __P((int, void *, size_t));
off_t	olseek __P((int, off_t, int));
#endif
