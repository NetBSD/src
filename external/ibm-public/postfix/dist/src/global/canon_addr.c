/*	$NetBSD: canon_addr.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	canon_addr 3
/* SUMMARY
/*	simple address canonicalization
/* SYNOPSIS
/*	#include <canon_addr.h>
/*
/*	VSTRING	*canon_addr_external(result, address)
/*	VSTRING	*result;
/*	const char *address;
/*
/*	VSTRING	*canon_addr_internal(result, address)
/*	VSTRING	*result;
/*	const char *address;
/* DESCRIPTION
/*	This module provides a simple interface to the address
/*	canonicalization service that is provided by the address
/*	rewriting service.
/*
/*	canon_addr_external() transforms an address in external (i.e.
/*	quoted) RFC822 form to a fully-qualified address (user@domain).
/*
/*	canon_addr_internal() transforms an address in internal (i.e.
/*	unquoted) RFC822 form to a fully-qualified address (user@domain).
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages).
/* SEE ALSO
/*	rewrite_clnt(3) address rewriting client interface
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

#include <vstring.h>
#include <mymalloc.h>

/* Global library. */

#include "rewrite_clnt.h"
#include "canon_addr.h"

/* canon_addr_external - make address fully qualified, external form */

VSTRING *canon_addr_external(VSTRING *result, const char *addr)
{
    return (rewrite_clnt(REWRITE_CANON, addr, result));
}

/* canon_addr_internal - make address fully qualified, internal form */

VSTRING *canon_addr_internal(VSTRING *result, const char *addr)
{
    return (rewrite_clnt_internal(REWRITE_CANON, addr, result));
}
