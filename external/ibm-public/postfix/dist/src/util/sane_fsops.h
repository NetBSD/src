/*	$NetBSD: sane_fsops.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _SANE_FSOPS_H_
#define _SANE_FSOPS_H_

/*++
/* NAME
/*	sane_rename 3h
/* SUMMARY
/*	sanitize rename() error returns
/* SYNOPSIS
/*	#include <sane_rename.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int sane_rename(const char *, const char *);
extern int sane_link(const char *, const char *);

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
