/*	$NetBSD: match_ops.h,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

#ifndef _MATCH_OPS_H_INCLUDED_
#define _MATCH_OPS_H_INCLUDED_

/*++
/* NAME
/*	match_ops 3h
/* SUMMARY
/*	simple string or host pattern matching
/* SYNOPSIS
/*	#include <match_ops.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

#define MATCH_FLAG_NONE		0
#define MATCH_FLAG_PARENT	(1<<0)
#define MATCH_FLAG_ALL		(MATCH_FLAG_PARENT)

extern int match_string(int, const char *, const char *);
extern int match_hostname(int, const char *, const char *);
extern int match_hostaddr(int, const char *, const char *);

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
