/*	$NetBSD: connect.h,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

#ifndef _CONNECT_H_INCLUDED_
#define _CONNECT_H_INCLUDED_

/*++
/* NAME
/*	connect 3h
/* SUMMARY
/*	client interface file
/* SYNOPSIS
/*	#include <connect.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <iostuff.h>

 /*
  * Client external interface.
  */
extern int unix_connect(const char *, int, int);
extern int inet_connect(const char *, int, int);
extern int stream_connect(const char *, int, int);
extern int unix_dgram_connect(const char *, int);

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
