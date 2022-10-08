/*	$NetBSD: config_known_tcp_ports.h,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

#ifndef _CONFIG_KNOWN_TCP_PORTS_H_INCLUDED_
#define _CONFIG_KNOWN_TCP_PORTS_H_INCLUDED_

/*++
/* NAME
/*	config_known_tcp_ports 3h
/* SUMMARY
/*	parse and store known TCP port configuration
/* SYNOPSIS
/*	#include <config_known_tcp_ports.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void config_known_tcp_ports(const char *source, const char *settings);

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
