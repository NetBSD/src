/*	$NetBSD: timed_connect.h,v 1.1.1.2 2004/05/31 00:25:01 heas Exp $	*/

#ifndef _TIMED_CONNECT_H_INCLUDED_
#define _TIMED_CONNECT_H_INCLUDED_

/*++
/* NAME
/*	timed_connect 3h
/* SUMMARY
/*	connect operation with timeout
/* SYNOPSIS
/*	#include <sys/socket.h>
/*	#include <timed_connect.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern int timed_connect(int, struct sockaddr *, int, int);

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
