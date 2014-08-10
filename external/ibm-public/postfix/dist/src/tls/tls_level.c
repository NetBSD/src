/*	$NetBSD: tls_level.c,v 1.1.1.1.26.1 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	tls_level 3
/* SUMMARY
/*	TLS security level conversion
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	int	tls_level_lookup(name)
/*	const char *name;
/*
/*	const char *str_tls_level(level)
/*	int	level;
/* DESCRIPTION
/*	The macros in this module convert TLS levels from symbolic
/*	name to internal form and vice versa. The macros are safe
/*	because they evaluate their arguments only once.
/*
/*	tls_level_lookup() converts a TLS level from symbolic name
/*	to internal form. When an unknown level is specified,
/*	tls_level_lookup() logs no warning, and returns TLS_LEV_INVALID.
/*
/*	str_tls_level() converts a TLS level from internal form to
/*	symbolic name. The result is a null pointer for an unknown
/*	level.
/* SEE ALSO
/*	name_code(3) name to number mapping
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
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <name_code.h>

/* TLS library. */

#include <tls.h>

/* Application-specific. */

 /*
  * Numerical order of levels is critical (see tls.h):
  * 
  * - With "may" and higher, TLS is enabled.
  * 
  * - With "encrypt" and higher, TLS is required.
  * 
  * - With "fingerprint" and higher, the peer certificate must match.
  * 
  * - With "dane" and higher, the peer certificate must also be trusted,
  * possibly via TLSA RRs that make it its own authority.
  * 
  * The smtp(8) client will report trust failure in preference to reporting
  * failure to match, so we make "dane" larger than "fingerprint".
  */
const NAME_CODE tls_level_table[] = {
    "none", TLS_LEV_NONE,
    "may", TLS_LEV_MAY,
    "encrypt", TLS_LEV_ENCRYPT,
    "fingerprint", TLS_LEV_FPRINT,
    "dane", TLS_LEV_DANE,
    "dane-only", TLS_LEV_DANE_ONLY,
    "verify", TLS_LEV_VERIFY,
    "secure", TLS_LEV_SECURE,
    0, TLS_LEV_INVALID,
};
