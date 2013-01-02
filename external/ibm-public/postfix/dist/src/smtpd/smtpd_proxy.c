/*	$NetBSD: smtpd_proxy.c,v 1.1.1.6 2013/01/02 18:59:10 tron Exp $	*/

/*++
/* NAME
/*	smtpd_proxy 3
/* SUMMARY
/*	SMTP server pass-through proxy client
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_proxy.h>
/*
/*	typedef struct {
/* .in +4
/*		VSTREAM *stream;	/* SMTP proxy or replay log */
/*		VSTRING *buffer;	/* last SMTP proxy response */
/*		/* other fields... */
/* .in -4
/*	} SMTPD_PROXY;
/*
/*	int	smtpd_proxy_create(state, flags, service, timeout,
/*					ehlo_name, mail_from)
/*	SMTPD_STATE *state;
/*	int	flags;
/*	const char *service;
/*	int	timeout;
/*	const char *ehlo_name;
/*	const char *mail_from;
/*
/*	int	proxy->cmd(state, expect, format, ...)
/*	SMTPD_PROXY *proxy;
/*	SMTPD_STATE *state;
/*	int	expect;
/*	const char *format;
/*
/*	void	smtpd_proxy_disconnect(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_proxy_free(state)
/*	SMTPD_STATE *state;
/*
/*	int	smtpd_proxy_parse_opts(param_name, param_val)
/*	const char *param_name;
/*	const char *param_val;
/* RECORD-LEVEL ROUTINES
/*	int	proxy->rec_put(proxy->stream, rec_type, data, len)
/*	SMTPD_PROXY *proxy;
/*	int	rec_type;
/*	const char *data;
/*	ssize_t	len;
/*
/*	int	proxy->rec_fprintf(proxy->stream, rec_type, format, ...)
/*	SMTPD_PROXY *proxy;
/*	int	rec_type;
/*	cont char *format;
/* DESCRIPTION
/*	The functions in this module implement a pass-through proxy
/*	client.
/*
/*	In order to minimize the intrusiveness of pass-through
/*	proxying, 1) the proxy server must support the same MAIL
/*	FROM/RCPT syntax that Postfix supports, 2) the record-level
/*	routines for message content proxying have the same interface
/*	as the routines that are used for non-proxied mail.
/*
/*	smtpd_proxy_create() takes a description of a before-queue
/*	filter.  Depending on flags, it either arranges to buffer
/*	up commands and message content until the entire message
/*	is received, or it immediately connects to the proxy service,
/*	sends EHLO, sends client information with the XFORWARD
/*	command if possible, sends the MAIL FROM command, and
/*	receives the reply.
/*	A non-zero result value means trouble: either the proxy is
/*	unavailable, or it did not send the expected reply.
/*	All results are reported via the proxy->buffer field in a
/*	form that can be sent to the SMTP client.  An unexpected
/*	2xx or 3xx proxy server response is replaced by a generic
/*	error response to avoid support problems.
/*	In case of error, smtpd_proxy_create() updates the
/*	state->error_mask and state->err fields, and leaves the
/*	SMTPD_PROXY handle in an unconnected state.  Destroy the
/*	handle after reporting the error reply in the proxy->buffer
/*	field.
/*
/*	proxy->cmd() formats and either buffers up the command and
/*	expected response until the entire message is received, or
/*	it immediately sends the specified command to the proxy
/*	server, and receives the proxy server reply.
/*	A non-zero result value means trouble: either the proxy is
/*	unavailable, or it did not send the expected reply.
/*	All results are reported via the proxy->buffer field in a
/*	form that can be sent to the SMTP client.  An unexpected
/*	2xx or 3xx proxy server response is replaced by a generic
/*	error response to avoid support problems.
/*	In case of error, proxy->cmd() updates the state->error_mask
/*	and state->err fields.
/*
/*	smtpd_proxy_disconnect() disconnects from a proxy server.
/*	The last proxy server reply or error description remains
/*	available via the proxy->buffer field.
/*
/*	smtpd_proxy_free() destroys a proxy server handle and resets
/*	the state->proxy field.
/*
/*	smtpd_proxy_parse_opts() parses main.cf processing options.
/*
/*	proxy->rec_put() is a rec_put() clone that either buffers
/*	up arbitrary message content records until the entire message
/*	is received, or that immediately sends it to the proxy
/*	server.
/*	All data is expected to be in SMTP dot-escaped form.
/*	All errors are reported as a REC_TYPE_ERROR result value,
/*	with the state->error_mask, state->err and proxy-buffer
/*	fields given appropriate values.
/*
/*	proxy->rec_fprintf() is a rec_fprintf() clone that formats
/*	message content and either buffers up the record until the
/*	entire message is received, or that immediately sends it
/*	to the proxy server.
/*	All data is expected to be in SMTP dot-escaped form.
/*	All errors are reported as a REC_TYPE_ERROR result value,
/*	with the state->error_mask, state->err and proxy-buffer
/*	fields given appropriate values.
/*
/* Arguments:
/* .IP flags
/*	Zero, or SMTPD_PROXY_FLAG_SPEED_ADJUST to buffer up the entire
/*	message before contacting a before-queue content filter.
/*	Note: when this feature is requested, the before-queue
/*	filter MUST use the same 2xx, 4xx or 5xx reply code for all
/*	recipients of a multi-recipient message.
/* .IP server
/*	The SMTP proxy server host:port. The host or host: part is optional.
/*	This argument is not duplicated.
/* .IP timeout
/*	Time limit for connecting to the proxy server and for
/*	sending and receiving proxy server commands and replies.
/* .IP ehlo_name
/*	The EHLO Hostname that will be sent to the proxy server.
/*	This argument is not duplicated.
/* .IP mail_from
/*	The MAIL FROM command. This argument is not duplicated.
/* .IP state
/*	SMTP server state.
/* .IP expect
/*	Expected proxy server reply status code range. A warning is logged
/*	when an unexpected reply is received. Specify one of the following:
/* .RS
/* .IP SMTPD_PROX_WANT_OK
/*	The caller expects a reply in the 200 range.
/* .IP SMTPD_PROX_WANT_MORE
/*	The caller expects a reply in the 300 range.
/* .IP SMTPD_PROX_WANT_ANY
/*	The caller has no expectation. Do not warn for unexpected replies.
/* .IP SMTPD_PROX_WANT_NONE
/*	Do not bother waiting for a reply.
/* .RE
/* .IP format
/*	A format string.
/* .IP stream
/*	Connection to proxy server.
/* .IP data
/*	Pointer to the content of one message content record.
/* .IP len
/*	The length of a message content record.
/* SEE ALSO
/*	smtpd(8) Postfix smtp server
/* DIAGNOSTICS
/*	Panic: internal API violations.
/*
/*	Fatal errors: memory allocation problem.
/*
/*	Warnings: unexpected response from proxy server, unable
/*	to connect to proxy server, proxy server read/write error,
/*	proxy speed-adjust buffer read/write error.
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
#include <ctype.h>
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>
#include <connect.h>
#include <name_code.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_error.h>
#include <smtp_stream.h>
#include <cleanup_user.h>
#include <mail_params.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <mail_params.h>		/* null_format_string */
#include <xtext.h>
#include <record.h>
#include <mail_queue.h>

/* Application-specific. */

#include <smtpd.h>
#include <smtpd_proxy.h>

 /*
  * XFORWARD server features, recognized by the pass-through proxy client.
  */
#define SMTPD_PROXY_XFORWARD_NAME  (1<<0)	/* client name */
#define SMTPD_PROXY_XFORWARD_ADDR  (1<<1)	/* client address */
#define SMTPD_PROXY_XFORWARD_PROTO (1<<2)	/* protocol */
#define SMTPD_PROXY_XFORWARD_HELO  (1<<3)	/* client helo */
#define SMTPD_PROXY_XFORWARD_IDENT (1<<4)	/* message identifier */
#define SMTPD_PROXY_XFORWARD_DOMAIN (1<<5)	/* origin type */
#define SMTPD_PROXY_XFORWARD_PORT  (1<<6)	/* client port */

 /*
  * Spead-matching: we use an unlinked file for transient storage.
  */
static VSTREAM *smtpd_proxy_replay_stream;

 /*
  * Forward declarations.
  */
static void smtpd_proxy_fake_server_reply(SMTPD_STATE *, int);
static int smtpd_proxy_rdwr_error(SMTPD_STATE *, int);
static int smtpd_proxy_cmd(SMTPD_STATE *, int, const char *,...);
static int smtpd_proxy_rec_put(VSTREAM *, int, const char *, ssize_t);

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)
#define SMTPD_PROXY_CONN_FMT null_format_string
#define STREQ(x, y)	(strcmp((x), (y)) == 0)

/* smtpd_proxy_xforward_flush - flush forwarding information */

static int smtpd_proxy_xforward_flush(SMTPD_STATE *state, VSTRING *buf)
{
    int     ret;

    if (VSTRING_LEN(buf) > 0) {
	ret = smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK,
			      XFORWARD_CMD "%s", STR(buf));
	VSTRING_RESET(buf);
	return (ret);
    }
    return (0);
}

/* smtpd_proxy_xforward_send - send forwarding information */

static int smtpd_proxy_xforward_send(SMTPD_STATE *state, VSTRING *buf,
				             const char *name,
				             int value_available,
				             const char *value)
{
    size_t  new_len;
    int     ret;

#define CONSTR_LEN(s)	(sizeof(s) - 1)
#define PAYLOAD_LIMIT	(512 - CONSTR_LEN("250 " XFORWARD_CMD "\r\n"))

    if (!value_available)
	value = XFORWARD_UNAVAILABLE;

    /*
     * Encode the attribute value.
     */
    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(100);
    xtext_quote(state->expand_buf, value, "");

    /*
     * How much space does this attribute need? SPACE name = value.
     */
    new_len = strlen(name) + strlen(STR(state->expand_buf)) + 2;
    if (new_len > PAYLOAD_LIMIT)
	msg_warn("%s command payload %s=%.10s... exceeds SMTP protocol limit",
		 XFORWARD_CMD, name, value);

    /*
     * Flush the buffer if we need to, and store the attribute.
     */
    if (VSTRING_LEN(buf) > 0 && VSTRING_LEN(buf) + new_len > PAYLOAD_LIMIT)
	if ((ret = smtpd_proxy_xforward_flush(state, buf)) < 0)
	    return (ret);
    vstring_sprintf_append(buf, " %s=%s", name, STR(state->expand_buf));

    return (0);
}

/* smtpd_proxy_connect - open proxy connection */

static int smtpd_proxy_connect(SMTPD_STATE *state)
{
    SMTPD_PROXY *proxy = state->proxy;
    int     fd;
    char   *lines;
    char   *words;
    VSTRING *buf;
    int     bad;
    char   *word;
    static const NAME_CODE known_xforward_features[] = {
	XFORWARD_NAME, SMTPD_PROXY_XFORWARD_NAME,
	XFORWARD_ADDR, SMTPD_PROXY_XFORWARD_ADDR,
	XFORWARD_PORT, SMTPD_PROXY_XFORWARD_PORT,
	XFORWARD_PROTO, SMTPD_PROXY_XFORWARD_PROTO,
	XFORWARD_HELO, SMTPD_PROXY_XFORWARD_HELO,
	XFORWARD_IDENT, SMTPD_PROXY_XFORWARD_IDENT,
	XFORWARD_DOMAIN, SMTPD_PROXY_XFORWARD_DOMAIN,
	0, 0,
    };
    int     server_xforward_features;
    int     (*connect_fn) (const char *, int, int);
    const char *endpoint;

    /*
     * Find connection method (default inet)
     */
    if (strncasecmp("unix:", proxy->service_name, 5) == 0) {
	endpoint = proxy->service_name + 5;
	connect_fn = unix_connect;
    } else {
	if (strncasecmp("inet:", proxy->service_name, 5) == 0)
	    endpoint = proxy->service_name + 5;
	else
	    endpoint = proxy->service_name;
	connect_fn = inet_connect;
    }

    /*
     * Connect to proxy.
     */
    if ((fd = connect_fn(endpoint, BLOCKING, proxy->timeout)) < 0) {
	msg_warn("connect to proxy filter %s: %m", proxy->service_name);
	return (smtpd_proxy_rdwr_error(state, 0));
    }
    proxy->service_stream = vstream_fdopen(fd, O_RDWR);
    /* Needed by our DATA-phase record emulation routines. */
    vstream_control(proxy->service_stream, VSTREAM_CTL_CONTEXT,
		    (char *) state, VSTREAM_CTL_END);
    /* Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE. */
    if (connect_fn == inet_connect)
	vstream_tweak_tcp(proxy->service_stream);
    smtp_timeout_setup(proxy->service_stream, proxy->timeout);

    /*
     * Get server greeting banner.
     * 
     * If this fails then we have a problem because the proxy should always
     * accept our connection. Make up our own response instead of passing
     * back a negative greeting banner: the proxy open is delayed to the
     * point that the client expects a MAIL FROM or RCPT TO reply.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, SMTPD_PROXY_CONN_FMT)) {
	smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY);
	smtpd_proxy_close(state);
	return (-1);
    }

    /*
     * Send our own EHLO command. If this fails then we have a problem
     * because the proxy should always accept our EHLO command. Make up our
     * own response instead of passing back a negative EHLO reply: the proxy
     * open is delayed to the point that the remote SMTP client expects a
     * MAIL FROM or RCPT TO reply.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, "EHLO %s",
			proxy->ehlo_name)) {
	smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY);
	smtpd_proxy_close(state);
	return (-1);
    }

    /*
     * Parse the EHLO reply and see if we can forward logging information.
     */
    server_xforward_features = 0;
    lines = STR(proxy->buffer);
    while ((words = mystrtok(&lines, "\n")) != 0) {
	if (mystrtok(&words, "- ") && (word = mystrtok(&words, " \t")) != 0) {
	    if (strcasecmp(word, XFORWARD_CMD) == 0)
		while ((word = mystrtok(&words, " \t")) != 0)
		    server_xforward_features |=
			name_code(known_xforward_features,
				  NAME_CODE_FLAG_NONE, word);
	}
    }

    /*
     * Send XFORWARD attributes. For robustness, explicitly specify what SMTP
     * session attributes are known and unknown. Make up our own response
     * instead of passing back a negative XFORWARD reply: the proxy open is
     * delayed to the point that the remote SMTP client expects a MAIL FROM
     * or RCPT TO reply.
     */
    if (server_xforward_features) {
	buf = vstring_alloc(100);
	bad =
	    (((server_xforward_features & SMTPD_PROXY_XFORWARD_NAME)
	      && smtpd_proxy_xforward_send(state, buf, XFORWARD_NAME,
				  IS_AVAIL_CLIENT_NAME(FORWARD_NAME(state)),
					   FORWARD_NAME(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_ADDR)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_ADDR,
				  IS_AVAIL_CLIENT_ADDR(FORWARD_ADDR(state)),
					      FORWARD_ADDR(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_PORT)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_PORT,
				  IS_AVAIL_CLIENT_PORT(FORWARD_PORT(state)),
					      FORWARD_PORT(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_HELO)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_HELO,
				  IS_AVAIL_CLIENT_HELO(FORWARD_HELO(state)),
					      FORWARD_HELO(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_IDENT)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_IDENT,
				IS_AVAIL_CLIENT_IDENT(FORWARD_IDENT(state)),
					      FORWARD_IDENT(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_PROTO)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_PROTO,
				IS_AVAIL_CLIENT_PROTO(FORWARD_PROTO(state)),
					      FORWARD_PROTO(state)))
	     || ((server_xforward_features & SMTPD_PROXY_XFORWARD_DOMAIN)
		 && smtpd_proxy_xforward_send(state, buf, XFORWARD_DOMAIN, 1,
			 STREQ(FORWARD_DOMAIN(state), MAIL_ATTR_RWR_LOCAL) ?
				  XFORWARD_DOM_LOCAL : XFORWARD_DOM_REMOTE))
	     || smtpd_proxy_xforward_flush(state, buf));
	vstring_free(buf);
	if (bad) {
	    smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY);
	    smtpd_proxy_close(state);
	    return (-1);
	}
    }

    /*
     * Pass-through the remote SMTP client's MAIL FROM command. If this
     * fails, then we have a problem because the proxy should always accept
     * any MAIL FROM command that was accepted by us.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, "%s",
			proxy->mail_from) != 0) {
	/* NOT: smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY); */
	smtpd_proxy_close(state);
	return (-1);
    }
    return (0);
}

/* smtpd_proxy_fake_server_reply - produce generic error response */

static void smtpd_proxy_fake_server_reply(SMTPD_STATE *state, int status)
{
    const CLEANUP_STAT_DETAIL *detail;

    /*
     * Either we have no server reply (connection refused), or we have an
     * out-of-protocol server reply, so we make up a generic server error
     * response instead.
     */
    detail = cleanup_stat_detail(status);
    vstring_sprintf(state->proxy->buffer,
		    "%d %s Error: %s",
		    detail->smtp, detail->dsn, detail->text);
}

/* smtpd_proxy_replay_rdwr_error - report replay log I/O error */

static int smtpd_proxy_replay_rdwr_error(SMTPD_STATE *state)
{

    /*
     * Log an appropriate warning message.
     */
    msg_warn("proxy speed-adjust log I/O error: %m");

    /*
     * Set the appropriate flags and server reply.
     */
    state->error_mask |= MAIL_ERROR_RESOURCE;
    /* Update state->err in case we are past the client's DATA command. */
    state->err |= CLEANUP_STAT_PROXY;
    smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY);
    return (-1);
}

/* smtpd_proxy_rdwr_error - report proxy communication error */

static int smtpd_proxy_rdwr_error(SMTPD_STATE *state, int err)
{
    const char *myname = "smtpd_proxy_rdwr_error";
    SMTPD_PROXY *proxy = state->proxy;

    /*
     * Sanity check.
     */
    if (err != 0 && err != SMTP_ERR_NONE && proxy == 0)
	msg_panic("%s: proxy error %d without proxy handle", myname, err);

    /*
     * Log an appropriate warning message.
     */
    switch (err) {
    case 0:
    case SMTP_ERR_NONE:
	break;
    case SMTP_ERR_EOF:
	msg_warn("lost connection with proxy %s", proxy->service_name);
	break;
    case SMTP_ERR_TIME:
	msg_warn("timeout talking to proxy %s", proxy->service_name);
	break;
    default:
	msg_panic("%s: unknown proxy %s error %d",
		  myname, proxy->service_name, err);
    }

    /*
     * Set the appropriate flags and server reply.
     */
    state->error_mask |= MAIL_ERROR_SOFTWARE;
    /* Update state->err in case we are past the client's DATA command. */
    state->err |= CLEANUP_STAT_PROXY;
    smtpd_proxy_fake_server_reply(state, CLEANUP_STAT_PROXY);
    return (-1);
}

/* smtpd_proxy_replay_send - replay saved SMTP session from speed-match log */

static int smtpd_proxy_replay_send(SMTPD_STATE *state)
{
    const char *myname = "smtpd_proxy_replay_send";
    static VSTRING *replay_buf = 0;
    SMTPD_PROXY *proxy = state->proxy;
    int     rec_type;
    int     expect = SMTPD_PROX_WANT_BAD;

    /*
     * Sanity check.
     */
    if (smtpd_proxy_replay_stream == 0)
	msg_panic("%s: no before-queue filter speed-adjust log", myname);

    /*
     * Errors first.
     */
    if (vstream_ferror(smtpd_proxy_replay_stream)
	|| vstream_feof(smtpd_proxy_replay_stream)
	|| rec_put(smtpd_proxy_replay_stream, REC_TYPE_END, "", 0) != REC_TYPE_END
	|| vstream_fflush(smtpd_proxy_replay_stream))
	/* NOT: fsync(vstream_fileno(smtpd_proxy_replay_stream)) */
	return (smtpd_proxy_replay_rdwr_error(state));

    /*
     * Delayed connection to the before-queue filter.
     */
    if (smtpd_proxy_connect(state) < 0)
	return (-1);

    /*
     * Replay the speed-match log. We do sanity check record content, but we
     * don't implement a protocol state engine here, since we are reading
     * from a file that we just wrote ourselves.
     */
    if (replay_buf == 0)
	replay_buf = vstring_alloc(100);
    if (vstream_fseek(smtpd_proxy_replay_stream, (off_t) 0, SEEK_SET) < 0)
	return (smtpd_proxy_replay_rdwr_error(state));

    for (;;) {
	switch (rec_type = rec_get(smtpd_proxy_replay_stream, replay_buf,
				   REC_FLAG_NONE)) {

	    /*
	     * Message content.
	     */
	case REC_TYPE_NORM:
	case REC_TYPE_CONT:
	    if (smtpd_proxy_rec_put(proxy->service_stream, rec_type,
				    STR(replay_buf), LEN(replay_buf)) < 0)
		return (-1);
	    break;

	    /*
	     * Expected server reply type.
	     */
	case REC_TYPE_RCPT:
	    if (!alldig(STR(replay_buf))
		|| (expect = atoi(STR(replay_buf))) == SMTPD_PROX_WANT_BAD)
		msg_panic("%s: malformed server reply type: %s",
			  myname, STR(replay_buf));
	    break;

	    /*
	     * Client command, or void. Bail out on the first negative proxy
	     * response. This is OK, because the filter must use the same
	     * reply code for all recipients of a multi-recipient message.
	     */
	case REC_TYPE_FROM:
	    if (expect == SMTPD_PROX_WANT_BAD)
		msg_panic("%s: missing server reply type", myname);
	    if (smtpd_proxy_cmd(state, expect, *STR(replay_buf) ? "%s" :
				SMTPD_PROXY_CONN_FMT, STR(replay_buf)) < 0)
		return (-1);
	    expect = SMTPD_PROX_WANT_BAD;
	    break;

	    /*
	     * Explicit end marker, instead of implicit EOF.
	     */
	case REC_TYPE_END:
	    return (0);

	    /*
	     * Errors.
	     */
	case REC_TYPE_ERROR:
	    return (smtpd_proxy_replay_rdwr_error(state));
	default:
	    msg_panic("%s: unexpected record type; %d", myname, rec_type);
	}
    }
}

/* smtpd_proxy_save_cmd - save SMTP command + expected response to replay log */

static int smtpd_proxy_save_cmd(SMTPD_STATE *state, int expect, const char *fmt,...)
{
    va_list ap;

    /*
     * Errors first.
     */
    if (vstream_ferror(smtpd_proxy_replay_stream)
	|| vstream_feof(smtpd_proxy_replay_stream))
	return (smtpd_proxy_replay_rdwr_error(state));

    /*
     * Save the expected reply first, so that the replayer can safely
     * overwrite the input buffer with the command.
     */
    rec_fprintf(smtpd_proxy_replay_stream, REC_TYPE_RCPT, "%d", expect);

    /*
     * The command can be omitted at the start of an SMTP session. This is
     * not documented as part of the official interface because it is used
     * only internally to this module. Use an explicit null string in case
     * the SMTPD_PROXY_CONN_FMT implementation details change.
     */
    if (fmt == SMTPD_PROXY_CONN_FMT)
	fmt = "";

    /*
     * Save the command to the replay log, and send it to the before-queue
     * filter after we have received the entire message.
     */
    va_start(ap, fmt);
    rec_vfprintf(smtpd_proxy_replay_stream, REC_TYPE_FROM, fmt, ap);
    va_end(ap);

    /*
     * If we just saved the "." command, replay the log.
     */
    return (strcmp(fmt, ".") ? 0 : smtpd_proxy_replay_send(state));
}

/* smtpd_proxy_cmd_warn - report unexpected proxy reply */

static void smtpd_proxy_cmd_warn(SMTPD_STATE *state, const char *fmt,
				         va_list ap)
{
    SMTPD_PROXY *proxy = state->proxy;
    VSTRING *buf;

    /*
     * The command can be omitted at the start of an SMTP session. A null
     * format string is not documented as part of the official interface
     * because it is used only internally to this module.
     */
    buf = vstring_alloc(100);
    vstring_vsprintf(buf, fmt == SMTPD_PROXY_CONN_FMT ?
		     "connection request" : fmt, ap);
    msg_warn("proxy %s rejected \"%s\": \"%s\"",
	     proxy->service_name, STR(buf), STR(proxy->buffer));
    vstring_free(buf);
}

/* smtpd_proxy_cmd - send command to proxy, receive reply */

static int smtpd_proxy_cmd(SMTPD_STATE *state, int expect, const char *fmt,...)
{
    SMTPD_PROXY *proxy = state->proxy;
    va_list ap;
    char   *cp;
    int     last_char;
    int     err = 0;
    static VSTRING *buffer = 0;

    /*
     * Errors first. Be prepared for delayed errors from the DATA phase.
     */
    if (vstream_ferror(proxy->service_stream)
	|| vstream_feof(proxy->service_stream)
	|| (err = vstream_setjmp(proxy->service_stream)) != 0) {
	return (smtpd_proxy_rdwr_error(state, err));
    }

    /*
     * The command can be omitted at the start of an SMTP session. This is
     * not documented as part of the official interface because it is used
     * only internally to this module.
     */
    if (fmt != SMTPD_PROXY_CONN_FMT) {

	/*
	 * Format the command.
	 */
	va_start(ap, fmt);
	vstring_vsprintf(proxy->buffer, fmt, ap);
	va_end(ap);

	/*
	 * Optionally log the command first, so that we can see in the log
	 * what the program is trying to do.
	 */
	if (msg_verbose)
	    msg_info("> %s: %s", proxy->service_name, STR(proxy->buffer));

	/*
	 * Send the command to the proxy server. Since we're going to read a
	 * reply immediately, there is no need to flush buffers.
	 */
	smtp_fputs(STR(proxy->buffer), LEN(proxy->buffer),
		   proxy->service_stream);
    }

    /*
     * Early return if we don't want to wait for a server reply (such as
     * after sending QUIT).
     */
    if (expect == SMTPD_PROX_WANT_NONE)
	return (0);

    /*
     * Censor out non-printable characters in server responses and save
     * complete multi-line responses if possible.
     * 
     * We can't parse or store input that exceeds var_line_limit, so we just
     * skip over it to simplify the remainder of the code below.
     */
    VSTRING_RESET(proxy->buffer);
    if (buffer == 0)
	buffer = vstring_alloc(10);
    for (;;) {
	last_char = smtp_get(buffer, proxy->service_stream, var_line_limit,
			     SMTP_GET_FLAG_SKIP);
	printable(STR(buffer), '?');
	if (last_char != '\n')
	    msg_warn("%s: response longer than %d: %.30s...",
		     proxy->service_name, var_line_limit,
		     STR(buffer));
	if (msg_verbose)
	    msg_info("< %s: %.100s", proxy->service_name, STR(buffer));

	/*
	 * Defend against a denial of service attack by limiting the amount
	 * of multi-line text that we are willing to store.
	 */
	if (LEN(proxy->buffer) < var_line_limit) {
	    if (VSTRING_LEN(proxy->buffer))
		vstring_strcat(proxy->buffer, "\r\n");
	    vstring_strcat(proxy->buffer, STR(buffer));
	}

	/*
	 * Parse the response into code and text. Ignore unrecognized
	 * garbage. This means that any character except space (or end of
	 * line) will have the same effect as the '-' line continuation
	 * character.
	 */
	for (cp = STR(buffer); *cp && ISDIGIT(*cp); cp++)
	     /* void */ ;
	if (cp - STR(buffer) == 3) {
	    if (*cp == '-')
		continue;
	    if (*cp == ' ' || *cp == 0)
		break;
	}
	msg_warn("received garbage from proxy %s: %.100s",
		 proxy->service_name, STR(buffer));
    }

    /*
     * Log a warning in case the proxy does not send the expected response.
     * Silently accept any response when the client expressed no expectation.
     * 
     * Starting with Postfix 2.6 we don't pass through unexpected 2xx or 3xx
     * proxy replies. They are a source of support problems, so we replace
     * them by generic server error replies.
     */
    if (expect != SMTPD_PROX_WANT_ANY && expect != *STR(proxy->buffer)) {
	va_start(ap, fmt);
	smtpd_proxy_cmd_warn(state, fmt, ap);
	va_end(ap);
	if (*STR(proxy->buffer) == SMTPD_PROX_WANT_OK
	    || *STR(proxy->buffer) == SMTPD_PROX_WANT_MORE) {
	    smtpd_proxy_rdwr_error(state, 0);
	}
	return (-1);
    } else {
	return (0);
    }
}

/* smtpd_proxy_save_rec_put - save message content to replay log */

static int smtpd_proxy_save_rec_put(VSTREAM *stream, int rec_type,
				            const char *data, ssize_t len)
{
    const char *myname = "smtpd_proxy_save_rec_put";
    int     ret;

#define VSTREAM_TO_SMTPD_STATE(s) ((SMTPD_STATE *) vstream_context(s))

    /*
     * Sanity check.
     */
    if (stream == 0)
	msg_panic("%s: attempt to use closed stream", myname);

    /*
     * Send one content record. Errors and results must be as with rec_put().
     */
    if (rec_type == REC_TYPE_NORM || rec_type == REC_TYPE_CONT)
	ret = rec_put(stream, rec_type, data, len);
    else
	msg_panic("%s: need REC_TYPE_NORM or REC_TYPE_CONT", myname);

    /*
     * Errors last.
     */
    if (ret != rec_type) {
	(void) smtpd_proxy_replay_rdwr_error(VSTREAM_TO_SMTPD_STATE(stream));
	return (REC_TYPE_ERROR);
    }
    return (rec_type);
}

/* smtpd_proxy_rec_put - send message content, rec_put() clone */

static int smtpd_proxy_rec_put(VSTREAM *stream, int rec_type,
			               const char *data, ssize_t len)
{
    const char *myname = "smtpd_proxy_rec_put";
    int     err = 0;

    /*
     * Errors first.
     */
    if (vstream_ferror(stream) || vstream_feof(stream)
	|| (err = vstream_setjmp(stream)) != 0) {
	(void) smtpd_proxy_rdwr_error(VSTREAM_TO_SMTPD_STATE(stream), err);
	return (REC_TYPE_ERROR);
    }

    /*
     * Send one content record. Errors and results must be as with rec_put().
     */
    if (rec_type == REC_TYPE_NORM)
	smtp_fputs(data, len, stream);
    else if (rec_type == REC_TYPE_CONT)
	smtp_fwrite(data, len, stream);
    else
	msg_panic("%s: need REC_TYPE_NORM or REC_TYPE_CONT", myname);
    return (rec_type);
}

/* smtpd_proxy_save_rec_fprintf - save message content to replay log */

static int smtpd_proxy_save_rec_fprintf(VSTREAM *stream, int rec_type,
					        const char *fmt,...)
{
    const char *myname = "smtpd_proxy_save_rec_fprintf";
    va_list ap;
    int     ret;

    /*
     * Sanity check.
     */
    if (stream == 0)
	msg_panic("%s: attempt to use closed stream", myname);

    /*
     * Save one content record. Errors and results must be as with
     * rec_fprintf().
     */
    va_start(ap, fmt);
    if (rec_type == REC_TYPE_NORM)
	ret = rec_vfprintf(stream, rec_type, fmt, ap);
    else
	msg_panic("%s: need REC_TYPE_NORM", myname);
    va_end(ap);

    /*
     * Errors last.
     */
    if (ret != rec_type) {
	(void) smtpd_proxy_replay_rdwr_error(VSTREAM_TO_SMTPD_STATE(stream));
	return (REC_TYPE_ERROR);
    }
    return (rec_type);
}

/* smtpd_proxy_rec_fprintf - send message content, rec_fprintf() clone */

static int smtpd_proxy_rec_fprintf(VSTREAM *stream, int rec_type,
				           const char *fmt,...)
{
    const char *myname = "smtpd_proxy_rec_fprintf";
    va_list ap;
    int     err = 0;

    /*
     * Errors first.
     */
    if (vstream_ferror(stream) || vstream_feof(stream)
	|| (err = vstream_setjmp(stream)) != 0) {
	(void) smtpd_proxy_rdwr_error(VSTREAM_TO_SMTPD_STATE(stream), err);
	return (REC_TYPE_ERROR);
    }

    /*
     * Send one content record. Errors and results must be as with
     * rec_fprintf().
     */
    va_start(ap, fmt);
    if (rec_type == REC_TYPE_NORM)
	smtp_vprintf(stream, fmt, ap);
    else
	msg_panic("%s: need REC_TYPE_NORM", myname);
    va_end(ap);
    return (rec_type);
}

#ifndef NO_TRUNCATE

/* smtpd_proxy_replay_setup - prepare the replay logfile */

static int smtpd_proxy_replay_setup(SMTPD_STATE *state)
{
    const char *myname = "smtpd_proxy_replay_setup";
    off_t   file_offs;

    /*
     * Where possible reuse an existing replay logfile, because creating a
     * file is expensive compared to reading or writing. For security reasons
     * we must truncate the file before reuse. For performance reasons we
     * should truncate the file immediately after the end of a mail
     * transaction. We enforce the security guarantee upon reuse, by
     * requiring that no I/O happened since the file was truncated. This is
     * less expensive than truncating the file redundantly.
     */
    if (smtpd_proxy_replay_stream != 0) {
	/* vstream_ftell() won't invoke the kernel, so all errors are mine. */
	if ((file_offs = vstream_ftell(smtpd_proxy_replay_stream)) != 0)
	    msg_panic("%s: bad before-queue filter speed-adjust log offset %lu",
		      myname, (unsigned long) file_offs);
	vstream_clearerr(smtpd_proxy_replay_stream);
	if (msg_verbose)
	    msg_info("%s: reuse speed-adjust stream fd=%d", myname,
		     vstream_fileno(smtpd_proxy_replay_stream));
	/* Here, smtpd_proxy_replay_stream != 0 */
    }

    /*
     * Create a new replay logfile.
     */
    if (smtpd_proxy_replay_stream == 0) {
	smtpd_proxy_replay_stream = mail_queue_enter(MAIL_QUEUE_INCOMING, 0,
						     (struct timeval *) 0);
	if (smtpd_proxy_replay_stream == 0)
	    return (smtpd_proxy_replay_rdwr_error(state));
	if (unlink(VSTREAM_PATH(smtpd_proxy_replay_stream)) < 0)
	    msg_warn("remove before-queue filter speed-adjust log %s: %m",
		     VSTREAM_PATH(smtpd_proxy_replay_stream));
	if (msg_verbose)
	    msg_info("%s: new speed-adjust stream fd=%d", myname,
		     vstream_fileno(smtpd_proxy_replay_stream));
    }

    /*
     * Needed by our DATA-phase record emulation routines.
     */
    vstream_control(smtpd_proxy_replay_stream, VSTREAM_CTL_CONTEXT,
		    (char *) state, VSTREAM_CTL_END);
    return (0);
}

#endif

/* smtpd_proxy_create - set up smtpd proxy handle */

int     smtpd_proxy_create(SMTPD_STATE *state, int flags, const char *service,
			           int timeout, const char *ehlo_name,
			           const char *mail_from)
{
    SMTPD_PROXY *proxy;

    /*
     * When an operation has many arguments it is safer to use named
     * parameters, and have the compiler enforce the argument count.
     */
#define SMTPD_PROXY_ALLOC(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
	((p) = (SMTPD_PROXY *) mymalloc(sizeof(*(p))), (p)->a1, (p)->a2, \
	 (p)->a3, (p)->a4, (p)->a5, (p)->a6, (p)->a7, (p)->a8, (p)->a9, \
	 (p)->a10, (p)->a11, (p))

    /*
     * Sanity check.
     */
    if (state->proxy != 0)
	msg_panic("smtpd_proxy_create: handle still exists");

    /*
     * Connect to the before-queue filter immediately.
     */
    if ((flags & SMTPD_PROXY_FLAG_SPEED_ADJUST) == 0) {
	state->proxy =
	    SMTPD_PROXY_ALLOC(proxy, stream = 0, buffer = vstring_alloc(10),
			      cmd = smtpd_proxy_cmd,
			      rec_fprintf = smtpd_proxy_rec_fprintf,
			      rec_put = smtpd_proxy_rec_put,
			      flags = flags, service_stream = 0,
			      service_name = service, timeout = timeout,
			      ehlo_name = ehlo_name, mail_from = mail_from);
	if (smtpd_proxy_connect(state) < 0) {
	    /* NOT: smtpd_proxy_free(state); we still need proxy->buffer. */
	    return (-1);
	}
	proxy->stream = proxy->service_stream;
	return (0);
    }

    /*
     * Connect to the before-queue filter after we receive the entire
     * message. Open the replay logfile early to simplify code. The file is
     * reused for multiple mail transactions, so there is no need to minimize
     * its life time.
     */
    else {
#ifdef NO_TRUNCATE
	msg_panic("smtpd_proxy_create: speed-adjust support is not available");
#else
	if (smtpd_proxy_replay_setup(state) < 0)
	    return (-1);
	state->proxy =
	    SMTPD_PROXY_ALLOC(proxy, stream = smtpd_proxy_replay_stream,
			      buffer = vstring_alloc(10),
			      cmd = smtpd_proxy_save_cmd,
			      rec_fprintf = smtpd_proxy_save_rec_fprintf,
			      rec_put = smtpd_proxy_save_rec_put,
			      flags = flags, service_stream = 0,
			      service_name = service, timeout = timeout,
			      ehlo_name = ehlo_name, mail_from = mail_from);
	return (0);
#endif
    }
}

/* smtpd_proxy_close - close proxy connection without destroying handle */

void    smtpd_proxy_close(SMTPD_STATE *state)
{
    SMTPD_PROXY *proxy = state->proxy;

    /*
     * XXX We can't send QUIT if the stream is still good, because that would
     * overwrite the last server reply in proxy->buffer. We probably should
     * just bite the bullet and allocate separate buffers for sending and
     * receiving.
     */
    if (proxy->service_stream != 0) {
#if 0
	if (vstream_feof(proxy->service_stream) == 0
	    && vstream_ferror(proxy->service_stream) == 0)
	    (void) smtpd_proxy_cmd(state, SMTPD_PROX_WANT_NONE,
				   SMTPD_CMD_QUIT);
#endif
	(void) vstream_fclose(proxy->service_stream);
	if (proxy->stream == proxy->service_stream)
	    proxy->stream = 0;
	proxy->service_stream = 0;
    }
}

/* smtpd_proxy_free - destroy smtpd proxy handle */

void    smtpd_proxy_free(SMTPD_STATE *state)
{
    SMTPD_PROXY *proxy = state->proxy;

    /*
     * Clean up.
     */
    if (proxy->service_stream != 0)
	(void) smtpd_proxy_close(state);
    if (proxy->buffer != 0)
	vstring_free(proxy->buffer);
    myfree((char *) proxy);
    state->proxy = 0;

    /*
     * Reuse the replay logfile if possible. For security reasons we must
     * truncate the replay logfile before reuse. For performance reasons we
     * should truncate the replay logfile immediately after the end of a mail
     * transaction. We truncate the file here, and enforce the security
     * guarantee by requiring that no I/O happens before the file is reused.
     */
    if (smtpd_proxy_replay_stream == 0)
	return;
    if (vstream_ferror(smtpd_proxy_replay_stream)) {
	/* Errors are already reported. */
	(void) vstream_fclose(smtpd_proxy_replay_stream);
	smtpd_proxy_replay_stream = 0;
	return;
    }
    /* Flush output from aborted transaction before truncating the file!! */
    if (vstream_fseek(smtpd_proxy_replay_stream, (off_t) 0, SEEK_SET) < 0) {
	msg_warn("seek before-queue filter speed-adjust log: %m");
	(void) vstream_fclose(smtpd_proxy_replay_stream);
	smtpd_proxy_replay_stream = 0;
	return;
    }
    if (ftruncate(vstream_fileno(smtpd_proxy_replay_stream), (off_t) 0) < 0) {
	msg_warn("truncate before-queue filter speed-adjust log: %m");
	(void) vstream_fclose(smtpd_proxy_replay_stream);
	smtpd_proxy_replay_stream = 0;
	return;
    }
}

/* smtpd_proxy_parse_opts - parse main.cf options */

int     smtpd_proxy_parse_opts(const char *param_name, const char *param_val)
{
    static const NAME_MASK proxy_opts_table[] = {
	SMTPD_PROXY_NAME_SPEED_ADJUST, SMTPD_PROXY_FLAG_SPEED_ADJUST,
	0, 0,
    };
    int     flags;

    /*
     * The optional before-filter speed-adjust buffers use disk space.
     * However, we don't know if they compete for storage space with the
     * after-filter queue, so we can't simply bump up the free space
     * requirement to 2.5 * message_size_limit.
     */
    flags = name_mask(param_name, proxy_opts_table, param_val);
    if (flags & SMTPD_PROXY_FLAG_SPEED_ADJUST) {
#ifdef NO_TRUNCATE
	msg_warn("smtpd_proxy %s support is not available",
		 SMTPD_PROXY_NAME_SPEED_ADJUST);
	flags &= ~SMTPD_PROXY_FLAG_SPEED_ADJUST;
#endif
    }
    return (flags);
}
