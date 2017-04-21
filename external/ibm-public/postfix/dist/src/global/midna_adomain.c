/*	$NetBSD: midna_adomain.c,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	midna_adomain 3
/* SUMMARY
/*	address domain part conversion
/* SYNOPSIS
/*	#include <midna_adomain.h>
/*
/*	char	*midna_adomain_to_ascii(
/*	VSTRING	*dest,
/*	const char *name)
/*
/*	char	*midna_adomain_to_utf8(
/*	VSTRING	*dest,
/*	const char *name)
/* DESCRIPTION
/*	The functions in this module transform the domain portion
/*	of an email address between ASCII and UTF-8 form.  Both
/*	functions tolerate a missing domain, and both functions
/*	return a copy of the input when the domain portion requires
/*	no conversion.
/*
/*	midna_adomain_to_ascii() converts an UTF-8 or ASCII domain
/*	portion to ASCII.  The result is a null pointer when
/*	conversion fails.  This function verifies that the resulting
/*	domain passes valid_hostname().
/*
/*	midna_adomain_to_utf8() converts an UTF-8 or ASCII domain
/*	name to UTF-8.  The result is a null pointer when conversion
/*	fails.  This function verifies that the resulting domain,
/*	after conversion to ASCII, passes valid_hostname().
/* SEE ALSO
/*	midna_domain(3), Postfix ASCII/UTF-8 domain name conversion
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/*	Warnings: conversion error or result validation error.
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

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

#ifndef NO_EAI
#include <unicode/uidna.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <stringops.h>
#include <midna_domain.h>

 /*
  * Global library.
  */
#include <midna_adomain.h>

#define STR(x)	vstring_str(x)

/* midna_adomain_to_utf8 - convert address domain portion to UTF8 */

char   *midna_adomain_to_utf8(VSTRING *dest, const char *src)
{
    const char *cp;
    const char *domain_utf8;

    if ((cp = strrchr(src, '@')) == 0) {
	vstring_strcpy(dest, src);
    } else {
	vstring_sprintf(dest, "%*s@", (int) (cp - src), src);
	if (*(cp += 1)) {
	    if (allascii(cp) && strstr(cp, "--") == 0) {
		vstring_strcat(dest, cp);
	    } else if ((domain_utf8 = midna_domain_to_utf8(cp)) == 0) {
		return (0);
	    } else {
		vstring_strcat(dest, domain_utf8);
	    }
	}
    }
    return (STR(dest));
}

/* midna_adomain_to_ascii - convert address domain portion to ASCII */

char   *midna_adomain_to_ascii(VSTRING *dest, const char *src)
{
    const char *cp;
    const char *domain_ascii;

    if ((cp = strrchr(src, '@')) == 0) {
	vstring_strcpy(dest, src);
    } else {
	vstring_sprintf(dest, "%*s@", (int) (cp - src), src);
	if (*(cp += 1)) {
	    if (allascii(cp)) {
		vstring_strcat(dest, cp);
	    } else if ((domain_ascii = midna_domain_to_ascii(cp + 1)) == 0) {
		return (0);
	    } else {
		vstring_strcat(dest, domain_ascii);
	    }
	}
    }
    return (STR(dest));
}

#endif					/* NO_IDNA */
