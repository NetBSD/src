/*	$NetBSD: tls_level.c,v 1.1.1.1.4.2 2006/11/20 13:30:55 tron Exp $	*/

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
/*	to internal form. The result is TLS_NOTFOUND for an unknown
/*	level.
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
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <name_code.h>

/* TLS library. */

#include <tls.h>

/* Application-specific. */

NAME_CODE tls_level_table[] = {
    "none", TLS_LEV_NONE,
    "may", TLS_LEV_MAY,
    "encrypt", TLS_LEV_ENCRYPT,
    "verify", TLS_LEV_VERIFY,
    "secure", TLS_LEV_SECURE,
    0, TLS_LEV_NOTFOUND,
};
