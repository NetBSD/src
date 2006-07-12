/*	$NetBSD: wildcard_inet_addr.h,v 1.3.2.2 2006/07/12 15:06:39 tron Exp $	*/

#ifndef _WILDCARD_INET_ADDR_H_INCLUDED_
#define _WILDCARD_INET_ADDR_H_INCLUDED_

/*++
/* NAME
/*	wildcard_inet_addr 3h
/* SUMMARY
/*	grab the list of wildcard IP addresses.
/* SYNOPSIS
/*	#include <wildcard_inet_addr.h>
/* DESCRIPTION
/* .nf
/*--*/

 /*
  * Utility library.
  */
#include <inet_addr_list.h>

 /*
  * External interface.
  */
extern struct INET_ADDR_LIST *wildcard_inet_addr_list(void);

/* LICENSE
/* .ad
/* .fi
/*	foo
/* AUTHOR(S)
/*	Jun-ichiro itojun Hagino
/*--*/

#endif
