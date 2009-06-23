/*	$NetBSD: quote_821_local.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	quote_821_local 3h
/* SUMMARY
/*	quote rfc 821 local part
/* SYNOPSIS
/*	#include "quote_821_local.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Global library.
  */
#include <quote_flags.h>

 /*
  * External interface.
  */
extern VSTRING *quote_821_local_flags(VSTRING *, const char *, int);
#define quote_821_local(dst, src) \
	quote_821_local_flags((dst), (src), QUOTE_FLAG_8BITCLEAN)

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
