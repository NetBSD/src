/* $NetBSD: term_private.h,v 1.19 2020/06/21 15:05:23 roy Exp $ */

/*
 * Copyright (c) 2009, 2010, 2013, 2020 The NetBSD Foundation, Inc.
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
 *
 * Version 1 - types 1 and 2.
 * terminfo defines the largest number as 32767 and the largest
 * compiled entry as 4093 bytes long. Negative numbers are not allowed.
 * Thus, we store all numbers as uint16_t, including string length.
 * We reserve negative numbers -1 and -2 to mean absent or cancelled.
 * All strings are prefixed by length, including the null terminator.
 * The largest string length we can handle is 65535 bytes,
 * including the null terminator.
 * The largest capability block we can handle is 65535 bytes.
 *
 * Version 2 - type 3
 * Extends terminfo numbers upto 2147483647 by storing the value as a uint32_t.
 * This means that we exceed the current terminfo defined limits in every way.
 *
 * Type 1 capabilities are defined as:
 * header byte (always 1)
 * name
 * description,
 * cap length, num flags, index, char,
 * cap length, num numbers, index, number,
 * cap length, num strings, index, string,
 * cap length, num undefined caps, name, type (char), flag, number, string
 *
 * Type 2 entries are aliases and defined as:
 * header byte (always 2)
 * 32bit id of the corresponding terminal in the file
 * name
 *
 * Type 3 extends Type 1 so that it can store terminfo numbers
 * as uint32_t. All other numerics are still stored as uint16_t.
 *
 * The database itself is created using cdbw(3) and the numbers are
 * always stored as little endian.
 */

#include <sys/types.h>
#include <assert.h>
#include <limits.h>

#define _TERMINFO
#define TERMINFO_RTYPE_O1	1
#define TERMINFO_ALIAS		2
#define TERMINFO_RTYPE		3

/* , and | are the two print characters now allowed
 * in terminfo aliases or long descriptions.
 * As | is generally used to delimit aliases inside the
 * description, we use a comma. */
#define TERMINFO_VDELIM		','
#define TERMINFO_VDELIMSTR	","

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

typedef struct {
	const char *id;
	char type;
	char flag;
	int num;
	const char *str;
} TERMUSERDEF;

typedef struct {
	int fildes;
	/* We need to expose these so that the macros work */
	const char *name;
	const char *desc;
	signed char *flags;
	int *nums;
	const char **strs;
	/* Storage area for terminfo data */
	char *_area;
	size_t _arealen;
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
	/* aliases of the terminal, | separated */
	const char *_alias;
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

/* libterminfo can compile terminfo strings too */
#define TIC_WARNING	(1 << 0)
#define TIC_DESCRIPTION	(1 << 1)
#define TIC_ALIAS	(1 << 2)
#define TIC_COMMENT	(1 << 3)
#define TIC_EXTRA	(1 << 4)
#define TIC_COMPAT_V1	(1 << 5)

typedef struct {
	char *buf;
	size_t buflen;
	size_t bufpos;
	size_t entries;
} TBUF;

typedef struct {
	char *name;
	char *alias;
	char *desc;
	int  rtype;
	TBUF flags;
	TBUF nums;
	TBUF strs;
	TBUF extras;
} TIC;

#define _ti_numsize(tic) \
    ((tic)->rtype == TERMINFO_RTYPE_O1 ? sizeof(uint16_t) : sizeof(uint32_t))

int _ti_promote(TIC *);
char *_ti_grow_tbuf(TBUF *, size_t);
char *_ti_get_token(char **, char);
const char *_ti_find_cap(TIC *, TBUF *, char,  short);
const char *_ti_find_extra(TIC *, TBUF *, const char *);
char *_ti_getname(int, const char *);
size_t _ti_store_extra(TIC *, int, const char *, char, char, int,
    const char *, size_t, int);
TIC *_ti_compile(char *, int);
ssize_t _ti_flatten(uint8_t **, const TIC *);
void _ti_freetic(TIC *);

int _ti_encode_buf_id_num(TBUF *, int, int, size_t);
int _ti_encode_buf_id_count_str(TBUF *, int, const void *, size_t);
int _ti_encode_buf_id_flags(TBUF *, int, int);

#define TPARM_MAX 9	/* not likely to change */
int _ti_parm_analyse(const char *, int *, int);

static __inline int
_ti_decode_16(const char **cap)
{
	int num = (int16_t)le16dec(*cap);

	*cap += sizeof(uint16_t);
	return num;
}

static __inline int
_ti_decode_32(const char **cap)
{
	int num = (int32_t)le32dec(*cap);

	*cap += sizeof(uint32_t);
	return num;
}

static __inline int
_ti_decode_num(const char **cap, int rtype)
{
	if (rtype == TERMINFO_RTYPE_O1) {
		return _ti_decode_16(cap);
	} else {
		return _ti_decode_32(cap);
	}
}

static __inline void
_ti_encode_16(char **cap, size_t num)
{
	_DIAGASSERT(num <= UINT16_MAX);
	le16enc(*cap, (uint16_t)num);
	*cap += sizeof(uint16_t);
}

static __inline void
_ti_encode_32(char **cap, size_t num)
{
	_DIAGASSERT(num <= UINT32_MAX);
	le32enc(*cap, (uint32_t)num);
	*cap += sizeof(uint32_t);
}

static __inline void
_ti_encode_str(char **cap, const void *buf, size_t len)
{
	memcpy(*cap, buf, len);
	*cap += len;
}

static __inline void
_ti_encode_count_str(char **cap, const char *name, size_t len)
{
	_ti_encode_16(cap, (uint16_t)len);
	if (name == NULL)
		return;
	_ti_encode_str(cap, name, len);
}

static __inline void
_ti_encode_buf_16(TBUF *tbuf, size_t num)
{
	_DIAGASSERT(num <= UINT16_MAX);
	le16enc(tbuf->buf + tbuf->bufpos, (uint16_t)num);
	tbuf->bufpos += sizeof(uint16_t);
}

static __inline void
_ti_encode_buf_32(TBUF *tbuf, size_t num)
{
	_DIAGASSERT(num <= UINT32_MAX);
	le32enc(tbuf->buf + tbuf->bufpos, (uint32_t)num);
	tbuf->bufpos += sizeof(uint32_t);
}

static __inline void
_ti_encode_buf_count_str(TBUF *tbuf, const void *buf, size_t len)
{
	_ti_encode_buf_16(tbuf, len);
	memcpy(tbuf->buf + tbuf->bufpos, buf, len);
	tbuf->bufpos += len;
}

static __inline void
_ti_encode_buf_num(TBUF *tbuf, int num, int rtype)
{
	if (rtype == TERMINFO_RTYPE_O1) {
		if (num > INT16_MAX)
			num = INT16_MAX;
		_ti_encode_buf_16(tbuf, (uint16_t)num);
	} else {
		if (num > INT32_MAX)
			num = INT32_MAX;
		_ti_encode_buf_32(tbuf, (uint32_t)num);
	}
}

#endif
