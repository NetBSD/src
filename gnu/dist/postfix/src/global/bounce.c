/*++
/* NAME
/*	bounce 3
/* SUMMARY
/*	bounce service client
/* SYNOPSIS
/*	#include <bounce.h>
/*
/*	int	bounce_append(flags, id, stats, recipient, relay, dsn)
/*	int	flags;
/*	const char *id;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/*	DSN	*dsn;
/*
/*	int	bounce_flush(flags, queue, id, encoding, sender,
/*				dsn_envid, dsn_ret)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*
/*	int	bounce_flush_verp(flags, queue, id, encoding, sender,
/*				dsn_envid, dsn_ret, verp_delims)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	const char *verp_delims;
/*
/*	int	bounce_one(flags, queue, id, encoding, sender, envid, ret,
/*				stats, recipient, relay, dsn)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	MSG_STATS *stats;
/*	RECIPIENT *rcpt;
/*	const char *relay;
/*	DSN	*dsn;
/* DESCRIPTION
/*	This module implements the client interface to the message
/*	bounce service, which maintains a per-message log of status
/*	records with recipients that were bounced, and the dsn_text why.
/*
/*	bounce_append() appends a dsn_text for non-delivery to the
/*	bounce log for the named recipient, updates the address
/*	verification service, or updates a message delivery record
/*	on request by the sender. The flags argument determines
/*	the action.
/*
/*	bounce_flush() actually bounces the specified message to
/*	the specified sender, including the bounce log that was
/*	built with bounce_append(). The bounce logfile is removed
/*	upon successful completion.
/*
/*	bounce_flush_verp() is like bounce_flush(), but sends one
/*	notification per recipient, with the failed recipient encoded
/*	into the sender address.
/*
/*	bounce_one() bounces one recipient and immediately sends a
/*	notification to the sender. This procedure does not append
/*	the recipient and dsn_text to the per-message bounce log, and
/*	should be used when a delivery agent changes the error
/*	return address in a manner that depends on the recipient
/*	address.
/*
/*	Arguments:
/* .IP flags
/*	The bitwise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to request no special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the bounce log in case of an error (as in: pretend
/*	that we never even tried to bounce this message).
/* .IP BOUNCE_FLAG_DELRCPT
/*	When specified with a flush request, request that
/*	recipients be deleted from the queue file.
/*
/*	Note: the bounce daemon ignores this request when the
/*	recipient queue file offset is <= 0.
/* .IP DEL_REQ_FLAG_MTA_VRFY
/*	The message is an MTA-requested address verification probe.
/*	Update the address verification database instead of bouncing
/*	mail.
/* .IP DEL_REQ_FLAG_USR_VRFY
/*	The message is a user-requested address expansion probe.
/*	Update the message delivery record instead of bouncing mail.
/* .IP DEL_REQ_FLAG_RECORD
/*	This is a normal message with logged delivery. Update the
/*	message delivery record and bounce the mail.
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The message queue id if the original message file. The bounce log
/*	file has the same name as the original message file.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP rcpt
/*	Recipient information. See recipient_list(3).
/* .IP relay
/*	Name of the host that the message could not be delivered to.
/*	This information is used for syslogging only.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP sender
/*	The sender envelope address.
/* .IP dsn_envid
/*	Optional DSN envelope ID.
/* .IP dsn_ret
/*	Optional DSN return full/headers option.
/* .IP dsn
/*	Delivery status. See dsn(3). The specified action is ignored.
/* .IP verp_delims
/*	VERP delimiter characters, used when encoding the failed
/*	sender into the envelope sender address.
/* DIAGNOSTICS
/*	In case of success, these functions log the action, and return a
/*	zero value. Otherwise, the functions return a non-zero result,
/*	and when BOUNCE_FLAG_CLEAN is disabled, log that message
/*	delivery is deferred.
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
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <log_adhoc.h>
#include <dsn_util.h>
#include <rcpt_print.h>
#include <dsn_print.h>
#include <verify.h>
#include <defer.h>
#include <trace.h>
#include <bounce.h>

/* bounce_append - append dsn_text to per-message bounce log */

int     bounce_append(int flags, const char *id, MSG_STATS *stats,
		              RECIPIENT *rcpt, const char *relay,
		              DSN *dsn)
{
    DSN     my_dsn = *dsn;
    int     status;

    /*
     * Sanity check. If we're really confident, change this into msg_panic
     * (remember, this information may be under control by a hostile server).
     */
    if (my_dsn.status[0] != '5' || !dsn_valid(my_dsn.status)) {
	msg_warn("bounce_append: ignoring dsn code \"%s\"", my_dsn.status);
	my_dsn.status = "5.0.0";
    }

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_MTA_VRFY) {
	my_dsn.action = "undeliverable";
	status = verify_append(id, stats, rcpt, relay, &my_dsn,
			       DEL_RCPT_STAT_BOUNCE);
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
     * Normal (well almost) delivery. When we're pretending that we can't
     * bounce, don't create a defer log file when we wouldn't keep the bounce
     * log file. That's a lot of negatives in one sentence.
     */
    else if (var_soft_bounce && (flags & BOUNCE_FLAG_CLEAN)) {
	return (-1);
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     * 
     * XXX DSN We write all recipients to the bounce logfile regardless of DSN
     * NOTIFY options, because those options don't apply to postmaster
     * notifications.
     */
    else {
	char   *my_status = mystrdup(my_dsn.status);
	const char *log_status = var_soft_bounce ? "SOFTBOUNCE" : "bounced";

	/*
	 * Supply default action.
	 */
	my_dsn.status = my_status;
	if (var_soft_bounce) {
	    my_status[0] = '4';
	    my_dsn.action = "delayed";
	} else {
	    my_dsn.action = "failed";
	}

	if (mail_command_client(MAIL_CLASS_PRIVATE, var_soft_bounce ?
				var_defer_service : var_bounce_service,
			   ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_APPEND,
				ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
				ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
				ATTR_TYPE_FUNC, rcpt_print, (void *) rcpt,
				ATTR_TYPE_FUNC, dsn_print, (void *) &my_dsn,
				ATTR_TYPE_END) == 0
	    && ((flags & DEL_REQ_FLAG_RECORD) == 0
		|| trace_append(flags, id, stats, rcpt, relay,
				&my_dsn) == 0)) {
	    log_adhoc(id, stats, rcpt, relay, &my_dsn, log_status);
	    status = (var_soft_bounce ? -1 : 0);
	} else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	    VSTRING *junk = vstring_alloc(100);

	    my_dsn.status = "4.3.0";
	    vstring_sprintf(junk, "%s or %s service failure",
			    var_bounce_service, var_trace_service);
	    my_dsn.reason = vstring_str(junk);
	    status = defer_append(flags, id, stats, rcpt, relay, &my_dsn);
	    vstring_free(junk);
	} else {
	    status = -1;
	}
	myfree(my_status);
	return (status);
    }
}

/* bounce_flush - flush the bounce log and deliver to the sender */

int     bounce_flush(int flags, const char *queue, const char *id,
		             const char *encoding, const char *sender,
		             const char *dsn_envid, int dsn_ret)
{

    /*
     * When we're pretending that we can't bounce, don't send a bounce
     * message.
     */
    if (var_soft_bounce)
	return (-1);
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_bounce_service,
			    ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_FLUSH,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, dsn_ret,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	msg_info("%s: status=deferred (bounce failed)", id);
	return (-1);
    } else {
	return (-1);
    }
}

/* bounce_flush_verp - verpified notification */

int     bounce_flush_verp(int flags, const char *queue, const char *id,
			          const char *encoding, const char *sender,
			          const char *dsn_envid, int dsn_ret,
			          const char *verp_delims)
{

    /*
     * When we're pretending that we can't bounce, don't send a bounce
     * message.
     */
    if (var_soft_bounce)
	return (-1);
    if (mail_command_client(MAIL_CLASS_PRIVATE, var_bounce_service,
			    ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_VERP,
			    ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
			    ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
			    ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
			    ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			    ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
			    ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, dsn_ret,
			    ATTR_TYPE_STR, MAIL_ATTR_VERPDL, verp_delims,
			    ATTR_TYPE_END) == 0) {
	return (0);
    } else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	msg_info("%s: status=deferred (bounce failed)", id);
	return (-1);
    } else {
	return (-1);
    }
}

/* bounce_one - send notice for one recipient */

int     bounce_one(int flags, const char *queue, const char *id,
		           const char *encoding, const char *sender,
		           const char *dsn_envid, int dsn_ret,
		           MSG_STATS *stats, RECIPIENT *rcpt,
		           const char *relay, DSN *dsn)
{
    DSN     my_dsn = *dsn;
    int     status;

    /*
     * Sanity check.
     */
    if (my_dsn.status[0] != '5' || !dsn_valid(my_dsn.status)) {
	msg_warn("bounce_one: ignoring dsn code \"%s\"", my_dsn.status);
	my_dsn.status = "5.0.0";
    }

    /*
     * MTA-requested address verification information is stored in the verify
     * service database.
     */
    if (flags & DEL_REQ_FLAG_MTA_VRFY) {
	my_dsn.action = "undeliverable";
	status = verify_append(id, stats, rcpt, relay, &my_dsn,
			       DEL_RCPT_STAT_BOUNCE);
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
     * When we're not bouncing, then use the standard multi-recipient logfile
     * based procedure.
     */
    else if (var_soft_bounce) {
	return (bounce_append(flags, id, stats, rcpt, relay, &my_dsn));
    }

    /*
     * Normal mail delivery. May also send a delivery record to the user.
     * 
     * XXX DSN We send all recipients regardless of DSN NOTIFY options, because
     * those options don't apply to postmaster notifications.
     */
    else {

	/*
	 * Supply default action.
	 */
	my_dsn.action = "failed";

	if (mail_command_client(MAIL_CLASS_PRIVATE, var_bounce_service,
			      ATTR_TYPE_INT, MAIL_ATTR_NREQ, BOUNCE_CMD_ONE,
				ATTR_TYPE_INT, MAIL_ATTR_FLAGS, flags,
				ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
				ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
				ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
				ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
			      ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, dsn_envid,
				ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, dsn_ret,
				ATTR_TYPE_FUNC, rcpt_print, (void *) rcpt,
				ATTR_TYPE_FUNC, dsn_print, (void *) &my_dsn,
				ATTR_TYPE_END) == 0
	    && ((flags & DEL_REQ_FLAG_RECORD) == 0
		|| trace_append(flags, id, stats, rcpt, relay,
				&my_dsn) == 0)) {
	    log_adhoc(id, stats, rcpt, relay, &my_dsn, "bounced");
	    status = 0;
	} else if ((flags & BOUNCE_FLAG_CLEAN) == 0) {
	    VSTRING *junk = vstring_alloc(100);

	    my_dsn.status = "4.3.0";
	    vstring_sprintf(junk, "%s or %s service failure",
			    var_bounce_service, var_trace_service);
	    my_dsn.reason = vstring_str(junk);
	    status = defer_append(flags, id, stats, rcpt, relay, &my_dsn);
	    vstring_free(junk);
	} else {
	    status = -1;
	}
	return (status);
    }
}
