/*	$NetBSD: rfc2047_code.h,v 1.2 2025/02/25 19:15:46 christos Exp $	*/

#ifndef _RFC2047_ENCODE_H_INCLUDED_
#define _RFC2047_ENCODE_H_INCLUDED_

/*++
/* NAME
/*	rfc2047_code 3h
/* SUMMARY
/*	non-ASCII header content encoding
/* SYNOPSIS
/*	#include <rfc2047_code.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External API.
  */
#define RFC2047_HEADER_CONTEXT_COMMENT	(1)
#define RFC2047_HEADER_CONTEXT_PHRASE	(2)

extern char *rfc2047_encode(VSTRING *result, int header_context,
			              const char *charset,
			              const char *in, ssize_t len,
				      const char *out_separator);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
