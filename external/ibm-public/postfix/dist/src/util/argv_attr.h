/*	$NetBSD: argv_attr.h,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

#ifndef _ARGV_ATTR_H_INCLUDED_
#define _ARGV_ATTR_H_INCLUDED_

/*++
/* NAME
/*	argv_attr 3h
/* SUMMARY
/*	argv serialization/deserialization
/* SYNOPSIS
/*	#include <argv_attr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>
#include <attr.h>
#include <check_arg.h>
#include <vstream.h>

 /*
  * External API.
  */
#define ARGV_ATTR_SIZE	"argv_size"
#define ARGV_ATTR_VALUE	"argv_value"
#define ARGV_ATTR_MAX	1024

extern int argv_attr_print(ATTR_PRINT_COMMON_FN, VSTREAM *, int, const void *);
extern int argv_attr_scan(ATTR_SCAN_COMMON_FN, VSTREAM *, int, void *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
