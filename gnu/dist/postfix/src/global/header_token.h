/*	$NetBSD: header_token.h,v 1.1.1.2 2004/05/31 00:24:30 heas Exp $	*/

#ifndef _HEADER_TOKEN_H_INCLUDED_
#define _HEADER_TOKEN_H_INCLUDED_

/*++
/* NAME
/*	header_token 3h
/* SUMMARY
/*	mail header parser
/* SYNOPSIS
/*	#include "header_token.h"
 DESCRIPTION
 .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * HEADER header parser tokens. Specials and controls are represented by
  * themselves. Character pointers point to substrings in a token buffer.
  */
typedef struct HEADER_TOKEN {
    int     type;			/* see below */
    union {
	const char *value;		/* just a pointer, not a copy */
	int     offset;			/* index into token buffer */
    }       u;				/* indent beats any alternative */
} HEADER_TOKEN;

#define HEADER_TOK_TOKEN	256
#define HEADER_TOK_QSTRING	257

extern int header_token(HEADER_TOKEN *, int, VSTRING *, const char **, const char *, int);

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
