/*	$NetBSD: smtp_key.c,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

/*++
/* NAME
/*	smtp_key 3
/* SUMMARY
/*	cache/table lookup key management
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	char	*smtp_key_prefix(buffer, delim_na, iterator, context_flags)
/*	VSTRING	*buffer;
/*	const char *delim_na;
/*	SMTP_ITERATOR *iterator;
/*	int	context_flags;
/* DESCRIPTION
/*	The Postfix SMTP server accesses caches and lookup tables,
/*	using lookup keys that contain information from various
/*	contexts: per-server configuration, per-request envelope,
/*	and results from DNS queries.
/*
/*	These lookup keys sometimes share the same context information.
/*	The primary purpose of this API is to ensure that this
/*	shared context is used consistently, and that its use is
/*	made explicit (both are needed to verify that there is no
/*	false cache sharing).
/*
/*	smtp_key_prefix() constructs a lookup key prefix from context
/*	that may be shared with other lookup keys. The user is free
/*	to append additional application-specific context. The result
/*	value is a pointer to the result text.
/*
/*	Arguments:
/* .IP buffer
/*	Storage for the result.
/* .IP delim_na
/*	The field delimiter character, and the optional place holder
/*	character for a) information that is unavailable, b)
/*	information that is inapplicable, or c) that would result
/*	in an empty field.  Key fields that contain "delim_na"
/*	characters will be base64-encoded.
/*	Do not specify "delim_na" characters that are part of the
/*	base64 character set.
/* .IP iterator
/*	Information that will be selected by the specified flags.
/* .IP context_flags
/*	Bit-wise OR of one or more of the following.
/* .RS
/* .IP SMTP_KEY_FLAG_SERVICE
/*	The global service name. This is a proxy for
/*	destination-independent and request-independent context.
/* .IP SMTP_KEY_FLAG_SENDER
/*	The envelope sender address. This is a proxy for sender-dependent
/*	context, such as per-sender SASL authentication.
/* .IP SMTP_KEY_FLAG_REQ_NEXTHOP
/*	The delivery request nexthop destination, including optional
/*	[] and :port (the same form that users specify in a SASL
/*	password or TLS policy lookup table). This is a proxy for
/*	destination-dependent, but host-independent context.
/* .IP SMTP_KEY_FLAG_CUR_NEXTHOP
/*	The current iterator's nexthop destination (delivery request
/*	nexthop or fallback nexthop, including optional [] and
/*	:port).
/* .IP SMTP_KEY_FLAG_HOSTNAME
/*	The current iterator's remote hostname.
/* .IP SMTP_KEY_FLAG_ADDR
/*	The current iterator's remote address.
/* .IP SMTP_KEY_FLAG_PORT
/*	The current iterator's remote port.
/* .RE
/* DIAGNOSTICS
/*	Panic: undefined flag or zero flags. Fatal: out of memory.
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <netinet/in.h>			/* ntohs() for Solaris or BSD */
#include <arpa/inet.h>			/* ntohs() for Linux or BSD */
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <vstring.h>
#include <base64_code.h>

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * Application-specific.
  */
#include <smtp.h>

 /*
  * We use a configurable field terminator and optional place holder for data
  * that is unavailable or inapplicable. We base64-encode content that
  * contains these characters, and content that needs obfuscation.
  */

/* smtp_key_append_na - append place-holder key field */

static void smtp_key_append_na(VSTRING *buffer, const char *delim_na)
{
    if (delim_na[1] != 0)
	VSTRING_ADDCH(buffer, delim_na[1]);
    VSTRING_ADDCH(buffer, delim_na[0]);
}

/* smtp_key_append_str - append string-valued key field */

static void smtp_key_append_str(VSTRING *buffer, const char *str,
				        const char *delim_na)
{
    if (str == 0 || str[0] == 0) {
	smtp_key_append_na(buffer, delim_na);
    } else if (str[strcspn(str, delim_na)] != 0) {
	base64_encode_opt(buffer, str, strlen(str), BASE64_FLAG_APPEND);
	VSTRING_ADDCH(buffer, delim_na[0]);
    } else {
	vstring_sprintf_append(buffer, "%s%c", str, delim_na[0]);
    }
}

/* smtp_key_append_uint - append unsigned-valued key field */

static void smtp_key_append_uint(VSTRING *buffer, unsigned num,
				         const char *delim_na)
{
    vstring_sprintf_append(buffer, "%u%c", num, delim_na[0]);
}

/* smtp_key_prefix - format common elements in lookup key */

char   *smtp_key_prefix(VSTRING *buffer, const char *delim_na,
			        SMTP_ITERATOR *iter, int flags)
{
    static const char myname[] = "smtp_key_prefix";
    SMTP_STATE *state = iter->parent;	/* private member */

    /*
     * Sanity checks.
     */
    if (state == 0)
	msg_panic("%s: no parent state", myname);
    if (flags & ~SMTP_KEY_MASK_ALL)
	msg_panic("%s: unknown key flags 0x%x",
		  myname, flags & ~SMTP_KEY_MASK_ALL);
    if (flags == 0)
	msg_panic("%s: zero flags", myname);

    /*
     * Initialize.
     */
    VSTRING_RESET(buffer);

    /*
     * Per-service and per-request context.
     */
    if (flags & SMTP_KEY_FLAG_SERVICE)
	smtp_key_append_str(buffer, state->service, delim_na);
    if (flags & SMTP_KEY_FLAG_SENDER)
	smtp_key_append_str(buffer, state->request->sender, delim_na);

    /*
     * Per-destination context, non-canonicalized form.
     */
    if (flags & SMTP_KEY_FLAG_REQ_NEXTHOP)
	smtp_key_append_str(buffer, STR(iter->request_nexthop), delim_na);
    if (flags & SMTP_KEY_FLAG_CUR_NEXTHOP)
	smtp_key_append_str(buffer, STR(iter->dest), delim_na);

    /*
     * Per-host context, canonicalized form.
     */
    if (flags & SMTP_KEY_FLAG_HOSTNAME)
	smtp_key_append_str(buffer, STR(iter->host), delim_na);
    if (flags & SMTP_KEY_FLAG_ADDR)
	smtp_key_append_str(buffer, STR(iter->addr), delim_na);
    if (flags & SMTP_KEY_FLAG_PORT)
	smtp_key_append_uint(buffer, ntohs(iter->port), delim_na);

    /*
     * Requested TLS level, if applicable. TODO(tlsproxy) should the lookup
     * engine also try the requested TLS level and 'stronger', in case a
     * server hosts multiple domains with different TLS requirements?
     */
    if (flags & SMTP_KEY_FLAG_TLS_LEVEL)
#ifdef USE_TLS
	smtp_key_append_uint(buffer, state->tls->level, delim_na);
#else
	smtp_key_append_na(buffer, delim_na);
#endif

    VSTRING_TERMINATE(buffer);

    return STR(buffer);
}
