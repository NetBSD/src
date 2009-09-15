/*	$NetBSD: smtpd_xforward.c,v 1.1.1.1.2.2 2009/09/15 06:03:34 snj Exp $	*/

/*++
/* NAME
/*	smtpd_xforward 3
/* SUMMARY
/*	maintain XCLIENT information
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	void	smtpd_xforward_init(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_xforward_preset(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_xforward_reset(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	smtpd_xforward_init() zeroes the attributes for storage of
/*	XFORWARD command parameters.
/*
/*	smtpd_xforward_preset() takes the result from smtpd_xforward_init()
/*	and sets all fields to the same "unknown" value that regular
/*	client attributes would have.
/*
/*	smtpd_xforward_reset() restores the state from smtpd_xforward_init().
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

#include <mymalloc.h>
#include <msg.h>

/* Global library. */

#include <mail_proto.h>

/* Application-specific. */

#include <smtpd.h>

/* smtpd_xforward_init - initialize XCLIENT attributes */

void    smtpd_xforward_init(SMTPD_STATE *state)
{
    state->xforward.flags = 0;
    state->xforward.name = 0;
    state->xforward.addr = 0;
    state->xforward.port = 0;
    state->xforward.namaddr = 0;
    state->xforward.protocol = 0;
    state->xforward.helo_name = 0;
    state->xforward.ident = 0;
    state->xforward.domain = 0;
}

/* smtpd_xforward_preset - set xforward attributes to "unknown" */

void    smtpd_xforward_preset(SMTPD_STATE *state)
{

    /*
     * Sanity checks.
     */
    if (state->xforward.flags)
	msg_panic("smtpd_xforward_preset: bad flags: 0x%x",
		  state->xforward.flags);

    /*
     * This is a temporary solution. Unknown forwarded attributes get the
     * same values as unknown normal attributes, so that we don't break
     * assumptions in pre-existing code.
     */
    state->xforward.flags = SMTPD_STATE_XFORWARD_INIT;
    state->xforward.name = mystrdup(CLIENT_NAME_UNKNOWN);
    state->xforward.addr = mystrdup(CLIENT_ADDR_UNKNOWN);
    state->xforward.port = mystrdup(CLIENT_PORT_UNKNOWN);
    state->xforward.namaddr = mystrdup(CLIENT_NAMADDR_UNKNOWN);
    state->xforward.rfc_addr = mystrdup(CLIENT_ADDR_UNKNOWN);
    /* Leave helo at zero. */
    state->xforward.protocol = mystrdup(CLIENT_PROTO_UNKNOWN);
    /* Leave ident at zero. */
    /* Leave domain context at zero. */
}

/* smtpd_xforward_reset - reset XCLIENT attributes */

void    smtpd_xforward_reset(SMTPD_STATE *state)
{
#define FREE_AND_WIPE(s) { if (s) myfree(s); s = 0; }

    state->xforward.flags = 0;
    FREE_AND_WIPE(state->xforward.name);
    FREE_AND_WIPE(state->xforward.addr);
    FREE_AND_WIPE(state->xforward.port);
    FREE_AND_WIPE(state->xforward.namaddr);
    FREE_AND_WIPE(state->xforward.rfc_addr);
    FREE_AND_WIPE(state->xforward.protocol);
    FREE_AND_WIPE(state->xforward.helo_name);
    FREE_AND_WIPE(state->xforward.ident);
    FREE_AND_WIPE(state->xforward.domain);
}
