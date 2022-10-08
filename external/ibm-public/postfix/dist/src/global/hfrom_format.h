/*	$NetBSD: hfrom_format.h,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

#ifndef _HFROM_FORMAT_INCLUDED_
#define _HFROM_FORMAT_INCLUDED_

/*++
/* NAME
/*	hfrom_format 3h
/* SUMMARY
/*	Parse a header_from_format setting
/* SYNOPSIS
/*	#include <hfrom_format.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define HFROM_FORMAT_CODE_OBS	0	/* Obsolete */
#define HFROM_FORMAT_CODE_STD	1	/* Standard */

extern int hfrom_format_parse(const char *, const char *);
extern const char *str_hfrom_format_code(int);

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
