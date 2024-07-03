/* @(#)file.h	1.12 09/07/13 joerg */
/*
 * file.h - definitions for file(1) program
 * @(#)Id: file.h,v 1.25 1997/01/15 19:28:35 christos Exp
 *
 * Copyright (c) Ian F. Darwin, 1987.
 * Written by Ian F. Darwin.
 *
 * This software is not subject to any export provision of the United States
 * Department of Commerce, and may be exported to any country or planet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __file_h__
#define __file_h__

typedef int Int32_t;
typedef unsigned int UInt32_t;

#ifndef HOWMANY
# define HOWMANY 8192		/* how much of the file to look at */
#endif
#define MAXMAGIS 1000		/* max entries in /etc/magic */
#define MAXDESC	50		/* max leng of text description */
#define MAXstring 32		/* max leng of "string" types */

struct magic {
	short flag;		
#define INDIR	1		/* if '>(...)' appears,  */
#define	UNSIGNED 2		/* comparison is unsigned */
#define ADD	4		/* if '>&' appears,  */
	short cont_level;	/* level of ">" */
	struct {
		char type;	/* byte short long */
		Int32_t offset;	/* offset from indirection */
	} in;
	Int32_t offset;		/* offset to magic number */
	unsigned char reln;	/* relation (0=eq, '>'=gt, etc) */
	char type;		/* int, short, long or string. */
	char vallen;		/* length of string value, if any */
#define 			BYTE	1
#define				SHORT	2
#define				LONG	4
#define				STRING	5
#define				DATE	6
#define				BESHORT	7
#define				BELONG	8
#define				BEDATE	9
#define				LESHORT	10
#define				LELONG	11
#define				LEDATE	12
	union VALUETYPE {
		unsigned char b;
		unsigned short h;
		UInt32_t l;
		char s[MAXstring];
		unsigned char hs[2];	/* 2 bytes of a fixed-endian "short" */
		unsigned char hl[4];	/* 2 bytes of a fixed-endian "long" */
	} value;		/* either number or string */
	UInt32_t mask;	/* mask before comparison with value */
	char nospflag;		/* supress space character */
	char desc[MAXDESC];	/* description */
};

#include <stdio.h>

extern int   init_magic		(char *);
extern int   ascmagic		(unsigned char *, int);
extern void  ckfputs		(const char *, FILE *);
struct stat;
extern int   fsmagic		(const char *, struct stat *);
extern int   is_compress	(const unsigned char *, int *);
extern int   is_tar		(unsigned char *, int);
extern void  magwarn		(const char *, ...);
extern void  mdump		(struct magic *);
extern char *get_magic_magic	(const char *);
extern void  showstr		(FILE *, const char *, int);
extern char *softmagic		(unsigned char *, int);
extern int   tryit		(unsigned char *, int, int);
extern int   zmagic		(unsigned char *, int);
extern void  ckfprintf		(FILE *, const char *, ...);
extern UInt32_t signextend	(struct magic *, UInt32_t);
extern int internatmagic	(unsigned char *, int);
extern void tryelf		(int, char *, int);


extern char *progname;		/* the program name 			*/
extern char *magicfile;		/* name of the magic file		*/

extern struct magic *__f_magic;	/* array of magic entries		*/
extern int __f_nmagic;		/* number of valid magic[]s 		*/


extern int debug;		/* enable debugging?			*/
extern int zflag;		/* process compressed files?		*/
extern int lflag;		/* follow symbolic links?		*/

#endif /* __file_h__ */
