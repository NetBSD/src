/*	$NetBSD: inet_addr_host.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef INET_ADDR_HOST_H_INCLUDED_
#define INET_ADDR_HOST_H_INCLUDED_

/*++
/* NAME
/*	inet_addr_host 3h
/* SUMMARY
/*	determine all host internet interface addresses
/* SYNOPSIS
/*	#include <inet_addr_host.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <inet_addr_list.h>

 /*
  * External interface.
  */
extern int inet_addr_host(INET_ADDR_LIST *, const char *);

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
