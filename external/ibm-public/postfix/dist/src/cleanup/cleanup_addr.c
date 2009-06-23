/*	$NetBSD: cleanup_addr.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	cleanup_addr 3
/* SUMMARY
/*	process envelope addresses
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_addr_sender(state, addr)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*
/*	void	cleanup_addr_recipient(state, addr)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*
/*	void	cleanup_addr_bcc(state, addr)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/* DESCRIPTION
/*	This module processes envelope address records and writes the result
/*	to the queue file. Processing includes address rewriting and
/*	sender/recipient auto bcc address generation.
/*
/*	cleanup_addr_sender() processes sender envelope information and updates
/*	state->sender.
/*
/*	cleanup_addr_recipient() processes recipient envelope information
/*	and updates state->recip.
/*
/*	cleanup_addr_bcc() processes recipient envelope information. This
/*	is a separate function to avoid invoking cleanup_addr_recipient()
/*	recursively.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP buf
/*	Record content.
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
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <rec_type.h>
#include <cleanup_user.h>
#include <mail_params.h>
#include <ext_prop.h>
#include <mail_addr.h>
#include <canon_addr.h>
#include <mail_addr_find.h>
#include <mail_proto.h>
#include <dsn_mask.h>

/* Application-specific. */

#include "cleanup.h"

#define STR			vstring_str
#define IGNORE_EXTENSION	(char **) 0

/* cleanup_addr_sender - process envelope sender record */

void    cleanup_addr_sender(CLEANUP_STATE *state, const char *buf)
{
    VSTRING *clean_addr = vstring_alloc(100);
    const char *bcc;

    /*
     * Note: an unqualified envelope address is for all practical purposes
     * equivalent to a fully qualified local address, both for delivery and
     * for replying. Having to support both forms is error prone, therefore
     * an incomplete envelope address is rewritten to fully qualified form in
     * the local domain context.
     * 
     * 20000520: Replace mailer-daemon@$myorigin by the null address, to handle
     * bounced mail traffic more robustly.
     */
    cleanup_rewrite_internal(MAIL_ATTR_RWR_LOCAL, clean_addr, buf);
    if (strncasecmp(STR(clean_addr), MAIL_ADDR_MAIL_DAEMON "@",
		    sizeof(MAIL_ADDR_MAIL_DAEMON)) == 0) {
	canon_addr_internal(state->temp1, MAIL_ADDR_MAIL_DAEMON);
	if (strcasecmp(STR(clean_addr), STR(state->temp1)) == 0)
	    vstring_strcpy(clean_addr, "");
    }
    if (state->flags & CLEANUP_FLAG_MAP_OK) {
	if (cleanup_send_canon_maps
	    && (cleanup_send_canon_flags & CLEANUP_CANON_FLAG_ENV_FROM))
	    cleanup_map11_internal(state, clean_addr, cleanup_send_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps
	    && (cleanup_comm_canon_flags & CLEANUP_CANON_FLAG_ENV_FROM))
	    cleanup_map11_internal(state, clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_masq_domains
	    && (cleanup_masq_flags & CLEANUP_MASQ_FLAG_ENV_FROM))
	    cleanup_masquerade_internal(clean_addr, cleanup_masq_domains);
    }
    CLEANUP_OUT_BUF(state, REC_TYPE_FROM, clean_addr);
    if (state->sender)				/* XXX Can't happen */
	myfree(state->sender);
    state->sender = mystrdup(STR(clean_addr));	/* Used by Milter client */
    if ((state->flags & CLEANUP_FLAG_BCC_OK)
	&& *STR(clean_addr)
	&& cleanup_send_bcc_maps
	&& (bcc = mail_addr_find(cleanup_send_bcc_maps, STR(clean_addr),
				 IGNORE_EXTENSION)) != 0)
	cleanup_addr_bcc(state, bcc);
    vstring_free(clean_addr);
}

/* cleanup_addr_recipient - process envelope recipient */

void    cleanup_addr_recipient(CLEANUP_STATE *state, const char *buf)
{
    VSTRING *clean_addr = vstring_alloc(100);
    const char *bcc;

    /*
     * Note: an unqualified envelope address is for all practical purposes
     * equivalent to a fully qualified local address, both for delivery and
     * for replying. Having to support both forms is error prone, therefore
     * an incomplete envelope address is rewritten to fully qualified form in
     * the local domain context.
     */
    cleanup_rewrite_internal(MAIL_ATTR_RWR_LOCAL,
			     clean_addr, *buf ? buf : var_empty_addr);
    if (state->flags & CLEANUP_FLAG_MAP_OK) {
	if (cleanup_rcpt_canon_maps
	    && (cleanup_rcpt_canon_flags & CLEANUP_CANON_FLAG_ENV_RCPT))
	    cleanup_map11_internal(state, clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps
	    && (cleanup_comm_canon_flags & CLEANUP_CANON_FLAG_ENV_RCPT))
	    cleanup_map11_internal(state, clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_masq_domains
	    && (cleanup_masq_flags & CLEANUP_MASQ_FLAG_ENV_RCPT))
	    cleanup_masquerade_internal(clean_addr, cleanup_masq_domains);
    }
    cleanup_out_recipient(state, state->dsn_orcpt, state->dsn_notify,
			  state->orig_rcpt, STR(clean_addr));
    if (state->recip)				/* This can happen */
	myfree(state->recip);
    state->recip = mystrdup(STR(clean_addr));	/* Used by Milter client */
    if ((state->flags & CLEANUP_FLAG_BCC_OK)
	&& *STR(clean_addr)
	&& cleanup_rcpt_bcc_maps
	&& (bcc = mail_addr_find(cleanup_rcpt_bcc_maps, STR(clean_addr),
				 IGNORE_EXTENSION)) != 0)
	cleanup_addr_bcc(state, bcc);
    vstring_free(clean_addr);
}

/* cleanup_addr_bcc - process automatic BCC recipient */

void    cleanup_addr_bcc(CLEANUP_STATE *state, const char *bcc)
{
    VSTRING *clean_addr = vstring_alloc(100);

    /*
     * Note: BCC addresses are supplied locally, and must be rewritten in the
     * local address rewriting context.
     */
#define NO_DSN_ORCPT	((char *) 0)

    cleanup_rewrite_internal(MAIL_ATTR_RWR_LOCAL, clean_addr, bcc);
    if (state->flags & CLEANUP_FLAG_MAP_OK) {
	if (cleanup_rcpt_canon_maps
	    && (cleanup_rcpt_canon_flags & CLEANUP_CANON_FLAG_ENV_RCPT))
	    cleanup_map11_internal(state, clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps
	    && (cleanup_comm_canon_flags & CLEANUP_CANON_FLAG_ENV_RCPT))
	    cleanup_map11_internal(state, clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_masq_domains
	    && (cleanup_masq_flags & CLEANUP_MASQ_FLAG_ENV_RCPT))
	    cleanup_masquerade_internal(clean_addr, cleanup_masq_domains);
    }
    cleanup_out_recipient(state, NO_DSN_ORCPT, DSN_NOTIFY_NEVER,
			  STR(clean_addr), STR(clean_addr));
    vstring_free(clean_addr);
}
