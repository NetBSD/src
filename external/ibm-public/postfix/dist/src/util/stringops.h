/*	$NetBSD: stringops.h,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

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
extern char *printable(char *, int);
extern char *neuter(char *, const char *, int);
extern char *lowercase(char *);
extern char *uppercase(char *);
extern char *skipblanks(const char *);
extern char *trimblanks(char *, int);
extern char *concatenate(const char *,...);
extern char *mystrtok(char **, const char *);
extern char *translit(char *, const char *, const char *);
#ifndef HAVE_BASENAME
#define basename postfix_basename
extern char *basename(const char *);
#endif
extern char *sane_basename(VSTRING *, const char *);
extern char *sane_dirname(VSTRING *, const char *);
extern VSTRING *unescape(VSTRING *, const char *);
extern VSTRING *escape(VSTRING *, const char *, ssize_t);
extern int alldig(const char *);
extern int allprint(const char *);
extern int allspace(const char *);
extern int allascii(const char *);
extern const char *split_nameval(char *, char **, char **);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
