/*	$NetBSD: auto_clnt.h,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

#ifndef _AUTO_CLNT_H_INCLUDED_
#define _AUTO_CLNT_H_INCLUDED_

/*++
/* NAME
/*	auto_clnt 3h
/* SUMMARY
/*	client endpoint maintenance
/* SYNOPSIS
/*	#include <auto_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
typedef struct AUTO_CLNT AUTO_CLNT;
typedef int (*AUTO_CLNT_HANDSHAKE_FN) (VSTREAM *);

extern AUTO_CLNT *auto_clnt_create(const char *, int, int, int);
extern VSTREAM *auto_clnt_access(AUTO_CLNT *);
extern void auto_clnt_recover(AUTO_CLNT *);
extern const char *auto_clnt_name(AUTO_CLNT *);
extern void auto_clnt_free(AUTO_CLNT *);
extern void auto_clnt_control(AUTO_CLNT *, int,...);

#define AUTO_CLNT_CTL_END       0
#define AUTO_CLNT_CTL_HANDSHAKE 1	/* handshake before first request */

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
