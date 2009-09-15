/*	$NetBSD: debug_peer.h,v 1.1.1.1.2.2 2009/09/15 06:02:39 snj Exp $	*/

#ifndef _DEBUG_PEER_H_INCLUDED_
#define _DEBUG_PEER_H_INCLUDED_
/*++
/* NAME
/*	debug_peer 3h
/* SUMMARY
/*	increase verbose logging for specific peers
/* SYNOPSIS
/*	#include <debug_peer.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void debug_peer_init(void);
extern int debug_peer_check(const char *, const char *);
extern void debug_peer_restore(void);

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
