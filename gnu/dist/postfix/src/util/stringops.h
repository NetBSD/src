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
extern char *lowercase(char *);
extern char *skipblanks(const char *);
extern char *trimblanks(char *, int);
extern char *concatenate(const char *,...);
extern char *mystrtok(char **, const char *);
extern char *translit(char *, const char *, const char *);
#ifndef HAVE_BASENAME
extern char *basename(const char *);
#endif
extern VSTRING *unescape(VSTRING *, const char *);

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
