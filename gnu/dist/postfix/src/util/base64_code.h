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
extern VSTRING *base64_encode(VSTRING *, const char *, int);
extern VSTRING *base64_decode(VSTRING *, const char *, int);

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
