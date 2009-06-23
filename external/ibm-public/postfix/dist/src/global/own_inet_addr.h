/*	$NetBSD: own_inet_addr.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

#ifndef _OWN_INET_ADDR_H_INCLUDED_
#define _OWN_INET_ADDR_H_INCLUDED_

/*++
/* NAME
/*	own_inet_addr 3h
/* SUMMARY
/*	determine if IP address belongs to this mail system instance
/* SYNOPSIS
/*	#include <own_inet_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <inet_addr_list.h>

 /*
  * External interface.
  */
extern int own_inet_addr(struct sockaddr *);
extern struct INET_ADDR_LIST *own_inet_addr_list(void);
extern struct INET_ADDR_LIST *own_inet_mask_list(void);
extern int proxy_inet_addr(struct sockaddr *);
extern struct INET_ADDR_LIST *proxy_inet_addr_list(void);

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
