/*	$NetBSD: find_inet.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _FIND_INET_H_INCLUDED_
#define _FIND_INET_H_INCLUDED_

/*++
/* NAME
/*	find_inet 3h
/* SUMMARY
/*	inet-domain name services
/* SYNOPSIS
/*	#include <find_inet.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern unsigned find_inet_addr(const char *);
extern int find_inet_port(const char *, const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/* LAST MODIFICATION
/*	Thu Feb  6 12:46:36 EST 1997
/*--*/

#endif
