/*	$NetBSD: set_ugid.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _SET_UGID_H_INCLUDED_
#define _SET_UGID_H_INCLUDED_

/*++
/* NAME
/*	set_ugid 3h
/* SUMMARY
/*	set real, effective and saved user and group attributes
/* SYNOPSIS
/*	#include <set_ugid.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern void set_ugid(uid_t, gid_t);

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
