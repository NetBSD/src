/*	$NetBSD: defer.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	defer 3
/* SUMMARY
/*	defer service client interface
/* SYNOPSIS
/*	#include <defer.h>
/*
/*	int	defer_append(flags, id, stats, rcpt, relay, dsn)
/*	int	flags;
/*	const char *id;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/*	DSN	*dsn;
/*
/*	int	defer_flush(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*
/*	int	defer_warn(flags, queue, id, encoding, smtputf8, sender,
				dsn_envid, dsn_ret)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*
/*	int	defer_one(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, ret, stats, recipient, relay, dsn)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/*	DSN	*dsn;
/* INTERNAL API
/*	int	defer_append_intern(flags, id, stats, rcpt, relay, dsn)
/*	int	flags;
/*	const char *id;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/* DESCRIPTION
/*	This module implements a client interface to the defer service,
/*	which maintains a per-message logfile with status records for
/*	each recipient whose delivery is deferred, and the dsn_text why.
/*
/*	defer_append() appends a record to the per-message defer log,
/*	with the dsn_text for delayed delivery to the named rcpt,
/*	updates the address verification service, or updates a message
/*	delivery record on request by the sender. The flags argument
/*	determines the action.
/*	The result is a convenient non-zero value.
/*	When the fast flush cache is enabled, the fast flush server is
/*	notified of deferred mail.
/*
/*	defer_flush() bounces the specified message to the specified
/*	sender, including the defer log that was built with defer_append().
/*	defer_flush() requests that the deferred recipients are deleted
/*	from the original queue file; the defer logfile is deleted after
/*	successful completion.
/*	The result is zero in case of success, non-zero otherwise.
/*
/*	defer_warn() sends a warning message that the mail in
/*	question has been deferred.  The defer log is not deleted,
/*	and no recipients are deleted from the original queue file.
/*
/*	defer_one() implements dsn_filter(3) compatibility for the
/*	bounce_one() routine.
/*
/*	defer_append_intern() is for use after the DSN filter.
/*
/*	Arguments:
/* .IP flags
/*	The bit-wise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to explicitly request not special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the defer log in case of an error (as in: pretend
/*	that we never even tried to defer this message).
/* .IP BOUNCE_FLAG_DELRCPT
/*	When specified with a flush request, request that
/*	recipients be deleted from the queue file.
/*
/*	Note: the bounce daemon ignores this request when the
/*	recipient queue file offset is <= 0.
/* .IP DEL_REQ_FLAG_MTA_VRFY
/*	The message is an MTA-requested address verification probe.
/*	Update the address verification database instead of deferring
/*	mail.
/* .IP DEL_REQ_FLAG_USR_VRFY
/*	The message is a user-requested address expansion probe.
/*	Update the message delivery record instead of deferring
/*	mail.
/* .IP DEL_REQ_FLAG_RECORD
/*	This is a normal message with logged delivery. Update the
/*	message delivery record and defer mail delivery.
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The queue id of the original message file.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP rcpt
/*	Recipient information. See recipient_list(3).
/* .IP relay
/*	Host we could not talk to.
/* .IP dsn
/*	Delivery status. See dsn(3). The specified action is ignored.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP smtputf8
/*	The level of SMTPUTF8 support (to be defined).
/* .IP sender
/*	The sender envelope address.
/* .IP dsn_envid
/*	Optional DSN envelope ID.
/* .IP dsn_ret
/*	Optional DSN return full/headers option.
/* .PP
/*	For convenience, these functions always return a non-zero result.
/* DIAGNOSTICS
/*	Warnings: problems connecting to the defer service.
/*	Fatal: out of memory.
/* BUGS
/*	Should be replaced by routines with an attribute-value based
/*	interface instead of an interface that uses a rigid argument list.
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

#include <msg.h>
#include <vstring.h>

/* Global library. */

#define DSN_INTERN
#include <mail_params.h>
#include <mail_queue.h>
#include <mail_proto.h>
#include <flush_clnt.h>
#include <verify.h>
#include <dsn_util.h>
#include <rcpt_print.h>
#include <dsn_print.h>
#include <log_adhoc.h>
#include <trace.h>
#include <defer.h>

#define STR(x)	vstring_str(x)

/* defer_append - defer message delivery */

int     defer_append(int flags, const char *id, MSG_STATS *stats,
		             RECIPIENT *rcpt, const char *relay,
		             DSN *dsn)
{
    DSN     my_dsn = *dsn;
    DSN    *dsn_res;

    /*
     * Sanity check.
     */
    if (my_dsn.status[0] != '4' || !dsn_valid(my_dsn.status)) {
	msg_warn("defer_append: ignoring dsn code \"%s\"", my_dsn.status);
	my_dsn.status = "4.0.0";
    }

    /*
     * DSN filter (Postfix 3.0).
     */
    if (delivery_status_filter != 0
    && (dsn_res = dsn_filter_lookup(delivery_status_filter, &my_dsn)) != 0) {
	if (dsn_res->status[0] == '5')
	    return (bounce_append_intern(flags, id, stats, rcpt, relay, dsn_res));
	my_dsn = *dsn_res;
    }
    return (defer_append_intern(flags, id, stats, rcpt, relay, &my_dsn));
}

/* defer_append_intern - defer message delivery */

int     defer_append_intern(int flags, const char *id, MSG_STATS *stats,
			            RECIPIENT *rcpt, const char *relay,
			            DSN *dsn)
{
    const char *rcpt_domain;
    DSN     my_dsn = *dsn;
    int     status;

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_MTA_VRFY) {
	my_dsn.action = "undeliverable";
	status = verify_append(id, stats, rcpt, relay, &my_dsn,
			       DEL_RCPT_STAT_DEFER);
	return (status);
    }

    /*
     * User-requested address verification information is logged and mailed
     * to the requesting user.
     */
    if (flags & DEL_REQ_FLAG_USR_VRFY) {
	my_dsn.action = "undeliverable";
	status = trace_append(flags, id, stats, rcpt, relay, &my_dsn);
	return (status);
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     * 
     * XXX DSN We write all deferred recipients to the defer logfile regardless
     * of DSN NOTIFY options, because those options don't apply to mailq(1)
     * reports or to postmaster notifications.
     */
    else {

	/*
	 * Supply default action.
	 */
	my_dsn.action = "delayed";

	if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			   SEND_ATTR_INT(MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND),
				SEND_ATTR_INT(MAIL_ATTR_FLAGS, flags),
				SEND_ATTR_STR(MAIL_ATTR_QUEUEID, id),
				SEND_ATTR_FUNC(rcpt_print, (void *) rcpt),
				SEND_ATTR_FUNC(dsn_print, (void *) &my_dsn),
				ATTR_TYPE_END) != 0)
	    msg_warn("%s: %s service failure", id, var_defer_service);
	log_adhoc(id, stats, rcpt, relay, &my_dsn, "deferred");

	/*
	 * Traced delivery.
	 */
	if (flags & DEL_REQ_FLAG_RECORD)
	    if (trace_append(flags, id, stats, rcpt, relay, &my_dsn) != 0)
		msg_warn("%s: %s service failure", id, var_trace_service);

	/*
	 * Notify the fast flush service. XXX Should not this belong in the
	 * bounce/defer daemon? Well, doing it here is more robust.
	 */
	if ((rcpt_domain = strrchr(rcpt->address, '@')) != 0
	    && *++rcpt_domain != 0)
	    switch (flush_add(rcpt_domain, id)) {
	    case FLUSH_STAT_OK:
	    case FLUSH_STAT_DENY:
		break;
	    default:
		msg_warn("%s: %s service failure", id, var_flush_service);
		break;
	    }
	return (-1);
    }
}

/* defer_flush - flush the defer log and deliver to the sender */

int     defer_flush(int flags, const char *queue, const char *id,
		            const char *encoding, int smtputf8,
		            const char *sender, const char *dsn_envid,
		            int dsn_ret)
{
    flags |= BOUNCE_FLAG_DELRCPT;

    if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			    SEND_ATTR_INT(MAIL_ATTR_NREQ, BOUNCE_CMD_FLUSH),
			    SEND_ATTR_INT(MAIL_ATTR_FLAGS, flags),
			    SEND_ATTR_STR(MAIL_ATTR_QUEUE, queue),
			    SEND_ATTR_STR(MAIL_ATTR_QUEUEID, id),
			    SEND_ATTR_STR(MAIL_ATTR_ENCODING, encoding),
			    SEND_ATTR_INT(MAIL_ATTR_SMTPUTF8, smtputf8),
			    SEND_ATTR_STR(MAIL_ATTR_SENDER, sender),
			    SEND_ATTR_STR(MAIL_ATTR_DSN_ENVID, dsn_envid),
			    SEND_ATTR_INT(MAIL_ATTR_DSN_RET, dsn_ret),
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else {
	return (-1);
    }
}

/* defer_warn - send a copy of the defer log to the sender as a warning bounce
 * do not flush the log */

int     defer_warn(int flags, const char *queue, const char *id,
		           const char *encoding, int smtputf8,
		         const char *sender, const char *envid, int dsn_ret)
{
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_defer_service,
			    SEND_ATTR_INT(MAIL_ATTR_NREQ, BOUNCE_CMD_WARN),
			    SEND_ATTR_INT(MAIL_ATTR_FLAGS, flags),
			    SEND_ATTR_STR(MAIL_ATTR_QUEUE, queue),
			    SEND_ATTR_STR(MAIL_ATTR_QUEUEID, id),
			    SEND_ATTR_STR(MAIL_ATTR_ENCODING, encoding),
			    SEND_ATTR_INT(MAIL_ATTR_SMTPUTF8, smtputf8),
			    SEND_ATTR_STR(MAIL_ATTR_SENDER, sender),
			    SEND_ATTR_STR(MAIL_ATTR_DSN_ENVID, envid),
			    SEND_ATTR_INT(MAIL_ATTR_DSN_RET, dsn_ret),
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else {
	return (-1);
    }
}

/* defer_one - defer mail for one recipient */

int     defer_one(int flags, const char *queue, const char *id,
		          const char *encoding, int smtputf8,
		          const char *sender, const char *dsn_envid,
		          int dsn_ret, MSG_STATS *stats, RECIPIENT *rcpt,
		          const char *relay, DSN *dsn)
{
    DSN     my_dsn = *dsn;
    DSN    *dsn_res;

    /*
     * Sanity check.
     */
    if (my_dsn.status[0] != '4' || !dsn_valid(my_dsn.status)) {
	msg_warn("defer_one: ignoring dsn code \"%s\"", my_dsn.status);
	my_dsn.status = "4.0.0";
    }

    /*
     * DSN filter (Postfix 3.0).
     */
    if (delivery_status_filter != 0
    && (dsn_res = dsn_filter_lookup(delivery_status_filter, &my_dsn)) != 0) {
	if (dsn_res->status[0] == '5')
	    return (bounce_one_intern(flags, queue, id, encoding, smtputf8,
				      sender, dsn_envid, dsn_ret, stats,
				      rcpt, relay, dsn_res));
	my_dsn = *dsn_res;
    }
    return (defer_append_intern(flags, id, stats, rcpt, relay, &my_dsn));
}
