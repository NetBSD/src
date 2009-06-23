/*	$NetBSD: chroot_uid.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _CHROOT_UID_H_INCLUDED_
#define _CHROOT_UID_H_INCLUDED_

/*++
/* NAME
/*	chroot_uid 3h
/* SUMMARY
/*	limit possible damage a process can do
/* SYNOPSIS
/*	#include <chroot_uid.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern void chroot_uid(const char *, const char *);

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
