/*	$NetBSD: domain_list.h,v 1.1.1.1.16.1 2013/02/25 00:27:17 tls Exp $	*/

#ifndef _DOMAIN_LIST_H_INCLUDED_
#define _DOMAIN_LIST_H_INCLUDED_

/*++
/* NAME
/*	domain_list 3h
/* SUMMARY
/*	match a host or domain name against a pattern list
/* SYNOPSIS
/*	#include <domain_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <match_list.h>

 /*
  * External interface.
  */
#define DOMAIN_LIST	MATCH_LIST

#define domain_list_init(f, p)	match_list_init((f), (p), 1, match_hostname)
#define domain_list_match	match_list_match
#define domain_list_free	match_list_free

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
