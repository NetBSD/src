/*	$NetBSD: smtp-sink.c,v 1.1.1.6.2.2 2006/11/20 13:30:55 tron Exp $	*/

/*++
/* NAME
/*	smtp-sink 1
/* SUMMARY
/*	multi-threaded SMTP/LMTP test server
/* SYNOPSIS
/* .fi
/*	\fBsmtp-sink\fR [\fIoptions\fR] [\fBinet:\fR][\fIhost\fR]:\fIport\fR
/*	\fIbacklog\fR
/*
/*	\fBsmtp-sink\fR [\fIoptions\fR] \fBunix:\fR\fIpathname\fR \fIbacklog\fR
/* DESCRIPTION
/*	\fBsmtp-sink\fR listens on the named host (or address) and port.
/*	It takes SMTP messages from the network and throws them away.
/*	The purpose is to measure client performance, not protocol
/*	compliance.
/*
/*	Connections can be accepted on IPv4 or IPv6 endpoints, or on
/*	UNIX-domain sockets.
/*	IPv4 and IPv6 are the default.
/*	This program is the complement of the \fBsmtp-source\fR(1) program.
/*
/*	Arguments:
/* .IP \fB-4\fR
/*	Support IPv4 only. This option has no effect when
/*	Postfix is built without IPv6 support.
/* .IP \fB-6\fR
/*	Support IPv6 only. This option is not available when
/*	Postfix is built without IPv6 support.
/* .IP \fB-8\fR
/*	Do not announce 8BITMIME support.
/* .IP \fB-a\fR
/*	Do not announce SASL authentication support.
/* .IP \fB-c\fR
/*	Display running counters that are updated whenever an SMTP
/*	session ends, a QUIT command is executed, or when "." is
/*	received.
/* .IP \fB-C\fR
/*	Disable XCLIENT support.
/* .IP \fB-e\fR
/*	Do not announce ESMTP support.
/* .IP \fB-E\fR
/*	Do not announce ENHANCEDSTATUSCODES support.
/* .IP "\fB-f \fIcommand,command,...\fR"
/*	Reject the specified commands with a hard (5xx) error code.
/*	This option implies \fB-p\fR.
/* .sp
/*	Examples of commands are CONNECT, HELO, EHLO, LHLO, MAIL, RCPT, VRFY,
/*	DATA, ., RSET, NOOP, and QUIT. Separate command names by
/*	white space or commas, and use quotes to protect white space
/*	from the shell. Command names are case-insensitive.
/* .IP \fB-F\fR
/*	Disable XFORWARD support.
/* .IP "\fB-h\fI hostname\fR"
/*	Use \fIhostname\fR in the SMTP greeting, in the HELO response,
/*	and in the EHLO response. The default hostname is "smtp-sink".
/* .IP \fB-L\fR
/*	Enable LMTP instead of SMTP.
/* .IP "\fB-n \fIcount\fR"
/*	Terminate after \fIcount\fR sessions. This is for testing purposes.
/* .IP \fB-p\fR
/*	Do not announce support for ESMTP command pipelining.
/* .IP \fB-P\fR
/*	Change the server greeting so that it appears to come through
/*	a CISCO PIX system. Implies \fB-e\fR.
/* .IP "\fB-q \fIcommand,command,...\fR"
/*	Disconnect (without replying) after receiving one of the
/*	specified commands.
/* .sp
/*	Examples of commands are CONNECT, HELO, EHLO, LHLO, MAIL, RCPT, VRFY,
/*	DATA, ., RSET, NOOP, and QUIT. Separate command names by
/*	white space or commas, and use quotes to protect white space
/*	from the shell. Command names are case-insensitive.
/* .IP "\fB-r \fIcommand,command,...\fR"
/*	Reject the specified commands with a soft (4xx) error code.
/*	This option implies \fB-p\fR.
/* .sp
/*	Examples of commands are CONNECT, HELO, EHLO, LHLO, MAIL, RCPT, VRFY,
/*	DATA, ., RSET, NOOP, and QUIT. Separate command names by
/*	white space or commas, and use quotes to protect white space
/*	from the shell. Command names are case-insensitive.
/* .IP "\fB-s \fIcommand,command,...\fR"
/*	Log the named commands to syslogd.
/* .sp
/*	Examples of commands are CONNECT, HELO, EHLO, LHLO, MAIL, RCPT, VRFY,
/*	DATA, ., RSET, NOOP, and QUIT. Separate command names by
/*	white space or commas, and use quotes to protect white space
/*	from the shell. Command names are case-insensitive.
/* .IP "\fB-t \fItimeout\fR (default: 100)"
/*	Limit the time for receiving a command or sending a response.
/*	The time limit is specified in seconds.
/* .IP \fB-v\fR
/*	Show the SMTP conversations.
/* .IP "\fB-w \fIdelay\fR"
/*	Wait \fIdelay\fR seconds before responding to a DATA command.
/* .IP [\fBinet:\fR][\fIhost\fR]:\fIport\fR
/*	Listen on network interface \fIhost\fR (default: any interface)
/*	TCP port \fIport\fR. Both \fIhost\fR and \fIport\fR may be
/*	specified in numeric or symbolic form.
/* .IP \fBunix:\fR\fIpathname\fR
/*	Listen on the UNIX-domain socket at \fIpathname\fR.
/* .IP \fIbacklog\fR
/*	The maximum length the queue of pending connections,
/*	as defined by the \fBlisten\fR(2) system call.
/* SEE ALSO
/*	smtp-source(1), SMTP/LMTP message generator
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
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <get_hostname.h>
#include <listen.h>
#include <events.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <sane_accept.h>
#include <inet_proto.h>

/* Global library. */

#include <smtp_stream.h>

/* Application-specific. */

typedef struct SINK_STATE {
    VSTREAM *stream;
    VSTRING *buffer;
    int     data_state;
    int     (*read_fn) (struct SINK_STATE *);
    int     rcpts;
    char   *push_back_ptr;
} SINK_STATE;

#define ST_ANY			0
#define ST_CR			1
#define ST_CR_LF		2
#define ST_CR_LF_DOT		3
#define ST_CR_LF_DOT_CR		4
#define ST_CR_LF_DOT_CR_LF	5

#define PUSH_BACK_PEEK(state)		(*(state)->push_back_ptr != 0)
#define PUSH_BACK_GET(state)		(*(state)->push_back_ptr++)
#define PUSH_BACK_SET(state, text)	((state)->push_back_ptr = (text))

static int var_tmout = 100;
static int var_max_line_length = 2048;
static char *var_myhostname;
static int command_read(SINK_STATE *);
static int data_read(SINK_STATE *);
static void disconnect(SINK_STATE *);
static int count;
static int sess_count;
static int quit_count;
static int mesg_count;
static int max_quit_count;
static int disable_pipelining;
static int disable_8bitmime;
static int fixed_delay;
static int disable_esmtp;
static int enable_lmtp;
static int pretend_pix;
static int disable_saslauth;
static int disable_xclient;
static int disable_xforward;
static int disable_enh_status;

#define SOFT_ERROR_RESP		"450 4.3.0 Error: command failed"
#define HARD_ERROR_RESP		"500 5.3.0 Error: command failed"

/* do_stats - show counters */

static void do_stats(void)
{
    vstream_printf("sess=%d quit=%d mesg=%d\r",
		   sess_count, quit_count, mesg_count);
    vstream_fflush(VSTREAM_OUT);
}

/* hard_err_resp - generic hard error response */

static void hard_err_resp(SINK_STATE *state)
{
    smtp_printf(state->stream, HARD_ERROR_RESP);
    smtp_flush(state->stream);
}

/* soft_err_resp - generic soft error response */

static void soft_err_resp(SINK_STATE *state)
{
    smtp_printf(state->stream, SOFT_ERROR_RESP);
    smtp_flush(state->stream);
}

/* ehlo_response - respond to EHLO command */

static void ehlo_response(SINK_STATE *state)
{
    smtp_printf(state->stream, "250-%s", var_myhostname);
    if (!disable_pipelining)
	smtp_printf(state->stream, "250-PIPELINING");
    if (!disable_8bitmime)
	smtp_printf(state->stream, "250-8BITMIME");
    if (!disable_saslauth)
	smtp_printf(state->stream, "250-AUTH PLAIN LOGIN");
    if (!disable_xclient)
	smtp_printf(state->stream, "250-XCLIENT NAME HELO");
    if (!disable_xforward)
	smtp_printf(state->stream, "250-XFORWARD NAME ADDR PROTO HELO");
    if (!disable_enh_status)
	smtp_printf(state->stream, "250-ENHANCEDSTATUSCODES");
    smtp_printf(state->stream, "250 ");
    smtp_flush(state->stream);
}

/* helo_response - respond to HELO command */

static void helo_response(SINK_STATE *state)
{
    smtp_printf(state->stream, "250 %s", var_myhostname);
    smtp_flush(state->stream);
}

/* ok_response - send 250 OK */

static void ok_response(SINK_STATE *state)
{
    smtp_printf(state->stream, "250 2.0.0 Ok");
    smtp_flush(state->stream);
}

/* mail_response - reset recipient count, send 250 OK */

static void mail_response(SINK_STATE *state)
{
    state->rcpts = 0;
    smtp_printf(state->stream, "250 2.1.0 Ok");
    smtp_flush(state->stream);
}

/* rcpt_response - bump recipient count, send 250 OK */

static void rcpt_response(SINK_STATE *state)
{
    state->rcpts++;
    smtp_printf(state->stream, "250 2.1.5 Ok");
    smtp_flush(state->stream);
}

/* data_response - respond to DATA command */

static void data_response(SINK_STATE *state)
{
    /* Not: ST_ANY. */
    state->data_state = ST_CR_LF;
    smtp_printf(state->stream, "354 End data with <CR><LF>.<CR><LF>");
    smtp_flush(state->stream);
    state->read_fn = data_read;
}

/* data_event - delayed response to DATA command */

static void data_event(int unused_event, char *context)
{
    SINK_STATE *state = (SINK_STATE *) context;

    data_response(state);
}

/* dot_resp_hard - hard error response to . command */

static void dot_resp_hard(SINK_STATE *state)
{
    if (enable_lmtp) {
	while (state->rcpts-- > 0)	/* XXX this could block */
	    smtp_printf(state->stream, HARD_ERROR_RESP);
    } else {
	smtp_printf(state->stream, HARD_ERROR_RESP);
    }
    smtp_flush(state->stream);
}

/* dot_resp_soft - soft error response to . command */

static void dot_resp_soft(SINK_STATE *state)
{
    if (enable_lmtp) {
	while (state->rcpts-- > 0)	/* XXX this could block */
	    smtp_printf(state->stream, SOFT_ERROR_RESP);
    } else {
	smtp_printf(state->stream, SOFT_ERROR_RESP);
    }
    smtp_flush(state->stream);
}

/* dot_response - response to . command */

static void dot_response(SINK_STATE *state)
{
    if (enable_lmtp) {
	while (state->rcpts-- > 0)	/* XXX this could block */
	    smtp_printf(state->stream, "250 2.2.0 Ok");
    } else {
	smtp_printf(state->stream, "250 2.0.0 Ok");
    }
    smtp_flush(state->stream);
}

/* quit_response - respond to QUIT command */

static void quit_response(SINK_STATE *state)
{
    smtp_printf(state->stream, "221 Bye");
    smtp_flush(state->stream);
    if (count)
	quit_count++;
}

/* conn_response - respond to connect command */

static void conn_response(SINK_STATE *state)
{
    if (pretend_pix)
	smtp_printf(state->stream, "220 ********");
    else if (disable_esmtp)
	smtp_printf(state->stream, "220 %s", var_myhostname);
    else
	smtp_printf(state->stream, "220 %s ESMTP", var_myhostname);
    smtp_flush(state->stream);
}

/* data_read - read data from socket */

static int data_read(SINK_STATE *state)
{
    int     ch;
    struct data_trans {
	int     state;
	int     want;
	int     next_state;
    };
    static struct data_trans data_trans[] = {
	ST_ANY, '\r', ST_CR,
	ST_CR, '\n', ST_CR_LF,
	ST_CR_LF, '.', ST_CR_LF_DOT,
	ST_CR_LF_DOT, '\r', ST_CR_LF_DOT_CR,
	ST_CR_LF_DOT_CR, '\n', ST_CR_LF_DOT_CR_LF,
    };
    struct data_trans *dp;

    /*
     * A read may result in EOF, but is never supposed to time out - a time
     * out means that we were trying to read when no data was available.
     */
    for (;;) {
	if ((ch = VSTREAM_GETC(state->stream)) == VSTREAM_EOF)
	    return (-1);
	for (dp = data_trans; dp->state != state->data_state; dp++)
	     /* void */ ;

	/*
	 * Try to match the current character desired by the state machine.
	 * If that fails, try to restart the machine with a match for its
	 * first state.  This covers the case of a CR/LF/CR/LF sequence
	 * (empty line) right before the end of the message data.
	 */
	if (ch == dp->want)
	    state->data_state = dp->next_state;
	else if (ch == data_trans[0].want)
	    state->data_state = data_trans[0].next_state;
	else
	    state->data_state = ST_ANY;
	if (state->data_state == ST_CR_LF_DOT_CR_LF) {
	    PUSH_BACK_SET(state, ".\r\n");
	    state->read_fn = command_read;
	    state->data_state = ST_ANY;
	    if (count) {
		mesg_count++;
		do_stats();
	    }
	    break;
	}

	/*
	 * We must avoid blocking I/O, so get out of here as soon as both the
	 * VSTREAM and kernel read buffers dry up.
	 */
	if (vstream_peek(state->stream) <= 0
	    && readable(vstream_fileno(state->stream)) <= 0)
	    return (0);
    }
    return (0);
}

 /*
  * The table of all SMTP commands that we can handle.
  */
typedef struct SINK_COMMAND {
    const char *name;
    void    (*response) (SINK_STATE *);
    void    (*hard_response) (SINK_STATE *);
    void    (*soft_response) (SINK_STATE *);
    int     flags;
} SINK_COMMAND;

#define FLAG_ENABLE	(1<<0)		/* command is enabled */
#define FLAG_SYSLOG	(1<<1)		/* log the command */
#define FLAG_HARD_ERR	(1<<2)		/* report hard error */
#define FLAG_SOFT_ERR	(1<<3)		/* report soft error */
#define FLAG_DISCONNECT	(1<<4)		/* disconnect */

static SINK_COMMAND command_table[] = {
    "connect", conn_response, hard_err_resp, soft_err_resp, 0,
    "helo", helo_response, hard_err_resp, soft_err_resp, 0,
    "ehlo", ehlo_response, hard_err_resp, soft_err_resp, 0,
    "lhlo", ehlo_response, hard_err_resp, soft_err_resp, 0,
    "xclient", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "xforward", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "auth", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "mail", mail_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "rcpt", rcpt_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "data", data_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    ".", dot_response, dot_resp_hard, dot_resp_soft, FLAG_ENABLE,
    "rset", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "noop", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "vrfy", ok_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    "quit", quit_response, hard_err_resp, soft_err_resp, FLAG_ENABLE,
    0,
};

/* reset_cmd_flags - reset per-command command flags */

static void reset_cmd_flags(const char *cmd, int flags)
{
    SINK_COMMAND *cmdp;

    for (cmdp = command_table; cmdp->name != 0; cmdp++)
	if (strcasecmp(cmd, cmdp->name) == 0)
	    break;
    if (cmdp->name == 0)
	msg_fatal("unknown command: %s", cmd);
    cmdp->flags &= ~flags;
}

/* set_cmd_flags - set per-command command flags */

static void set_cmd_flags(const char *cmd, int flags)
{
    SINK_COMMAND *cmdp;

    for (cmdp = command_table; cmdp->name != 0; cmdp++)
	if (strcasecmp(cmd, cmdp->name) == 0)
	    break;
    if (cmdp->name == 0)
	msg_fatal("unknown command: %s", cmd);
    cmdp->flags |= flags;
}

/* set_cmds_flags - set per-command flags for multiple commands */

static void set_cmds_flags(const char *cmds, int flags)
{
    char   *saved_cmds;
    char   *cp;
    char   *cmd;

    saved_cmds = cp = mystrdup(cmds);
    while ((cmd = mystrtok(&cp, " \t\r\n,")) != 0)
	set_cmd_flags(cmd, flags);
    myfree(saved_cmds);
}

/* command_resp - respond to command */

static int command_resp(SINK_STATE *state, SINK_COMMAND *cmdp, const char *command, char *args)
{
    /* We use raw syslog. Sanitize data content and length. */
    if (cmdp->flags & FLAG_SYSLOG)
	syslog(LOG_INFO, "%s %.100s", command, printable(args, '?'));
    if (cmdp->flags & FLAG_DISCONNECT)
	return (-1);
    if (cmdp->flags & FLAG_HARD_ERR) {
	cmdp->hard_response(state);
	return (0);
    }
    if (cmdp->flags & FLAG_SOFT_ERR) {
	cmdp->soft_response(state);
	return (0);
    }
    if (cmdp->response == data_response && fixed_delay > 0) {
	event_request_timer(data_event, (char *) state, fixed_delay);
    } else {
	cmdp->response(state);
	if (cmdp->response == quit_response)
	    return (-1);
    }
    return (0);
}

/* command_read - talk the SMTP protocol, server side */

static int command_read(SINK_STATE *state)
{
    char   *command;
    SINK_COMMAND *cmdp;
    int     ch;
    struct cmd_trans {
	int     state;
	int     want;
	int     next_state;
    };
    static struct cmd_trans cmd_trans[] = {
	ST_ANY, '\r', ST_CR,
	ST_CR, '\n', ST_CR_LF,
    };
    struct cmd_trans *cp;
    char   *ptr;

    /*
     * A read may result in EOF, but is never supposed to time out - a time
     * out means that we were trying to read when no data was available.
     */
#define NEXT_CHAR(state) \
    (PUSH_BACK_PEEK(state) ? PUSH_BACK_GET(state) : VSTREAM_GETC(state->stream))

    for (;;) {
	if ((ch = NEXT_CHAR(state)) == VSTREAM_EOF)
	    return (-1);

	/*
	 * Sanity check. We don't want to store infinitely long commands.
	 */
	if (VSTRING_LEN(state->buffer) >= var_max_line_length) {
	    msg_warn("command line too long");
	    return (-1);
	}
	VSTRING_ADDCH(state->buffer, ch);

	/*
	 * Try to match the current character desired by the state machine.
	 * If that fails, try to restart the machine with a match for its
	 * first state.
	 */
	for (cp = cmd_trans; cp->state != state->data_state; cp++)
	     /* void */ ;
	if (ch == cp->want)
	    state->data_state = cp->next_state;
	else if (ch == cmd_trans[0].want)
	    state->data_state = cmd_trans[0].next_state;
	else
	    state->data_state = ST_ANY;
	if (state->data_state == ST_CR_LF)
	    break;

	/*
	 * We must avoid blocking I/O, so get out of here as soon as both the
	 * VSTREAM and kernel read buffers dry up.
	 * 
	 * XXX Solaris non-blocking read() may fail on a socket when ioctl
	 * FIONREAD reports there is unread data. Diagnosis by Max Pashkov.
	 * As a workaround we use readable() (which uses poll or select())
	 * instead of peek_fd() (which uses ioctl FIONREAD). Workaround added
	 * 20020604.
	 */
	if (PUSH_BACK_PEEK(state) == 0 && vstream_peek(state->stream) <= 0
	    && readable(vstream_fileno(state->stream)) <= 0)
	    return (0);
    }

    /*
     * Properly terminate the result, and reset the buffer write pointer for
     * reading the next command. This is ugly, but not as ugly as trying to
     * deal with all the early returns below.
     */
    vstring_truncate(state->buffer, VSTRING_LEN(state->buffer) - 2);
    VSTRING_TERMINATE(state->buffer);
    state->data_state = ST_ANY;
    VSTRING_RESET(state->buffer);

    /*
     * Got a complete command line. Parse it.
     */
    ptr = vstring_str(state->buffer);
    if (msg_verbose)
	msg_info("%s", ptr);
    if ((command = mystrtok(&ptr, " \t")) == 0) {
	smtp_printf(state->stream, "500 5.5.2 Error: unknown command");
	smtp_flush(state->stream);
	return (0);
    }
    for (cmdp = command_table; cmdp->name != 0; cmdp++)
	if (strcasecmp(command, cmdp->name) == 0)
	    break;
    if (cmdp->name == 0 || (cmdp->flags & FLAG_ENABLE) == 0) {
	smtp_printf(state->stream, "500 5.5.1 Error: unknown command");
	smtp_flush(state->stream);
	return (0);
    }
    return (command_resp(state, cmdp, command, ptr));
}

/* read_timeout - handle timer event */

static void read_timeout(int unused_event, char *context)
{
    SINK_STATE *state = (SINK_STATE *) context;

    /*
     * We don't send anything to the client, because we would have to set up
     * an smtp_stream exception handler first. And that is just too much
     * trouble.
     */
    msg_warn("read timeout");
    disconnect(state);
}

/* read_event - handle command or data read events */

static void read_event(int unused_event, char *context)
{
    SINK_STATE *state = (SINK_STATE *) context;

    /*
     * The input reading routine not only reads input (with vstream calls)
     * but also produces output (with smtp_stream calls). Because the output
     * routines can raise timeout or EOF exceptions with vstream_longjmp(),
     * the input reading routine needs to set up corresponding exception
     * handlers with vstream_setjmp(). Guarding the input operations in the
     * same manner is not useful: we must read input in non-blocking mode, so
     * we never get called when the socket stays unreadable too long. And EOF
     * is already trivial to detect with the vstream calls.
     */
    do {
	switch (vstream_setjmp(state->stream)) {

	default:
	    msg_panic("unknown read/write error");
	    /* NOTREACHED */

	case SMTP_ERR_TIME:
	    msg_warn("write timeout");
	    disconnect(state);
	    return;

	case SMTP_ERR_EOF:
	    msg_warn("lost connection");
	    disconnect(state);
	    return;

	case 0:
	    if (state->read_fn(state) < 0) {
		if (msg_verbose)
		    msg_info("disconnect");
		disconnect(state);
		return;
	    }
	}
    } while (PUSH_BACK_PEEK(state) != 0 || vstream_peek(state->stream) > 0);

    /*
     * Reset the idle timer. Wait until the next input event, or until the
     * idle timer goes off.
     */
    event_request_timer(read_timeout, (char *) state, var_tmout);
}

/* disconnect - handle disconnection events */

static void disconnect(SINK_STATE *state)
{
    event_disable_readwrite(vstream_fileno(state->stream));
    event_cancel_timer(read_timeout, (char *) state);
    if (count) {
	sess_count++;
	do_stats();
    }
    vstream_fclose(state->stream);
    vstring_free(state->buffer);
    myfree((char *) state);
    if (max_quit_count > 0 && quit_count >= max_quit_count)
	exit(0);
}

/* connect_event - handle connection events */

static void connect_event(int unused_event, char *context)
{
    int     sock = CAST_CHAR_PTR_TO_INT(context);
    struct sockaddr sa;
    SOCKADDR_SIZE len = sizeof(sa);
    SINK_STATE *state;
    int     fd;

    if ((fd = sane_accept(sock, &sa, &len)) >= 0) {
	if (msg_verbose)
	    msg_info("connect (%s)",
#ifdef AF_LOCAL
		     sa.sa_family == AF_LOCAL ? "AF_LOCAL" :
#else
		     sa.sa_family == AF_UNIX ? "AF_UNIX" :
#endif
		     sa.sa_family == AF_INET ? "AF_INET" :
#ifdef AF_INET6
		     sa.sa_family == AF_INET6 ? "AF_INET6" :
#endif
		     "unknown protocol family");
	non_blocking(fd, NON_BLOCKING);
	state = (SINK_STATE *) mymalloc(sizeof(*state));
	state->stream = vstream_fdopen(fd, O_RDWR);
	state->buffer = vstring_alloc(1024);
	state->read_fn = command_read;
	state->data_state = ST_ANY;
	PUSH_BACK_SET(state, "");
	smtp_timeout_setup(state->stream, var_tmout);

	/*
	 * We use the smtp_stream module to produce output. That module
	 * throws an exception via vstream_longjmp() in case of a timeout or
	 * lost connection error. Therefore we must prepare to handle these
	 * exceptions with vstream_setjmp().
	 */
	switch (vstream_setjmp(state->stream)) {

	default:
	    msg_panic("unknown read/write error");
	    /* NOTREACHED */

	case SMTP_ERR_TIME:
	    msg_warn("write timeout");
	    disconnect(state);
	    return;

	case SMTP_ERR_EOF:
	    msg_warn("lost connection");
	    disconnect(state);
	    return;

	case 0:
	    if (command_resp(state, command_table, "connect", "") < 0)
		disconnect(state);
	    else {
		event_enable_read(fd, read_event, (char *) state);
		event_request_timer(read_timeout, (char *) state, var_tmout);
	    }
	}
    }
}

/* usage - explain */

static void usage(char *myname)
{
    msg_fatal("usage: %s [-468acCeEFLpPv] [-f commands] [-h hostname] [-n count] [-q commands] [-r commands] [-s commands] [-w delay] [host]:port backlog", myname);
}

int     main(int argc, char **argv)
{
    int     sock;
    int     backlog;
    int     ch;
    const char *protocols = INET_PROTO_NAME_ALL;
    INET_PROTO_INFO *proto_info;

    /*
     * Fix 20051207.
     */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Initialize diagnostics.
     */
    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "468acCeEf:Fh:Ln:pPq:r:s:t:vw:")) > 0) {
	switch (ch) {
	case '4':
	    protocols = INET_PROTO_NAME_IPV4;
	    break;
	case '6':
	    protocols = INET_PROTO_NAME_IPV6;
	    break;
	case '8':
	    disable_8bitmime = 1;
	    break;
	case 'a':
	    disable_saslauth = 1;
	    break;
	case 'c':
	    count++;
	    break;
	case 'C':
	    disable_xclient = 1;
	    reset_cmd_flags("xclient", FLAG_ENABLE);
	    break;
	case 'e':
	    disable_esmtp = 1;
	    break;
	case 'E':
	    disable_enh_status = 1;
	    break;
	case 'f':
	    set_cmds_flags(optarg, FLAG_HARD_ERR);
	    disable_pipelining = 1;
	    break;
	case 'F':
	    disable_xforward = 1;
	    reset_cmd_flags("xforward", FLAG_ENABLE);
	    break;
	case 'h':
	    var_myhostname = optarg;
	    break;
	case 'L':
	    enable_lmtp = 1;
	    break;
	case 'n':
	    if ((max_quit_count = atoi(optarg)) <= 0)
		msg_fatal("bad count: %s", optarg);
	    break;
	case 'p':
	    disable_pipelining = 1;
	    break;
	case 'P':
	    pretend_pix = 1;
	    disable_esmtp = 1;
	    break;
	case 'q':
	    set_cmds_flags(optarg, FLAG_DISCONNECT);
	    break;
	case 'r':
	    set_cmds_flags(optarg, FLAG_SOFT_ERR);
	    disable_pipelining = 1;
	    break;
	case 's':
	    openlog(basename(argv[0]), LOG_PID, LOG_MAIL);
	    set_cmds_flags(optarg, FLAG_SYSLOG);
	    break;
	case 't':
	    if ((var_tmout = atoi(optarg)) <= 0)
		msg_fatal("bad timeout: %s", optarg);
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	case 'w':
	    if ((fixed_delay = atoi(optarg)) <= 0)
		usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc - optind != 2)
	usage(argv[0]);
    if ((backlog = atoi(argv[optind + 1])) <= 0)
	usage(argv[0]);

    /*
     * Initialize.
     */
    if (var_myhostname == 0)
	var_myhostname = "smtp-sink";
    set_cmds_flags(enable_lmtp ? "lhlo" :
		   disable_esmtp ? "helo" :
		   "helo, ehlo", FLAG_ENABLE);
    proto_info = inet_proto_init("protocols", protocols);
    if (strncmp(argv[optind], "unix:", 5) == 0) {
	sock = unix_listen(argv[optind] + 5, backlog, BLOCKING);
    } else {
	if (strncmp(argv[optind], "inet:", 5) == 0)
	    argv[optind] += 5;
	sock = inet_listen(argv[optind], backlog, BLOCKING);
    }

    /*
     * Start the event handler.
     */
    event_enable_read(sock, connect_event, CAST_INT_TO_CHAR_PTR(sock));
    for (;;)
	event_loop(-1);
}
