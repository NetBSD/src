/*
 * $Header: /cvsroot/src/include/rmt.h,v 1.1 1996/08/09 03:56:56 jtc Exp $
 *
 * $Log: rmt.h,v $
 * Revision 1.1  1996/08/09 03:56:56  jtc
 * Remote mag tape library from volume 18 of comp.sources.unix.
 *
 * Revision 1.1  86/10/09  16:17:20  root
 * Initial revision
 * 
 */

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


#ifndef access		/* avoid multiple redefinition */
#ifndef lint		/* in this case what lint doesn't know won't hurt it */
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

extern long rmtlseek ();	/* all the rest are int's */
#endif
#endif
