/*	$NetBSD: rmt.h,v 1.2 1996/08/09 03:59:40 jtc Exp $	*/

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

#endif /* _RMT_H_ */
