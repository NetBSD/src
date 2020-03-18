/*	$NetBSD: smtp_misc.c,v 1.2 2020/03/18 19:05:20 christos Exp $	*/

/*++
/* NAME
/*	smtp_misc 3
/* SUMMARY
/*	SMTP client address rewriting
/* SYNOPSIS
/*	#include <smtp_addr.h>
/*
/*	void	smtp_rewrite_generic_internal(
/*	VSTRING *dst,
/*	const char *src);
/*
/*	void	smtp_quote_822_address_flags(
/*	VSTRING *dst,
/*	const char *src,
/*	int flags);
/*
/*	void	smtp_quote_821_address(
/*	VSTRING *dst,
/*	const char *src);
/* DESCRIPTION
/*	smtp_rewrite_generic_internal() rewrites a non-empty address
/*	if generic mapping is enabled, otherwise copies it literally.
/*
/*	smtp_quote_822_address_flags() is a wrapper around
/*	quote_822_local_flags(), except for the empty address which
/*	is copied literally.
/*
/*	smtp_quote_821_address() is a wrapper around quote_821_local(),
/*	except for the empty address or with "smtp_quote_rfc821_envelope
/*	= no"; in those cases the address is copied literally.
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Global library.
  */
#include <ext_prop.h>
#include <mail_params.h>
#include <quote_821_local.h>
#include <quote_822_local.h>

 /*
  * Application-specific.
  */
#include <smtp.h>

/* smtp_rewrite_generic_internal - generic non-empty address rewriting */

void    smtp_rewrite_generic_internal(VSTRING *dst, const char *src)
{
    vstring_strcpy(dst, src);
    if (*src && smtp_generic_maps)
	smtp_map11_internal(dst, smtp_generic_maps,
			    smtp_ext_prop_mask & EXT_PROP_GENERIC);
}

/* smtp_quote_822_address_flags - quote non-empty header address */

void    smtp_quote_822_address_flags(VSTRING *dst, const char *src, int flags)
{
    if (*src) {
	quote_822_local_flags(dst, src, flags);
    } else if (flags & QUOTE_FLAG_APPEND) {
	vstring_strcat(dst, src);
    } else {
	vstring_strcpy(dst, src);
    }
}

/* smtp_quote_821_address - quote non-empty envelope address */

void    smtp_quote_821_address(VSTRING *dst, const char *src)
{
    if (*src && var_smtp_quote_821_env) {
	quote_821_local(dst, src);
    } else {
	vstring_strcpy(dst, src);
    }
}
