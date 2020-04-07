/*	$NetBSD: quote_flags.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

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
  * External interface.
  */
#define QUOTE_FLAG_8BITCLEAN	(1<<0)	/* be 8-bit clean */
#define QUOTE_FLAG_EXPOSE_AT	(1<<1)	/* @ is ordinary text */
#define QUOTE_FLAG_APPEND	(1<<2)	/* append, not overwrite */

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
