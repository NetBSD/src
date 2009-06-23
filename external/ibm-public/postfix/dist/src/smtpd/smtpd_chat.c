/*	$NetBSD: smtpd_chat.c,v 1.1.1.1 2009/06/23 10:08:55 tron Exp $	*/

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
/*	replaced by 4xx (try again). In case of a 421 reply the
/*	SMTPD_FLAG_HANGUP flag is set for orderly disconnect.
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

static void smtp_chat_append(SMTPD_STATE *state, char *direction,
			             const char *text)
{
    char   *line;

    if (state->notify_mask == 0)
	return;

    if (state->history == 0)
	state->history = argv_alloc(10);
    line = concatenate(direction, text, (char *) 0);
    argv_add(state->history, line, (char *) 0);
    myfree(line);
}

/* smtpd_chat_query - receive and record an SMTP request */

void    smtpd_chat_query(SMTPD_STATE *state)
{
    int     last_char;

    last_char = smtp_get(state->buffer, state->client, var_line_limit);
    smtp_chat_append(state, "In:  ", STR(state->buffer));
    if (last_char != '\n')
	msg_warn("%s: request longer than %d: %.30s...",
		 state->namaddr, var_line_limit,
		 printable(STR(state->buffer), '?'));

    if (msg_verbose)
	msg_info("< %s: %s", state->namaddr, STR(state->buffer));
}

/* smtpd_chat_reply - format, send and record an SMTP response */

void    smtpd_chat_reply(SMTPD_STATE *state, const char *format,...)
{
    va_list ap;
    int     delay = 0;
    char   *cp;
    char   *next;
    char   *end;

    /*
     * Slow down clients that make errors. Sleep-on-anything slows down
     * clients that make an excessive number of errors within a session.
     */
    if (state->error_count >= var_smtpd_soft_erlim)
	sleep(delay = var_smtpd_err_sleep);

    va_start(ap, format);
    vstring_vsprintf(state->buffer, format, ap);
    va_end(ap);
    /* All 5xx replies must have a 5.xx.xx detail code. */
    for (cp = STR(state->buffer), end = cp + strlen(STR(state->buffer));;) {
	if (var_soft_bounce) {
	    if (cp[0] == '5') {
		cp[0] = '4';
		if (cp[4] == '5')
		    cp[4] = '4';
	    }
	}
	/* This is why we use strlen() above instead of VSTRING_LEN(). */
	if ((next = strstr(cp, "\r\n")) != 0) {
	    *next = 0;
	} else {
	    next = end;
	}
	smtp_chat_append(state, "Out: ", cp);

	if (msg_verbose)
	    msg_info("> %s: %s", state->namaddr, cp);

	smtp_fputs(cp, next - cp, state->client);
	if (next < end)
	    cp = next + 2;
	else
	    break;
    }

    /*
     * Flush unsent output if no I/O happened for a while. This avoids
     * timeouts with pipelined SMTP sessions that have lots of server-side
     * delays (tarpit delays or DNS lookups for UCE restrictions).
     */
    if (delay || time((time_t *) 0) - vstream_ftime(state->client) > 10)
	vstream_fflush(state->client);

    /*
     * Abort immediately if the connection is broken.
     */
    if (vstream_ftimeout(state->client))
	vstream_longjmp(state->client, SMTP_ERR_TIME);
    if (vstream_ferror(state->client))
	vstream_longjmp(state->client, SMTP_ERR_EOF);

    /*
     * Orderly disconnect in case of 421 or 521 reply.
     */
    if (strncmp(STR(state->buffer), "421", 3) == 0
	|| strncmp(STR(state->buffer), "521", 3) == 0)
	state->flags |= SMTPD_FLAG_HANGUP;
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
    const char *myname = "smtpd_chat_notify";
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
#define NULL_TRACE_FLAGS	0
#define NO_QUEUE_ID		((VSTRING *) 0)
#define LENGTH	78
#define INDENT	4

    notice = post_mail_fopen_nowait(mail_addr_double_bounce(),
				    var_error_rcpt,
				    INT_FILT_MASK_NOTIFY,
				    NULL_TRACE_FLAGS, NO_QUEUE_ID);
    if (notice == 0) {
	msg_warn("postmaster notify: %m");
	return;
    }
    post_mail_fprintf(notice, "From: %s (Mail Delivery System)",
		      mail_addr_mail_daemon());
    post_mail_fprintf(notice, "To: %s (Postmaster)", var_error_rcpt);
    post_mail_fprintf(notice, "Subject: %s SMTP server: errors from %s",
		      var_mail_name, state->namaddr);
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
    post_mail_fputs(notice, "");
    post_mail_fprintf(notice, "For other details, see the local mail logfile");
    (void) post_mail_fclose(notice);
}
