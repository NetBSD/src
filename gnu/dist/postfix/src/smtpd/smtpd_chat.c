/*++
/* NAME
/*	smtpd_chat 3
/* SUMMARY
/*	SMTP server request/response support
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_chat.h>
/*
/*	void	smtpd_chat_query(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_chat_reply(state, format, ...)
/*	SMTPD_STATE *state;
/*	char	*format;
/*
/*	void	smtpd_chat_notify(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_chat_reset(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	This module implements SMTP server support for request/reply
/*	conversations, and maintains a limited SMTP transaction log.
/*
/*	smtpd_chat_query() receives a client request and appends a copy
/*	to the SMTP transaction log.
/*
/*	smtpd_chat_reply() formats a server reply, sends it to the
/*	client, and appends a copy to the SMTP transaction log.
/*	When soft_bounce is enabled, all 5xx (reject) reponses are
/*	replaced by 4xx (try again).
/*
/*	smtpd_chat_notify() sends a copy of the SMTP transaction log
/*	to the postmaster for review. The postmaster notice is sent only
/*	when delivery is possible immediately. It is an error to call
/*	smtpd_chat_notify() when no SMTP transaction log exists.
/*
/*	smtpd_chat_reset() resets the transaction log. This is
/*	typically done at the beginning of an SMTP session, or
/*	within a session to discard non-error information.
/* DIAGNOSTICS
/*	Panic: interface violations. Fatal errors: out of memory.
/*	internal protocol errors.
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
#include <setjmp.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <line_wrap.h>
#include <mymalloc.h>

/* Global library. */

#include <smtp_stream.h>
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <mail_addr.h>
#include <post_mail.h>
#include <mail_error.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_chat.h"

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* smtp_chat_reset - reset SMTP transaction log */

void    smtpd_chat_reset(SMTPD_STATE *state)
{
    if (state->history) {
	argv_free(state->history);
	state->history = 0;
    }
}

/* smtp_chat_append - append record to SMTP transaction log */

static void smtp_chat_append(SMTPD_STATE *state, char *direction)
{
    char   *line;

    if (state->notify_mask == 0)
	return;

    if (state->history == 0)
	state->history = argv_alloc(10);
    line = concatenate(direction, STR(state->buffer), (char *) 0);
    argv_add(state->history, line, (char *) 0);
    myfree(line);
}

/* smtpd_chat_query - receive and record an SMTP request */

void    smtpd_chat_query(SMTPD_STATE *state)
{
    int     last_char;

    last_char = smtp_get(state->buffer, state->client, var_line_limit);
    smtp_chat_append(state, "In:  ");
    if (last_char != '\n')
	msg_warn("%s[%s]: request longer than %d: %.30s...",
		 state->name, state->addr, var_line_limit,
		 printable(STR(state->buffer), '?'));

    if (msg_verbose)
	msg_info("< %s[%s]: %s", state->name, state->addr, STR(state->buffer));
}

/* smtpd_chat_reply - format, send and record an SMTP response */

void    smtpd_chat_reply(SMTPD_STATE *state, char *format,...)
{
    va_list ap;

    va_start(ap, format);
    vstring_vsprintf(state->buffer, format, ap);
    va_end(ap);
    if (var_soft_bounce && STR(state->buffer)[0] == '5')
	STR(state->buffer)[0] = '4';
    smtp_chat_append(state, "Out: ");

    if (msg_verbose)
	msg_info("> %s[%s]: %s", state->name, state->addr, STR(state->buffer));

    /*
     * Slow down clients that make errors. Sleep-on-error slows down clients
     * that abort the connection and go into a connect-error-disconnect loop;
     * sleep-on-anything slows down clients that make an excessive number of
     * errors within a session.
     */
    if (state->error_count > var_smtpd_soft_erlim)
	sleep(state->error_count);
    else if (STR(state->buffer)[0] == '4' || STR(state->buffer)[0] == '5')
	sleep(var_smtpd_err_sleep);

    smtp_fputs(STR(state->buffer), LEN(state->buffer), state->client);

    /*
     * Flush unsent output if no I/O happened for a while. This avoids
     * timeouts with pipelined SMTP sessions that have lots of server-side
     * delays (tarpit delays or DNS lookups for UCE restrictions).
     */
    if (time((time_t *) 0) - vstream_ftime(state->client) > 10)
	vstream_fflush(state->client);
}

/* print_line - line_wrap callback */

static void print_line(const char *str, int len, int indent, char *context)
{
    VSTREAM *notice = (VSTREAM *) context;

    post_mail_fprintf(notice, " %*s%.*s", indent, "", len, str);
}

/* smtpd_chat_notify - notify postmaster */

void    smtpd_chat_notify(SMTPD_STATE *state)
{
    char   *myname = "smtpd_chat_notify";
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
     * is unavailable, and use the double bounce sender address to prevent
     * mail bounce wars. Always prepend one space to message content that we
     * generate from untrusted data.
     */
#define NULL_CLEANUP_FLAGS	0
#define LENGTH	78
#define INDENT	4

    notice = post_mail_fopen_nowait(mail_addr_double_bounce(),
				    var_error_rcpt,
				    NULL_CLEANUP_FLAGS);
    if (notice == 0) {
	msg_warn("postmaster notify: %m");
	return;
    }
    post_mail_fprintf(notice, "From: %s (Mail Delivery System)",
		      mail_addr_mail_daemon());
    post_mail_fprintf(notice, "To: %s (Postmaster)", var_error_rcpt);
    post_mail_fprintf(notice, "Subject: %s SMTP server: errors from %s[%s]",
		      var_mail_name, state->name, state->addr);
    post_mail_fputs(notice, "");
    post_mail_fputs(notice, "Transcript of session follows.");
    post_mail_fputs(notice, "");
    argv_terminate(state->history);
    for (cpp = state->history->argv; *cpp; cpp++)
	line_wrap(printable(*cpp, '?'), LENGTH, INDENT, print_line,
		  (char *) notice);
    post_mail_fputs(notice, "");
    if (state->reason)
	post_mail_fprintf(notice, "Session aborted, reason: %s", state->reason);
    (void) post_mail_fclose(notice);
}
