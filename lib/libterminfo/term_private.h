/* $NetBSD: term_private.h,v 1.1 2010/02/03 15:16:32 roy Exp $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _TERM_PRIVATE_H_
#define	_TERM_PRIVATE_H_

/* This header should only be used by libterminfo, tic and infocmp. */

/* The terminfo database structure is private to us,
 * so it's documented here.
 * terminfo defines the largest number as 32767 and the largest
 * compiled entry as 4093 bytes long.
 * Thus, we store all numbers as uint16_t, including string length.
 * All strings are prefixed by length, including the null terminator.
 * The largest string length we can handle is 65535 bytes,
 * including the null terminator.
 * The largest capabaility block we can handle is 65535 bytes.
 * This means that we exceed the current terminfo defined limits.
 * The header is version (char), name.
 * version 0 is an alias, and the name refers to the real terminfo.
 * version 1 capabilities are defined as so:
 * description,
 * cap length, num flags, index, char,
 * cap length, num numbers, index, number,
 * cap length, num strings, index, string,
 * cap lelngth, num undefined caps, name, type (char), flag, number, string
 * The database itself is created using ndbm(3) and the numbers are
 * always stored as little endian.
 */

#include <sys/types.h>

#define _TERMINFO

/* We use the same ncurses tic macros so that our data is identical
 * when a caller uses the long name macros to access te terminfo data
 * directly. */
#define ABSENT_BOOLEAN		((signed char)-1)       /* 255 */
#define ABSENT_NUMERIC		(-1)
#define ABSENT_STRING		(char *)0
#define CANCELLED_BOOLEAN	((signed char)-2)       /* 254 */
#define CANCELLED_NUMERIC	(-2)
#define CANCELLED_STRING	(char *)(-1)
#define VALID_BOOLEAN(s) ((unsigned char)(s) <= 1)	/* reject "-1" */
#define VALID_NUMERIC(s) ((s) >= 0)
#define VALID_STRING(s)  ((s) != CANCELLED_STRING && (s) != ABSENT_STRING)

typedef struct termextra {
	const char *id;
	char type;
	char flag;
	short num;
	const char *str;
} TERMUSERDEF;

typedef struct terminal {
	int fildes;
	/* We need to expose these so that the macros work */
	char *name;
	char *desc;
	char *flags;
	short *nums;
	const char **strs;
	/* Storage area for terminfo data */
	char *_area;
	size_t _nuserdefs;
	TERMUSERDEF *_userdefs;
	/* So we don't rely on the global ospeed */
	short _ospeed;
	/* Output buffer for tparm */
	char *_buf;
	size_t _buflen;
	size_t _bufpos;
	/* A-Z static variables for tparm  */
	long _snums[26];
} TERMINAL;

extern const char *	_ti_database;

ssize_t		_ti_flagindex(const char *);
ssize_t		_ti_numindex(const char *);
ssize_t		_ti_strindex(const char *);
const char *	_ti_flagid(ssize_t);
const char *	_ti_numid(ssize_t);
const char *	_ti_strid(ssize_t);
int		_ti_getterm(TERMINAL *, const char *, int);
void		_ti_setospeed(TERMINAL *);
void		_ti_freeterm(TERMINAL *);

#endif
