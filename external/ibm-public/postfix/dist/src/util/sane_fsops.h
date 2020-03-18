/*	$NetBSD: sane_fsops.h,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

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

extern int WARN_UNUSED_RESULT sane_rename(const char *, const char *);
extern int WARN_UNUSED_RESULT sane_link(const char *, const char *);

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
