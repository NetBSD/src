/*	$NetBSD: dsn_filter.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _DSN_FILTER_H_INCLUDED_
#define _DSN_FILTER_H_INCLUDED_

/*++
/* NAME
/*	dsn_filter 3h
/* SUMMARY
/*	delivery status filter
/* SYNOPSIS
/*	#include <dsn_filter.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct DSN_FILTER DSN_FILTER;

extern DSN_FILTER *dsn_filter_create(const char *, const char *);
extern DSN *dsn_filter_lookup(DSN_FILTER *, DSN *);
extern void dsn_filter_free(DSN_FILTER *);

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
