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
/*		/* other fields... */
/*		VSTREAM *proxy;		/* connection to SMTP proxy */
/*		VSTRING *proxy_buffer;	/* last SMTP proxy response */
/*		/* other fields... */
/* .in -4
/*	} SMTPD_STATE;
/*
/*	int	smtpd_proxy_open(state, service, timeout, ehlo_name, mail_from)
/*	SMTPD_STATE *state;
/*	const char *service;
/*	int	timeout;
/*	const char *ehlo_name;
/*	const char *mail_from;
/*
/*	int	smtpd_proxy_cmd(state, expect, format, ...)
/*	SMTPD_STATE *state;
/*	int	expect;
/*	cont char *format;
/*
/*	void	smtpd_proxy_close(state)
/*	SMTPD_STATE *state;
/* RECORD-LEVEL ROUTINES
/*	int	smtpd_proxy_rec_put(stream, rec_type, data, len)
/*	VSTREAM *stream;
/*	int	rec_type;
/*	const char *data;
/*	ssize_t	len;
/*
/*	int	smtpd_proxy_rec_fprintf(stream, rec_type, format, ...)
/*	VSTREAM *stream;
/*	int	rec_type;
/*	cont char *format;
/* DESCRIPTION
/*	The functions in this module implement a pass-through proxy
/*	client.
/*
/*	In order to minimize the intrusiveness of pass-through proxying, 1) the
/*	proxy server must support the same MAIL FROM/RCPT syntax that Postfix
/*	supports, 2) the record-level routines for message content proxying
/*	have the same interface as the routines that are used for non-proxied
/*	mail.
/*
/*	smtpd_proxy_open() connects to the proxy service, sends EHLO, sends
/*	client information with the XFORWARD command if possible, sends
/*	the MAIL FROM command, and receives the reply. A non-zero result means
/*	trouble: either the proxy is unavailable, or it did not send the
/*	expected reply.
/*	The result is reported via the state->proxy_buffer field in a form
/*	that can be sent to the SMTP client. In case of error, the
/*	state->error_mask and state->err fields are updated.
/*	A state->proxy_buffer field is created automatically; this field
/*	persists beyond the end of a proxy session.
/*
/*	smtpd_proxy_cmd() formats and sends the specified command to the
/*	proxy server, and receives the proxy server reply. A non-zero result
/*	means trouble: either the proxy is unavailable, or it did not send the
/*      expected reply.
/*	All results are reported via the state->proxy_buffer field in a form
/*	that can be sent to the SMTP client. In case of error, the
/*	state->error_mask and state->err fields are updated.
/*
/*	smtpd_proxy_close() disconnects from a proxy server and resets
/*	the state->proxy field. The last proxy server reply or error
/*	description remains available via state->proxy-reply.
/*
/*	smtpd_proxy_rec_put() is a rec_put() clone that passes arbitrary
/*	message content records to the proxy server. The data is expected
/*	to be in SMTP dot-escaped form. All errors are reported as a
/*	REC_TYPE_ERROR result value.
/*
/*	smtpd_proxy_rec_fprintf() is a rec_fprintf() clone that formats
/*	message content and sends it to the proxy server. Leading dots are
/*	not escaped. All errors are reported as a REC_TYPE_ERROR result
/*	value.
/*
/* Arguments:
/* .IP server
/*	The SMTP proxy server host:port. The host or host: part is optional.
/* .IP timeout
/*	Time limit for connecting to the proxy server and for
/*	sending and receiving proxy server commands and replies.
/* .IP ehlo_name
/*	The EHLO Hostname that will be sent to the proxy server.
/* .IP mail_from
/*	The MAIL FROM command.
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
/*	Fatal errors: memory allocation problem.
/*
/*	Warnings: unexpected response from proxy server, unable
/*	to connect to proxy server, proxy server read/write error.
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

/* Global library. */

#include <mail_error.h>
#include <smtp_stream.h>
#include <cleanup_user.h>
#include <mail_params.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <mail_params.h>		/* null_format_string */
#include <xtext.h>

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
  * SLMs.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)
#define SMTPD_PROXY_CONNECT null_format_string
#define STREQ(x, y)	(strcmp((x), (y)) == 0)

/* smtpd_xforward_flush - flush forwarding information */

static int smtpd_xforward_flush(SMTPD_STATE *state, VSTRING *buf)
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

/* smtpd_xforward - send forwarding information */

static int smtpd_xforward(SMTPD_STATE *state, VSTRING *buf, const char *name,
			          int value_available, const char *value)
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
	if ((ret = smtpd_xforward_flush(state, buf)) < 0)
	    return (ret);
    vstring_sprintf_append(buf, " %s=%s", name, STR(state->expand_buf));

    return (0);
}

/* smtpd_proxy_open - open proxy connection */

int     smtpd_proxy_open(SMTPD_STATE *state, const char *service,
			         int timeout, const char *ehlo_name,
			         const char *mail_from)
{
    int     fd;
    char   *lines;
    char   *words;
    VSTRING *buf;
    int     bad;
    char   *word;
    static const NAME_CODE xforward_features[] = {
	XFORWARD_NAME, SMTPD_PROXY_XFORWARD_NAME,
	XFORWARD_ADDR, SMTPD_PROXY_XFORWARD_ADDR,
	XFORWARD_PORT, SMTPD_PROXY_XFORWARD_PORT,
	XFORWARD_PROTO, SMTPD_PROXY_XFORWARD_PROTO,
	XFORWARD_HELO, SMTPD_PROXY_XFORWARD_HELO,
	XFORWARD_DOMAIN, SMTPD_PROXY_XFORWARD_DOMAIN,
	0, 0,
    };
    const CLEANUP_STAT_DETAIL *detail;
    int     (*connect_fn) (const char *, int, int);
    const char *endpoint;

    /*
     * This buffer persists beyond the end of a proxy session so we can
     * inspect the last command's reply.
     */
    if (state->proxy_buffer == 0)
	state->proxy_buffer = vstring_alloc(10);

    /*
     * Find connection method (default inet)
     */
    if (strncasecmp("unix:", service, 5) == 0) {
	endpoint = service + 5;
	connect_fn = unix_connect;
    } else {
	if (strncasecmp("inet:", service, 5) == 0)
	    endpoint = service + 5;
	else
	    endpoint = service;
	connect_fn = inet_connect;
    }

    /*
     * Connect to proxy.
     */
    if ((fd = connect_fn(endpoint, BLOCKING, timeout)) < 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	state->err |= CLEANUP_STAT_PROXY;
	msg_warn("connect to proxy service %s: %m", service);
	detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	vstring_sprintf(state->proxy_buffer,
			"%d %s Error: %s",
			detail->smtp, detail->dsn, detail->text);
	return (-1);
    }
    state->proxy = vstream_fdopen(fd, O_RDWR);
    vstream_control(state->proxy, VSTREAM_CTL_PATH, service, VSTREAM_CTL_END);
    smtp_timeout_setup(state->proxy, timeout);

    /*
     * Get server greeting banner.
     * 
     * If this fails then we have a problem because the proxy should always
     * accept our connection. Make up our own response instead of passing
     * back the greeting banner: the proxy open might be delayed to the point
     * that the client expects a MAIL FROM or RCPT TO reply.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, SMTPD_PROXY_CONNECT) != 0) {
	detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	vstring_sprintf(state->proxy_buffer,
			"%d %s Error: %s",
			detail->smtp, detail->dsn, detail->text);
	smtpd_proxy_close(state);
	return (-1);
    }

    /*
     * Send our own EHLO command. If this fails then we have a problem
     * because the proxy should always accept our EHLO command. Make up our
     * own response instead of passing back the EHLO reply: the proxy open
     * might be delayed to the point that the client expects a MAIL FROM or
     * RCPT TO reply.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, "EHLO %s", ehlo_name) != 0) {
	detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	vstring_sprintf(state->proxy_buffer,
			"%d %s Error: %s",
			detail->smtp, detail->dsn, detail->text);
	smtpd_proxy_close(state);
	return (-1);
    }

    /*
     * Parse the EHLO reply and see if we can forward logging information.
     */
    state->proxy_xforward_features = 0;
    lines = STR(state->proxy_buffer);
    while ((words = mystrtok(&lines, "\n")) != 0) {
	if (mystrtok(&words, "- ") && (word = mystrtok(&words, " \t")) != 0) {
	    if (strcasecmp(word, XFORWARD_CMD) == 0)
		while ((word = mystrtok(&words, " \t")) != 0)
		    state->proxy_xforward_features |=
			name_code(xforward_features, NAME_CODE_FLAG_NONE, word);
	}
    }

    /*
     * Send XFORWARD attributes. For robustness, explicitly specify what SMTP
     * session attributes are known and unknown.
     */
    if (state->proxy_xforward_features) {
	buf = vstring_alloc(100);
	bad = 0;
	if (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_NAME)
	    bad = smtpd_xforward(state, buf, XFORWARD_NAME,
				 IS_AVAIL_CLIENT_NAME(FORWARD_NAME(state)),
				 FORWARD_NAME(state));
	if (bad == 0
	    && (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_ADDR))
	    bad = smtpd_xforward(state, buf, XFORWARD_ADDR,
				 IS_AVAIL_CLIENT_ADDR(FORWARD_ADDR(state)),
				 FORWARD_ADDR(state));
	if (bad == 0
	    && (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_PORT))
	    bad = smtpd_xforward(state, buf, XFORWARD_PORT,
				 IS_AVAIL_CLIENT_PORT(FORWARD_PORT(state)),
				 FORWARD_PORT(state));
	if (bad == 0
	    && (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_HELO))
	    bad = smtpd_xforward(state, buf, XFORWARD_HELO,
				 IS_AVAIL_CLIENT_HELO(FORWARD_HELO(state)),
				 FORWARD_HELO(state));
	if (bad == 0
	    && (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_PROTO))
	    bad = smtpd_xforward(state, buf, XFORWARD_PROTO,
				 IS_AVAIL_CLIENT_PROTO(FORWARD_PROTO(state)),
				 FORWARD_PROTO(state));
	if (bad == 0
	  && (state->proxy_xforward_features & SMTPD_PROXY_XFORWARD_DOMAIN))
	    bad = smtpd_xforward(state, buf, XFORWARD_DOMAIN, 1,
			 STREQ(FORWARD_DOMAIN(state), MAIL_ATTR_RWR_LOCAL) ?
				 XFORWARD_DOM_LOCAL : XFORWARD_DOM_REMOTE);
	if (bad == 0)
	    bad = smtpd_xforward_flush(state, buf);
	vstring_free(buf);
	if (bad) {
	    detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	    vstring_sprintf(state->proxy_buffer,
			    "%d %s Error: %s",
			    detail->smtp, detail->dsn, detail->text);
	    smtpd_proxy_close(state);
	    return (-1);
	}
    }

    /*
     * Pass-through the client's MAIL FROM command. If this fails, then we
     * have a problem because the proxy should always accept any MAIL FROM
     * command that was accepted by us.
     */
    if (smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK, "%s", mail_from) != 0) {
	smtpd_proxy_close(state);
	return (-1);
    }
    return (0);
}

/* smtpd_proxy_rdwr_error - report proxy communication error */

static int smtpd_proxy_rdwr_error(VSTREAM *stream, int err)
{
    switch (err) {
	case SMTP_ERR_EOF:
	msg_warn("lost connection with proxy %s", VSTREAM_PATH(stream));
	return (err);
    case SMTP_ERR_TIME:
	msg_warn("timeout talking to proxy %s", VSTREAM_PATH(stream));
	return (err);
    default:
	msg_panic("smtpd_proxy_rdwr_error: unknown proxy %s stream error %d",
		  VSTREAM_PATH(stream), err);
    }
}

/* smtpd_proxy_cmd_error - report unexpected proxy reply */

static void smtpd_proxy_cmd_error(SMTPD_STATE *state, const char *fmt,
				          va_list ap)
{
    VSTRING *buf;

    /*
     * The command can be omitted at the start of an SMTP session. A null
     * format string is not documented as part of the official interface
     * because it is used only internally to this module.
     */
    buf = vstring_alloc(100);
    vstring_vsprintf(buf, fmt == SMTPD_PROXY_CONNECT ?
		     "connection request" : fmt, ap);
    msg_warn("proxy %s rejected \"%s\": \"%s\"", VSTREAM_PATH(state->proxy),
	     STR(buf), STR(state->proxy_buffer));
    vstring_free(buf);
}

/* smtpd_proxy_cmd - send command to proxy, receive reply */

int     smtpd_proxy_cmd(SMTPD_STATE *state, int expect, const char *fmt,...)
{
    va_list ap;
    char   *cp;
    int     last_char;
    int     err = 0;
    static VSTRING *buffer = 0;
    const CLEANUP_STAT_DETAIL *detail;

    /*
     * Errors first. Be prepared for delayed errors from the DATA phase.
     */
    if (vstream_ferror(state->proxy)
	|| vstream_feof(state->proxy)
	|| ((err = vstream_setjmp(state->proxy)) != 0
	    && smtpd_proxy_rdwr_error(state->proxy, err))) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	state->err |= CLEANUP_STAT_PROXY;
	detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	vstring_sprintf(state->proxy_buffer,
			"%d %s Error: %s",
			detail->smtp, detail->dsn, detail->text);
	return (-1);
    }

    /*
     * The command can be omitted at the start of an SMTP session. This is
     * not documented as part of the official interface because it is used
     * only internally to this module.
     */
    if (fmt != SMTPD_PROXY_CONNECT) {

	/*
	 * Format the command.
	 */
	va_start(ap, fmt);
	vstring_vsprintf(state->proxy_buffer, fmt, ap);
	va_end(ap);

	/*
	 * Optionally log the command first, so that we can see in the log
	 * what the program is trying to do.
	 */
	if (msg_verbose)
	    msg_info("> %s: %s", VSTREAM_PATH(state->proxy),
		     STR(state->proxy_buffer));

	/*
	 * Send the command to the proxy server. Since we're going to read a
	 * reply immediately, there is no need to flush buffers.
	 */
	smtp_fputs(STR(state->proxy_buffer), LEN(state->proxy_buffer),
		   state->proxy);
    }

    /*
     * Early return if we don't want to wait for a server reply (such as
     * after sending QUIT.
     */
    if (expect == SMTPD_PROX_WANT_NONE)
	return (0);

    /*
     * Censor out non-printable characters in server responses and save
     * complete multi-line responses if possible.
     */
    VSTRING_RESET(state->proxy_buffer);
    if (buffer == 0)
	buffer = vstring_alloc(10);
    for (;;) {
	last_char = smtp_get(buffer, state->proxy, var_line_limit);
	printable(STR(buffer), '?');
	if (last_char != '\n')
	    msg_warn("%s: response longer than %d: %.30s...",
		     VSTREAM_PATH(state->proxy), var_line_limit,
		     STR(buffer));
	if (msg_verbose)
	    msg_info("< %s: %.100s", VSTREAM_PATH(state->proxy),
		     STR(buffer));

	/*
	 * Defend against a denial of service attack by limiting the amount
	 * of multi-line text that we are willing to store.
	 */
	if (LEN(state->proxy_buffer) < var_line_limit) {
	    if (VSTRING_LEN(state->proxy_buffer))
		VSTRING_ADDCH(state->proxy_buffer, '\n');
	    vstring_strcat(state->proxy_buffer, STR(buffer));
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
		 VSTREAM_PATH(state->proxy), STR(buffer));
    }

    /*
     * Log a warning in case the proxy does not send the expected response.
     * Silently accept any response when the client expressed no expectation.
     * 
     * Don't pass through misleading 2xx replies. it confuses naive users and
     * SMTP clients, and creates support problems.
     */
    if (expect != SMTPD_PROX_WANT_ANY && expect != *STR(state->proxy_buffer)) {
	va_start(ap, fmt);
	smtpd_proxy_cmd_error(state, fmt, ap);
	va_end(ap);
	if (*STR(state->proxy_buffer) == SMTPD_PROX_WANT_OK
	    || *STR(state->proxy_buffer) == SMTPD_PROX_WANT_MORE) {
	    state->error_mask |= MAIL_ERROR_SOFTWARE;
	    state->err |= CLEANUP_STAT_PROXY;
	    detail = cleanup_stat_detail(CLEANUP_STAT_PROXY);
	    vstring_sprintf(state->proxy_buffer,
			    "%d %s Error: %s",
			    detail->smtp, detail->dsn, detail->text);
	}
	return (-1);
    } else {
	return (0);
    }
}

/* smtpd_proxy_rec_put - send message content, rec_put() clone */

int     smtpd_proxy_rec_put(VSTREAM *stream, int rec_type,
			            const char *data, ssize_t len)
{
    int     err;

    /*
     * Errors first.
     */
    if (vstream_ferror(stream) || vstream_feof(stream))
	return (REC_TYPE_ERROR);
    if ((err = vstream_setjmp(stream)) != 0)
	return (smtpd_proxy_rdwr_error(stream, err), REC_TYPE_ERROR);

    /*
     * Send one content record. Errors and results must be as with rec_put().
     */
    if (rec_type == REC_TYPE_NORM)
	smtp_fputs(data, len, stream);
    else if (rec_type == REC_TYPE_CONT)
	smtp_fwrite(data, len, stream);
    else
	msg_panic("smtpd_proxy_rec_put: need REC_TYPE_NORM or REC_TYPE_CONT");
    return (rec_type);
}

/* smtpd_proxy_rec_fprintf - send message content, rec_fprintf() clone */

int     smtpd_proxy_rec_fprintf(VSTREAM *stream, int rec_type,
				        const char *fmt,...)
{
    va_list ap;
    int     err;

    /*
     * Errors first.
     */
    if (vstream_ferror(stream) || vstream_feof(stream))
	return (REC_TYPE_ERROR);
    if ((err = vstream_setjmp(stream)) != 0)
	return (smtpd_proxy_rdwr_error(stream, err), REC_TYPE_ERROR);

    /*
     * Send one content record. Errors and results must be as with
     * rec_fprintf().
     */
    va_start(ap, fmt);
    if (rec_type == REC_TYPE_NORM)
	smtp_vprintf(stream, fmt, ap);
    else
	msg_panic("smtpd_proxy_rec_fprintf: need REC_TYPE_NORM");
    va_end(ap);
    return (rec_type);
}

/* smtpd_proxy_close - close proxy connection */

void    smtpd_proxy_close(SMTPD_STATE *state)
{
    (void) vstream_fclose(state->proxy);
    state->proxy = 0;
}
