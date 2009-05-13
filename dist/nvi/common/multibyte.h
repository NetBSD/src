/*	$NetBSD: multibyte.h,v 1.2.2.1 2009/05/13 19:17:36 jym Exp $ */

#ifndef MULTIBYTE_H
#define MULTIBYTE_H

#ifdef USE_WIDECHAR
#include <wchar.h>
#include <wctype.h>

typedef wchar_t		RCHAR_T;
typedef wchar_t		CHAR_T;
#if defined(__NetBSD__)
#define MAX_CHAR_T	0xffffffff
#else
#define MAX_CHAR_T	WCHAR_MAX
#endif
typedef u_int		UCHAR_T;

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
typedef	u_char		CHAR_T;
#define	MAX_CHAR_T	0xff
typedef	u_char		UCHAR_T;

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
