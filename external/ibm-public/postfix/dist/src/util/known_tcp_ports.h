/*	$NetBSD: known_tcp_ports.h,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

#ifndef _KNOWN_TCP_PORTS_H_INCLUDED_
#define _KNOWN_TCP_PORTS_H_INCLUDED_

/*++
/* NAME
/*	known_tcp_port 3h
/* SUMMARY
/*	reduce dependency on the services(5) database
/* SYNOPSIS
/*	#include <known_tcp_ports.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern const char *add_known_tcp_port(const char *name, const char *port);
extern const char *filter_known_tcp_port(const char *name_or_port);
extern void clear_known_tcp_ports(void);
extern char *export_known_tcp_ports(VSTRING *out);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
