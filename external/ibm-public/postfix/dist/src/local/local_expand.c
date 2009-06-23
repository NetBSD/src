/*	$NetBSD: local_expand.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	local_expand 3
/* SUMMARY
/*	set up attribute list for $name expansion
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	local_expand(result, pattern, state, usr_attr, filter)
/*	VSTRING	*result;
/*	const	char *pattern;
/*	LOCAL_STATE state;
/*	USER_ATTR usr_attr;
/*	const char *filter;
/* DESCRIPTION
/*	local_expand() performs conditional and unconditional $name
/*	expansion based on message delivery attributes.
/*	The result is the bitwise OR or zero or more of the following:
/* .IP LOCAL_EXP_EXTENSION_MATCHED
/*	The result of expansion contains the $extension attribute.
/* .IP MAC_PARSE_XXX
/*	See mac_parse(3).
/* .PP
/*	Attributes:
/* .IP client_address
/*	The client network address.
/* .IP client_helo
/*	The client HELO command parameter.
/* .IP client_hostname
/*	The client hostname.
/* .IP client_protocol
/*	The client protocol.
/* .IP domain
/*	The recipient address domain.
/* .IP extension
/*	The recipient address extension.
/* .IP home
/*	The recipient home directory.
/* .IP local
/*	The entire recipient address localpart.
/* .IP recipient
/*	The entire recipient address.
/* .IP recipient_delimiter
/*	The recipient delimiter.
/* .IP shell
/*	The recipient shell program.
/* .IP sasl_method
/*	The SASL authentication method.
/* .IP sasl_sender
/*	The SASL MAIL FROM address.
/* .IP sasl_username
/*	The SASL login name.
/* .IP user
/*	The recipient user name.
/* .PP
/*	Arguments:
/* .IP result
/*	Storage for the result of expansion. The buffer is truncated
/*	upon entry.
/* .IP pattern
/*	The string with unconditional and conditional macro expansions.
/* .IP state
/*	Message delivery attributes (sender, recipient etc.).
/*	Attributes describing alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/*	A table with delivered-to: addresses taken from the message.
/* .IP usr_attr
/*	Attributes describing user rights and environment.
/* .IP filter
/*	A null pointer, or a string of allowed characters in $name
/*	expansions. Illegal characters are replaced by underscores.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
/* SEE ALSO
/*	mac_expand(3) macro expansion
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
#include <string.h>

/* Utility library. */

#include <vstring.h>
#include <mac_expand.h>

/* Global library */

#include <mail_params.h>

/* Application-specific. */

#include "local.h"

typedef struct {
    LOCAL_STATE *state;
    USER_ATTR *usr_attr;
    int     status;
} LOCAL_EXP;

/* local_expand_lookup - mac_expand() lookup routine */

static const char *local_expand_lookup(const char *name, int mode, char *ptr)
{
    LOCAL_EXP *local = (LOCAL_EXP *) ptr;

#define STREQ(x,y) (*(x) == *(y) && strcmp((x), (y)) == 0)

    if (STREQ(name, "user")) {
	return (local->state->msg_attr.user);
    } else if (STREQ(name, "home")) {
	return (local->usr_attr->home);
    } else if (STREQ(name, "shell")) {
	return (local->usr_attr->shell);
    } else if (STREQ(name, "domain")) {
	return (local->state->msg_attr.domain);
    } else if (STREQ(name, "local")) {
	return (local->state->msg_attr.local);
    } else if (STREQ(name, "mailbox")) {
	return (local->state->msg_attr.local);
    } else if (STREQ(name, "recipient")) {
	return (local->state->msg_attr.rcpt.address);
    } else if (STREQ(name, "extension")) {
	if (mode == MAC_EXP_MODE_USE)
	    local->status |= LOCAL_EXP_EXTENSION_MATCHED;
	return (local->state->msg_attr.extension);
    } else if (STREQ(name, "recipient_delimiter")) {
	return (*var_rcpt_delim ? var_rcpt_delim : 0);
#if 0
    } else if (STREQ(name, "client_hostname")) {
	return (local->state->msg_attr.request->client_name);
    } else if (STREQ(name, "client_address")) {
	return (local->state->msg_attr.request->client_addr);
    } else if (STREQ(name, "client_protocol")) {
	return (local->state->msg_attr.request->client_proto);
    } else if (STREQ(name, "client_helo")) {
	return (local->state->msg_attr.request->client_helo);
    } else if (STREQ(name, "sasl_method")) {
	return (local->state->msg_attr.request->sasl_method);
    } else if (STREQ(name, "sasl_sender")) {
	return (local->state->msg_attr.request->sasl_sender);
    } else if (STREQ(name, "sasl_username")) {
	return (local->state->msg_attr.request->sasl_username);
#endif
    } else {
	return (0);
    }
}

/* local_expand - expand message delivery attributes */

int     local_expand(VSTRING *result, const char *pattern,
	        LOCAL_STATE *state, USER_ATTR *usr_attr, const char *filter)
{
    LOCAL_EXP local;
    int     expand_status;

    local.state = state;
    local.usr_attr = usr_attr;
    local.status = 0;
    expand_status = mac_expand(result, pattern, MAC_EXP_FLAG_NONE,
			       filter, local_expand_lookup, (char *) &local);
    return (local.status | expand_status);
}
