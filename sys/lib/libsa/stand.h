/*	$NetBSD: stand.h,v 1.59 2006/01/13 22:32:10 christos Exp $	*/

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
 *	@(#)stand.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _LIBSA_STAND_H_
#define	_LIBSA_STAND_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include "saioctl.h"
#include "saerrno.h"

#ifndef NULL
#define	NULL	0
#endif

#ifdef LIBSA_RENAME_PRINTF
#define getchar		libsa_getchar
#define gets		libsa_gets
#define printf		libsa_printf
#define putchar		libsa_putchar
#define sprintf		libsa_sprintf
#define vprintf		libsa_vprintf
#define vsprintf	libsa_vsprintf
#endif
#define bcmp(s1, s2, l)	memcmp(s1, s2, l)
#ifdef LIBSA_USE_MEMSET
#define	bzero(s, l)	memset(s, 0, l)
#endif
#ifdef LIBSA_USE_MEMCPY
#define	bcopy(s, d, l)	memcpy(d, s, l)	/* For non-overlapping copies only */
#endif

struct open_file;

#define FS_DEF(fs) \
	extern int	__CONCAT(fs,_open)(const char *, struct open_file *); \
	extern int	__CONCAT(fs,_close)(struct open_file *); \
	extern int	__CONCAT(fs,_read)(struct open_file *, void *, \
						size_t, size_t *); \
	extern int	__CONCAT(fs,_write)(struct open_file *, void *, \
						size_t, size_t *); \
	extern off_t	__CONCAT(fs,_seek)(struct open_file *, off_t, int); \
	extern int	__CONCAT(fs,_stat)(struct open_file *, struct stat *)

/*
 * This structure is used to define file system operations in a file system
 * independent way.
 */
#if !defined(LIBSA_SINGLE_FILESYSTEM)
struct fs_ops {
	int	(*open)(const char *, struct open_file *);
	int	(*close)(struct open_file *);
	int	(*read)(struct open_file *, void *, size_t, size_t *);
	int	(*write)(struct open_file *, void *, size_t size, size_t *);
	off_t	(*seek)(struct open_file *, off_t, int);
	int	(*stat)(struct open_file *, struct stat *);
};

extern struct fs_ops file_system[];
extern int nfsys;

#define FS_OPS(fs) { \
	__CONCAT(fs,_open), \
	__CONCAT(fs,_close), \
	__CONCAT(fs,_read), \
	__CONCAT(fs,_write), \
	__CONCAT(fs,_seek), \
	__CONCAT(fs,_stat) }

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

FS_DEF(LIBSA_SINGLE_FILESYSTEM);

#endif

/* where values for lseek(2) */
#define	SEEK_SET	0	/* set file offset to offset */
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#define	SEEK_END	2	/* set file offset to EOF plus offset */

/* Device switch */
#if !defined(LIBSA_SINGLE_DEVICE)

struct devsw {
	char	*dv_name;
	int	(*dv_strategy)(void *, int, daddr_t, size_t, void *, size_t *);
	int	(*dv_open)(struct open_file *, ...);
	int	(*dv_close)(struct open_file *);
	int	(*dv_ioctl)(struct open_file *, u_long, void *);
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

/* These may be #defines which must not be expanded here, hence the extra () */
int	(DEV_STRATEGY(unused))(void *, int, daddr_t, size_t, void *, size_t *);
int	(DEV_OPEN(unused))(struct open_file *, ...);
int	(DEV_CLOSE(unused))(struct open_file *);
int	(DEV_IOCTL(unused))(struct open_file *, u_long, void *);

#endif

struct open_file {
	int		f_flags;	/* see F_* below */
#if !defined(LIBSA_SINGLE_DEVICE)
	const struct devsw	*f_dev;	/* pointer to device operations */
#endif
	void		*f_devdata;	/* device specific data */
#if !defined(LIBSA_SINGLE_FILESYSTEM)
	const struct fs_ops	*f_ops;	/* pointer to file system operations */
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

int	(devopen)(struct open_file *, const char *, char **);
#ifdef HEAP_VARIABLE
void	setheap(void *, void *);
#endif
void	*alloc(unsigned int);
void	free(void *, unsigned int);
struct	disklabel;
char	*getdisklabel(const char *, struct disklabel *);
int	dkcksum(const struct disklabel *);

void	printf(const char *, ...);
int	sprintf(char *, const char *, ...);
int	snprintf(char *, size_t, const char *, ...);
void	vprintf(const char *, _BSD_VA_LIST_);
int	vsprintf(char *, const char *, _BSD_VA_LIST_);
int	vsnprintf(char *, size_t, const char *, _BSD_VA_LIST_);
void	twiddle(void);
void	gets(char *);
int	getfile(char *prompt, int mode);
char	*strerror(int);
__dead void	exit(int) __attribute__((noreturn));
__dead void	panic(const char *, ...) __attribute__((noreturn));
__dead void	_rtt(void) __attribute__((noreturn));
void	(bcopy)(const void *, void *, size_t);
void	*memcpy(void *, const void *, size_t);
void	*memmove(void *, const void *, size_t);
int	memcmp(const void *, const void *, size_t);
void	*memset(void *, int, size_t);
void	exec(char *, char *, int);
int	open(const char *, int);
int	close(int);
void	closeall(void);
ssize_t	read(int, void *, size_t);
ssize_t	write(int, const void *, size_t);
off_t	lseek(int, off_t, int);
int	ioctl(int, u_long, char *);
int	stat(const char *, struct stat *);
int	fstat(int, struct stat *);

typedef int cmp_t __P((const void *, const void *));
void	qsort(void *, size_t, size_t, cmp_t * cmp);

extern int opterr, optind, optopt, optreset;
extern char *optarg;
int	getopt(int, char * const *, const char *);

char	*getpass(const char *);
int	checkpasswd(void);
int	check_password(const char *);

int	nodev(void);
int	noioctl(struct open_file *, u_long, void *);
void	nullsys(void);

FS_DEF(null);

/* Machine dependent functions */
void	machdep_start(char *, int, char *, char *, char *);
int	getchar(void);
void	putchar(int);

#ifdef __INTERNAL_LIBSA_CREAD
int	oopen(const char *, int);
int	oclose(int);
ssize_t	oread(int, void *, size_t);
off_t	olseek(int, off_t, int);
#endif

extern const char HEXDIGITS[];
extern const char hexdigits[];

#endif /* _LIBSA_STAND_H_ */
