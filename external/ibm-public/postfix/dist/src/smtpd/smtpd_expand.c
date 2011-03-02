/*	$NetBSD: smtpd_expand.c,v 1.1.1.1 2011/03/02 19:32:37 tron Exp $	*/

/*++
/* NAME
/*	smtpd_expand 3
/* SUMMARY
/*	SMTP server macro expansion
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_expand.h>
/*
/*	void	smtpd_expand_init()
/*
/*	int	smtpd_expand(state, result, template, flags)
/*	SMTPD_STATE *state;
/*	VSTRING	*result;
/*	const char *template;
/*	int	flags;
/* LOW_LEVEL INTERFACE
/*	VSTRING *smtpd_expand_filter;
/*
/*	const char *smtpd_expand_lookup(name, unused_mode, context)
/*	const char *name;
/*	int	unused_mode;
/*	char	*context;
/*	const char *template;
/* DESCRIPTION
/*	This module expands session-related macros.
/*
/*	smtpd_expand_init() performs one-time initialization.
/*
/*	smtpd_expand() expands macros in the template, using session
/*	attributes in the state argument, and writes the result to
/*	the result argument. The flags and result value are as with
/*	mac_expand().
/*
/*	smtpd_expand_filter and smtpd_expand_lookup() provide access
/*	to lower-level interfaces that are used by smtpd_expand().
/*	smtpd_expand_lookup() returns null when a string is not
/*	found (or when it is a null pointer).
/* DIAGNOSTICS
/*	Panic: interface violations. Fatal errors: out of memory.
/*	internal protocol errors. smtpd_expand() returns the binary
/*	OR of MAC_PARSE_ERROR (syntax error) and MAC_PARSE_UNDEF
/*	(undefined macro name).
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
#include <time.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mac_expand.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>

/* Application-specific. */

#include <smtpd.h>
#include <smtpd_expand.h>

 /*
  * Pre-parsed expansion filter.
  */
VSTRING *smtpd_expand_filter;

 /*
  * SLMs.
  */
#define STR	vstring_str

/* smtpd_expand_init - initialize once during process lifetime */

void    smtpd_expand_init(void)
{

    /*
     * Expand the expansion filter :-)
     */
    smtpd_expand_filter = vstring_alloc(10);
    unescape(smtpd_expand_filter, var_smtpd_exp_filter);
}

/* smtpd_expand_unknown - report unknown macro name */

static void smtpd_expand_unknown(const char *name)
{
    msg_warn("unknown macro name \"%s\" in expansion request", name);
}

/* smtpd_expand_addr - return address or substring thereof */

static const char *smtpd_expand_addr(VSTRING *buf, const char *addr,
				           const char *name, int prefix_len)
{
    const char *p;
    const char *suffix;

    /*
     * Return NULL only for unknown names in expansion requests.
     */
    if (addr == 0)
	return ("");

    suffix = name + prefix_len;

    /*
     * MAIL_ATTR_SENDER or MAIL_ATTR_RECIP.
     */
    if (*suffix == 0) {
	if (*addr)
	    return (addr);
	else
	    return ("<>");
    }

    /*
     * "sender_name" or "recipient_name".
     */
#define STREQ(x,y) (*(x) == *(y) && strcmp((x), (y)) == 0)

    else if (STREQ(suffix, MAIL_ATTR_S_NAME)) {
	if (*addr) {
	    if ((p = strrchr(addr, '@')) != 0) {
		vstring_strncpy(buf, addr, p - addr);
		return (STR(buf));
	    } else {
		return (addr);
	    }
	} else
	    return ("<>");
    }

    /*
     * "sender_domain" or "recipient_domain".
     */
    else if (STREQ(suffix, MAIL_ATTR_S_DOMAIN)) {
	if (*addr) {
	    if ((p = strrchr(addr, '@')) != 0) {
		return (p + 1);
	    } else {
		return ("");
	    }
	} else
	    return ("");
    }

    /*
     * Unknown. Return NULL to indicate an "unknown name" error.
     */
    else {
	smtpd_expand_unknown(name);
	return (0);
    }
}

/* smtpd_expand_lookup - generic SMTP attribute $name expansion */

const char *smtpd_expand_lookup(const char *name, int unused_mode,
				        char *context)
{
    SMTPD_STATE *state = (SMTPD_STATE *) context;
    time_t  now;
    struct tm *lt;

    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(10);

    if (msg_verbose > 1)
	msg_info("smtpd_expand_lookup: ${%s}", name);

#define STREQN(x,y,n) (*(x) == *(y) && strncmp((x), (y), (n)) == 0)
#define CONST_LEN(x)  (sizeof(x) - 1)

    /*
     * Don't query main.cf parameters, as the result of expansion could
     * reveal system-internal information in server replies.
     * 
     * XXX: This said, multiple servers may be behind a single client-visible
     * name or IP address, and each may generate its own logs. Therefore, it
     * may be useful to expose the replying MTA id (myhostname) in the
     * contact footer, to identify the right logs. So while we don't expose
     * the raw configuration dictionary, we do expose "$myhostname" as
     * expanded in var_myhostname.
     * 
     * Return NULL only for non-existent names.
     */
    if (STREQ(name, MAIL_ATTR_SERVER_NAME)) {
	return (var_myhostname);
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT)) {
	return (state->namaddr);
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT_PORT)) {
	return (state->port);
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT_ADDR)) {
	return (state->addr);
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT_NAME)) {
	return (state->name);
    } else if (STREQ(name, MAIL_ATTR_ACT_REVERSE_CLIENT_NAME)) {
	return (state->reverse_name);
    } else if (STREQ(name, MAIL_ATTR_ACT_HELO_NAME)) {
	return (state->helo_name ? state->helo_name : "");
    } else if (STREQN(name, MAIL_ATTR_SENDER, CONST_LEN(MAIL_ATTR_SENDER))) {
	return (smtpd_expand_addr(state->expand_buf, state->sender,
				  name, CONST_LEN(MAIL_ATTR_SENDER)));
    } else if (STREQN(name, MAIL_ATTR_RECIP, CONST_LEN(MAIL_ATTR_RECIP))) {
	return (smtpd_expand_addr(state->expand_buf, state->recipient,
				  name, CONST_LEN(MAIL_ATTR_RECIP)));
    } if (STREQ(name, MAIL_ATTR_LOCALTIME)) {
	if (time(&now) == (time_t) - 1)
	    msg_fatal("time lookup failed: %m");
	lt = localtime(&now);
	VSTRING_RESET(state->expand_buf);
	do {
	    VSTRING_SPACE(state->expand_buf, 100);
	} while (strftime(STR(state->expand_buf),
			  vstring_avail(state->expand_buf),
			  "%b %d %H:%M:%S", lt) == 0);
	return (STR(state->expand_buf));
    } else {
	smtpd_expand_unknown(name);
	return (0);
    }
}

/* smtpd_expand - expand session attributes in string */

int     smtpd_expand(SMTPD_STATE *state, VSTRING *result,
		             const char *template, int flags)
{
    return (mac_expand(result, template, flags, STR(smtpd_expand_filter),
		       smtpd_expand_lookup, (char *) state));
}
