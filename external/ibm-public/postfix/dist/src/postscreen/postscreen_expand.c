/*	$NetBSD: postscreen_expand.c,v 1.1.1.1 2011/03/02 19:32:26 tron Exp $	*/

/*++
/* NAME
/*	postscreen_expand 3
/* SUMMARY
/*	SMTP server macro expansion
/* SYNOPSIS
/*	#include <postscreen.h>
/*
/*	void	psc_expand_init()
/*
/*	VSTRING *psc_expand_filter;
/*
/*	const char *psc_expand_lookup(name, unused_mode, context)
/*	const char *name;
/*	int	unused_mode;
/*	char	*context;
/* DESCRIPTION
/*	This module expands session-related macros.
/*
/*	psc_expand_init() performs one-time initialization
/*	of the psc_expand_filter buffer.
/*
/*	The psc_expand_filter buffer contains the characters
/*	that are allowed in macro expansion, as specified with the
/*	psc_expand_filter configuration parameter.
/*
/*	psc_expand_lookup() returns the value of the named
/*	macro or a null pointer.
/*
/*	Arguments:
/* .IP name
/*	Macro name.
/* .IP context
/*	Call-back context (a PSC_STATE pointer).
/* DIAGNOSTICS
/*	Panic: interface violations. Fatal errors: out of memory.
/*	internal protocol errors. postscreen_expand() returns the
/*	binary OR of MAC_PARSE_ERROR (syntax error) and MAC_PARSE_UNDEF
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
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>

/* Application-specific. */

#include <postscreen.h>

 /*
  * Pre-parsed expansion filter.
  */
VSTRING *psc_expand_filter;

/* psc_expand_init - initialize once during process lifetime */

void    psc_expand_init(void)
{

    /*
     * Expand the expansion filter :-)
     */
    psc_expand_filter = vstring_alloc(10);
    unescape(psc_expand_filter, var_psc_exp_filter);
}

/* psc_expand_lookup - generic SMTP attribute $name expansion */

const char *psc_expand_lookup(const char *name, int unused_mode,
				        char *context)
{
    PSC_STATE *state = (PSC_STATE *) context;
    time_t  now;
    struct tm *lt;

    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(10);

    if (msg_verbose > 1)
	msg_info("psc_expand_lookup: ${%s}", name);

#define STREQ(x,y)    (*(x) == *(y) && strcmp((x), (y)) == 0)
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
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT_ADDR)) {
	return (state->smtp_client_addr);
    } else if (STREQ(name, MAIL_ATTR_ACT_CLIENT_PORT)) {
	return (state->smtp_client_port);
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
	msg_warn("unknown macro name \"%s\" in expansion request", name);
	return (0);
    }
}
