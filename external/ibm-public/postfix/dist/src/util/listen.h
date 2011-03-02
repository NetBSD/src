/*	$NetBSD: listen.h,v 1.1.1.2 2011/03/02 19:32:44 tron Exp $	*/

#ifndef _LISTEN_H_INCLUDED_
#define _LISTEN_H_INCLUDED_

/*++
/* NAME
/*	listen 3h
/* SUMMARY
/*	listener interface file
/* SYNOPSIS
/*	#include <listen.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <iostuff.h>

 /*
  * Listener external interface.
  */
extern int unix_listen(const char *, int, int);
extern int inet_listen(const char *, int, int);
extern int fifo_listen(const char *, int, int);
extern int stream_listen(const char *, int, int);

#define unix_pass_listen	unix_listen
#define stream_pass_listen	stream_listen

extern int inet_accept(int);
extern int unix_accept(int);
extern int stream_accept(int);
extern int unix_pass_accept(int);

#define stream_pass_accept	stream_accept

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
