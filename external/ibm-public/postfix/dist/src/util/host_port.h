/*	$NetBSD: host_port.h,v 1.3 2020/03/18 19:05:21 christos Exp $	*/

#ifndef _HOST_PORT_H_INCLUDED_
#define _HOST_PORT_H_INCLUDED_

/*++
/* NAME
/*	host_port 3h
/* SUMMARY
/*	split string into host and port, destroy string
/* SYNOPSIS
/*	#include <host_port.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern const char *WARN_UNUSED_RESULT host_port(char *, char **, char *,
						        char **, char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
