/*	$NetBSD: haproxy_srvr.h,v 1.1.1.1 2013/09/25 19:06:30 tron Exp $	*/

#ifndef _HAPROXY_SRVR_H_INCLUDED_
#define _HAPROXY_SRVR_H_INCLUDED_

/*++
/* NAME
/*	haproxy_srvr 3h
/* SUMMARY
/*	server-side haproxy protocol support
/* SYNOPSIS
/*	#include <haproxy_srvr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <myaddrinfo.h>

 /*
  * External interface.
  */
extern const char *haproxy_srvr_parse(const char *,
			             MAI_HOSTADDR_STR *, MAI_SERVPORT_STR *,
			            MAI_HOSTADDR_STR *, MAI_SERVPORT_STR *);

#define HAPROXY_PROTO_NAME	"haproxy"
#define HAPROXY_MAX_LEN		(256 + 2)

#ifndef DO_GRIPE
#define DO_GRIPE 	1
#define DONT_GRIPE	0
#endif

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
