/*	$NetBSD: multibyte.h,v 1.1.1.2.2.2 2008/09/17 04:45:08 wrstuden Exp $ */

#ifndef MULTIBYTE_H
#define MULTIBYTE_H

#ifdef USE_WIDECHAR
#include <wchar.h>
#include <wctype.h>

typedef	wchar_t		RCHAR_T;
#define RCHAR_T_MAX	((1 << 24)-1)
typedef	wchar_t		CHAR_T;
#define	MAX_CHAR_T	0xffffff    /* XXXX */
typedef	u_int		UCHAR_T;
#define RCHAR_BIT	24

#define STRLEN		wcslen
#define STRTOL		wcstol
#define STRTOUL		wcstoul
#define SPRINTF		swprintf
#define STRCMP		wcscmp
#define STRPBRK		wcspbrk
#define TOUPPER		towupper
#define STRSET		wmemset

#define L(ch)		L ## ch

#else
typedef	char		RCHAR_T;
#define RCHAR_T_MAX	CHAR_MAX
typedef	u_char		CHAR_T;
#define	MAX_CHAR_T	0xff
typedef	u_char		UCHAR_T;
#define RCHAR_BIT	CHAR_BIT

#define STRLEN		strlen
#define STRTOL		strtol
#define STRTOUL		strtoul
#define SPRINTF		snprintf
#define STRCMP		strcmp
#define STRPBRK		strpbrk
#define TOUPPER		toupper
#define STRSET		memset

#define L(ch)		ch

#endif

#define MEMCMP(to, from, n) 						    \
	memcmp(to, from, (n) * sizeof(*(to)))
#define	MEMMOVE(p, t, len)	memmove(p, t, (len) * sizeof(*(p)))
#define	MEMCPY(p, t, len)	memcpy(p, t, (len) * sizeof(*(p)))
#define SIZE(w)		(sizeof(w)/sizeof(*w))

#endif
