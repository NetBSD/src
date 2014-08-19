/*	$NetBSD: xtext.h,v 1.1.1.1.16.1 2014/08/19 23:59:42 tls Exp $	*/

#ifndef _XTEXT_H_INCLUDED_
#define _XTEXT_H_INCLUDED_

/*++
/* NAME
/*	xtext 3h
/* SUMMARY
/*	quote/unquote text, xtext style.
/* SYNOPSIS
/*	#include <xtext.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *xtext_quote(VSTRING *, const char *, const char *);
extern VSTRING *xtext_quote_append(VSTRING *, const char *, const char *);
extern VSTRING *xtext_unquote(VSTRING *, const char *);
extern VSTRING *xtext_unquote_append(VSTRING *, const char *);

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
