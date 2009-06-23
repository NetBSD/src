/*	$NetBSD: base64_code.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _BASE64_CODE_H_INCLUDED_
#define _BASE64_CODE_H_INCLUDED_

/*++
/* NAME
/*	base64_code 3h
/* SUMMARY
/*	encode/decode data, base 64 style
/* SYNOPSIS
/*	#include <base64_code.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
 */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *base64_encode(VSTRING *, const char *, ssize_t);
extern VSTRING *base64_decode(VSTRING *, const char *, ssize_t);

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
