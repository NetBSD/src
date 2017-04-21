/*	$NetBSD: midna_adomain.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _MIDNA_ADOMAIN_H_INCLUDED_
#define _MIDNA_ADOMAIN_H_INCLUDED_

/*++
/* NAME
/*	midna_adomain 3h
/* SUMMARY
/*	domain name conversion
/* SYNOPSIS
/*	#include <midna_adomain.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern char *midna_adomain_to_utf8(VSTRING *, const char *);
extern char *midna_adomain_to_ascii(VSTRING *, const char *);

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
