/*	$NetBSD: milter8.c,v 1.1.1.6.12.2 2007/08/02 08:05:20 heas Exp $	*/

/*++
/* NAME
/*	milter8 3
/* SUMMARY
/*	MTA-side Sendmail 8 Milter protocol
/* SYNOPSIS
/*	#include <milter8.h>
/*
/*	MILTER	*milter8_create(name, conn_timeout, cmd_timeout, msg_timeout,
/*				protocol, def_action, parent)
/*	const char *name;
/*	int conn_timeout;
/*	int cmd_timeout;
/*	int msg_timeout;
/*	const char *protocol;
/*	const char *def_action;
/*	MILTERS *parent;
/*
/*	MILTER	*milter8_receive(stream)
/*	VSTREAM	*stream;
/* DESCRIPTION
/*	This module implements the MTA side of the Sendmail 8 mail
/*	filter protocol.
/*
/*	milter8_create() creates a MILTER data structure with virtual
/*	functions that implement a client for the Sendmail 8 Milter
/*	protocol. These virtual functions are then invoked via the
/*	milter(3) interface.  The *timeout, protocol and def_action
/*	arguments come directly from milter_create(). The parent
/*	argument specifies a context for content editing.
/*
/*	milter8_receive() receives a mail filter definition from the
/*	specified stream. The result is zero in case of success.
/*
/*	Arguments:
/* .IP name
/*	The Milter application endpoint, either inet:host:port or
/*	unix:/pathname.
/* DIAGNOSTICS
/*	Panic: interface violation.  Fatal errors: out of memory.
/* CONFIGURATION PARAMETERS
/*	milter8_protocol, protocol version and extensions
/* SEE ALSO
/*	milter(3) generic Milter interface
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef SHUT_RDWR
#define SHUT_RDWR	2
#endif

/* Sendmail 8 Milter protocol. */

#ifdef USE_LIBMILTER_INCLUDES

 /*
  * Use the include files that match the installed libmilter library. This
  * requires that the libmilter files are installed before Postfix can be
  * built with milter support, and requires that Postfix is rebuilt whenever
  * protocol version in these files changes. The other option (below) is to
  * use our own protocol definitions.
  */
#include <libmilter/mfapi.h>
#include <libmilter/mfdef.h>

 /*
  * Compatibility for missing definitions or for names that have changed over
  * time.
  */
#ifndef SMFIF_CHGBODY
#define SMFIF_CHGBODY	SMFIF_MODBODY
#endif
#ifndef SMFIF_CHGHDRS
#define SMFIF_CHGHDRS	SMFIF_MODHDRS
#endif
#if defined(SMFIC_UNKNOWN) && !defined(SMFIP_NOUNKNOWN)
#define SMFIP_NOUNKNOWN 	(1L<<8)	/* MTA should not send unknown cmd */
#endif
#if defined(SMFIC_DATA) && !defined(SMFIP_NODATA)
#define SMFIP_NODATA		(1L<<9)	/* MTA should not send DATA */
#endif

#else

 /*
  * Use our own protocol definitions, so that Postfix can be built even when
  * libmilter is not installed. This means that we must specify the libmilter
  * protocol version in main.cf. The other option (above) is to compile
  * Postfix with the installed libmilter include files and to support only
  * that protocol version.
  */

 /*
  * Commands from MTA to filter.
  */
#define SMFIC_ABORT		'A'	/* Abort */
#define SMFIC_BODY		'B'	/* Body chunk */
#define SMFIC_CONNECT		'C'	/* Connection information */
#define SMFIC_MACRO		'D'	/* Define macro */
#define SMFIC_BODYEOB		'E'	/* final body chunk (End) */
#define SMFIC_HELO		'H'	/* HELO/EHLO */
#define SMFIC_HEADER		'L'	/* Header */
#define SMFIC_MAIL		'M'	/* MAIL from */
#define SMFIC_EOH		'N'	/* EOH */
#define SMFIC_OPTNEG		'O'	/* Option negotiation */
#define SMFIC_QUIT		'Q'	/* QUIT */
#define SMFIC_RCPT		'R'	/* RCPT to */
#define SMFIC_DATA		'T'	/* DATA */
#define SMFIC_UNKNOWN		'U'	/* Any unknown command */

 /*
  * Responses from filter to MTA.
  */
#define SMFIR_ADDRCPT		'+'	/* add recipient */
#define SMFIR_DELRCPT		'-'	/* remove recipient */
#define SMFIR_ACCEPT		'a'	/* accept */
#define SMFIR_REPLBODY		'b'	/* replace body (chunk) */
#define SMFIR_CONTINUE		'c'	/* continue */
#define SMFIR_DISCARD		'd'	/* discard */
#define SMFIR_CONN_FAIL		'f'	/* cause a connection failure */
#define SMFIR_CHGHEADER		'm'	/* change header */
#define SMFIR_PROGRESS		'p'	/* progress */
#define SMFIR_REJECT		'r'	/* reject */
#define SMFIR_TEMPFAIL		't'	/* tempfail */
#define SMFIR_SHUTDOWN		'4'	/* 421: shutdown (internal to MTA) */
#define SMFIR_ADDHEADER		'h'	/* add header */
#define SMFIR_INSHEADER		'i'	/* insert header */
#define SMFIR_REPLYCODE		'y'	/* reply code etc */
#define SMFIR_QUARANTINE	'q'	/* quarantine */

 /*
  * Commands that the filter does not want to receive, and replies that the
  * filter will not send.
  */
#define SMFIP_NOCONNECT		(1L<<0)	/* MTA should not send connect info */
#define SMFIP_NOHELO		(1L<<1)	/* MTA should not send HELO info */
#define SMFIP_NOMAIL		(1L<<2)	/* MTA should not send MAIL info */
#define SMFIP_NORCPT		(1L<<3)	/* MTA should not send RCPT info */
#define SMFIP_NOBODY		(1L<<4)	/* MTA should not send body */
#define SMFIP_NOHDRS		(1L<<5)	/* MTA should not send headers */
#define SMFIP_NOEOH		(1L<<6)	/* MTA should not send EOH */
#define SMFIP_NOHREPL		(1L<<7)	/* filter will not reply per header */
#define SMFIP_NOUNKNOWN 	(1L<<8)	/* MTA should not send unknown cmd */
#define SMFIP_NODATA		(1L<<9)	/* MTA should not send DATA */

 /*
  * Modifications that the filter may request at the end of the message body.
  */
#define SMFIF_ADDHDRS		(1L<<0)	/* filter may add headers */
#define SMFIF_CHGBODY		(1L<<1)	/* filter may replace body */
#define SMFIF_ADDRCPT		(1L<<2)	/* filter may add recipients */
#define SMFIF_DELRCPT		(1L<<3)	/* filter may delete recipients */
#define SMFIF_CHGHDRS		(1L<<4)	/* filter may change/delete headers */
#define SMFIF_QUARANTINE 	(1L<<5)	/* filter may quarantine envelope */

 /*
  * Network protocol families, used when sending CONNECT information.
  */
#define SMFIA_UNKNOWN		'U'	/* unknown */
#define SMFIA_UNIX		'L'	/* unix/local */
#define SMFIA_INET		'4'	/* inet */
#define SMFIA_INET6		'6'	/* inet6 */

 /*
  * How much buffer space is available for receiving body content.
  */
#define MILTER_CHUNK_SIZE	65535	/* body chunk size */

#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <connect.h>
#include <argv.h>
#include <name_mask.h>
#include <name_code.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>		/* var_line_limit */
#include <mail_proto.h>
#include <rec_type.h>
#include <record.h>
#include <mime_state.h>
#include <is_header.h>

/* Postfix Milter library. */

#include <milter.h>

/* Application-specific. */

/*#define msg_verbose 2*/

 /*
  * Sendmail 8 mail filter client.
  */
typedef struct {
    MILTER  m;				/* parent class */
    int     conn_timeout;		/* connect timeout */
    int     cmd_timeout;		/* per-command timeout */
    int     msg_timeout;		/* content inspection timeout */
    char   *protocol;			/* protocol version/extension */
    char   *def_action;			/* action if unavailable */
    int     version;			/* application protocol version */
    int     rq_mask;			/* application requests (SMFIF_*) */
    int     ev_mask;			/* application events (SMFIP_*) */
#ifndef USE_LIBMILTER_INCLUDES
    int     np_mask;			/* events outside my protocol version */
#endif
    VSTRING *buf;			/* I/O buffer */
    VSTRING *body;			/* I/O buffer */
    VSTREAM *fp;			/* stream or null (closed) */

    /*
     * Following fields must be reset after successful CONNECT, to avoid
     * leakage from one use to another.
     */
    int     state;			/* MILTER8_STAT_mumble */
    char   *def_reply;			/* error response or null */
} MILTER8;

 /*
  * XXX Sendmail 8 libmilter automatically closes the MTA-to-filter socket
  * when it finds out that the SMTP client has disconnected. Because of this
  * behavior, Postfix has to open a new MTA-to-filter socket each time an
  * SMTP client connects.
  */
#define LIBMILTER_AUTO_DISCONNECT

 /*
  * Milter internal state. For the external representation we use SMTP
  * replies (4XX X.Y.Z text, 5XX X.Y.Z text) and one-letter strings
  * (H=quarantine, D=discard, S=shutdown).
  */
#define MILTER8_STAT_ERROR	1	/* error, must be non-zero */
#define MILTER8_STAT_CLOSED	2	/* no connection */
#define MILTER8_STAT_READY	3	/* wait for connect event */
#define MILTER8_STAT_ENVELOPE	4	/* in envelope */
#define MILTER8_STAT_MESSAGE	5	/* in message */
#define MILTER8_STAT_ACCEPT_CON	6	/* accept all commands */
#define MILTER8_STAT_ACCEPT_MSG	7	/* accept one message */
#define MILTER8_STAT_REJECT_CON	8	/* reject all commands */

 /*
  * Protocol formatting requests. Note: the terms "long" and "short" refer to
  * the data types manipulated by htonl(), htons() and friends. These types
  * are network specific, not host platform specific.
  */
#define MILTER8_DATA_END	0	/* no more arguments */
#define MILTER8_DATA_HLONG	1	/* host long */
#define MILTER8_DATA_BUFFER	2	/* network-formatted buffer */
#define MILTER8_DATA_STRING	3	/* null-terminated string */
#define MILTER8_DATA_NSHORT	4	/* network short */
#define MILTER8_DATA_ARGV	5	/* array of null-terminated strings */
#define MILTER8_DATA_OCTET	6	/* byte */

 /*
  * We don't accept insane amounts of data.
  */
#define XXX_MAX_DATA	(MILTER_CHUNK_SIZE * 2)
#define XXX_TIMEOUT	10

#ifndef USE_LIBMILTER_INCLUDES

 /*
  * If we're not using Sendmail's libmilter include files, then we implement
  * the protocol up to and including version 4, and configure in main.cf what
  * protocol version we will use. However, we must send only events that are
  * defined for the specified protocol version, otherwise libmilter will
  * disconnect.
  * 
  * The following events are supported by all milter protocol implementations.
  */
#define MILTER8_V1_PROTO_MASK \
	(SMFIP_NOCONNECT | SMFIP_NOHELO | SMFIP_NOMAIL | SMFIP_NORCPT | \
	SMFIP_NOBODY | SMFIP_NOHDRS | SMFIP_NOEOH)

 /*
  * What events we can send to the milter application. The milter8_protocol
  * parameter can specify a protocol version as well as protocol extensions
  * such as "no_header_reply", a feature that speeds up the protocol by not
  * sending a filter reply for every individual message header.
  * 
  * This looks unclean because the user can specify multiple protocol versions,
  * but that is taken care of by the table that follows this one.
  */
static NAME_CODE milter8_event_masks[] = {
    "2", MILTER8_V1_PROTO_MASK,
    "3", MILTER8_V1_PROTO_MASK | SMFIP_NOUNKNOWN,
    "4", MILTER8_V1_PROTO_MASK | SMFIP_NOUNKNOWN | SMFIP_NODATA,
    "no_header_reply", SMFIP_NOHREPL,
    0, -1,
};

 /*
  * The following table lets us use the same milter8_protocol parameter
  * setting to derive the protocol version number. In this case we ignore
  * protocol extensions such as "no_header_reply", and require that exactly
  * one version number is specified.
  */
static NAME_CODE milter8_versions[] = {
    "2", 2,
    "3", 3,
    "4", 4,
    "no_header_reply", 0,
    0, -1,
};

#endif

 /*
  * Tables to map the above symbolic constants to printable strings. We use
  * NAME_CODE for commands and replies, and NAME_MASK for bit mask values.
  */
static NAME_CODE smfic_table[] = {
    "SMFIC_ABORT", SMFIC_ABORT,
    "SMFIC_BODY", SMFIC_BODY,
    "SMFIC_CONNECT", SMFIC_CONNECT,
    "SMFIC_MACRO", SMFIC_MACRO,
    "SMFIC_BODYEOB", SMFIC_BODYEOB,
    "SMFIC_HELO", SMFIC_HELO,
    "SMFIC_HEADER", SMFIC_HEADER,
    "SMFIC_MAIL", SMFIC_MAIL,
    "SMFIC_EOH", SMFIC_EOH,
    "SMFIC_OPTNEG", SMFIC_OPTNEG,
    "SMFIC_QUIT", SMFIC_QUIT,
    "SMFIC_RCPT", SMFIC_RCPT,
#ifdef SMFIC_DATA
    "SMFIC_DATA", SMFIC_DATA,
#endif
#ifdef SMFIC_UNKNOWN
    "SMFIC_UNKNOWN", SMFIC_UNKNOWN,
#endif
    0, 0,
};

static NAME_CODE smfir_table[] = {
    "SMFIR_ADDRCPT", SMFIR_ADDRCPT,
    "SMFIR_DELRCPT", SMFIR_DELRCPT,
    "SMFIR_ACCEPT", SMFIR_ACCEPT,
    "SMFIR_REPLBODY", SMFIR_REPLBODY,
    "SMFIR_CONTINUE", SMFIR_CONTINUE,
    "SMFIR_DISCARD", SMFIR_DISCARD,
#ifdef SMFIR_CONN_FAIL
    "SMFIR_CONN_FAIL", SMFIR_CONN_FAIL,
#endif
#ifdef SMFIR_CHGHEADER
    "SMFIR_CHGHEADER", SMFIR_CHGHEADER,
#endif
    "SMFIR_PROGRESS", SMFIR_PROGRESS,
    "SMFIR_REJECT", SMFIR_REJECT,
    "SMFIR_TEMPFAIL", SMFIR_TEMPFAIL,
#ifdef SMFIR_SHUTDOWN
    "SMFIR_SHUTDOWN", SMFIR_SHUTDOWN,
#endif
    "SMFIR_ADDHEADER", SMFIR_ADDHEADER,
#ifdef SMFIR_INSHEADER
    "SMFIR_INSHEADER", SMFIR_INSHEADER,
#endif
    "SMFIR_REPLYCODE", SMFIR_REPLYCODE,
#ifdef SMFIR_QUARANTINE
    "SMFIR_QUARANTINE", SMFIR_QUARANTINE,
#endif
    0, 0,
};

static NAME_MASK smfip_table[] = {
    "SMFIP_NOCONNECT", SMFIP_NOCONNECT,
    "SMFIP_NOHELO", SMFIP_NOHELO,
    "SMFIP_NOMAIL", SMFIP_NOMAIL,
    "SMFIP_NORCPT", SMFIP_NORCPT,
    "SMFIP_NOBODY", SMFIP_NOBODY,
    "SMFIP_NOHDRS", SMFIP_NOHDRS,
    "SMFIP_NOEOH", SMFIP_NOEOH,
#ifdef SMFIP_NOHREPL
    "SMFIP_NOHREPL", SMFIP_NOHREPL,
#endif
#ifdef SMFIP_NOUNKNOWN
    "SMFIP_NOUNKNOWN", SMFIP_NOUNKNOWN,
#endif
#ifdef SMFIP_NODATA
    "SMFIP_NODATA", SMFIP_NODATA,
#endif
    0, 0,
};

static NAME_MASK smfif_table[] = {
    "SMFIF_ADDHDRS", SMFIF_ADDHDRS,
    "SMFIF_CHGBODY", SMFIF_CHGBODY,
    "SMFIF_ADDRCPT", SMFIF_ADDRCPT,
    "SMFIF_DELRCPT", SMFIF_DELRCPT,
    "SMFIF_CHGHDRS", SMFIF_CHGHDRS,
#ifdef SMFIF_QUARANTINE
    "SMFIF_QUARANTINE", SMFIF_QUARANTINE,
#endif
    0, 0,
};

/* SLMs. */

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* milter8_def_reply - set persistent response */

static const char *milter8_def_reply(MILTER8 *milter, const char *reply)
{
    if (milter->def_reply)
	myfree(milter->def_reply);
    milter->def_reply = reply ? mystrdup(reply) : 0;
    return (milter->def_reply);
}

/* milter8_conf_error - local/remote configuration error */

static int milter8_conf_error(MILTER8 *milter)
{
    const char *reply;

    /*
     * XXX When the cleanup server closes its end of the Milter socket while
     * editing a queue file, the SMTP server is left out of sync with the
     * Milter. Sending an ABORT to the Milters will not restore
     * synchronization, because there may be any number of Milter replies
     * already in flight. Workaround: poison the socket and force the SMTP
     * server to abandon it.
     */
    if (milter->fp != 0) {
	(void) shutdown(vstream_fileno(milter->fp), SHUT_RDWR);
	(void) vstream_fclose(milter->fp);
	milter->fp = 0;
    }
    if (strcasecmp(milter->def_action, "accept") == 0) {
	reply = 0;
    } else {
	reply = "451 4.3.5 Server configuration problem - try again later";
    }
    milter8_def_reply(milter, reply);
    return (milter->state = MILTER8_STAT_ERROR);
}

/* milter8_comm_error - read/write/format communication error */

static int milter8_comm_error(MILTER8 *milter)
{
    const char *reply;

    /*
     * XXX When the cleanup server closes its end of the Milter socket while
     * editing a queue file, the SMTP server is left out of sync with the
     * Milter. Sending an ABORT to the Milters will not restore
     * synchronization, because there may be any number of Milter replies
     * already in flight. Workaround: poison the socket and force the SMTP
     * server to abandon it.
     */
    if (milter->fp != 0) {
	(void) shutdown(vstream_fileno(milter->fp), SHUT_RDWR);
	(void) vstream_fclose(milter->fp);
	milter->fp = 0;
    }
    if (strcasecmp(milter->def_action, "accept") == 0) {
	reply = 0;
    } else if (strcasecmp(milter->def_action, "reject") == 0) {
	reply = "550 5.5.0 Service unavailable";
    } else if (strcasecmp(milter->def_action, "tempfail") == 0) {
	reply = "451 4.7.1 Service unavailable - try again later";
    } else {
	msg_warn("milter %s: unrecognized default action: %s",
		 milter->m.name, milter->def_action);
	reply = "451 4.3.5 Server configuration problem - try again later";
    }
    milter8_def_reply(milter, reply);
    return (milter->state = MILTER8_STAT_ERROR);
}

/* milter8_close_stream - close stream to milter application */

static void milter8_close_stream(MILTER8 *milter)
{
    if (milter->fp != 0) {
	(void) vstream_fclose(milter->fp);
	milter->fp = 0;
    }
    milter->state = MILTER8_STAT_CLOSED;
}

/* milter8_read_resp - receive command code now, receive data later */

static int milter8_read_resp(MILTER8 *milter, int event, unsigned char *command,
			             ssize_t *data_len)
{
    UINT32_TYPE len;
    ssize_t pkt_len;
    const char *smfic_name;
    int     cmd;

    /*
     * Receive the packet length.
     */
    if ((vstream_fread(milter->fp, (char *) &len, UINT32_SIZE))
	!= UINT32_SIZE) {
	smfic_name = str_name_code(smfic_table, event);
	msg_warn("milter %s: can't read %s reply packet header: %m",
		 milter->m.name, smfic_name != 0 ?
		 smfic_name : "(unknown MTA event)");
	return (milter8_comm_error(milter));
    } else if ((pkt_len = ntohl(len)) < 1) {
	msg_warn("milter %s: bad packet length: %ld",
		 milter->m.name, (long) pkt_len);
	return (milter8_comm_error(milter));
    } else if (pkt_len > XXX_MAX_DATA) {
	msg_warn("milter %s: unreasonable packet length: %ld",
		 milter->m.name, (long) pkt_len);
	return (milter8_comm_error(milter));
    }

    /*
     * Receive the command code.
     */
    else if ((cmd = VSTREAM_GETC(milter->fp)) == VSTREAM_EOF) {
	msg_warn("milter %s: EOF while reading command code: %m",
		 milter->m.name);
	return (milter8_comm_error(milter));
    }

    /*
     * All is well.
     */
    else {
	*command = cmd;
	*data_len = pkt_len - 1;
	return (0);
    }
}

/* vmilter8_read_data - read command data */

static int vmilter8_read_data(MILTER8 *milter, ssize_t data_len, va_list ap)
{
    const char *myname = "milter8_read_data";
    int     arg_type;
    int     data_left;
    UINT32_TYPE net_long;
    UINT32_TYPE *host_long_ptr;
    VSTRING *buf;
    int     ch;

    for (data_left = data_len; (arg_type = va_arg(ap, int)) > 0; /* void */ ) {
	switch (arg_type) {

	    /*
	     * Host order long.
	     */
	case MILTER8_DATA_HLONG:
	    if (data_left < UINT32_SIZE) {
		msg_warn("milter %s: input packet too short for network long",
			 milter->m.name);
		return (milter8_comm_error(milter));
	    }
	    host_long_ptr = va_arg(ap, UINT32_TYPE *);
	    if (vstream_fread(milter->fp, (char *) &net_long, UINT32_SIZE)
		!= UINT32_SIZE) {
		msg_warn("milter %s: EOF while reading network long: %m",
			 milter->m.name);
		return (milter8_comm_error(milter));
	    }
	    data_left -= UINT32_SIZE;
	    *host_long_ptr = ntohl(net_long);
	    break;

	    /*
	     * Raw on-the-wire format, without explicit null terminator.
	     */
	case MILTER8_DATA_BUFFER:
	    if (data_left < 0) {
		msg_warn("milter %s: no data in input packet", milter->m.name);
		return (milter8_comm_error(milter));
	    }
	    buf = va_arg(ap, VSTRING *);
	    VSTRING_RESET(buf);
	    VSTRING_SPACE(buf, data_left);
	    if (vstream_fread(milter->fp, (char *) STR(buf), data_left)
		!= data_left) {
		msg_warn("milter %s: EOF while reading data: %m", milter->m.name);
		return (milter8_comm_error(milter));
	    }
	    VSTRING_AT_OFFSET(buf, data_left);
	    data_left = 0;
	    break;

	    /*
	     * Pointer to null-terminated string.
	     */
	case MILTER8_DATA_STRING:
	    if (data_left < 1) {
		msg_warn("milter %s: packet too short for string",
			 milter->m.name);
		return (milter8_comm_error(milter));
	    }
	    buf = va_arg(ap, VSTRING *);
	    VSTRING_RESET(buf);
	    for (;;) {
		if ((ch = VSTREAM_GETC(milter->fp)) == VSTREAM_EOF) {
		    msg_warn("%s: milter %s: EOF while reading string: %m",
			     myname, milter->m.name);
		    return (milter8_comm_error(milter));
		}
		data_left -= 1;
		if (ch == 0)
		    break;
		VSTRING_ADDCH(buf, ch);
		if (data_left <= 0) {
		    msg_warn("%s: milter %s: missing string null termimator",
			     myname, milter->m.name);
		    return (milter8_comm_error(milter));
		}
	    }
	    VSTRING_TERMINATE(buf);
	    break;

	    /*
	     * Error.
	     */
	default:
	    msg_panic("%s: unknown argument type: %d", myname, arg_type);
	}
    }

    /*
     * Sanity checks. We may have excess data when the sender is confused. We
     * may have a negative count when we're confused ourselves.
     */
    if (data_left > 0) {
	msg_warn("%s: left-over data %ld bytes", myname, (long) data_left);
	return (milter8_comm_error(milter));
    }
    if (data_left < 0)
	msg_panic("%s: bad left-over data count %ld",
		  myname, (long) data_left);
    return (0);
}

/* milter8_read_data - read command data */

static int milter8_read_data(MILTER8 *milter, ssize_t data_len,...)
{
    va_list ap;
    int     ret;

    va_start(ap, data_len);
    ret = vmilter8_read_data(milter, data_len, ap);
    va_end(ap);
    return (ret);
}

/* vmilter8_size_data - compute command data length */

static ssize_t vmilter8_size_data(va_list ap)
{
    const char *myname = "vmilter8_size_data";
    ssize_t data_len;
    int     arg_type;
    VSTRING *buf;
    const char *str;
    const char **cpp;

    /*
     * Compute data size.
     */
    for (data_len = 0; (arg_type = va_arg(ap, int)) > 0; /* void */ ) {
	switch (arg_type) {

	    /*
	     * Host order long.
	     */
	case MILTER8_DATA_HLONG:
	    (void) va_arg(ap, UINT32_TYPE);
	    data_len += UINT32_SIZE;
	    break;

	    /*
	     * Raw on-the-wire format.
	     */
	case MILTER8_DATA_BUFFER:
	    buf = va_arg(ap, VSTRING *);
	    data_len += LEN(buf);
	    break;

	    /*
	     * Pointer to null-terminated string.
	     */
	case MILTER8_DATA_STRING:
	    str = va_arg(ap, char *);
	    data_len += strlen(str) + 1;
	    break;

	    /*
	     * Array of pointers to null-terminated strings.
	     */
	case MILTER8_DATA_ARGV:
	    for (cpp = va_arg(ap, const char **); *cpp; cpp++)
		data_len += strlen(*cpp) + 1;
	    break;

	    /*
	     * Network order short, promoted to int.
	     */
	case MILTER8_DATA_NSHORT:
	    (void) va_arg(ap, unsigned);
	    data_len += UINT16_SIZE;
	    break;

	    /*
	     * Octet, promoted to int.
	     */
	case MILTER8_DATA_OCTET:
	    (void) va_arg(ap, unsigned);
	    data_len += 1;
	    break;

	    /*
	     * Error.
	     */
	default:
	    msg_panic("%s: bad argument type: %d", myname, arg_type);
	}
    }
    va_end(ap);
    return (data_len);
}

/* vmilter8_write_cmd - write command to Sendmail 8 Milter */

static int vmilter8_write_cmd(MILTER8 *milter, int command, ssize_t data_len,
			              va_list ap)
{
    const char *myname = "vmilter8_write_cmd";
    int     arg_type;
    UINT32_TYPE pkt_len;
    UINT32_TYPE host_long;
    UINT32_TYPE net_long;
    UINT16_TYPE net_short;
    VSTRING *buf;
    const char *str;
    const char **cpp;
    char    ch;

    /*
     * Deliver the packet.
     */
    if ((pkt_len = 1 + data_len) < 1)
	msg_panic("%s: bad packet length %d", myname, pkt_len);
    pkt_len = htonl(pkt_len);
    (void) vstream_fwrite(milter->fp, (char *) &pkt_len, UINT32_SIZE);
    (void) VSTREAM_PUTC(command, milter->fp);
    while ((arg_type = va_arg(ap, int)) > 0) {
	switch (arg_type) {

	    /*
	     * Network long.
	     */
	case MILTER8_DATA_HLONG:
	    host_long = va_arg(ap, UINT32_TYPE);
	    net_long = htonl(host_long);
	    (void) vstream_fwrite(milter->fp, (char *) &net_long, UINT32_SIZE);
	    break;

	    /*
	     * Raw on-the-wire format.
	     */
	case MILTER8_DATA_BUFFER:
	    buf = va_arg(ap, VSTRING *);
	    (void) vstream_fwrite(milter->fp, STR(buf), LEN(buf));
	    break;

	    /*
	     * Pointer to null-terminated string.
	     */
	case MILTER8_DATA_STRING:
	    str = va_arg(ap, char *);
	    (void) vstream_fwrite(milter->fp, str, strlen(str) + 1);
	    break;

	    /*
	     * Octet, promoted to int.
	     */
	case MILTER8_DATA_OCTET:
	    ch = va_arg(ap, unsigned);
	    (void) vstream_fwrite(milter->fp, &ch, 1);
	    break;

	    /*
	     * Array of pointers to null-terminated strings.
	     */
	case MILTER8_DATA_ARGV:
	    for (cpp = va_arg(ap, const char **); *cpp; cpp++)
		(void) vstream_fwrite(milter->fp, *cpp, strlen(*cpp) + 1);
	    break;

	    /*
	     * Network order short, promoted to int.
	     */
	case MILTER8_DATA_NSHORT:
	    net_short = va_arg(ap, unsigned);
	    (void) vstream_fwrite(milter->fp, (char *) &net_short, UINT16_SIZE);
	    break;

	    /*
	     * Error.
	     */
	default:
	    msg_panic("%s: bad argument type: %d", myname, arg_type);
	}

	/*
	 * Report errors immediately.
	 */
	if (vstream_ferror(milter->fp)) {
	    msg_warn("milter %s: error writing command: %m", milter->m.name);
	    milter8_comm_error(milter);
	    break;
	}
    }
    va_end(ap);
    return (milter->state == MILTER8_STAT_ERROR);
}

/* milter8_write_cmd - write command to Sendmail 8 Milter */

static int milter8_write_cmd(MILTER8 *milter, int command,...)
{
    va_list ap;
    ssize_t data_len;
    int     err;

    /*
     * Size the command data.
     */
    va_start(ap, command);
    data_len = vmilter8_size_data(ap);
    va_end(ap);

    /*
     * Send the command and data.
     */
    va_start(ap, command);
    err = vmilter8_write_cmd(milter, command, data_len, ap);
    va_end(ap);

    return (err);
}

/* milter8_event - report event and receive reply */

static const char *milter8_event(MILTER8 *milter, int event,
				         int skip_event_flag,
				         int skip_reply,
				         ARGV *macros,...)
{
    const char *myname = "milter8_event";
    va_list ap;
    ssize_t data_len;
    int     err;
    unsigned char cmd;
    ssize_t data_size;
    const char *smfic_name;
    const char *smfir_name;
    MILTERS *parent = milter->m.parent;
    UINT32_TYPE index;
    const char *edit_resp = 0;
    const char *retval = 0;
    VSTRING *body_line_buf = 0;
    int     done = 0;
    int     body_edit_lockout = 0;

#define DONT_SKIP_REPLY	0

    /*
     * Sanity check.
     */
    if (milter->fp == 0 || milter->def_reply != 0) {
	msg_warn("%s: attempt to send event %s to milter %s after error",
		 myname,
		 (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		 smfic_name : "(unknown MTA event)", milter->m.name);
	return (milter->def_reply);
    }

    /*
     * Skip this event if it doesn't exist in the protocol that I announced.
     */
#ifndef USE_LIBMILTER_INCLUDES
    if ((skip_event_flag & milter->np_mask) != 0) {
	if (msg_verbose)
	    msg_info("skipping non-protocol event %s for milter %s",
		     (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		     smfic_name : "(unknown MTA event)", milter->m.name);
	return (milter->def_reply);
    }
#endif

    /*
     * Send the macros for this event, even when we're not reporting the
     * event itself. This does not introduce a performance problem because
     * we're sending macros and event parameters in one VSTREAM transaction.
     */
    if (msg_verbose) {
	VSTRING *buf = vstring_alloc(100);

	if (macros) {
	    if (macros->argc > 0) {
		char  **cpp;

		for (cpp = macros->argv; *cpp && cpp[1]; cpp += 2)
		    vstring_sprintf_append(buf, " %s=%s", *cpp, cpp[1]);
	    }
	}
	msg_info("event: %s; macros:%s",
		 (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		 smfic_name : "(unknown MTA event)", *STR(buf) ?
		 STR(buf) : " (none)");
	vstring_free(buf);
    }
    if (macros) {
	if (milter8_write_cmd(milter, SMFIC_MACRO,
			      MILTER8_DATA_OCTET, event,
			      MILTER8_DATA_ARGV, macros->argv,
			      MILTER8_DATA_END) != 0)
	    return (milter->def_reply);
    }

    /*
     * Skip this event if the Milter told us not to send it.
     */
    if ((skip_event_flag & milter->ev_mask) != 0) {
	if (msg_verbose)
	    msg_info("skipping event %s for milter %s",
		     (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		     smfic_name : "(unknown MTA event)", milter->m.name);
	return (milter->def_reply);
    }

    /*
     * Compute the command data size. This is necessary because the protocol
     * sends length before content.
     */
    va_start(ap, macros);
    data_len = vmilter8_size_data(ap);
    va_end(ap);

    /*
     * Send the command and data.
     */
    va_start(ap, macros);
    err = vmilter8_write_cmd(milter, event, data_len, ap);
    va_end(ap);
    if (err != 0)
	return (milter->def_reply);

    /*
     * Special feature: don't wait for one reply per header. This allows us
     * to send multiple headers in one VSTREAM transaction, and improves
     * over-all performance.
     */
    if (skip_reply) {
	if (msg_verbose)
	    msg_info("skipping reply %s for milter %s",
		     (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		     smfic_name : "(unknown MTA event)", milter->m.name);
	return (milter->def_reply);
    }

    /*
     * Receive the reply or replies.
     * 
     * Intercept all loop exits so that we can do post header/body edit
     * processing.
     * 
     * XXX Bound the loop iteration count.
     * 
     * In the end-of-body stage, the Milter may reply with one or more queue
     * file edit requests before it replies with its final decision: accept,
     * reject, etc. After a local queue file edit error (file too big, media
     * write error), do not close the Milter socket in the cleanup server.
     * Instead skip all further Milter replies until the final decision. This
     * way the Postfix SMTP server stays in sync with the Milter, and Postfix
     * doesn't have to lose the ability to handle multiple deliveries within
     * the same SMTP session. This requires that the Postfix SMTP server uses
     * something other than CLEANUP_STAT_WRITE when it loses contact with the
     * cleanup server.
     */
#define IN_CONNECT_EVENT(e) ((e) == SMFIC_CONNECT || (e) == SMFIC_HELO)

    /*
     * XXX Don't evaluate this macro's argument multiple times. Since we use
     * "continue" the macro can't be enclosed in do .. while (0).
     */
#define MILTER8_EVENT_BREAK(s) { \
	retval = (s); \
	done = 1; \
	continue; \
    }

    while (done == 0) {
	char   *cp;
	char   *rp;
	char    ch;

	if (milter8_read_resp(milter, event, &cmd, &data_size) != 0)
	    MILTER8_EVENT_BREAK(milter->def_reply);
	if (msg_verbose)
	    msg_info("reply: %s data %ld bytes",
		     (smfir_name = str_name_code(smfir_table, cmd)) != 0 ?
		     smfir_name : "unknown", (long) data_size);

	/*
	 * Handle unfinished message body replacement first.
	 * 
	 * XXX When SMFIR_REPLBODY is followed by some different request, we
	 * assume that the body replacement operation is complete. The queue
	 * file editing implementation currently does not support sending
	 * part 1 of the body replacement text, doing some other queue file
	 * updates, and then sending part 2 of the body replacement text. To
	 * avoid loss of data, we log an error when SMFIR_REPLBODY requests
	 * are alternated with other requests.
	 */
	if (body_line_buf != 0 && cmd != SMFIR_REPLBODY) {
	    /* In case the last body replacement line didn't end in CRLF. */
	    if (edit_resp == 0 && LEN(body_line_buf) > 0)
		edit_resp = parent->repl_body(parent->chg_context,
					      MILTER_BODY_LINE,
					      body_line_buf);
	    if (edit_resp == 0)
		edit_resp = parent->repl_body(parent->chg_context,
					      MILTER_BODY_END,
					      (VSTRING *) 0);
	    body_edit_lockout = 1;
	    vstring_free(body_line_buf);
	    body_line_buf = 0;
	}
	switch (cmd) {

	    /*
	     * Still working on it.
	     */
	case SMFIR_PROGRESS:
	    if (data_size != 0)
		break;
	    continue;

	    /*
	     * Decision: continue processing.
	     */
	case SMFIR_CONTINUE:
	    if (data_size != 0)
		break;
	    MILTER8_EVENT_BREAK(milter->def_reply);

	    /*
	     * Decision: accept this message, or accept all further commands
	     * in this SMTP connection. This decision is final (i.e. Sendmail
	     * 8 changes receiver state).
	     */
	case SMFIR_ACCEPT:
	    if (data_size != 0)
		break;
	    if (IN_CONNECT_EVENT(event)) {
#ifdef LIBMILTER_AUTO_DISCONNECT
		milter8_close_stream(milter);
#endif
		/* No more events for this SMTP connection. */
		milter->state = MILTER8_STAT_ACCEPT_CON;
	    } else {
		/* No more events for this message. */
		milter->state = MILTER8_STAT_ACCEPT_MSG;
	    }
	    MILTER8_EVENT_BREAK(milter->def_reply);

	    /*
	     * Decision: accept and silently discard this message. According
	     * to the milter API documentation there will be no action when
	     * this is requested by a connection-level function. This
	     * decision is final (i.e. Sendmail 8 changes receiver state).
	     */
	case SMFIR_DISCARD:
	    if (data_size != 0)
		break;
	    if (IN_CONNECT_EVENT(event)) {
		msg_warn("milter %s: DISCARD action is not allowed "
			 "for connect or helo", milter->m.name);
		MILTER8_EVENT_BREAK(milter->def_reply);
	    } else {
		/* No more events for this message. */
		milter->state = MILTER8_STAT_ACCEPT_MSG;
		MILTER8_EVENT_BREAK("D");
	    }

	    /*
	     * Decision: reject connection, message or recipient. This
	     * decision is final (i.e. Sendmail 8 changes receiver state).
	     */
	case SMFIR_REJECT:
	    if (data_size != 0)
		break;
	    if (IN_CONNECT_EVENT(event)) {
#ifdef LIBMILTER_AUTO_DISCONNECT
		milter8_close_stream(milter);
#endif
		milter->state = MILTER8_STAT_REJECT_CON;
		MILTER8_EVENT_BREAK(milter8_def_reply(milter, "550 5.7.1 Command rejected"));
	    } else {
		MILTER8_EVENT_BREAK("550 5.7.1 Command rejected");
	    }

	    /*
	     * Decision: tempfail. This decision is final (i.e. Sendmail 8
	     * changes receiver state).
	     */
	case SMFIR_TEMPFAIL:
	    if (data_size != 0)
		break;
	    if (IN_CONNECT_EVENT(event)) {
#ifdef LIBMILTER_AUTO_DISCONNECT
		milter8_close_stream(milter);
#endif
		milter->state = MILTER8_STAT_REJECT_CON;
		MILTER8_EVENT_BREAK(milter8_def_reply(milter,
			"451 4.7.1 Service unavailable - try again later"));
	    } else {
		MILTER8_EVENT_BREAK("451 4.7.1 Service unavailable - try again later");
	    }

	    /*
	     * Decision: disconnect. This decision is final (i.e. Sendmail 8
	     * changes receiver state).
	     */
#ifdef SMFIR_SHUTDOWN
	case SMFIR_SHUTDOWN:
	    if (data_size != 0)
		break;
#ifdef LIBMILTER_AUTO_DISCONNECT
	    milter8_close_stream(milter);
#endif
	    milter->state = MILTER8_STAT_REJECT_CON;
	    MILTER8_EVENT_BREAK(milter8_def_reply(milter, "S"));
#endif

	    /*
	     * Decision: "ddd d.d+.d+ text". This decision is final (i.e.
	     * Sendmail 8 changes receiver state). Note: the reply may be in
	     * multi-line SMTP format.
	     * 
	     * XXX Sendmail compatibility: sendmail 8 uses the reply as a format
	     * string; therefore any '%' characters in the reply are doubled.
	     * Postfix doesn't use replies as format strings; we replace '%%'
	     * by '%', and remove single (i.e. invalid) '%' characters.
	     */
	case SMFIR_REPLYCODE:
	    if (milter8_read_data(milter, data_size,
				  MILTER8_DATA_BUFFER, milter->buf,
				  MILTER8_DATA_END) != 0)
		MILTER8_EVENT_BREAK(milter->def_reply);
	    if ((STR(milter->buf)[0] != '4' && STR(milter->buf)[0] != '5')
		|| !ISDIGIT(STR(milter->buf)[1])
		|| !ISDIGIT(STR(milter->buf)[2])
		|| (STR(milter->buf)[3] != ' ' && STR(milter->buf)[3] != '-')
		|| STR(milter->buf)[4] != STR(milter->buf)[0]) {
		msg_warn("milter %s: malformed reply: %s",
			 milter->m.name, STR(milter->buf));
		milter8_conf_error(milter);
		MILTER8_EVENT_BREAK(milter->def_reply);
	    }
	    if ((rp = cp = strchr(STR(milter->buf), '%')) != 0) {
		for (;;) {
		    if ((ch = *cp++) == '%')
			ch = *cp++;
		    *rp++ = ch;
		    if (ch == 0)
			break;
		}
	    }
	    if (IN_CONNECT_EVENT(event)) {
#ifdef LIBMILTER_AUTO_DISCONNECT
		milter8_close_stream(milter);
#endif
		milter->state = MILTER8_STAT_REJECT_CON;
		MILTER8_EVENT_BREAK(milter8_def_reply(milter, STR(milter->buf)));
	    } else {
		MILTER8_EVENT_BREAK(STR(milter->buf));
	    }

	    /*
	     * Decision: quarantine. In Sendmail 8.13 this does not imply a
	     * transition in the receiver state (reply, reject, tempfail,
	     * accept, discard).
	     */
#ifdef SMFIR_QUARANTINE
	case SMFIR_QUARANTINE:
	    /* XXX What to do with the "reason" text? */
	    if (milter8_read_data(milter, data_size,
				  MILTER8_DATA_BUFFER, milter->buf,
				  MILTER8_DATA_END) != 0)
		MILTER8_EVENT_BREAK(milter->def_reply);
	    MILTER8_EVENT_BREAK("H");
#endif

	    /*
	     * Modification request or error.
	     */
	default:
	    if (event == SMFIC_BODYEOB) {
		switch (cmd) {

		    /*
		     * Modification request: replace, insert or delete
		     * header. Index 1 means the first instance.
		     */
#ifdef SMFIR_CHGHEADER
		case SMFIR_CHGHEADER:
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_HLONG, &index,
					  MILTER8_DATA_STRING, milter->buf,
					  MILTER8_DATA_STRING, milter->body,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    /* XXX Sendmail 8 compatibility. */
		    if (index == 0)
			index = 1;
		    if ((ssize_t) index < 1) {
			msg_warn("milter %s: bad change header index: %ld",
				 milter->m.name, (long) index);
			milter8_conf_error(milter);
			MILTER8_EVENT_BREAK(milter->def_reply);
		    }
		    if (LEN(milter->buf) == 0) {
			msg_warn("milter %s: null change header name",
				 milter->m.name);
			milter8_conf_error(milter);
			MILTER8_EVENT_BREAK(milter->def_reply);
		    }
		    if (STR(milter->body)[0])
			edit_resp = parent->upd_header(parent->chg_context,
						       (ssize_t) index,
						       STR(milter->buf),
						       STR(milter->body));
		    else
			edit_resp = parent->del_header(parent->chg_context,
						       (ssize_t) index,
						       STR(milter->buf));
		    continue;
#endif

		    /*
		     * Modification request: append header.
		     */
		case SMFIR_ADDHEADER:
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_STRING, milter->buf,
					  MILTER8_DATA_STRING, milter->body,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    edit_resp = parent->add_header(parent->chg_context,
						   STR(milter->buf),
						   STR(milter->body));
		    continue;

		    /*
		     * Modification request: insert header. With Sendmail 8,
		     * index 0 means the top-most header. We use 1-based
		     * indexing for consistency with header change
		     * operations.
		     */
#ifdef SMFIR_INSHEADER
		case SMFIR_INSHEADER:
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_HLONG, &index,
					  MILTER8_DATA_STRING, milter->buf,
					  MILTER8_DATA_STRING, milter->body,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    if ((ssize_t) index + 1 < 1) {
			msg_warn("milter %s: bad insert header index: %ld",
				 milter->m.name, (long) index);
			milter8_conf_error(milter);
			MILTER8_EVENT_BREAK(milter->def_reply);
		    }
		    edit_resp = parent->ins_header(parent->chg_context,
						   (ssize_t) index + 1,
						   STR(milter->buf),
						   STR(milter->body));
		    continue;
#endif

		    /*
		     * Modification request: append recipient.
		     */
		case SMFIR_ADDRCPT:
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_STRING, milter->buf,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    edit_resp = parent->add_rcpt(parent->chg_context,
						 STR(milter->buf));
		    continue;

		    /*
		     * Modification request: delete (expansion of) recipient.
		     */
		case SMFIR_DELRCPT:
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_STRING, milter->buf,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    edit_resp = parent->del_rcpt(parent->chg_context,
						 STR(milter->buf));
		    continue;

		    /*
		     * Modification request: replace the message body, and
		     * update the message size.
		     */
		case SMFIR_REPLBODY:
		    if (body_edit_lockout) {
			msg_warn("milter %s: body replacement requests can't "
				 "currently be mixed with other requests",
				 milter->m.name);
			milter8_conf_error(milter);
			MILTER8_EVENT_BREAK(milter->def_reply);
		    }
		    if (milter8_read_data(milter, data_size,
					  MILTER8_DATA_BUFFER, milter->body,
					  MILTER8_DATA_END) != 0)
			MILTER8_EVENT_BREAK(milter->def_reply);
		    /* Skip to the next request after previous edit error. */
		    if (edit_resp)
			continue;
		    /* Start body replacement. */
		    if (body_line_buf == 0) {
			body_line_buf = vstring_alloc(var_line_limit);
			edit_resp = parent->repl_body(parent->chg_context,
						      MILTER_BODY_START,
						      (VSTRING *) 0);
		    }
		    /* Extract lines from the on-the-wire CRLF format. */
		    for (cp = STR(milter->body); edit_resp == 0
			 && cp < vstring_end(milter->body); cp++) {
			ch = *(unsigned char *) cp;
			if (ch == '\n') {
			    if (LEN(body_line_buf) > 0
				&& vstring_end(body_line_buf)[-1] == '\r')
				vstring_truncate(body_line_buf,
						 LEN(body_line_buf) - 1);
			    edit_resp = parent->repl_body(parent->chg_context,
							  MILTER_BODY_LINE,
							  body_line_buf);
			    VSTRING_RESET(body_line_buf);
			} else {
			    VSTRING_ADDCH(body_line_buf, ch);
			}
		    }
		    continue;
		}
	    }
	    msg_warn("milter %s: unexpected filter response %s after event %s",
		     milter->m.name,
		     (smfir_name = str_name_code(smfir_table, cmd)) != 0 ?
		     smfir_name : "(unknown filter reply)",
		     (smfic_name = str_name_code(smfic_table, event)) != 0 ?
		     smfic_name : "(unknown MTA event)");
	    milter8_comm_error(milter);
	    MILTER8_EVENT_BREAK(milter->def_reply);
	}

	/*
	 * Get here when the reply was followed by data bytes that weren't
	 * supposed to be there.
	 */
	msg_warn("milter %s: reply %s was followed by %ld data bytes",
	milter->m.name, (smfir_name = str_name_code(smfir_table, cmd)) != 0 ?
		 smfir_name : "unknown", (long) data_len);
	milter8_comm_error(milter);
	MILTER8_EVENT_BREAK(milter->def_reply);
    }

    /*
     * Clean up after aborted message body replacement.
     */
    if (body_line_buf)
	vstring_free(body_line_buf);

    /*
     * XXX Some cleanup clients ask the cleanup server to bounce mail for
     * them. In that case we must override a hard reject retval result after
     * queue file update failure. This is not a big problem; the odds are
     * small that a Milter application sends a hard reject after replacing
     * the message body.
     */
    if (edit_resp && (retval == 0 || strchr("DS4", retval[0]) == 0))
	retval = edit_resp;
    return (retval);
}

/* milter8_connect - connect to filter */

static void milter8_connect(MILTER8 *milter)
{
    const char *myname = "milter8_connect";
    ssize_t data_len;
    unsigned char cmd;
    char   *transport;
    char   *endpoint;
    int     (*connect_fn) (const char *, int, int);
    int     fd;
    const UINT32_TYPE my_actions = (SMFIF_ADDHDRS | SMFIF_ADDRCPT
				    | SMFIF_DELRCPT | SMFIF_CHGHDRS
				    | SMFIF_CHGBODY
#ifdef SMFIF_QUARANTINE
				    | SMFIF_QUARANTINE
#endif
    );

#ifdef USE_LIBMILTER_INCLUDES
    const UINT32_TYPE my_version = SMFI_VERSION;
    const UINT32_TYPE my_events = (SMFIP_NOCONNECT | SMFIP_NOHELO
				   | SMFIP_NOMAIL | SMFIP_NORCPT
				   | SMFIP_NOBODY | SMFIP_NOHDRS
				   | SMFIP_NOEOH
#ifdef SMFIP_NOHREPL
				   | SMFIP_NOHREPL
#endif
#ifdef SMFIP_NOUNKNOWN
				   | SMFIP_NOUNKNOWN
#endif
#ifdef SMFIP_NODATA
				   | SMFIP_NODATA
#endif
    );

#else
    UINT32_TYPE my_version = 0;
    UINT32_TYPE my_events = 0;
    char   *saved_version;
    char   *cp;
    char   *name;

#endif

    /*
     * Sanity check.
     */
    if (milter->fp != 0)
	msg_panic("%s: milter %s: socket is not closed",
		  myname, milter->m.name);

#ifndef USE_LIBMILTER_INCLUDES

    /*
     * For user friendliness reasons the milter_protocol configuration
     * parameter can specify both the protocol version and protocol
     * extensions (e.g., don't reply for each individual message header).
     * 
     * The protocol version is sent as is to the milter application.
     * 
     * The version and extensions determine what events we can send to the
     * milter application.
     * 
     * We don't announce support for events that aren't defined for my protocol
     * version. Today's libmilter implementations don't seem to care, but we
     * don't want to take the risk that a future version will be more picky.
     */
    cp = saved_version = mystrdup(milter->protocol);
    while ((name = mystrtok(&cp, " ,\t\r\n")) != 0) {
	int     mask;
	int     vers;

	if ((mask = name_code(milter8_event_masks,
			      NAME_CODE_FLAG_NONE, name)) == -1
	    || (vers = name_code(milter8_versions,
				 NAME_CODE_FLAG_NONE, name)) == -1
	    || (vers != 0 && my_version != 0)) {
	    msg_warn("milter %s: bad protocol information: %s",
		     milter->m.name, name);
	    milter8_conf_error(milter);
	    return;
	}
	if (vers != 0)
	    my_version = vers;
	my_events |= mask;
    }
    myfree(saved_version);
    if (my_events == 0 || my_version == 0) {
	msg_warn("milter %s: no protocol version information", milter->m.name);
	milter8_conf_error(milter);
	return;
    }

    /*
     * Don't send events that aren't defined for my protocol version. This is
     * somewhat tricky: unlike the other SMFIP_mumble bits, the SMFIP_NOHREPL
     * bit specifies something that the milter application won't send. We
     * must not inadvertently turn this bit on, because the MTA would get out
     * of sync with the application.
     */
    milter->np_mask = ~(my_events | SMFIP_NOHREPL);
    if (msg_verbose)
	msg_info("%s: non-protocol events for protocol version %d: %s",
		 myname, my_version,
		 str_name_mask_opt(milter->buf, "non-protocol event mask",
			   smfip_table, milter->np_mask, NAME_MASK_NUMBER));
#endif

    /*
     * Parse the Milter application endpoint.
     */
#define FREE_TRANSPORT_AND_BAIL_OUT(milter, milter_error) do { \
	myfree(transport); \
	milter_error(milter); \
	return; \
    } while (0);

    transport = mystrdup(milter->m.name);
    if ((endpoint = split_at(transport, ':')) == 0
	|| *endpoint == 0 || *transport == 0) {
	msg_warn("Milter service needs transport:endpoint instead of \"%s\"",
		 milter->m.name);
	FREE_TRANSPORT_AND_BAIL_OUT(milter, milter8_conf_error);
    }
    if (msg_verbose)
	msg_info("%s: transport=%s endpoint=%s", myname, transport, endpoint);
    if (strcmp(transport, "inet") == 0) {
	connect_fn = inet_connect;
    } else if (strcmp(transport, "unix") == 0) {
	connect_fn = unix_connect;
    } else if (strcmp(transport, "local") == 0) {
	connect_fn = LOCAL_CONNECT;
    } else {
	msg_warn("invalid transport name: %s in Milter service: %s",
		 transport, milter->m.name);
	FREE_TRANSPORT_AND_BAIL_OUT(milter, milter8_conf_error);
    }

    /*
     * Connect to the Milter application.
     */
    if ((fd = connect_fn(endpoint, BLOCKING, milter->conn_timeout)) < 0) {
	msg_warn("connect to Milter service %s: %m", milter->m.name);
	FREE_TRANSPORT_AND_BAIL_OUT(milter, milter8_comm_error);
    }
    myfree(transport);
    milter->fp = vstream_fdopen(fd, O_RDWR);
    vstream_control(milter->fp,
		    VSTREAM_CTL_DOUBLE,
		    VSTREAM_CTL_TIMEOUT, milter->cmd_timeout,
		    VSTREAM_CTL_END);
    /* Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE. */
    if (connect_fn == inet_connect)
	vstream_tweak_tcp(milter->fp);

    /*
     * Open the negotiations by sending what actions the Milter may request
     * and what events the Milter can receive.
     */
    if (msg_verbose) {
	msg_info("%s: my_version=0x%lx", myname, (long) my_version);
	msg_info("%s: my_actions=0x%lx %s", myname, (long) my_actions,
		 str_name_mask_opt(milter->buf, "request mask",
				smfif_table, my_actions, NAME_MASK_NUMBER));
	msg_info("%s: my_events=0x%lx %s", myname, (long) my_events,
		 str_name_mask_opt(milter->buf, "event mask",
				 smfip_table, my_events, NAME_MASK_NUMBER));
    }
    errno = 0;
    if (milter8_write_cmd(milter, SMFIC_OPTNEG,
			  MILTER8_DATA_HLONG, my_version,
			  MILTER8_DATA_HLONG, my_actions,
			  MILTER8_DATA_HLONG, my_events,
			  MILTER8_DATA_END) != 0) {
	msg_warn("milter %s: write error in initial handshake",
		 milter->m.name);
    }

    /*
     * Receive the filter's response and verify that we are compatible.
     */
    else if (milter8_read_resp(milter, SMFIC_OPTNEG, &cmd, &data_len) != 0) {
	msg_warn("milter %s: read error in initial handshake", milter->m.name);
	/* milter8_read_resp() called milter8_comm_error() */
    } else if (cmd != SMFIC_OPTNEG) {
	msg_warn("milter %s: unexpected reply \"%c\" in initial handshake",
		 milter->m.name, cmd);
	(void) milter8_comm_error(milter);
    } else if (milter8_read_data(milter, data_len,
				 MILTER8_DATA_HLONG, &milter->version,
				 MILTER8_DATA_HLONG, &milter->rq_mask,
				 MILTER8_DATA_HLONG, &milter->ev_mask,
				 MILTER8_DATA_END) != 0) {
	msg_warn("milter %s: read error in initial handshake", milter->m.name);
	/* milter8_read_data() called milter8_comm_error() */
    } else if (milter->version > my_version) {
	msg_warn("milter %s: protocol version %d conflict"
		 " with MTA protocol version %d",
		 milter->m.name, milter->version, my_version);
	(void) milter8_comm_error(milter);
    } else if ((milter->rq_mask & my_actions) != milter->rq_mask) {
	msg_warn("milter %s: request mask 0x%x conflict"
		 " with MTA request mask 0x%lx",
		 milter->m.name, milter->rq_mask, (long) my_actions);
	(void) milter8_comm_error(milter);
    }

    /*
     * Successful negotiations completed.
     */
    else {
	if (msg_verbose) {
	    if ((milter->ev_mask & my_events) != milter->ev_mask)
		msg_info("milter %s: event mask 0x%x includes features not"
			 " offered in MTA event mask 0x%lx",
			 milter->m.name, milter->ev_mask, (long) my_events);
	    msg_info("%s: milter %s version %d",
		     myname, milter->m.name, milter->version);
	    msg_info("%s: events %s", myname,
		     str_name_mask_opt(milter->buf, "event mask",
			   smfip_table, milter->ev_mask, NAME_MASK_NUMBER));
	    msg_info("%s: requests %s", myname,
		     str_name_mask_opt(milter->buf, "request mask",
			   smfif_table, milter->rq_mask, NAME_MASK_NUMBER));
	}
	milter->state = MILTER8_STAT_READY;
	milter8_def_reply(milter, 0);
    }
}

/* milter8_conn_event - report connect event to Sendmail 8 milter */

static const char *milter8_conn_event(MILTER *m,
				              const char *client_name,
				              const char *client_addr,
				              const char *client_port,
				              unsigned addr_family,
				              ARGV *macros)
{
    const char *myname = "milter8_conn_event";
    MILTER8 *milter = (MILTER8 *) m;
    int     port;

    /*
     * XXX Sendmail 8 libmilter closes the MTA-to-filter socket when it finds
     * out that the SMTP client has disconnected. Because of this, Postfix
     * has to open a new MTA-to-filter socket for each SMTP client.
     */
#ifdef LIBMILTER_AUTO_DISCONNECT
    milter8_connect(milter);
#endif

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_READY:
	if (msg_verbose)
	    msg_info("%s: milter %s: connect %s/%s",
		     myname, milter->m.name, client_name, client_addr);
	if (client_port == 0) {
	    port = 0;
	} else if (!alldig(client_port) || (port = atoi(client_port)) < 0
		   || port > 65535) {
	    msg_warn("milter %s: bad client port number %s",
		     milter->m.name, client_port);
	    port = 0;
	}
	milter->state = MILTER8_STAT_ENVELOPE;
	switch (addr_family) {
	case AF_INET:
	    return (milter8_event(milter, SMFIC_CONNECT, SMFIP_NOCONNECT,
				  DONT_SKIP_REPLY, macros,
				  MILTER8_DATA_STRING, client_name,
				  MILTER8_DATA_OCTET, SMFIA_INET,
				  MILTER8_DATA_NSHORT, htons(port),
				  MILTER8_DATA_STRING, client_addr,
				  MILTER8_DATA_END));
#ifdef HAS_IPV6
	case AF_INET6:
	    return (milter8_event(milter, SMFIC_CONNECT, SMFIP_NOCONNECT,
				  DONT_SKIP_REPLY, macros,
				  MILTER8_DATA_STRING, client_name,
				  MILTER8_DATA_OCTET, SMFIA_INET6,
				  MILTER8_DATA_NSHORT, htons(port),
				  MILTER8_DATA_STRING, client_addr,
				  MILTER8_DATA_END));
#endif
	case AF_UNIX:
	    return (milter8_event(milter, SMFIC_CONNECT, SMFIP_NOCONNECT,
				  DONT_SKIP_REPLY, macros,
				  MILTER8_DATA_STRING, client_name,
				  MILTER8_DATA_OCTET, SMFIA_UNIX,
				  MILTER8_DATA_NSHORT, htons(0),
				  MILTER8_DATA_STRING, client_addr,
				  MILTER8_DATA_END));
	default:
	    return (milter8_event(milter, SMFIC_CONNECT, SMFIP_NOCONNECT,
				  DONT_SKIP_REPLY, macros,
				  MILTER8_DATA_STRING, client_name,
				  MILTER8_DATA_OCTET, SMFIA_UNKNOWN,
				  MILTER8_DATA_END));
	}
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

/* milter8_helo_event - report HELO/EHLO command to Sendmail 8 milter */

static const char *milter8_helo_event(MILTER *m, const char *helo_name,
				              int unused_esmtp,
				              ARGV *macros)
{
    const char *myname = "milter8_helo_event";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
    case MILTER8_STAT_ACCEPT_MSG:
	/* With HELO after MAIL, smtpd(8) calls milter8_abort() next. */
	if (msg_verbose)
	    msg_info("%s: milter %s: helo %s",
		     myname, milter->m.name, helo_name);
	return (milter8_event(milter, SMFIC_HELO, SMFIP_NOHELO,
			      DONT_SKIP_REPLY, macros,
			      MILTER8_DATA_STRING, helo_name,
			      MILTER8_DATA_END));
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

/* milter8_mail_event - report MAIL command to Sendmail 8 milter */

static const char *milter8_mail_event(MILTER *m, const char **argv,
				              ARGV *macros)
{
    const char *myname = "milter8_mail_event";
    MILTER8 *milter = (MILTER8 *) m;
    const char **cpp;

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
	if (msg_verbose) {
	    VSTRING *buf = vstring_alloc(100);

	    for (cpp = argv; *cpp; cpp++)
		vstring_sprintf_append(buf, " %s", *cpp);
	    msg_info("%s: milter %s: mail%s",
		     myname, milter->m.name, STR(buf));
	    vstring_free(buf);
	}
	return (milter8_event(milter, SMFIC_MAIL, SMFIP_NOMAIL,
			      DONT_SKIP_REPLY, macros,
			      MILTER8_DATA_ARGV, argv,
			      MILTER8_DATA_END));
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

/* milter8_rcpt_event - report RCPT command to Sendmail 8 milter */

static const char *milter8_rcpt_event(MILTER *m, const char **argv,
				              ARGV *macros)
{
    const char *myname = "milter8_rcpt_event";
    MILTER8 *milter = (MILTER8 *) m;
    const char **cpp;

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
	if (msg_verbose) {
	    VSTRING *buf = vstring_alloc(100);

	    for (cpp = argv; *cpp; cpp++)
		vstring_sprintf_append(buf, " %s", *cpp);
	    msg_info("%s: milter %s: rcpt%s",
		     myname, milter->m.name, STR(buf));
	    vstring_free(buf);
	}
	return (milter8_event(milter, SMFIC_RCPT, SMFIP_NORCPT,
			      DONT_SKIP_REPLY, macros,
			      MILTER8_DATA_ARGV, argv,
			      MILTER8_DATA_END));
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

/* milter8_data_event - report DATA command to Sendmail 8 milter */

#ifdef SMFIC_DATA

static const char *milter8_data_event(MILTER *m, ARGV *macros)
{
    const char *myname = "milter8_data_event";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
	if (msg_verbose)
	    msg_info("%s: milter %s: data command", myname, milter->m.name);
	return (milter8_event(milter, SMFIC_DATA, SMFIP_NODATA,
			      DONT_SKIP_REPLY, macros,
			      MILTER8_DATA_END));
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

#else
#define milter8_data_event	0
#endif

/* milter8_unknown_event - report unknown SMTP command to Sendmail 8 milter */

#ifdef SMFIC_UNKNOWN

static const char *milter8_unknown_event(MILTER *m, const char *command,
					         ARGV *macros)
{
    const char *myname = "milter8_unknown_event";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * Report the event.
     */
    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
	if (msg_verbose)
	    msg_info("%s: milter %s: unknown command: %s",
		     myname, milter->m.name, command);
	/* XXX Sendmail doesn't send macros (checked with 8.6.13). */
	return (milter8_event(milter, SMFIC_UNKNOWN, SMFIP_NOUNKNOWN,
			      DONT_SKIP_REPLY, macros,
			      MILTER8_DATA_STRING, command,
			      MILTER8_DATA_END));
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

#else
#define milter8_unknown_event	0
#endif

/* milter8_other_event - reply for other event */

static const char *milter8_other_event(MILTER *m)
{
    const char *myname = "milter8_other_event";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * Return the default reply.
     */
    if (msg_verbose)
	msg_info("%s: milter %s", myname, milter->m.name);
    return (milter->def_reply);
}

/* milter8_abort - cancel one milter's message receiving state */

static void milter8_abort(MILTER *m)
{
    const char *myname = "milter8_abort";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * XXX Sendmail 8 libmilter closes the MTA-to-filter socket when it finds
     * out that the SMTP client has disconnected. Because of this, Postfix
     * has to open a new MTA-to-filter socket for each SMTP client.
     */
    switch (milter->state) {
    case MILTER8_STAT_CLOSED:
    case MILTER8_STAT_READY:
	return;
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
	if (msg_verbose)
	    msg_info("%s: skip milter %s", myname, milter->m.name);
	break;
    case MILTER8_STAT_ENVELOPE:
    case MILTER8_STAT_MESSAGE:
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: abort milter %s", myname, milter->m.name);
	(void) milter8_write_cmd(milter, SMFIC_ABORT, MILTER8_DATA_END);
	if (milter->state != MILTER8_STAT_ERROR)
	    milter->state = MILTER8_STAT_ENVELOPE;
	break;
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

/* milter8_disc_event - report client disconnect event */

static void milter8_disc_event(MILTER *m)
{
    const char *myname = "milter8_disc_event";
    MILTER8 *milter = (MILTER8 *) m;

    /*
     * XXX Sendmail 8 libmilter closes the MTA-to-filter socket when it finds
     * out that the SMTP client has disconnected. Because of this, Postfix
     * has to open a new MTA-to-filter socket for each SMTP client.
     */
    switch (milter->state) {
    case MILTER8_STAT_CLOSED:
    case MILTER8_STAT_READY:
	return;
    case MILTER8_STAT_ERROR:
#ifdef LIBMILTER_AUTO_DISCONNECT
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
#endif
	if (msg_verbose)
	    msg_info("%s: skip quit milter %s", myname, milter->m.name);
	break;
    case MILTER8_STAT_ENVELOPE:
    case MILTER8_STAT_MESSAGE:
#ifndef LIBMILTER_AUTO_DISCONNECT
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
#endif
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: quit milter %s", myname, milter->m.name);
	(void) milter8_write_cmd(milter, SMFIC_QUIT, MILTER8_DATA_END);
	break;
    }
#ifdef LIBMILTER_AUTO_DISCONNECT
    milter8_close_stream(milter);
#else
    if (milter->state != MILTER8_STAT_ERROR)
	milter->state = MILTER8_STAT_READY;
#endif
    milter8_def_reply(milter, 0);
}

 /*
  * Structure to ship context across the MIME_STATE engine.
  */
typedef struct {
    MILTER8 *milter;			/* milter client */
    ARGV   *macros;			/* end-of-body macros */
    int     first_header;		/* first header */
    int     first_body;			/* first body line */
    const char *resp;			/* milter application response */
} MILTER_MSG_CONTEXT;

/* milter8_header - milter8_message call-back for message header */

static void milter8_header(void *ptr, int unused_header_class,
			           HEADER_OPTS *header_info,
			           VSTRING *buf, off_t unused_offset)
{
    const char *myname = "milter8_header";
    MILTER_MSG_CONTEXT *msg_ctx = (MILTER_MSG_CONTEXT *) ptr;
    MILTER8 *milter = msg_ctx->milter;
    char   *cp;
    int     skip_reply;

    /*
     * XXX Workaround: mime_state_update() may invoke multiple call-backs
     * before returning to the caller.
     */
#define MILTER8_MESSAGE_DONE(milter, msg_ctx) \
	((milter)->state != MILTER8_STAT_MESSAGE || (msg_ctx)->resp != 0)

    if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
	return;

    /*
     * XXX Sendmail compatibility. Don't expose our first (received) header
     * to mail filter applications. See also cleanup_milter.c for code to
     * ensure that header replace requests are relative to the message
     * content as received, that is, without our own first (received) header,
     * while header insert requests are relative to the message as delivered,
     * that is, including our own first (received) header.
     * 
     * XXX But this breaks when they delete our own Received: header with
     * header_checks before it reaches the queue file. Even then we must not
     * expose the first header to mail filter applications, otherwise the
     * dk-filter signature will be inserted at the wrong position. It should
     * precede the headers that it signs.
     * 
     * XXX Sendmail compatibility. It eats the first space (not tab) after the
     * header label and ":".
     */
    if (msg_ctx->first_header) {
	msg_ctx->first_header = 0;
	return;
    }

    /*
     * Sendmail 8 sends multi-line headers as text separated by newline.
     * 
     * We destroy the header buffer to split it into label and value. Changing
     * the buffer is explicitly allowed by the mime_state(3) interface.
     */
    if (msg_verbose > 1)
	msg_info("%s: header milter %s: %.100s",
		 myname, milter->m.name, STR(buf));
    cp = STR(buf) + (header_info ? strlen(header_info->name) :
		     is_header(STR(buf)));
    /* XXX Following matches is_header.c */
    while (*cp == ' ' || *cp == '\t')
	*cp++ = 0;
    if (*cp != ':')
	msg_panic("%s: header label not followed by ':'", myname);
    *cp++ = 0;
    /* XXX Sendmail 8.13.6 eats one space (not tab) after colon. */
    if (*cp == ' ')
	cp++;
#ifdef SMFIP_NOHREPL
    skip_reply = ((milter->ev_mask & SMFIP_NOHREPL) != 0);
#else
    skip_reply = DONT_SKIP_REPLY;
#endif
    msg_ctx->resp =
	milter8_event(milter, SMFIC_HEADER, SMFIP_NOHDRS,
		      skip_reply, msg_ctx->macros,
		      MILTER8_DATA_STRING, STR(buf),
		      MILTER8_DATA_STRING, cp,
		      MILTER8_DATA_END);
}

/* milter8_eoh - milter8_message call-back for end-of-header */

static void milter8_eoh(void *ptr)
{
    const char *myname = "milter8_eoh";
    MILTER_MSG_CONTEXT *msg_ctx = (MILTER_MSG_CONTEXT *) ptr;
    MILTER8 *milter = msg_ctx->milter;

    if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
	return;
    if (msg_verbose)
	msg_info("%s: eoh milter %s", myname, milter->m.name);
    msg_ctx->resp =
	milter8_event(milter, SMFIC_EOH, SMFIP_NOEOH,
		      DONT_SKIP_REPLY, msg_ctx->macros,
		      MILTER8_DATA_END);
}

/* milter8_body - milter8_message call-back for body content */

static void milter8_body(void *ptr, int rec_type,
			         const char *buf, ssize_t len,
			         off_t offset)
{
    const char *myname = "milter8_body";
    MILTER_MSG_CONTEXT *msg_ctx = (MILTER_MSG_CONTEXT *) ptr;
    MILTER8 *milter = msg_ctx->milter;
    ssize_t todo = len;
    const char *bp = buf;
    ssize_t space;
    ssize_t count;

    if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
	return;

    /*
     * XXX Sendmail compatibility: don't expose our first body line.
     */
    if (msg_ctx->first_body) {
	msg_ctx->first_body = 0;
	return;
    }

    /*
     * XXX I thought I was going to delegate all the on-the-wire formatting
     * to a common lower layer, but unfortunately it's not practical. If we
     * were to do MILTER_CHUNK_SIZE buffering in a common lower layer, then
     * we would have to pass along call-backs and state, so that the
     * call-back can invoke milter8_event() with the right arguments when the
     * MILTER_CHUNK_SIZE buffer reaches capacity. That's just too ugly.
     * 
     * To recover the cost of making an extra copy of body content from Milter
     * buffer to VSTREAM buffer, we could make vstream_fwrite() a little
     * smarter so that it does large transfers directly from the user buffer
     * instead of copying the data one block at a time into a VSTREAM buffer.
     */
    if (msg_verbose > 1)
	msg_info("%s: body milter %s: %.100s", myname, milter->m.name, buf);
    /* To append \r\n, simply redirect input to another buffer. */
    if (rec_type == REC_TYPE_NORM && todo == 0) {
	bp = "\r\n";
	todo = 2;
	rec_type = REC_TYPE_EOF;
    }
    while (todo > 0) {
	/* Append one REC_TYPE_NORM or REC_TYPE_CONT to body chunk buffer. */
	space = MILTER_CHUNK_SIZE - LEN(milter->body);
	if (space <= 0)
	    msg_panic("%s: bad buffer size: %ld",
		      myname, (long) LEN(milter->body));
	count = (todo > space ? space : todo);
	vstring_memcat(milter->body, bp, count);
	bp += count;
	todo -= count;
	/* Flush body chunk buffer when full. See also milter8_eob(). */
	if (LEN(milter->body) == MILTER_CHUNK_SIZE) {
	    msg_ctx->resp =
		milter8_event(milter, SMFIC_BODY, SMFIP_NOBODY,
			      DONT_SKIP_REPLY, msg_ctx->macros,
			      MILTER8_DATA_BUFFER, milter->body,
			      MILTER8_DATA_END);
	    if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
		break;
	    VSTRING_RESET(milter->body);
	}
	/* To append \r\n, simply redirect input to another buffer. */
	if (rec_type == REC_TYPE_NORM && todo == 0) {
	    bp = "\r\n";
	    todo = 2;
	    rec_type = REC_TYPE_EOF;
	}
    }
}

/* milter8_eob - milter8_message call-back for end-of-body */

static void milter8_eob(void *ptr)
{
    const char *myname = "milter8_eob";
    MILTER_MSG_CONTEXT *msg_ctx = (MILTER_MSG_CONTEXT *) ptr;
    MILTER8 *milter = msg_ctx->milter;

    if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
	return;
    if (msg_verbose)
	msg_info("%s: eob milter %s", myname, milter->m.name);

    /*
     * Flush partial body chunk buffer. See also milter8_body().
     * 
     * XXX Sendmail 8 libmilter accepts SMFIC_EOB+data, and delivers it to the
     * application as two events: SMFIC_BODY+data followed by SMFIC_EOB. This
     * breaks with the PMilter 0.95 protocol re-implementation, which
     * delivers the SMFIC_EOB event and ignores the data. To avoid such
     * compatibility problems we separate the events in the client. With
     * this, we also prepare for a future where different event types can
     * have different macro lists.
     */
    if (LEN(milter->body) > 0) {
	msg_ctx->resp =
	    milter8_event(milter, SMFIC_BODY, SMFIP_NOBODY,
			  DONT_SKIP_REPLY, msg_ctx->macros,
			  MILTER8_DATA_BUFFER, milter->body,
			  MILTER8_DATA_END);
	if (MILTER8_MESSAGE_DONE(milter, msg_ctx))
	    return;
    }
    msg_ctx->resp =
	milter8_event(msg_ctx->milter, SMFIC_BODYEOB, 0,
		      DONT_SKIP_REPLY, msg_ctx->macros,
		      MILTER8_DATA_END);
}

/* milter8_message - send message content and receive reply */

static const char *milter8_message(MILTER *m, VSTREAM *qfile,
				           off_t data_offset,
				           ARGV *macros)
{
    const char *myname = "milter8_message";
    MILTER8 *milter = (MILTER8 *) m;
    MIME_STATE *mime_state;
    int     rec_type;
    MIME_STATE_DETAIL *detail;
    int     mime_errs = 0;
    MILTER_MSG_CONTEXT msg_ctx;
    VSTRING *buf;

    switch (milter->state) {
    case MILTER8_STAT_ERROR:
    case MILTER8_STAT_ACCEPT_CON:
    case MILTER8_STAT_REJECT_CON:
    case MILTER8_STAT_ACCEPT_MSG:
	if (msg_verbose)
	    msg_info("%s: skip message to milter %s", myname, milter->m.name);
	return (milter->def_reply);
    case MILTER8_STAT_ENVELOPE:
	if (msg_verbose)
	    msg_info("%s: message to milter %s", myname, milter->m.name);
	if (vstream_fseek(qfile, data_offset, SEEK_SET) < 0) {
	    msg_warn("%s: vstream_fseek %s: %m", myname, VSTREAM_PATH(qfile));
	    return ("450 4.3.0 Queue file write error");
	}
	msg_ctx.milter = milter;
	msg_ctx.macros = macros;
	msg_ctx.first_header = 1;
	msg_ctx.first_body = 1;
	msg_ctx.resp = 0;
	mime_state =
	    mime_state_alloc(MIME_OPT_DISABLE_MIME,
			     (milter->ev_mask & SMFIP_NOHDRS) ?
			     (MIME_STATE_HEAD_OUT) 0 : milter8_header,
			     (milter->ev_mask & SMFIP_NOEOH) ?
			     (MIME_STATE_ANY_END) 0 : milter8_eoh,
			     (milter->ev_mask & SMFIP_NOBODY) ?
			     (MIME_STATE_BODY_OUT) 0 : milter8_body,
			     milter8_eob,
			     (MIME_STATE_ERR_PRINT) 0,
			     (void *) &msg_ctx);
	buf = vstring_alloc(100);
	milter->state = MILTER8_STAT_MESSAGE;
	VSTRING_RESET(milter->body);
	vstream_control(milter->fp,
			VSTREAM_CTL_DOUBLE,
			VSTREAM_CTL_TIMEOUT, milter->msg_timeout,
			VSTREAM_CTL_END);

	/*
	 * XXX When the message (not MIME body part) does not end in CRLF
	 * (i.e. the last record was REC_TYPE_CONT), do we send a CRLF
	 * terminator before triggering the end-of-body condition?
	 */
	for (;;) {
	    if ((rec_type = rec_get(qfile, buf, 0)) < 0) {
		msg_warn("%s: error reading %s: %m",
			 myname, VSTREAM_PATH(qfile));
		msg_ctx.resp = "450 4.3.0 Queue file write error";
		break;
	    }
	    /* Invoke the appropriate call-back routine. */
	    mime_errs = mime_state_update(mime_state, rec_type,
					  STR(buf), LEN(buf));
	    if (mime_errs) {
		detail = mime_state_detail(mime_errs);
		msg_warn("%s: MIME problem %s in %s",
			 myname, detail->text, VSTREAM_PATH(qfile));
		msg_ctx.resp = "450 4.3.0 Queue file write error";
		break;
	    }
	    if (MILTER8_MESSAGE_DONE(milter, &msg_ctx))
		break;
	    if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT)
		break;
	}
	mime_state_free(mime_state);
	vstring_free(buf);
	if (milter->fp)
	    vstream_control(milter->fp,
			    VSTREAM_CTL_DOUBLE,
			    VSTREAM_CTL_TIMEOUT, milter->cmd_timeout,
			    VSTREAM_CTL_END);
	if (milter->state == MILTER8_STAT_MESSAGE
	    || milter->state == MILTER8_STAT_ACCEPT_MSG)
	    milter->state = MILTER8_STAT_ENVELOPE;
	return (msg_ctx.resp);
    default:
	msg_panic("%s: milter %s: bad state %d",
		  myname, milter->m.name, milter->state);
    }
}

 /*
  * Preliminary protocol to send/receive milter instances. This needs to be
  * extended with type information once we support multiple milter protocols.
  */
#define MAIL_ATTR_MILT_NAME	"milter_name"
#define MAIL_ATTR_MILT_VERS	"milter_version"
#define MAIL_ATTR_MILT_ACTS	"milter_actions"
#define MAIL_ATTR_MILT_EVTS	"milter_events"
#define MAIL_ATTR_MILT_NPTS	"milter_non_events"
#define MAIL_ATTR_MILT_STAT	"milter_state"
#define MAIL_ATTR_MILT_CONN	"milter_conn_timeout"
#define MAIL_ATTR_MILT_CMD	"milter_cmd_timeout"
#define MAIL_ATTR_MILT_MSG	"milter_msg_timeout"
#define MAIL_ATTR_MILT_ACT	"milter_action"

/* milter8_active - report if this milter still wants events */

static int milter8_active(MILTER *m)
{
    MILTER8 *milter = (MILTER8 *) m;

    return (milter->fp != 0
	    && (milter->state == MILTER8_STAT_ENVELOPE
		|| milter->state == MILTER8_STAT_READY));
}

/* milter8_send - send milter instance */

static int milter8_send(MILTER *m, VSTREAM *stream)
{
    const char *myname = "milter8_send";
    MILTER8 *milter = (MILTER8 *) m;

    if (msg_verbose)
	msg_info("%s: milter %s", myname, milter->m.name);

    if (attr_print(stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_NAME, milter->m.name,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_VERS, milter->version,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_ACTS, milter->rq_mask,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_EVTS, milter->ev_mask,
#ifndef USE_LIBMILTER_INCLUDES
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_NPTS, milter->np_mask,
#endif
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_STAT, milter->state,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_CONN, milter->conn_timeout,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_CMD, milter->cmd_timeout,
		   ATTR_TYPE_INT, MAIL_ATTR_MILT_MSG, milter->msg_timeout,
		   ATTR_TYPE_STR, MAIL_ATTR_MILT_ACT, milter->def_action,
		   ATTR_TYPE_END) != 0
	|| vstream_fflush(stream) != 0) {
	return (-1);
#ifdef CANT_WRITE_BEFORE_SENDING_FD
    } else if (attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, milter->buf,
			 ATTR_TYPE_END) != 1) {
	return (-1);
#endif
    } else if (LOCAL_SEND_FD(vstream_fileno(stream),
			     vstream_fileno(milter->fp)) < 0) {
	return (-1);
#ifdef MUST_READ_AFTER_SENDING_FD
    } else if (attr_scan(stream, ATTR_FLAG_STRICT,
			 ATTR_TYPE_STR, MAIL_ATTR_DUMMY, milter->buf,
			 ATTR_TYPE_END) != 1) {
	return (-1);
#endif
    } else {
	return (0);
    }
}

static MILTER8 *milter8_alloc(const char *, int, int, int, const char *,
			              const char *, MILTERS *);

/* milter8_receive - receive milter instance */

MILTER *milter8_receive(VSTREAM *stream, MILTERS *parent)
{
    const char *myname = "milter8_receive";
    static VSTRING *name_buf;
    static VSTRING *act_buf;
    MILTER8 *milter;
    int     version;
    int     rq_mask;
    int     ev_mask;
    int     np_mask;
    int     state;
    int     conn_timeout;
    int     cmd_timeout;
    int     msg_timeout;
    int     fd;

    if (name_buf == 0) {
	name_buf = vstring_alloc(10);
	act_buf = vstring_alloc(10);
    }
    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_NAME, name_buf,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_VERS, &version,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_ACTS, &rq_mask,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_EVTS, &ev_mask,
#ifndef USE_LIBMILTER_INCLUDES
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_NPTS, &np_mask,
#endif
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_STAT, &state,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_CONN, &conn_timeout,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_CMD, &cmd_timeout,
		  ATTR_TYPE_INT, MAIL_ATTR_MILT_MSG, &msg_timeout,
		  ATTR_TYPE_STR, MAIL_ATTR_MILT_ACT, act_buf,
		  ATTR_TYPE_END) < 9) {
	return (0);
#ifdef CANT_WRITE_BEFORE_SENDING_FD
    } else if (attr_print(stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
			  ATTR_TYPE_END) != 0
	       || vstream_fflush(stream) != 0) {
	return (0);
#endif
    } else if ((fd = LOCAL_RECV_FD(vstream_fileno(stream))) < 0) {
	return (0);
#ifdef MUST_READ_AFTER_SENDING_FD
    } else if (attr_print(stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
			  ATTR_TYPE_END) != 0) {
	return (0);
#endif
    } else {
#define NO_PROTOCOL	((char *) 0)

	if (msg_verbose)
	    msg_info("%s: milter %s", myname, STR(name_buf));

	milter = milter8_alloc(STR(name_buf), conn_timeout, cmd_timeout,
			    msg_timeout, NO_PROTOCOL, STR(act_buf), parent);
	milter->fp = vstream_fdopen(fd, O_RDWR);
	vstream_control(milter->fp, VSTREAM_CTL_DOUBLE, VSTREAM_CTL_END);
	/* Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE. */
	vstream_tweak_sock(milter->fp);
	milter->version = version;
	milter->rq_mask = rq_mask;
	milter->ev_mask = ev_mask;
#ifndef USE_LIBMILTER_INCLUDES
	milter->np_mask = np_mask;
#endif
	milter->state = state;
	return (&milter->m);
    }
}

/* milter8_free - destroy Milter instance */

static void milter8_free(MILTER *m)
{
    MILTER8 *milter = (MILTER8 *) m;

    if (msg_verbose)
	msg_info("free milter %s", milter->m.name);
    if (milter->fp)
	(void) vstream_fclose(milter->fp);
    myfree(milter->m.name);
    vstring_free(milter->buf);
    vstring_free(milter->body);
    if (milter->protocol)
	myfree(milter->protocol);
    myfree(milter->def_action);
    if (milter->def_reply)
	myfree(milter->def_reply);
    myfree((char *) milter);
}

/* milter8_alloc - create MTA-side Sendmail 8 Milter instance */

static MILTER8 *milter8_alloc(const char *name, int conn_timeout,
			              int cmd_timeout, int msg_timeout,
			              const char *protocol,
			              const char *def_action,
			              MILTERS *parent)
{
    MILTER8 *milter;

    /*
     * Fill in the structure.
     */
    milter = (MILTER8 *) mymalloc(sizeof(*milter));
    milter->m.name = mystrdup(name);
    milter->m.next = 0;
    milter->m.parent = parent;
    milter->m.conn_event = milter8_conn_event;
    milter->m.helo_event = milter8_helo_event;
    milter->m.mail_event = milter8_mail_event;
    milter->m.rcpt_event = milter8_rcpt_event;
    milter->m.data_event = milter8_data_event;	/* may be null */
    milter->m.message = milter8_message;
    milter->m.unknown_event = milter8_unknown_event;	/* may be null */
    milter->m.other_event = milter8_other_event;
    milter->m.abort = milter8_abort;
    milter->m.disc_event = milter8_disc_event;
    milter->m.active = milter8_active;
    milter->m.send = milter8_send;
    milter->m.free = milter8_free;
    milter->fp = 0;
    milter->buf = vstring_alloc(100);
    milter->body = vstring_alloc(100);
    milter->version = 0;
    milter->rq_mask = 0;
    milter->ev_mask = 0;
    milter->state = MILTER8_STAT_CLOSED;
    milter->conn_timeout = conn_timeout;
    milter->cmd_timeout = cmd_timeout;
    milter->msg_timeout = msg_timeout;
    milter->protocol = (protocol ? mystrdup(protocol) : 0);
    milter->def_action = mystrdup(def_action);
    milter->def_reply = 0;

    return (milter);
}

/* milter8_create - create MTA-side Sendmail 8 Milter instance */

MILTER *milter8_create(const char *name, int conn_timeout, int cmd_timeout,
		               int msg_timeout, const char *protocol,
		               const char *def_action, MILTERS *parent)
{
    MILTER8 *milter;

    /*
     * Fill in the structure.
     */
    milter = milter8_alloc(name, conn_timeout, cmd_timeout, msg_timeout,
			   protocol, def_action, parent);

    /*
     * XXX Sendmail 8 libmilter closes the MTA-to-filter socket when it finds
     * out that the SMTP client has disconnected. Because of this, Postfix
     * has to open a new MTA-to-filter socket for each SMTP client.
     */
#ifndef LIBMILTER_AUTO_DISCONNECT
    milter8_connect(milter);
#endif
    return (&milter->m);
}
