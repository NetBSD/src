/*	$NetBSD: stringops.h,v 1.4.2.1 2023/12/25 12:43:38 martin Exp $	*/

#ifndef _STRINGOPS_H_INCLUDED_
#define _STRINGOPS_H_INCLUDED_

/*++
/* NAME
/*	stringops 3h
/* SUMMARY
/*	string operations
/* SYNOPSIS
/*	#include <stringops.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern int util_utf8_enable;
extern char *printable_except(char *, int, const char *);
extern char *neuter(char *, const char *, int);
extern char *lowercase(char *);
extern char *casefoldx(int, VSTRING *, const char *, ssize_t);
extern char *uppercase(char *);
extern char *skipblanks(const char *);
extern char *trimblanks(char *, ssize_t);
extern char *concatenate(const char *,...);
extern char *mystrtok(char **, const char *);
extern char *mystrtokq(char **, const char *, const char *);
extern char *mystrtokdq(char **, const char *);
extern char *mystrtok_cw(char **, const char *, const char *);
extern char *mystrtokq_cw(char **, const char *, const char *, const char *);
extern char *mystrtokdq_cw(char **, const char *, const char *);
extern char *translit(char *, const char *, const char *);

#define mystrtok(cp, sp) mystrtok_cw((cp), (sp), (char *) 0)
#define mystrtokq(cp, sp, pp) mystrtokq_cw((cp), (sp), (pp), (char *) 0)
#define mystrtokdq(cp, sp) mystrtokdq_cw((cp), (sp), (char *) 0)

#define printable(string, replacement) \
	printable_except((string), (replacement), (char *) 0)

#ifndef HAVE_BASENAME
#define basename postfix_basename
extern char *basename(const char *);

#endif
extern char *sane_basename(VSTRING *, const char *);
extern char *sane_dirname(VSTRING *, const char *);
extern VSTRING *unescape(VSTRING *, const char *);
extern VSTRING *escape(VSTRING *, const char *, ssize_t);
extern int alldig(const char *);
extern int allalnum(const char *);
extern int allprint(const char *);
extern int allspace(const char *);
extern int allascii_len(const char *, ssize_t);
extern const char *WARN_UNUSED_RESULT split_nameval(char *, char **, char **);
extern const char *WARN_UNUSED_RESULT split_qnameval(char *, char **, char **);
extern int valid_utf8_string(const char *, ssize_t);
extern size_t balpar(const char *, const char *);
extern char *WARN_UNUSED_RESULT extpar(char **, const char *, int);
extern int strcasecmp_utf8x(int, const char *, const char *);
extern int strncasecmp_utf8x(int, const char *, const char *, ssize_t);

#define EXTPAR_FLAG_NONE	(0)
#define EXTPAR_FLAG_STRIP	(1<<0)	/* "{ text }" -> "text" */
#define EXTPAR_FLAG_EXTRACT	(1<<1)	/* hint from caller's caller */

#define CASEF_FLAG_UTF8		(1<<0)
#define CASEF_FLAG_APPEND	(1<<1)

 /*
  * Convenience wrappers for most-common use cases.
  */
#define allascii(s)	allascii_len((s), -1)
#define casefold(dst, src) \
    casefoldx(util_utf8_enable ? CASEF_FLAG_UTF8 : 0, (dst), (src), -1)
#define casefold_len(dst, src, len) \
    casefoldx(util_utf8_enable ? CASEF_FLAG_UTF8 : 0, (dst), (src), (len))
#define casefold_append(dst, src) \
    casefoldx((util_utf8_enable ? CASEF_FLAG_UTF8 : 0) | CASEF_FLAG_APPEND, \
		(dst), (src), -1)

#define strcasecmp_utf8(s1, s2) \
    strcasecmp_utf8x(util_utf8_enable ? CASEF_FLAG_UTF8 : 0, (s1), (s2))
#define strncasecmp_utf8(s1, s2, l) \
    strncasecmp_utf8x(util_utf8_enable ? CASEF_FLAG_UTF8 : 0, (s1), (s2), (l))

 /*
  * Use STRREF(x) instead of x, to shut up compiler warnings when the operand
  * is a string literal.
  */
#define STRREF(x)		(&x[0])

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
