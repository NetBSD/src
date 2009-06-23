/*	$NetBSD: verify.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	verify 3
/* SUMMARY
/*	update verify database
/* SYNOPSIS
/*	#include <verify.h>
/*
/*	int	verify_append(queue_id, stats, recipient, relay, dsn,
/*				verify_status)
/*	const char *queue_id;
/*	MSG_STATS *stats;
/*	RECIPIENT *recipient;
/*	const char *relay;
/*	DSN	*dsn;
/*	int	verify_status;
/* DESCRIPTION
/*	This module implements an impedance adaptor between the
/*	verify_clnt interface and the interface expected by the
/*	bounce/defer/sent modules.
/*
/*	verify_append() updates the address verification database
/*	and logs the action to the mailer logfile.
/*
/*	Arguments:
/* .IP queue_id
/*	The message queue id.
/* .IP stats
/*	Time stamps from different message delivery stages
/*	and session reuse count.
/* .IP recipient
/*	Recipient information. See recipient_list(3).
/* .IP relay
/*	Name of the host we're talking to.
/* .IP dsn
/*	Delivery status information. See dsn(3).
/*	The action is one of "deliverable" or "undeliverable".
/* .IP verify_status
/*	One of the following recipient verification status codes:
/* .RS
/* .IP DEL_REQ_RCPT_STAT_OK
/*	Successful delivery.
/* .IP DEL_REQ_RCPT_STAT_DEFER
/*	Temporary delivery error.
/* .IP DEL_REQ_RCPT_STAT_BOUNCE
/*	Hard delivery error.
/* .RE
/* DIAGNOSTICS
/*	A non-zero result means the operation failed.
/*
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <verify_clnt.h>
#include <log_adhoc.h>
#include <verify.h>

/* verify_append - update address verification database */

int     verify_append(const char *queue_id, MSG_STATS *stats,
		              RECIPIENT *recipient, const char *relay,
		              DSN *dsn, int vrfy_stat)
{
    int     req_stat;
    DSN     my_dsn = *dsn;

    /*
     * Impedance adaptor between bounce/defer/sent and verify_clnt.
     * 
     * XXX No DSN check; this routine is called from bounce/defer/sent, which
     * know what the DSN initial digit should look like.
     * 
     * XXX vrfy_stat is competely redundant because of dsn.
     */
    if (var_verify_neg_cache || vrfy_stat == DEL_RCPT_STAT_OK) {
	req_stat = verify_clnt_update(recipient->orig_addr, vrfy_stat,
				      my_dsn.reason);
	if (req_stat == VRFY_STAT_OK && strcasecmp(recipient->address,
						 recipient->orig_addr) != 0)
	    req_stat = verify_clnt_update(recipient->address, vrfy_stat,
					  my_dsn.reason);
    } else {
	my_dsn.action = "undeliverable-but-not-cached";
	req_stat = VRFY_STAT_OK;
    }
    if (req_stat == VRFY_STAT_OK) {
	log_adhoc(queue_id, stats, recipient, relay, dsn, my_dsn.action);
	req_stat = 0;
    } else {
	msg_warn("%s: %s service failure", queue_id, var_verify_service);
	req_stat = -1;
    }
    return (req_stat);
}
