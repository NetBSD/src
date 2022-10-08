/*	$NetBSD: sane_strtol.h,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	sane_strtol 3h
/* SUMMARY
/*	strtol() with mandatory errno reset
/* SYNOPSIS
/*	#include <sane_strtol.h>
/* DESCRIPTION
/* .nf

 /*
  * External API.
  */
extern long sane_strtol(const char *start, char **end, int);
extern unsigned long sane_strtoul(const char *start, char **end, int);

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
