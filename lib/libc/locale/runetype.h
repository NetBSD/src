/*	$NetBSD: runetype.h,v 1.5 2001/03/26 19:55:43 tshiozak Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 *	@(#)runetype.h	8.1 (Berkeley) 6/2/93
 */

#ifndef	_RUNETYPE_H_
#define	_RUNETYPE_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>

#include <machine/int_types.h>
typedef __uint64_t	__runepad_t;

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_WCHAR_T_
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#ifdef	_BSD_WINT_T_
typedef	_BSD_WINT_T_	wint_t;
#undef	_BSD_WINT_T_
#endif

#ifdef	_BSD_MBSTATE_T_
typedef	_BSD_MBSTATE_T_	mbstate_t;
#undef	_BSD_MBSTATE_T_
#endif

#ifdef	_BSD_RUNE_T_
typedef	_BSD_RUNE_T_	rune_t;
#undef	_BSD_RUNE_T_
#endif

extern size_t __mb_len_max_runtime;
#define __MB_LEN_MAX_RUNTIME	__mb_len_max_runtime


#define	_CACHED_RUNES	(1 << 8)	/* Must be a power of 2 */


/*
 * The lower 8 bits of runetype[] contain the digit value of the rune.
 */
typedef __uint32_t _RuneType;
#define	_CTYPE_A	0x00000100L	/* Alpha */
#define	_CTYPE_C	0x00000200L	/* Control */
#define	_CTYPE_D	0x00000400L	/* Digit */
#define	_CTYPE_G	0x00000800L	/* Graph */
#define	_CTYPE_L	0x00001000L	/* Lower */
#define	_CTYPE_P	0x00002000L	/* Punct */
#define	_CTYPE_S	0x00004000L	/* Space */
#define	_CTYPE_U	0x00008000L	/* Upper */
#define	_CTYPE_X	0x00010000L	/* X digit */
#define	_CTYPE_B	0x00020000L	/* Blank */
#define	_CTYPE_R	0x00040000L	/* Print */
#define	_CTYPE_I	0x00080000L	/* Ideogram */
#define	_CTYPE_T	0x00100000L	/* Special */
#define	_CTYPE_Q	0x00200000L	/* Phonogram */
#define	_CTYPE_SWM	0xc0000000L	/* Mask to get screen width data */
#define	_CTYPE_SWS	30		/* Bits to shift to get width */
#define	_CTYPE_SW0	0x00000000L	/* 0 width character */
#define	_CTYPE_SW1	0x40000000L	/* 1 width character */
#define	_CTYPE_SW2	0x80000000L	/* 2 width character */
#define	_CTYPE_SW3	0xc0000000L	/* 3 width character */


/*
 * rune file format.  network endian.
 */
typedef struct {
	__int32_t	__min;		/* First rune of the range */
	__int32_t	__max;		/* Last rune (inclusive) of the range */
	__int32_t	__map;		/* What first maps to in maps */
	__uint32_t	__pad1;		/* backward compatibility */
	__runepad_t	__pad2;		/* backward compatibility */
} _FileRuneEntry __attribute__((__packed__));


typedef struct {
	__uint32_t	__nranges;	/* Number of ranges stored */
	__uint32_t	__pad1;		/* backward compatibility */
	__runepad_t	__pad2;		/* backward compatibility */
} _FileRuneRange __attribute__((__packed__));


typedef struct {
	char		__magic[8];	/* Magic saying what version we are */
	char		__encoding[32];	/* ASCII name of this encoding */

	__runepad_t	__pad1;		/* backward compatibility */
	__runepad_t	__pad2;		/* backward compatibility */
	__int32_t	__invalid_rune;
	__uint32_t	__pad3;		/* backward compatibility */

	_RuneType	__runetype[_CACHED_RUNES];
	__int32_t	__maplower[_CACHED_RUNES];
	__int32_t	__mapupper[_CACHED_RUNES];

	/*
	 * The following are to deal with Runes larger than _CACHED_RUNES - 1.
	 * Their data is actually contiguous with this structure so as to make
	 * it easier to read/write from/to disk.
	 */
	_FileRuneRange	__runetype_ext;
	_FileRuneRange	__maplower_ext;
	_FileRuneRange	__mapupper_ext;

	__runepad_t	__pad4;		/* backward compatibility */
	__int32_t	__variable_len;	/* how long that data is */
	__uint32_t	__pad5;		/* backward compatibility */

	/* variable size data follows */
} _FileRuneLocale __attribute__((__packed__));


/*
 * expanded rune locale declaration.  local to the host.  host endian.
 */
typedef struct {
	rune_t		__min;		/* First rune of the range */
	rune_t		__max;		/* Last rune (inclusive) of the range */
	rune_t		__map;		/* What first maps to in maps */
	_RuneType	*__rune_types;	/* Array of types in range */
} _RuneEntry;


typedef struct {
	__uint32_t	__nranges;	/* Number of ranges stored */
	_RuneEntry	*__rune_ranges;
} _RuneRange;


struct _RuneLocale;
typedef struct _RuneState {
	size_t		__sizestate;
	void		(*__initstate) __P((struct _RuneLocale *, void *));
	void		(*__packstate)
		__P((struct _RuneLocale *, mbstate_t *, void *));
	void		(*__unpackstate)
		__P((struct _RuneLocale *, void *, const mbstate_t *));
} _RuneState;


typedef size_t (*__rune_mbrtowc_t) __P((struct _RuneLocale *, rune_t *,
	const char *, size_t, void *));
typedef size_t (*__rune_wcrtomb_t) __P((struct _RuneLocale *, char *, size_t,
	const rune_t, void *));

typedef struct _RuneLocale {
	/*
	 * copied from _FileRuneLocale
	 */
	char		__magic[8];	/* Magic saying what version we are */
	char		__encoding[32];	/* ASCII name of this encoding */
	rune_t		__invalid_rune;
	_RuneType	__runetype[_CACHED_RUNES];
	rune_t		__maplower[_CACHED_RUNES];
	rune_t		__mapupper[_CACHED_RUNES];
	_RuneRange	__runetype_ext;
	_RuneRange	__maplower_ext;
	_RuneRange	__mapupper_ext;

	/*
	 * __variable_len is copied from _FileRuneLocale
	 */
	/* Data which depends on the encoding */
	size_t		__variable_len;	/* how long that data is */
	void *		__rune_variable;

	/*
	 * the following portion is generated on the fly
	 */
	__rune_mbrtowc_t __rune_mbrtowc;
	__rune_wcrtomb_t __rune_wcrtomb;

	struct _RuneState  *__rune_RuneState;
	size_t		__rune_mb_cur_max;

	void		*__rune_RuneModule;
	char		*__rune_codeset;
} _RuneLocale;

/* magic number for LC_CTYPE (rune)locale declaration */
#define	_RUNE_MAGIC_1	"RuneCT10"	/* Indicates version 0 of RuneLocale */

/* magic string for dynamic link module - type should be like "LC_CTYPE" */
#define	_RUNE_MODULE_1(type)	"RuneModule10." type

/* codeset tag */
#define _RUNE_CODESET "CODESET="

extern _RuneLocale _DefaultRuneLocale;
extern _RuneLocale *_CurrentRuneLocale;
extern void **_StreamStateTable;
extern char *_PathLocale;

#endif	/* !_RUNETYPE_H_ */
