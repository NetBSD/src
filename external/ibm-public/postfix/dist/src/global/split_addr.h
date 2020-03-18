/*	$NetBSD: split_addr.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _SPLIT_ADDR_H_INCLUDED_
#define _SPLIT_ADDR_H_INCLUDED_

/*++
/* NAME
/*	split_addr 3h
/* SUMMARY
/*	recipient localpart address splitter
/* SYNOPSIS
/*	#include <split_addr.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern char *split_addr_internal(char *, const char *);

 /* Legacy API. */

#define split_addr	split_addr_internal

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
