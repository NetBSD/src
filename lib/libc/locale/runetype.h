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

#ifdef	_BSD_RUNE_T_
#ifdef _COMPAT_RUNE
typedef	_BSD_RUNE_T_	rune_t;
#endif
typedef	_BSD_RUNE_T_	_rune_t;
#undef	_BSD_RUNE_T_
#endif

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

#define	_CACHED_RUNES	(1 <<8 )	/* Must be a power of 2 */
#define	_CRMASK		(~(_CACHED_RUNES - 1))

/*
 * The lower 8 bits of runetype[] contain the digit value of the rune.
 */
typedef struct {
	_rune_t		__min;		/* First rune of the range */
	_rune_t		__max;		/* Last rune (inclusive) of the range */
	_rune_t		__map;		/* What first maps to in maps */
	unsigned long	*__types;	/* Array of types in range */
} _RuneEntry;

typedef struct {
	int		__nranges;	/* Number of ranges stored */
	_RuneEntry	*__ranges;	/* Pointer to the ranges */
} _RuneRange;

typedef struct {
	char		__magic[8];	/* Magic saying what version we are */
	char		__encoding[32];	/* ASCII name of this encoding */

	_rune_t		(*___sgetrune)
			__P((const char *, size_t, char const **, void *));
	int		(*___sputrune)
			__P((_rune_t, char *, size_t, char **, void *));
	_rune_t		__invalid_rune;

	unsigned long	__runetype[_CACHED_RUNES];
#if defined(__FreeBSD__)
#define	_A	0x00000100L		/* Alpha */
#define	_C	0x00000200L		/* Control */
#define	_D	0x00000400L		/* Digit */
#define	_G	0x00000800L		/* Graph */
#define	_L	0x00001000L		/* Lower */
#define	_P	0x00002000L		/* Punct */
#define	_S	0x00004000L		/* Space */
#define	_U	0x00008000L		/* Upper */
#define	_X	0x00010000L		/* X digit */
#define	_B	0x00020000L		/* Blank */
#define	_R	0x00040000L		/* Print */
#define	_I	0x00080000L		/* Ideogram */
#define	_T	0x00100000L		/* Special */
#define	_Q	0x00200000L		/* Phonogram */
#define	_SWM	0xc0000000L		/* Mask to get screen width data */
#define	_SWS	30			/* Bits to shift to get width */
#define	_SW0	0x00000000L		/* 0 width character */
#define	_SW1	0x40000000L		/* 1 width character */
#define	_SW2	0x80000000L		/* 2 width character */
#define	_SW3	0xc0000000L		/* 3 width character */
#else  /* __FreeBSD__ */
#define	___A	0x00000100L		/* Alpha */
#define	___C	0x00000200L		/* Control */
#define	___D	0x00000400L		/* Digit */
#define	___G	0x00000800L		/* Graph */
#define	___L	0x00001000L		/* Lower */
#define	___P	0x00002000L		/* Punct */
#define	___S	0x00004000L		/* Space */
#define	___U	0x00008000L		/* Upper */
#define	___X	0x00010000L		/* X digit */
#define	___B	0x00020000L		/* Blank */
#define	___R	0x00040000L		/* Print */
#define	___I	0x00080000L		/* Ideogram */
#define	___T	0x00100000L		/* Special */
#define	___Q	0x00200000L		/* Phonogram */
#define	___SWM	0xc0000000L		/* Mask to get screen width data */
#define	___SWS	30			/* Bits to shift to get width */
#define	___SW0	0x00000000L		/* 0 width character */
#define	___SW1	0x40000000L		/* 1 width character */
#define	___SW2	0x80000000L		/* 2 width character */
#define	___SW3	0xc0000000L		/* 3 width character */
#endif /* __FreeBSD__ */
	_rune_t		__maplower[_CACHED_RUNES];
	_rune_t		__mapupper[_CACHED_RUNES];

	/*
	 * The following are to deal with Runes larger than _CACHED_RUNES - 1.
	 * Their data is actually contiguous with this structure so as to make
	 * it easier to read/write from/to disk.
	 */
	_RuneRange	__runetype_ext;
	_RuneRange	__maplower_ext;
	_RuneRange	__mapupper_ext;

	void		*__variable;	/* Data which depends on the encoding */
	int		__variable_len;	/* how long that data is */
} _RuneLocale;



typedef struct {
	size_t		__sizestate;
	void		(*__initstate) __P((void *));
	void		(*__packstate) __P((mbstate_t *, void *));
	void		(*__unpackstate) __P((void *, const mbstate_t *));
} _RuneState;


#define	_RUNE_MAGIC_1	"RuneMagi"	/* Indicates version 0 of RuneLocale */

extern _RuneLocale _DefaultRuneLocale;
extern _RuneLocale *_CurrentRuneLocale;
extern _RuneState _DefaultRuneState;
extern _RuneState *_CurrentRuneState;
extern void **_StreamStateTable;


#define	__istype(c,f)	(!!__maskrune((c),(f)))

#ifdef _COMPAT_RUNE
/* See comments in <machine/ansi.h> about _BSD_CT_RUNE_T_. */
__BEGIN_DECLS
unsigned long	___runetype __P((_BSD_CT_RUNE_T_));
_BSD_CT_RUNE_T_	___tolower __P((_BSD_CT_RUNE_T_));
_BSD_CT_RUNE_T_	___toupper __P((_BSD_CT_RUNE_T_));
__END_DECLS
#endif

/*
 * _EXTERNALIZE_CTYPE_INLINES_ is defined in locale/nomacros.c to tell us
 * to generate code for extern versions of all our inline functions.
 */
#ifdef _EXTERNALIZE_CTYPE_INLINES_
#define	_USE_CTYPE_INLINE_
#define	static
#define	__inline
#endif

/*
 * Use inline functions if we are allowed to and the compiler supports them.
 */
#if !defined(_DONT_USE_CTYPE_INLINE_) && \
    (defined(_USE_CTYPE_INLINE_) || defined(__GNUC__) || defined(__cplusplus))
#ifdef _COMPAT_RUNE
static __inline int
__maskrune(_BSD_CT_RUNE_T_ _c, unsigned long _f)
#else
static __inline int
__maskrune(int _c, unsigned long _f)
#endif
{
#ifdef _COMPAT_RUNE
	return ((_c < 0 || _c >= _CACHED_RUNES) ? ___runetype(_c) :
		_CurrentRuneLocale->__runetype[_c]) & _f;
#else
	return ((_c < 0 || _c >= _CACHED_RUNES) ? 0 :
		_CurrentRuneLocale->__runetype[_c]) & _f;
#endif
}

#ifdef _COMPAT_RUNE
static __inline int
__isctype(_BSD_CT_RUNE_T_ _c, unsigned long _f)
#else
static __inline int
__isctype(int _c, unsigned long _f)
#endif
{
	return (_c < 0 || _c >= _CACHED_RUNES) ? 0 :
	       !!(_DefaultRuneLocale.__runetype[_c] & _f);
}

#ifdef _COMPAT_RUNE
static __inline _BSD_CT_RUNE_T_
__toupper(_BSD_CT_RUNE_T_ _c)
#else
static __inline int
__toupper(int _c)
#endif
{
#ifdef _COMPAT_RUNE
	return (_c < 0 || _c >= _CACHED_RUNES) ? ___toupper(_c) :
	       _CurrentRuneLocale->__mapupper[_c];
#else
	return (_c < 0 || _c >= _CACHED_RUNES) ? 0 :
	       _CurrentRuneLocale->__mapupper[_c];
#endif
}

#ifdef _COMPAT_RUNE
static __inline _BSD_CT_RUNE_T_
__tolower(_BSD_CT_RUNE_T_ _c)
#else
static __inline int
__tolower(int _c)
#endif
{
#ifdef _COMPAT_RUNE
	return (_c < 0 || _c >= _CACHED_RUNES) ? ___tolower(_c) :
	       _CurrentRuneLocale->__maplower[_c];
#else
	return (_c < 0 || _c >= _CACHED_RUNES) ? 0 :
	       _CurrentRuneLocale->__maplower[_c];
#endif
}

#else /* not using inlines */

__BEGIN_DECLS
#ifdef _COMPAT_RUNE
int		__maskrune __P((_BSD_CT_RUNE_T_, unsigned long));
int		__isctype __P((_BSD_CT_RUNE_T_, unsigned long));
_BSD_CT_RUNE_T_	__toupper __P((_BSD_CT_RUNE_T_));
_BSD_CT_RUNE_T_	__tolower __P((_BSD_CT_RUNE_T_));
#else
int	__maskrune __P((int, unsigned long));
int	__isctype __P((int, unsigned long));
int	__toupper __P((int));
int	__tolower __P((int));
#endif
__END_DECLS
#endif /* using inlines */



#define	__istype_w(c,f)	(!!__maskrune_w((c),(f)))

/* See comments in <machine/ansi.h> about _BSD_CT_RUNE_T_. */
__BEGIN_DECLS
unsigned long	___runetype_mb __P((wchar_t));
_BSD_CT_RUNE_T_	___tolower_mb __P((_BSD_CT_RUNE_T_));
_BSD_CT_RUNE_T_	___toupper_mb __P((_BSD_CT_RUNE_T_));
__END_DECLS

/*
 * _EXTERNALIZE_CTYPE_INLINES_ is defined in locale/nomacros.c to tell us
 * to generate code for extern versions of all our inline functions.
 */
#ifdef _EXTERNALIZE_CTYPE_INLINES_
#define	_USE_CTYPE_INLINE_
#define	static
#define	__inline
#endif

/*
 * Use inline functions if we are allowed to and the compiler supports them.
 */
#if !defined(_DONT_USE_CTYPE_INLINE_) && \
    (defined(_USE_CTYPE_INLINE_) || defined(__GNUC__) || defined(__cplusplus))
#ifndef __MASKRUNE_W_
#define __MASKRUNE_W_
static __inline int
__maskrune_w(wint_t _c, unsigned long _f)
{
	return ((_c < 0 || _c >= _CACHED_RUNES) ? ___runetype_mb(_c) :
		_CurrentRuneLocale->__runetype[_c]) & _f;
}

static __inline int
__isctype_w(wint_t _c, unsigned long _f)
{
	return (_c < 0 || _c >= _CACHED_RUNES) ? 0 :
	       !!(_DefaultRuneLocale.__runetype[_c] & _f);
}

static __inline wint_t
__toupper_w(wint_t _c)
{
	return (_c < 0 || _c >= _CACHED_RUNES) ? ___toupper_mb(_c) :
	       _CurrentRuneLocale->__mapupper[_c];
}

static __inline wint_t
__tolower_w(wint_t _c)
{
	return (_c < 0 || _c >= _CACHED_RUNES) ? ___tolower_mb(_c) :
	       _CurrentRuneLocale->__maplower[_c];
}
#endif /* __MASKRUNE_W_ */
#else /* not using inlines */

__BEGIN_DECLS
int		__maskrune_w __P((wint_t, unsigned long));
int		__isctype_w __P((wint_t, unsigned long));
wint_t		__toupper_w __P((wint_t));
wint_t		__tolower_w __P((wint_t));
__END_DECLS
#endif /* using inlines */

#endif	/* !_RUNETYPE_H_ */
