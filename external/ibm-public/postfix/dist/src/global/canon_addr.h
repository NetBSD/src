/*	$NetBSD: canon_addr.h,v 1.1.1.1.2.2 2009/09/15 06:02:38 snj Exp $	*/

#ifndef _CANON_ADDR_H_INCLUDED_
#define _CANON_ADDR_H_INCLUDED_

/*++
/* NAME
/*	canon_addr 3h
/* SUMMARY
/*	simple address canonicalization
/* SYNOPSIS
/*	#include <canon_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *canon_addr_external(VSTRING *, const char *);
extern VSTRING *canon_addr_internal(VSTRING *, const char *);

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
