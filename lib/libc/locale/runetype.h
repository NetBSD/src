/*	$NetBSD: runetype.h,v 1.11 2003/03/02 22:18:15 tshiozak Exp $	*/

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
#include <sys/types.h>

#ifdef _BSD_RUNE_T_
typedef	_BSD_RUNE_T_	rune_t;
#undef _BSD_RUNE_T_
#endif

typedef uint64_t	__runepad_t;

extern size_t __mb_len_max_runtime;
#define __MB_LEN_MAX_RUNTIME	__mb_len_max_runtime


#define	_CACHED_RUNES		(1 << 8)	/* Must be a power of 2 */
#define _RUNE_ISCACHED(c)	((c)>=0 || (c)<_CACHED_RUNES)


/*
 * The lower 8 bits of runetype[] contain the digit value of the rune.
 */
typedef uint32_t _RuneType;
#define	_CTYPE_A	0x00000100U	/* Alpha */
#define	_CTYPE_C	0x00000200U	/* Control */
#define	_CTYPE_D	0x00000400U	/* Digit */
#define	_CTYPE_G	0x00000800U	/* Graph */
#define	_CTYPE_L	0x00001000U	/* Lower */
#define	_CTYPE_P	0x00002000U	/* Punct */
#define	_CTYPE_S	0x00004000U	/* Space */
#define	_CTYPE_U	0x00008000U	/* Upper */
#define	_CTYPE_X	0x00010000U	/* X digit */
#define	_CTYPE_B	0x00020000U	/* Blank */
#define	_CTYPE_R	0x00040000U	/* Print */
#define	_CTYPE_I	0x00080000U	/* Ideogram */
#define	_CTYPE_T	0x00100000U	/* Special */
#define	_CTYPE_Q	0x00200000U	/* Phonogram */
#define	_CTYPE_SWM	0xc0000000U	/* Mask to get screen width data */
#define	_CTYPE_SWS	30		/* Bits to shift to get width */
#define	_CTYPE_SW0	0x00000000U	/* 0 width character */
#define	_CTYPE_SW1	0x40000000U	/* 1 width character */
#define	_CTYPE_SW2	0x80000000U	/* 2 width character */
#define	_CTYPE_SW3	0xc0000000U	/* 3 width character */


/*
 * rune file format.  network endian.
 */
typedef struct {
	int32_t		fre_min;	/* First rune of the range */
	int32_t		fre_max;	/* Last rune (inclusive) of the range */
	int32_t		fre_map;	/* What first maps to in maps */
	uint32_t	fre_pad1;	/* backward compatibility */
	__runepad_t	fre_pad2;	/* backward compatibility */
} __attribute__((__packed__)) _FileRuneEntry;


typedef struct {
	uint32_t	frr_nranges;	/* Number of ranges stored */
	uint32_t	frr_pad1;	/* backward compatibility */
	__runepad_t	frr_pad2;	/* backward compatibility */
} __attribute__((__packed__)) _FileRuneRange;


typedef struct {
	char		frl_magic[8];	/* Magic saying what version we are */
	char		frl_encoding[32];/* ASCII name of this encoding */

	__runepad_t	frl_pad1;	/* backward compatibility */
	__runepad_t	frl_pad2;	/* backward compatibility */
	int32_t		frl_invalid_rune;
	uint32_t	frl_pad3;	/* backward compatibility */

	_RuneType	frl_runetype[_CACHED_RUNES];
	int32_t		frl_maplower[_CACHED_RUNES];
	int32_t		frl_mapupper[_CACHED_RUNES];

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
} __attribute__((__packed__)) _FileRuneLocale;


/*
 * expanded rune locale declaration.  local to the host.  host endian.
 */
typedef struct {
	rune_t		re_min;		/* First rune of the range */
	rune_t		re_max;		/* Last rune (inclusive) of the range */
	rune_t		re_map;		/* What first maps to in maps */
	_RuneType	*re_rune_types;	/* Array of types in range */
} _RuneEntry;


typedef struct {
	uint32_t	rr_nranges;	/* Number of ranges stored */
	_RuneEntry	*rr_rune_ranges;
} _RuneRange;


/*
 * wctrans stuffs.
 */
typedef struct _WCTransEntry {
	char		*te_name;
	rune_t		*te_cached;
	_RuneRange	*te_extmap;
} _WCTransEntry;
#define _WCTRANS_INDEX_LOWER	0
#define _WCTRANS_INDEX_UPPER	1
#define _WCTRANS_NINDEXES	2

/*
 * wctype stuffs.
 */
typedef struct _WCTypeEntry {
	char		*te_name;
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

typedef struct _RuneLocale {
	/*
	 * copied from _FileRuneLocale
	 */
	char		rl_magic[8];	/* Magic saying what version we are */
	char		rl_encoding[32];/* ASCII name of this encoding */
	rune_t		rl_invalid_rune;
	_RuneType	rl_runetype[_CACHED_RUNES];
	rune_t		rl_maplower[_CACHED_RUNES];
	rune_t		rl_mapupper[_CACHED_RUNES];
	_RuneRange	rl_runetype_ext;
	_RuneRange	rl_maplower_ext;
	_RuneRange	rl_mapupper_ext;

	void		*rl_variable;
	size_t		rl_variable_len;

	/*
	 * the following portion is generated on the fly
	 */
	char				*rl_codeset;
	struct _citrus_ctype_rec	*rl_citrus_ctype;
	_WCTransEntry			rl_wctrans[_WCTRANS_NINDEXES];
	_WCTypeEntry			rl_wctype[_WCTYPE_NINDEXES];
} _RuneLocale;


/* magic number for LC_CTYPE (rune)locale declaration */
#define	_RUNE_MAGIC_1	"RuneCT10"	/* Indicates version 0 of RuneLocale */

/* magic string for dynamic link module - type should be like "LC_CTYPE" */
#define	_RUNE_MODULE_1(type)	"RuneModule10." type

/* codeset tag */
#define _RUNE_CODESET "CODESET="

extern _RuneLocale _DefaultRuneLocale;
extern _RuneLocale *_CurrentRuneLocale;
extern char *_PathLocale;

#endif	/* !_RUNETYPE_H_ */
