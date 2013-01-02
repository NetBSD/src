/*	$NetBSD: addr_match_list.h,v 1.1.1.2 2013/01/02 18:58:56 tron Exp $	*/

#ifndef _ADDR_MATCH_LIST_H_INCLUDED_
#define _ADDR_MATCH_LIST_H_INCLUDED_

/*++
/* NAME
/*	addr 3h
/* SUMMARY
/*	address list membership
/* SYNOPSIS
/*	#include <addr_match_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <match_list.h>

 /*
  * External interface.
  */
#define ADDR_MATCH_LIST MATCH_LIST

#define addr_match_list_init(f, p) \
	match_list_init((f), (p), 1, match_hostaddr)
#define addr_match_list_match(l, a) \
	match_list_match((l), (a))
#define addr_match_list_free	match_list_free

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
