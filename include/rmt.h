/*	$NetBSD: rmt.h,v 1.2.4.2 1997/10/22 07:05:38 thorpej Exp $	*/

/*
 *	rmt.h
 *
 *	Added routines to replace open(), close(), lseek(), ioctl(), etc.
 *	The preprocessor can be used to remap these the rmtopen(), etc
 *	thus minimizing source changes.
 *
 *	This file must be included before <sys/stat.h>, since it redefines
 *	stat to be rmtstat, so that struct stat xyzzy; declarations work
 *	properly.
 *
 *	-- Fred Fish (w/some changes by Arnold Robbins)
 */

#ifndef _RMT_H_
#define _RMT_H_

#include <sys/cdefs.h>

__BEGIN_DECLS
int	rmtaccess __P((const char *, int));
int	rmtclose __P((int));
int	rmtcreat __P((const char *, mode_t));
int	rmtdup __P((int));
int	rmtfcntl __P((int, int, ...));
int	rmtfstat __P((int, struct stat *));
int	rmtioctl __P((int, unsigned long, ...));
int	rmtisatty __P((int));
off_t	rmtlseek __P((int, off_t, int));
int	rmtlstat __P((const char *, struct stat *));
int	rmtopen __P((const char *, int, ...));
ssize_t	rmtread __P((int, void *, size_t));
int	rmtstat __P((const char *, struct stat *));
ssize_t	rmtwrite __P((int, const void *, size_t));
__END_DECLS

#ifndef __RMTLIB_PRIVATE	/* don't remap if building librmt */
#define access rmtaccess
#define close rmtclose
#define creat rmtcreat
#define dup rmtdup
#define fcntl rmtfcntl
#define fstat rmtfstat
#define ioctl rmtioctl
#define isatty rmtisatty
#define lseek rmtlseek
#define lstat rmtlstat
#define open rmtopen
#define read rmtread
#define stat rmtstat
#define write rmtwrite
#endif /* __RMTLIB_PRIVATE */

#endif /* _RMT_H_ */
