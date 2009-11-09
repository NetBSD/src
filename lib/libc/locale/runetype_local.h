/*	$NetBSD: runetype_local.h,v 1.3 2009/11/09 14:17:47 tnozaki Exp $	*/

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
 *	@(#)runetype.h	8.1 (Berkeley) 6/2/93
 */

#ifndef	_RUNETYPE_LOCAL_H_
#define	_RUNETYPE_LOCAL_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <ctype.h>

/* for cross host tools on older systems */
#ifndef UINT32_C
/* assumes sizeof(unsigned int)>=4 */
#define UINT32_C(c) ((uint32_t)(c##U))
#endif

typedef uint32_t	__nbrune_t;
typedef uint64_t	__runepad_t;

#define	_NB_CACHED_RUNES	(1 << 8)	/* Must be a power of 2 */
#define _NB_RUNE_ISCACHED(c)	((c)>=0 && (c)<_CACHED_RUNES)

#define _NB_DEFAULT_INVALID_RUNE ((__nbrune_t)-3)

/* XXX FIXME */
#if defined(__NetBSD__) && defined(_CTYPE_A)
#define _NB_CTYPE_A	_CTYPE_A
#define _NB_CTYPE_C	_CTYPE_C
#define _NB_CTYPE_D	_CTYPE_D
#define _NB_CTYPE_G	_CTYPE_G
#define _NB_CTYPE_L	_CTYPE_L
#define _NB_CTYPE_P	_CTYPE_P
#define _NB_CTYPE_S	_CTYPE_S
#define _NB_CTYPE_U	_CTYPE_U
#define _NB_CTYPE_X	_CTYPE_X
#define _NB_CTYPE_B	_CTYPE_B
#define _NB_CTYPE_R	_CTYPE_R
#define _NB_CTYPE_I	_CTYPE_I
#define _NB_CTYPE_T	_CTYPE_T
#define _NB_CTYPE_Q	_CTYPE_Q
#else
#define _NB_CTYPE_A	0x0001
#define _NB_CTYPE_C	0x0002
#define _NB_CTYPE_D	0x0004
#define _NB_CTYPE_G	0x0008
#define _NB_CTYPE_L	0x0010
#define _NB_CTYPE_P	0x0020
#define _NB_CTYPE_S	0x0040
#define _NB_CTYPE_U	0x0080
#define _NB_CTYPE_X	0x0100
#define _NB_CTYPE_B	0x0200
#define _NB_CTYPE_R	0x0400
#define _NB_CTYPE_I	0x0800
#define _NB_CTYPE_T	0x1000
#define _NB_CTYPE_Q	0x2000
#endif

/*
 * The lower 8 bits of runetype[] contain the digit value of the rune.
 */
typedef uint32_t _RuneType;
#define	_RUNETYPE_A	UINT32_C(_NB_CTYPE_A << 8) /* Alpha */
#define	_RUNETYPE_C	UINT32_C(_NB_CTYPE_C << 8) /* Control */
#define	_RUNETYPE_D	UINT32_C(_NB_CTYPE_D << 8) /* Digit */
#define	_RUNETYPE_G	UINT32_C(_NB_CTYPE_G << 8) /* Graph */
#define	_RUNETYPE_L	UINT32_C(_NB_CTYPE_L << 8) /* Lower */
#define	_RUNETYPE_P	UINT32_C(_NB_CTYPE_P << 8) /* Punct */
#define	_RUNETYPE_S	UINT32_C(_NB_CTYPE_S << 8) /* Space */
#define	_RUNETYPE_U	UINT32_C(_NB_CTYPE_U << 8) /* Upper */
#define	_RUNETYPE_X	UINT32_C(_NB_CTYPE_X << 8) /* X digit */
#define	_RUNETYPE_B	UINT32_C(_NB_CTYPE_B << 8) /* Blank */
#define	_RUNETYPE_R	UINT32_C(_NB_CTYPE_R << 8) /* Print */
#define	_RUNETYPE_I	UINT32_C(_NB_CTYPE_I << 8) /* Ideogram */
#define	_RUNETYPE_T	UINT32_C(_NB_CTYPE_T << 8) /* Special */
#define	_RUNETYPE_Q	UINT32_C(_NB_CTYPE_Q << 8) /* Phonogram */
#define	_RUNETYPE_SWM	UINT32_C(0xc0000000)/* Mask to get screen width data */
#define	_RUNETYPE_SWS	30		/* Bits to shift to get width */
#define	_RUNETYPE_SW0	UINT32_C(0x20000000)	/* 0 width character */
#define	_RUNETYPE_SW1	UINT32_C(0x40000000)	/* 1 width character */
#define	_RUNETYPE_SW2	UINT32_C(0x80000000)	/* 2 width character */
#define	_RUNETYPE_SW3	UINT32_C(0xc0000000)	/* 3 width character */


/*
 * rune file format.  network endian.
 */
typedef struct {
	int32_t		fre_min;	/* First rune of the range */
	int32_t		fre_max;	/* Last rune (inclusive) of the range */
	int32_t		fre_map;	/* What first maps to in maps */
	uint32_t	fre_pad1;	/* backward compatibility */
	__runepad_t	fre_pad2;	/* backward compatibility */
} __packed _FileRuneEntry;


typedef struct {
	uint32_t	frr_nranges;	/* Number of ranges stored */
	uint32_t	frr_pad1;	/* backward compatibility */
	__runepad_t	frr_pad2;	/* backward compatibility */
} __packed _FileRuneRange;


typedef struct {
	char		frl_magic[8];	/* Magic saying what version we are */
	char		frl_encoding[32];/* ASCII name of this encoding */

	__runepad_t	frl_pad1;	/* backward compatibility */
	__runepad_t	frl_pad2;	/* backward compatibility */
	int32_t		frl_invalid_rune;
	uint32_t	frl_pad3;	/* backward compatibility */

	_RuneType	frl_runetype[_NB_CACHED_RUNES];
	int32_t		frl_maplower[_NB_CACHED_RUNES];
	int32_t		frl_mapupper[_NB_CACHED_RUNES];

	/*
	 * The following are to deal with Runes larger than _CACHED_RUNES - 1.
	 * Their data is actually contiguous with this structure so as to make
	 * it easier to read/write from/to disk.
	 */
	_FileRuneRange	frl_runetype_ext;
	_FileRuneRange	frl_maplower_ext;
	_FileRuneRange	frl_mapupper_ext;

	__runepad_t	frl_pad4;	/* backward compatibility */
	int32_t		frl_variable_len;/* how long that data is */
	uint32_t	frl_pad5;	/* backward compatibility */

	/* variable size data follows */
} __packed _FileRuneLocale;


/*
 * expanded rune locale declaration.  local to the host.  host endian.
 */
typedef struct {
	__nbrune_t	re_min;		/* First rune of the range */
	__nbrune_t	re_max;		/* Last rune (inclusive) of the range */
	__nbrune_t	re_map;		/* What first maps to in maps */
	_RuneType	*re_rune_types;	/* Array of types in range */
} _NBRuneEntry;


typedef struct {
	uint32_t	rr_nranges;	/* Number of ranges stored */
	_NBRuneEntry	*rr_rune_ranges;
} _NBRuneRange;


/*
 * wctrans stuffs.
 */
typedef struct _WCTransEntry {
	const char	*te_name;
	__nbrune_t	*te_cached;
	_NBRuneRange	*te_extmap;
} _WCTransEntry;
#define _WCTRANS_INDEX_LOWER	0
#define _WCTRANS_INDEX_UPPER	1
#define _WCTRANS_NINDEXES	2

/*
 * wctype stuffs.
 */
typedef struct _WCTypeEntry {
	const char	*te_name;
	_RuneType	te_mask;
} _WCTypeEntry;
#define _WCTYPE_INDEX_ALNUM	0
#define _WCTYPE_INDEX_ALPHA	1
#define _WCTYPE_INDEX_BLANK	2
#define _WCTYPE_INDEX_CNTRL	3
#define _WCTYPE_INDEX_DIGIT	4
#define _WCTYPE_INDEX_GRAPH	5
#define _WCTYPE_INDEX_LOWER	6
#define _WCTYPE_INDEX_PRINT	7
#define _WCTYPE_INDEX_PUNCT	8
#define _WCTYPE_INDEX_SPACE	9
#define _WCTYPE_INDEX_UPPER	10
#define _WCTYPE_INDEX_XDIGIT	11
#define _WCTYPE_NINDEXES	12

/*
 * ctype stuffs
 */

typedef struct _NBRuneLocale {
	/*
	 * copied from _FileRuneLocale
	 */
	char		rl_magic[8];	/* Magic saying what version we are */
	char		rl_encoding[32];/* ASCII name of this encoding */
	__nbrune_t	rl_invalid_rune;
	_RuneType	rl_runetype[_NB_CACHED_RUNES];
	__nbrune_t	rl_maplower[_NB_CACHED_RUNES];
	__nbrune_t	rl_mapupper[_NB_CACHED_RUNES];
	_NBRuneRange	rl_runetype_ext;
	_NBRuneRange	rl_maplower_ext;
	_NBRuneRange	rl_mapupper_ext;

	void		*rl_variable;
	size_t		rl_variable_len;

	/*
	 * the following portion is generated on the fly
	 */
	const char			*rl_codeset;
	struct _citrus_ctype_rec	*rl_citrus_ctype;
	_WCTransEntry			rl_wctrans[_WCTRANS_NINDEXES];
	_WCTypeEntry			rl_wctype[_WCTYPE_NINDEXES];

#if defined(__LIBC13_SOURCE__)
	const unsigned short		*rl_ctype_tab;
#else
	const unsigned char		*rl_ctype_tab;
#endif
	const short			*rl_tolower_tab;
	const short			*rl_toupper_tab;
#if !defined(__LIBC13_SOURCE__)
	const unsigned short		*rl_ctype50_tab;
#endif
} _NBRuneLocale;


/* magic number for LC_CTYPE (rune)locale declaration */
#define	_NB_RUNE_MAGIC_1 "RuneCT10"	/* Indicates version 0 of RuneLocale */

/* magic string for dynamic link module - type should be like "LC_CTYPE" */
#define	_NB_RUNE_MODULE_1(type)	"RuneModule10." type

/* codeset tag */
#define _NB_RUNE_CODESET "CODESET="

#endif	/* !_RUNETYPE_LOCAL_H_ */
