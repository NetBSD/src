/*	$NetBSD: strip_addr.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _STRIP_ADDR_H_INCLUDED_
#define _STRIP_ADDR_H_INCLUDED_

/*++
/* NAME
/*	strip_addr 3h
/* SUMMARY
/*	strip extension from full address
/* SYNOPSIS
/*	#include <strip_addr.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern char *strip_addr_internal(const char *, char **, const char *);

#define strip_addr	strip_addr_internal

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
