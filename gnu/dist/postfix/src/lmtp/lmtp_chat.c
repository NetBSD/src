/*++
/* NAME
/*	lmtp_chat 3
/* SUMMARY
/*	LMTP client request/response support
/* SYNOPSIS
/*	#include "lmtp.h"
/*
/*	typedef struct {
/* .in +4
/*		int code;
/*		char *str;
/*		VSTRING *buf;
/* .in -4
/*	} LMTP_RESP;
/*
/*	void	lmtp_chat_cmd(state, format, ...)
/*	LMTP_STATE *state;
/*	char	*format;
/*
/*	LMTP_RESP *lmtp_chat_resp(state)
/*	LMTP_STATE *state;
/*
/*	void	lmtp_chat_notify(state)
/*	LMTP_STATE *state;
/*
/*	void	lmtp_chat_reset(state)
/*	LMTP_STATE *state;
/* DESCRIPTION
/*	This module implements LMTP client support for request/reply
/*	conversations, and maintains a limited LMTP transaction log.
/*
/*	lmtp_chat_cmd() formats a command and sends it to an LMTP server.
/*	Optionally, the command is logged.
/*
/*	lmtp_chat_resp() read one LMTP server response. It separates the
/*	numerical status code from the text, and concatenates multi-line
/*	responses to one string, using a newline as separator.
/*	Optionally, the server response is logged.
/*
/*	lmtp_chat_notify() sends a copy of the LMTP transaction log
/*	to the postmaster for review. The postmaster notice is sent only
/*	when delivery is possible immediately. It is an error to call
/*	lmtp_chat_notify() when no LMTP transaction log exists.
/*
/*	lmtp_chat_reset() resets the transaction log. This is
/*	typically done at the beginning or end of an LMTP session,
/*	or within a session to discard non-error information.
/*	In addition, lmtp_chat_reset() resets the per-session error
/*	status bits and flags.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem, server response exceeds
/*	configurable limit.
/*	All other exceptions are handled by long jumps (see smtp_stream(3)).
/* SEE ALSO
/*	smtp_stream(3) LMTP session I/O support
/*	msg(3) generic logging interface
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Alterations for LMTP by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Additional work on LMTP by:
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <stringops.h>
#include <line_wrap.h>
#include <mymalloc.h>

/* Global library. */

#include <recipient_list.h>
#include <deliver_request.h>
#include <smtp_stream.h>
#include <mail_params.h>
#include <mail_addr.h>
#include <post_mail.h>
#include <mail_error.h>

/* Application-specific. */

#include "lmtp.h"

#define STR(x)	((char *) vstring_str(x))
#define LEN	VSTRING_LEN

/* lmtp_chat_reset - reset LMTP transaction log */

void    lmtp_chat_reset(LMTP_STATE *state)
{
    if (state->history) {
	argv_free(state->history);
	state->history = 0;
    }
    /* What's status without history? */
    state->status = 0;
    state->error_mask = 0;
}

/* lmtp_chat_append - append record to LMTP transaction log */

static void lmtp_chat_append(LMTP_STATE *state, char *direction, char *data)
{
    char   *line;

    if (state->history == 0)
	state->history = argv_alloc(10);
    line = concatenate(direction, data, (char *) 0);
    argv_add(state->history, line, (char *) 0);
    myfree(line);
}

/* lmtp_chat_cmd - send an LMTP command */

void    lmtp_chat_cmd(LMTP_STATE *state, char *fmt,...)
{
    LMTP_SESSION *session = state->session;
    va_list ap;

    /*
     * Format the command, and update the transaction log.
     */
    va_start(ap, fmt);
    vstring_vsprintf(state->buffer, fmt, ap);
    va_end(ap);
    lmtp_chat_append(state, "Out: ", STR(state->buffer));

    /*
     * Optionally log the command first, so we can see in the log what the
     * program is trying to do.
     */
    if (msg_verbose)
	msg_info("> %s: %s", session->namaddr, STR(state->buffer));

    /*
     * Send the command to the LMTP server.
     */
    smtp_fputs(STR(state->buffer), LEN(state->buffer), session->stream);
}

/* lmtp_chat_resp - read and process LMTP server response */

LMTP_RESP *lmtp_chat_resp(LMTP_STATE *state)
{
    LMTP_SESSION *session = state->session;
    static LMTP_RESP rdata;
    char   *cp;
    int     last_char;

    /*
     * Initialize the response data buffer.
     */
    if (rdata.buf == 0)
	rdata.buf = vstring_alloc(100);

    /*
     * Censor out non-printable characters in server responses. Concatenate
     * multi-line server responses. Separate the status code from the text.
     * Leave further parsing up to the application.
     */
    VSTRING_RESET(rdata.buf);
    for (;;) {
	last_char = smtp_get(state->buffer, session->stream, var_line_limit);
	printable(STR(state->buffer), '?');
	if (last_char != '\n')
	    msg_warn("%s: response longer than %d: %.30s...",
		     session->namaddr, var_line_limit, STR(state->buffer));
	if (msg_verbose)
	    msg_info("< %s: %s", session->namaddr, STR(state->buffer));

	/*
	 * Defend against a denial of service attack by limiting the amount
	 * of multi-line text that we are willing to store.
	 */
	if (LEN(rdata.buf) < var_line_limit) {
	    if (VSTRING_LEN(rdata.buf))
		VSTRING_ADDCH(rdata.buf, '\n');
	    vstring_strcat(rdata.buf, STR(state->buffer));
	    lmtp_chat_append(state, "In:  ", STR(state->buffer));
	}

	/*
	 * Parse into code and text. Ignore unrecognized garbage. This means
	 * that any character except space (or end of line) will have the
	 * same effect as the '-' line continuation character.
	 */
	for (cp = STR(state->buffer); *cp && ISDIGIT(*cp); cp++)
	     /* void */ ;
	if (cp - STR(state->buffer) == 3) {
	    if (*cp == '-')
		continue;
	    if (*cp == ' ' || *cp == 0)
		break;
	}
	state->error_mask |= MAIL_ERROR_PROTOCOL;
    }
    rdata.code = atoi(STR(state->buffer));
    VSTRING_TERMINATE(rdata.buf);
    rdata.str = STR(rdata.buf);
    return (&rdata);
}

/* print_line - line_wrap callback */

static void print_line(const char *str, int len, int indent, char *context)
{
    VSTREAM *notice = (VSTREAM *) context;

    post_mail_fprintf(notice, " %*s%.*s", indent, "", len, str);
}

/* lmtp_chat_notify - notify postmaster */

void    lmtp_chat_notify(LMTP_STATE *state)
{
    char   *myname = "lmtp_chat_notify";
    LMTP_SESSION *session = state->session;
    VSTREAM *notice;
    char  **cpp;

    /*
     * Sanity checks.
     */
    if (state->history == 0)
	msg_panic("%s: no conversation history", myname);
    if (msg_verbose)
	msg_info("%s: notify postmaster", myname);

    /*
     * Construct a message for the postmaster, explaining what this is all
     * about. This is junk mail: don't send it when the mail posting service
     * is unavailable, and use the double bounce sender address, to prevent
     * mail bounce wars. Always prepend one space to message content that we
     * generate from untrusted data.
     */
#define NULL_CLEANUP_FLAGS	0
#define LENGTH	78
#define INDENT	4

    notice = post_mail_fopen_nowait(mail_addr_double_bounce(),
				    var_error_rcpt,
				    NULL_CLEANUP_FLAGS, "NOTICE");
    if (notice == 0) {
	msg_warn("postmaster notify: %m");
	return;
    }
    post_mail_fprintf(notice, "From: %s (Mail Delivery System)",
		      mail_addr_mail_daemon());
    post_mail_fprintf(notice, "To: %s (Postmaster)", var_error_rcpt);
    post_mail_fprintf(notice, "Subject: %s LMTP client: errors from %s",
		      var_mail_name, session->namaddr);
    post_mail_fputs(notice, "");
    post_mail_fprintf(notice, "Unexpected response from %s.", session->namaddr);
    post_mail_fputs(notice, "");
    post_mail_fputs(notice, "Transcript of session follows.");
    post_mail_fputs(notice, "");
    argv_terminate(state->history);
    for (cpp = state->history->argv; *cpp; cpp++)
	line_wrap(printable(*cpp, '?'), LENGTH, INDENT, print_line,
		  (char *) notice);
    (void) post_mail_fclose(notice);
}
