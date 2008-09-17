/*++
/* NAME
/*	deliver_pass 3
/* SUMMARY
/*	deliver request pass_through
/* SYNOPSIS
/*	#include <deliver_request.h>
/*
/*	int	deliver_pass(class, service, request, recipient)
/*	const char *class;
/*	const char *service;
/*	DELIVER_REQUEST *request;
/*	RECIPIENT *recipient;
/*
/*	int	deliver_pass_all(class, service, request)
/*	const char *class;
/*	const char *service;
/*	DELIVER_REQUEST *request;
/* DESCRIPTION
/*	This module implements the client side of the `queue manager
/*	to delivery agent' protocol, passing one recipient on from
/*	one delivery agent to another.
/*
/*	deliver_pass() delegates delivery of the named recipient.
/*
/*	deliver_pass_all() delegates an entire delivery request.
/*
/*	Arguments:
/* .IP class
/*	Destination delivery agent service class
/* .IP service
/*	String of the form \fItransport\fR:\fInexthop\fR. Either transport
/*	or nexthop are optional. For details see the transport map manual page.
/* .IP request
/*	Delivery request with queue file information.
/* .IP recipient
/*	Recipient information. See recipient_list(3).
/* DIAGNOSTICS
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* BUGS
/*	One recipient at a time; this is OK for mailbox deliveries.
/*
/*	Hop status information cannot be passed back.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <split_at.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <deliver_pass.h>
#include <dsb_scan.h>
#include <defer.h>
#include <rcpt_print.h>

#define DELIVER_PASS_DEFER	1
#define DELIVER_PASS_UNKNOWN	2

/* deliver_pass_initial_reply - retrieve initial delivery process response */

static int deliver_pass_initial_reply(VSTREAM *stream)
{
    int     stat;

    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_INT, MAIL_ATTR_STATUS, &stat,
		  ATTR_TYPE_END) != 1) {
	msg_warn("%s: malformed response", VSTREAM_PATH(stream));
	stat = -1;
    }
    return (stat);
}

/* deliver_pass_send_request - send delivery request to delivery process */

static int deliver_pass_send_request(VSTREAM *stream, DELIVER_REQUEST *request,
				             const char *nexthop,
				             RECIPIENT *rcpt)
{
    int     stat;

    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, request->flags,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUE, request->queue_name,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, request->queue_id,
	       ATTR_TYPE_LONG, MAIL_ATTR_OFFSET, request->data_offset,
	       ATTR_TYPE_LONG, MAIL_ATTR_SIZE, request->data_size,
	       ATTR_TYPE_STR, MAIL_ATTR_NEXTHOP, nexthop,
	       ATTR_TYPE_STR, MAIL_ATTR_ENCODING, request->encoding,
	       ATTR_TYPE_STR, MAIL_ATTR_SENDER, request->sender,
	       ATTR_TYPE_STR, MAIL_ATTR_DSN_ENVID, request->dsn_envid,
	       ATTR_TYPE_INT, MAIL_ATTR_DSN_RET, request->dsn_ret,
	       ATTR_TYPE_FUNC, msg_stats_print, (void *) &request->msg_stats,
    /* XXX Should be encapsulated with ATTR_TYPE_FUNC. */
	     ATTR_TYPE_STR, MAIL_ATTR_LOG_CLIENT_NAME, request->client_name,
	     ATTR_TYPE_STR, MAIL_ATTR_LOG_CLIENT_ADDR, request->client_addr,
	     ATTR_TYPE_STR, MAIL_ATTR_LOG_CLIENT_PORT, request->client_port,
	     ATTR_TYPE_STR, MAIL_ATTR_LOG_PROTO_NAME, request->client_proto,
	       ATTR_TYPE_STR, MAIL_ATTR_LOG_HELO_NAME, request->client_helo,
    /* XXX Should be encapsulated with ATTR_TYPE_FUNC. */
	       ATTR_TYPE_STR, MAIL_ATTR_SASL_METHOD, request->sasl_method,
	     ATTR_TYPE_STR, MAIL_ATTR_SASL_USERNAME, request->sasl_username,
	       ATTR_TYPE_STR, MAIL_ATTR_SASL_SENDER, request->sasl_sender,
    /* XXX Ditto if we want to pass TLS certificate info. */
	     ATTR_TYPE_STR, MAIL_ATTR_RWR_CONTEXT, request->rewrite_context,
	       ATTR_TYPE_INT, MAIL_ATTR_RCPT_COUNT, 1,
	       ATTR_TYPE_END);
    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_FUNC, rcpt_print, (void *) rcpt,
	       ATTR_TYPE_END);

    if (vstream_fflush(stream)) {
	msg_warn("%s: bad write: %m", VSTREAM_PATH(stream));
	stat = -1;
    } else {
	stat = 0;
    }
    return (stat);
}

/* deliver_pass_final_reply - retrieve final delivery status response */

static int deliver_pass_final_reply(VSTREAM *stream, DSN_BUF *dsb)
{
    int     stat;

    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_FUNC, dsb_scan, (void *) dsb,
		  ATTR_TYPE_INT, MAIL_ATTR_STATUS, &stat,
		  ATTR_TYPE_END) != 2) {
	msg_warn("%s: malformed response", VSTREAM_PATH(stream));
	return (DELIVER_PASS_UNKNOWN);
    } else {
	return (stat ? DELIVER_PASS_DEFER : 0);
    }
}

/* deliver_pass - deliver one per-site queue entry */

int     deliver_pass(const char *class, const char *service,
		             DELIVER_REQUEST *request,
		             RECIPIENT *rcpt)
{
    VSTREAM *stream;
    DSN_BUF *dsb;
    DSN     dsn;
    int     status;
    char   *saved_service;
    char   *transport;
    char   *nexthop;

    /*
     * Parse service into transport:nexthop form, and allow for omission of
     * optional fields
     */
    transport = saved_service = mystrdup(service);
    if ((nexthop = split_at(saved_service, ':')) == 0 || *nexthop == 0)
	nexthop = request->nexthop;
    if (*transport == 0)
	msg_fatal("missing transport name in \"%s\"", service);

    /*
     * Initialize.
     */
    stream = mail_connect_wait(class, transport);
    dsb = dsb_create();

    /*
     * Get the delivery process initial response. Send the queue file info
     * and recipient info to the delivery process. Retrieve the delivery
     * agent status report. The numerical status code indicates if delivery
     * should be tried again. The reason text is sent only when a destination
     * should be avoided for a while, so that the queue manager can log why
     * it does not even try to schedule delivery to the affected recipients.
     * XXX Can't pass back hop status info because the problem is with a
     * different transport.
     */
    if (deliver_pass_initial_reply(stream) != 0
	|| deliver_pass_send_request(stream, request, nexthop, rcpt) != 0) {
	(void) DSN_SIMPLE(&dsn, "4.3.0", "mail transport unavailable");
	status = defer_append(DEL_REQ_TRACE_FLAGS(request->flags),
			      request->queue_id, &request->msg_stats,
			      rcpt, "none", &dsn);
    } else if ((status = deliver_pass_final_reply(stream, dsb))
	       == DELIVER_PASS_UNKNOWN) {
	(void) DSN_SIMPLE(&dsn, "4.3.0", "unknown mail transport error");
	status = defer_append(DEL_REQ_TRACE_FLAGS(request->flags),
			      request->queue_id, &request->msg_stats,
			      rcpt, "none", &dsn);
    }

    /*
     * Clean up.
     */
    vstream_fclose(stream);
    dsb_free(dsb);
    myfree(saved_service);

    return (status);
}

/* deliver_pass_all - pass entire delivery request */

int     deliver_pass_all(const char *class, const char *service,
			         DELIVER_REQUEST *request)
{
    RECIPIENT_LIST *list;
    RECIPIENT *rcpt;
    int     status = 0;

    /*
     * XXX We should find out if the target transport can handle
     * multi-recipient requests. Unfortunately such code is hard to test,
     * rarely used, and therefore will be buggy.
     */
    list = &request->rcpt_list;
    for (rcpt = list->info; rcpt < list->info + list->len; rcpt++)
	status |= deliver_pass(class, service, request, rcpt);
    return (status);
}
