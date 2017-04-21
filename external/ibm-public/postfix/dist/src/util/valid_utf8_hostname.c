/*	$NetBSD: valid_utf8_hostname.c,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	valid_utf8_hostname 3
/* SUMMARY
/*	validate (maybe UTF-8) domain name
/* SYNOPSIS
/*	#include <valid_utf8_hostname.h>
/*
/*	int	valid_utf8_hostname(
/*	int	enable_utf8,
/*	const char *domain,
/*	int	gripe)
/* DESCRIPTION
/*	valid_utf8_hostname() is a wrapper around valid_hostname().
/*	If EAI support is compiled in, and enable_utf8 is true, the
/*	name is converted from UTF-8 to ASCII per IDNA rules, before
/*	invoking valid_hostname().
/* SEE ALSO
/*	valid_hostname(3) STD3 hostname validation.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/*	Warnings: malformed domain name.
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

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <valid_hostname.h>
#include <midna_domain.h>
#include <valid_utf8_hostname.h>

/* valid_utf8_hostname - validate internationalized domain name */

int     valid_utf8_hostname(int enable_utf8, const char *name, int gripe)
{
    static const char myname[] = "valid_utf8_hostname";

    /*
     * Trivial cases first.
     */
    if (*name == 0) {
	if (gripe)
	    msg_warn("%s: empty domain name", myname);
	return (0);
    }

    /*
     * Convert non-ASCII domain name to ASCII and validate the result per
     * STD3. midna_domain_to_ascii() applies valid_hostname() to the result.
     * Propagate the gripe parameter for better diagnostics (note that
     * midna_domain_to_ascii() logs a problem only when the result is not
     * cached).
     */
#ifndef NO_EAI
    if (enable_utf8 && !allascii(name)) {
	if (midna_domain_to_ascii(name) == 0) {
	    if (gripe)
		msg_warn("%s: malformed UTF-8 domain name", myname);
	    return (0);
	} else {
	    return (1);
	}
    }
#endif

    /*
     * Validate ASCII name per STD3.
     */
    return (valid_hostname(name, gripe));
}
