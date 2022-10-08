/*	$NetBSD: sane_strtol.c,v 1.2 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	sane_strtol 3
/* SUMMARY
/*	strtol() with mandatory errno reset
/* SYNOPSIS
/*	#include <sane_strtol.h>
/*
/*	long	sane_strtol(
/*	const char *start, 
/*	char **restrict end, 
/*	int	base)
/*
/*	unsigned long sane_strtoul(
/*	const char *start,
/*	char **restrict end,
/*	int	base)
/* DESCRIPTION
/*	These functions are wrappers around the strtol() and strtoul()
/*	standard library functions that reset errno first, so that a
/*	prior ERANGE error won't cause false errors.
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

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <errno.h>

 /*
  * Utility library.
  */
#include <sane_strtol.h>

/* sane_strtol - strtol() with mandatory initialization */

long    sane_strtol(const char *start, char **end, int base)
{
    errno = 0;
    return (strtol(start, end, base));
}

/* sane_strtoul - strtoul() with mandatory initialization */

unsigned long sane_strtoul(const char *start, char **end, int base)
{
    errno = 0;
    return (strtoul(start, end, base));
}
