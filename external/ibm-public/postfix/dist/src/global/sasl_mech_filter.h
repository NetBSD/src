/*	$NetBSD: sasl_mech_filter.h,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

#ifndef _SASL_MECH_FILTER_H_INCLUDED_
#define _SASL_MECH_FILTER_H_INCLUDED_

/*++
/* NAME
/*	sasl_mech_filter 3h
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include "sasl_mech_filter.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <string_list.h>

 /*
  * External interface.
  */
extern const char *sasl_mech_filter(STRING_LIST *, const char *);

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
