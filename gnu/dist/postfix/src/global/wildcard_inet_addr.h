#ifndef _WILDCARD_INET_ADDR_H_INCLUDED_
#define _WILDCARD_INET_ADDR_H_INCLUDED_

/*++
/* NAME
/*	wildcard_inet_addr_list 3h
/* SUMMARY
/*	grab the list of wildcard IP addresses.
/* SYNOPSIS
/*	#include <own_inet_addr.h>
/* DESCRIPTION
/* .nf
/*--*/

 /*
  * System library.
  */
#include <netinet/in.h>
#ifdef INET6
#include <sys/socket.h>
#endif

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
