/*	$NetBSD: hex_quote.h,v 1.1.1.1.2.2 2009/09/15 06:03:58 snj Exp $	*/

#ifndef _HEX_QUOTE_H_INCLUDED_
#define _HEX_QUOTE_H_INCLUDED_

/*++
/* NAME
/*	hex_quote 3h
/* SUMMARY
/*	quote/unquote text, HTTP style.
/* SYNOPSIS
/*	#include <hex_quote.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
 */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *hex_quote(VSTRING *, const char *);
extern VSTRING *hex_unquote(VSTRING *, const char *);

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
