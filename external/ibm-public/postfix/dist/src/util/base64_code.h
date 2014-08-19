/*	$NetBSD: base64_code.h,v 1.1.1.1.16.1 2014/08/19 23:59:45 tls Exp $	*/

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
extern VSTRING *base64_encode_opt(VSTRING *, const char *, ssize_t, int);
extern VSTRING *base64_decode_opt(VSTRING *, const char *, ssize_t, int);

#define BASE64_FLAG_NONE	0
#define BASE64_FLAG_APPEND	(1<<0)

#define base64_encode(bp, cp, ln) \
	base64_encode_opt((bp), (cp), (ln), BASE64_FLAG_NONE)
#define base64_decode(bp, cp, ln) \
	base64_decode_opt((bp), (cp), (ln), BASE64_FLAG_NONE)

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
