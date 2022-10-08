/*	$NetBSD: hex_code.h,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

#ifndef _HEX_CODE_H_INCLUDED_
#define _HEX_CODE_H_INCLUDED_

/*++
/* NAME
/*	hex_code 3h
/* SUMMARY
/*	encode/decode data, hexadecimal style
/* SYNOPSIS
/*	#include <hex_code.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
 */
#include <vstring.h>

 /*
  * External interface.
  */
#define HEX_ENCODE_FLAG_NONE		(0)
#define HEX_ENCODE_FLAG_USE_COLON	(1<<0)

#define HEX_DECODE_FLAG_NONE	(0)
#define HEX_DECODE_FLAG_ALLOW_COLON	(1<<0)

extern VSTRING *hex_encode(VSTRING *, const char *, ssize_t);
extern VSTRING *WARN_UNUSED_RESULT hex_decode(VSTRING *, const char *, ssize_t);
extern VSTRING *hex_encode_opt(VSTRING *, const char *, ssize_t, int);
extern VSTRING *WARN_UNUSED_RESULT hex_decode_opt(VSTRING *, const char *, ssize_t, int);

#define hex_encode(res, in, len) \
	hex_encode_opt((res), (in), (len), HEX_ENCODE_FLAG_NONE)
#define hex_decode(res, in, len) \
	hex_decode_opt((res), (in), (len), HEX_DECODE_FLAG_NONE)

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
