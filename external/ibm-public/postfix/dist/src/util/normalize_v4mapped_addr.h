/*	$NetBSD: normalize_v4mapped_addr.h,v 1.1.1.1 2026/05/09 18:39:24 christos Exp $	*/

#ifndef _NORMALIZE_V4MAPPED_ADDR_H_INCLUDED_
#define _NORMALIZE_V4MAPPED_ADDR_H_INCLUDED_

/*++
/* NAME
/*	normalize_v4mapped_addr 3h
/* SUMMARY
/*	normalize v4mapped IPv6 address
/* SYNOPSIS
/*	#include <normalize_v4mapped_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/socket.h>

 /*
  * Utility library.
  */
#include <myaddrinfo.h>

 /*
  * External interface.
  */
extern int normalize_v4mapped_sockaddr(struct sockaddr *, SOCKADDR_SIZE *);
extern int normalize_v4mapped_hostaddr(MAI_HOSTADDR_STR *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
