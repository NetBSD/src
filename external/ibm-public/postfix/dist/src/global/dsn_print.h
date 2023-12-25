/*	$NetBSD: dsn_print.h,v 1.1.1.1.52.1 2023/12/25 12:54:59 martin Exp $	*/

#ifndef _DSN_PRINT_H_INCLUDED_
#define _DSN_PRINT_H_INCLUDED_

/*++
/* NAME
/*	dsn_print 3h
/* SUMMARY
/*	write DSN structure to stream
/* SYNOPSIS
/*	#include <dsn_print.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <attr.h>

 /*
  * Global library.
  */
#include <dsn.h>

 /*
  * External interface.
  */
extern int dsn_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);

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
