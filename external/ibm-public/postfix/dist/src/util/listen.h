/*	$NetBSD: listen.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

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
#include <htable.h>

 /*
  * Listener external interface.
  */
extern int unix_listen(const char *, int, int);
extern int inet_listen(const char *, int, int);
extern int fifo_listen(const char *, int, int);
extern int stream_listen(const char *, int, int);

extern int inet_accept(int);
extern int unix_accept(int);
extern int stream_accept(int);

extern int WARN_UNUSED_RESULT recv_pass_attr(int, HTABLE **, int, ssize_t);
extern int pass_accept(int);
extern int pass_accept_attr(int, HTABLE **);

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
