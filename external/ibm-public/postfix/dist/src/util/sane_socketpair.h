/*	$NetBSD: sane_socketpair.h,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

#ifndef _SANE_SOCKETPAIR_H_
#define _SANE_SOCKETPAIR_H_

/*++
/* NAME
/*	sane_socketpair 3h
/* SUMMARY
/*	sanitize socketpair() error returns
/* SYNOPSIS
/*	#include <sane_socketpair.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int WARN_UNUSED_RESULT sane_socketpair(int, int, int, int *);

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
