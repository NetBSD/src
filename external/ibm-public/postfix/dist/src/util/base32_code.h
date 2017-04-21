/*	$NetBSD: base32_code.h,v 1.1.1.1.20.1 2017/04/21 16:52:52 bouyer Exp $	*/

#ifndef _BASE32_CODE_H_INCLUDED_
#define _BASE32_CODE_H_INCLUDED_

/*++
/* NAME
/*	base32_code 3h
/* SUMMARY
/*	encode/decode data, base 32 style
/* SYNOPSIS
/*	#include <base32_code.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
 */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *base32_encode(VSTRING *, const char *, ssize_t);
extern VSTRING *WARN_UNUSED_RESULT base32_decode(VSTRING *, const char *, ssize_t);

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
