/*	$NetBSD: match_service.h,v 1.1.1.1.10.1 2013/01/23 00:05:03 yamt Exp $	*/

#ifndef _MATCH_SERVICE_H_INCLUDED_
#define _MATCH_SERVICE_H_INCLUDED_

/*++
/* NAME
/*	match_service 3h
/* SUMMARY
/*	simple master.cf service name.type pattern matcher
/* SYNOPSIS
/*	#include <match_service.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern ARGV *match_service_init(const char *);
extern ARGV *match_service_init_argv(char **);
extern int match_service_match(ARGV *, const char *);
extern void match_service_free(ARGV *);

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
