/*	$NetBSD: xsasl_cyrus_security.c,v 1.1.1.1.2.2 2009/09/15 06:04:13 snj Exp $	*/

/*++
/* NAME
/*	xsasl_cyrus_security 3
/* SUMMARY
/*	convert Cyrus SASL security properties to bit mask
/* SYNOPSIS
/*	#include <xsasl_cyrus_common.h>
/*
/*	int	xsasl_cyrus_security_parse_opts(properties)
/*	const char *properties;
/* DESCRIPTION
/*	xsasl_cyrus_security_parse_opts() converts a list of security
/*	properties to a bit mask. The result is zero in case of error.
/*
/*	Arguments:
/* .IP properties
/*	A comma or space separated list of zero or more of the
/*	following:
/* .RS
/* .IP noplaintext
/*	Disallow authentication methods that use plaintext passwords.
/* .IP noactive
/*	Disallow authentication methods that are vulnerable to
/*	non-dictionary active attacks.
/* .IP nodictionary
/*	Disallow authentication methods that are vulnerable to
/*	passive dictionary attack.
/* .IP noanonymous
/*	Disallow anonymous logins.
/* .RE
/* DIAGNOSTICS:
/*	Warning: bad input.
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <name_mask.h>

/* Application-specific. */

#include <xsasl_cyrus_common.h>

#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

#include <sasl.h>

 /*
  * SASL Security options.
  */
static const NAME_MASK xsasl_cyrus_sec_mask[] = {
    "noplaintext", SASL_SEC_NOPLAINTEXT,
    "noactive", SASL_SEC_NOACTIVE,
    "nodictionary", SASL_SEC_NODICTIONARY,
    "noanonymous", SASL_SEC_NOANONYMOUS,
#if SASL_VERSION_MAJOR >= 2
    "mutual_auth", SASL_SEC_MUTUAL_AUTH,
#endif
    0,
};

/* xsasl_cyrus_security - parse security options */

int     xsasl_cyrus_security_parse_opts(const char *sasl_opts_val)
{
    return (name_mask_opt("SASL security options", xsasl_cyrus_sec_mask,
			  sasl_opts_val, NAME_MASK_RETURN));
}

#endif
