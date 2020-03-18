/*	$NetBSD: quote_flags.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	quote_flags 3h
/* SUMMARY
/*	quote rfc 821/822 local part
/* SYNOPSIS
/*	#include "quote_flags.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
 */
#include <vstring.h>

 /*
  * External interface.
  */
#define QUOTE_FLAG_8BITCLEAN	(1<<0)	/* be 8-bit clean */
#define QUOTE_FLAG_EXPOSE_AT	(1<<1)	/* @ is ordinary text */
#define QUOTE_FLAG_APPEND	(1<<2)	/* append, not overwrite */
#define QUOTE_FLAG_BARE_LOCALPART (1<<3)/* all localpart, no @domain */

#define QUOTE_FLAG_DEFAULT	QUOTE_FLAG_8BITCLEAN

extern int quote_flags_from_string(const char *);
extern const char *quote_flags_to_string(VSTRING *, int);

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
