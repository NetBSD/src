/*	$NetBSD: postscreen_haproxy.h,v 1.1.1.1.8.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	postscreen_haproxy 3h
/* SUMMARY
/*	postscreen haproxy protocol support
/* SYNOPSIS
/*	#include <postscreen_haproxy.h>
/* DESCRIPTION
/* .nf

 /*
  * haproxy protocol interface.
  */
extern void psc_endpt_haproxy_lookup(VSTREAM *, PSC_ENDPT_LOOKUP_FN);

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
