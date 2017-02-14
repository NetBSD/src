/*	$NetBSD: cleanup_addr.c,v 1.2 2017/02/14 01:16:44 christos Exp $	*/

/*++
/* NAME
/*	cleanup_addr 3
/* SUMMARY
/*	process envelope addresses
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	off_t	cleanup_addr_sender(state, addr)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*
/*	void	cleanup_addr_recipient(state, addr)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*
/*	void	cleanup_addr_bcc_dsn(state, addr, dsn_orcpt, dsn_notify)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*	const char *dsn_orcpt;
/*	int	dsn_notify;
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
/*	state->sender. The result value is the offset of the record that
/*	follows the sender record if milters are enabled, otherwise zero.
/*
/*	cleanup_addr_recipient() processes recipient envelope information
/*	and updates state->recip.
/*
/*	cleanup_addr_bcc_dsn() processes recipient envelope information. This
/*	is a separate function to avoid invoking cleanup_addr_recipient()
/*	recursively.
/*
/*	cleanup_addr_bcc() is a backwards-compatibility wrapper for
/*	cleanup_addr_bcc_dsn() that requests no delivery status
/*	notification for the recipient.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP buf
/*	Record content.
/* .IP dsn_orcpt
/*	The DSN original recipient (or NO_DSN_ORCPT to specify none).
/* .IP dsn_notify
/*	DSN notification options. Specify NO_DSN_NOTIFY to disable
/*	notification, and DEF_DSN_NOTIFY for default notification.
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <rec_type.h>
#include <record.h>
#include <cleanup_user.h>
#include <mail_params.h>
#include <ext_prop.h>
#include <mail_addr.h>
#include <canon_addr.h>
#include <mail_addr_find.h>
#include <mail_proto.h>
#include <dsn_mask.h>
#include <smtputf8.h>

/* Application-specific. */

#include "cleanup.h"

#define STR			vstring_str
#define LEN			VSTRING_LEN
#define IGNORE_EXTENSION	(char **) 0

/* cleanup_addr_sender - process envelope sender record */

off_t   cleanup_addr_sender(CLEANUP_STATE *state, const char *buf)
{
    const char myname[] = "cleanup_addr_sender";
    VSTRING *clean_addr = vstring_alloc(100);
    off_t   after_sender_offs = 0;
    const char *bcc;
    size_t  len;

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
    if (strncasecmp_utf8(STR(clean_addr), MAIL_ADDR_MAIL_DAEMON "@",
			 sizeof(MAIL_ADDR_MAIL_DAEMON)) == 0) {
	canon_addr_internal(state->temp1, MAIL_ADDR_MAIL_DAEMON);
	if (strcasecmp_utf8(STR(clean_addr), STR(state->temp1)) == 0)
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
	    cleanup_masquerade_internal(state, clean_addr, cleanup_masq_domains);
    }
    /* Fix 20140711: Auto-detect an UTF8 sender. */
    if (var_smtputf8_enable && *STR(clean_addr) && !allascii(STR(clean_addr))
	&& valid_utf8_string(STR(clean_addr), LEN(clean_addr))) {
	state->smtputf8 |= SMTPUTF8_FLAG_SENDER;
	/* Fix 20140713: request SMTPUTF8 support selectively. */
	if (state->flags & CLEANUP_FLAG_AUTOUTF8)
	    state->smtputf8 |= SMTPUTF8_FLAG_REQUESTED;
    }
    CLEANUP_OUT_BUF(state, REC_TYPE_FROM, clean_addr);
    if (state->sender)				/* XXX Can't happen */
	myfree(state->sender);
    state->sender = mystrdup(STR(clean_addr));	/* Used by Milter client */
    /* Fix 20160310: Moved from cleanup_envelope.c. */
    if (state->milters || cleanup_milters) {
	/* Make room to replace sender. */
	if ((len = LEN(clean_addr)) < REC_TYPE_PTR_PAYL_SIZE)
	    rec_pad(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_PAYL_SIZE - len);
	/* Remember the after-sender record offset. */
	if ((after_sender_offs = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
    }
    if ((state->flags & CLEANUP_FLAG_BCC_OK)
	&& *STR(clean_addr)
	&& cleanup_send_bcc_maps) {
	if ((bcc = mail_addr_find(cleanup_send_bcc_maps, STR(clean_addr),
				  IGNORE_EXTENSION)) != 0) {
	    cleanup_addr_bcc(state, bcc);
	} else if (cleanup_send_bcc_maps->error) {
	    msg_warn("%s: %s map lookup problem -- "
		     "message not accepted, try again later",
		     state->queue_id, cleanup_send_bcc_maps->title);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
    }
    vstring_free(clean_addr);
    return after_sender_offs;
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
	    cleanup_masquerade_internal(state, clean_addr, cleanup_masq_domains);
    }
    /* Fix 20140711: Auto-detect an UTF8 recipient. */
    if (var_smtputf8_enable && *STR(clean_addr) && !allascii(STR(clean_addr))
	&& valid_utf8_string(STR(clean_addr), LEN(clean_addr))) {
	/* Fix 20140713: request SMTPUTF8 support selectively. */
	if (state->flags & CLEANUP_FLAG_AUTOUTF8)
	    state->smtputf8 |= SMTPUTF8_FLAG_REQUESTED;
    }
    /* Fix 20141024: Don't fake up a "bare" DSN original rcpt in smtp(8). */
    if (state->dsn_orcpt == 0 && *STR(clean_addr) != 0)
	state->dsn_orcpt = concatenate((!allascii(STR(clean_addr))
			   && (state->smtputf8 & SMTPUTF8_FLAG_REQUESTED)) ?
		      "utf-8" : "rfc822", ";", STR(clean_addr), (char *) 0);
    cleanup_out_recipient(state, state->dsn_orcpt, state->dsn_notify,
			  state->orig_rcpt, STR(clean_addr));
    if (state->recip)				/* This can happen */
	myfree(state->recip);
    state->recip = mystrdup(STR(clean_addr));	/* Used by Milter client */
    if ((state->flags & CLEANUP_FLAG_BCC_OK)
	&& *STR(clean_addr)
	&& cleanup_rcpt_bcc_maps) {
	if ((bcc = mail_addr_find(cleanup_rcpt_bcc_maps, STR(clean_addr),
				  IGNORE_EXTENSION)) != 0) {
	    cleanup_addr_bcc(state, bcc);
	} else if (cleanup_rcpt_bcc_maps->error) {
	    msg_warn("%s: %s map lookup problem -- "
		     "message not accepted, try again later",
		     state->queue_id, cleanup_rcpt_bcc_maps->title);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
    }
    vstring_free(clean_addr);
}

/* cleanup_addr_bcc_dsn - process automatic BCC recipient */

void    cleanup_addr_bcc_dsn(CLEANUP_STATE *state, const char *bcc,
			             const char *dsn_orcpt, int dsn_notify)
{
    VSTRING *clean_addr = vstring_alloc(100);

    /*
     * Note: BCC addresses are supplied locally, and must be rewritten in the
     * local address rewriting context.
     */
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
	    cleanup_masquerade_internal(state, clean_addr, cleanup_masq_domains);
    }
    /* Fix 20140711: Auto-detect an UTF8 recipient. */
    if (var_smtputf8_enable && *STR(clean_addr) && !allascii(STR(clean_addr))
	&& valid_utf8_string(STR(clean_addr), LEN(clean_addr))) {
	/* Fix 20140713: request SMTPUTF8 support selectively. */
	if (state->flags & CLEANUP_FLAG_AUTOUTF8)
	    state->smtputf8 |= SMTPUTF8_FLAG_REQUESTED;
    }
    cleanup_out_recipient(state, dsn_orcpt, dsn_notify,
			  STR(clean_addr), STR(clean_addr));
    vstring_free(clean_addr);
}
