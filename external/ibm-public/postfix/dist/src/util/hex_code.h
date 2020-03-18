/*	$NetBSD: hex_code.h,v 1.3 2020/03/18 19:05:21 christos Exp $	*/

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
extern VSTRING *hex_encode(VSTRING *, const char *, ssize_t);
extern VSTRING *WARN_UNUSED_RESULT hex_decode(VSTRING *, const char *, ssize_t);

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
