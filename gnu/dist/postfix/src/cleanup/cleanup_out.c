/*++
/* NAME
/*	cleanup_out 3
/* SUMMARY
/*	record output support
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	int	CLEANUP_OUT_OK(state)
/*	CLEANUP_STATE *state;
/*
/*	void	cleanup_out(state, type, data, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *data;
/*	ssize_t	len;
/*
/*	void	cleanup_out_string(state, type, str)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *str;
/*
/*	void	CLEANUP_OUT_BUF(state, type, buf)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	VSTRING	*buf;
/*
/*	void	cleanup_out_format(state, type, format, ...)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *format;
/*
/*	void	cleanup_out_header(state, buf)
/*	CLEANUP_STATE *state;
/*	VSTRING	*buf;
/* DESCRIPTION
/*	This module writes records to the output stream.
/*
/*	CLEANUP_OUT_OK() is a macro that evaluates to non-zero
/*	as long as it makes sense to produce output. All output
/*	routines below check for this condition.
/*
/*	cleanup_out() is the main record output routine. It writes
/*	one record of the specified type, with the specified data
/*	and length to the output stream.
/*
/*	cleanup_out_string() outputs one string as a record.
/*
/*	CLEANUP_OUT_BUF() is an unsafe macro that outputs
/*	one string buffer as a record.
/*
/*	cleanup_out_format() formats its arguments and writes
/*	the result as a record.
/*
/*	cleanup_out_header() outputs a multi-line header as records
/*	of the specified type. The input is expected to be newline
/*	separated (not newline terminated), and is modified.
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
#include <errno.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <split_at.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <mail_params.h>
#include <lex_822.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_out - output one single record */

void    cleanup_out(CLEANUP_STATE *state, int type, const char *string, ssize_t len)
{
    int     err = 0;

    /*
     * Long message header lines have to be read and written as multiple
     * records. Other header/body content, and envelope data, is copied one
     * record at a time. Be sure to not skip a zero-length request.
     * 
     * XXX We don't know if we're writing a message header or not, but that is
     * not a problem. A REC_TYPE_NORM or REC_TYPE_CONT record can always be
     * chopped up into an equivalent set of REC_TYPE_CONT plus REC_TYPE_NORM
     * records.
     */
    if (CLEANUP_OUT_OK(state) == 0)
	return;

#define TEXT_RECORD(t)	((t) == REC_TYPE_NORM || (t) == REC_TYPE_CONT)

    if (var_line_limit <= 0)
	msg_panic("cleanup_out: bad line length limit: %d", var_line_limit);
    do {
	if (len > var_line_limit && TEXT_RECORD(type)) {
	    err = rec_put(state->dst, REC_TYPE_CONT, string, var_line_limit);
	    string += var_line_limit;
	    len -= var_line_limit;
	} else {
	    err = rec_put(state->dst, type, string, len);
	    break;
	}
    } while (len > 0 && err >= 0);

    if (err < 0) {
	if (errno == EFBIG) {
	    msg_warn("%s: queue file size limit exceeded",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_SIZE;
	} else {
	    msg_warn("%s: write queue file: %m", state->queue_id);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
    }
}

/* cleanup_out_string - output string to one single record */

void    cleanup_out_string(CLEANUP_STATE *state, int type, const char *string)
{
    cleanup_out(state, type, string, strlen(string));
}

/* cleanup_out_format - output one formatted record */

void    cleanup_out_format(CLEANUP_STATE *state, int type, const char *fmt,...)
{
    static VSTRING *vp;
    va_list ap;

    if (vp == 0)
	vp = vstring_alloc(100);
    va_start(ap, fmt);
    vstring_vsprintf(vp, fmt, ap);
    va_end(ap);
    CLEANUP_OUT_BUF(state, type, vp);
}

/* cleanup_out_header - output one multi-line header as a bunch of records */

void    cleanup_out_header(CLEANUP_STATE *state, VSTRING *header_buf)
{
    char   *start = vstring_str(header_buf);
    char   *line;
    char   *next_line;
    ssize_t line_len;

    /*
     * Prepend a tab to continued header lines that went through the address
     * rewriting machinery. See cleanup_fold_header(state) below for the form
     * of such header lines. NB: This code destroys the header. We could try
     * to avoid clobbering it, but we're not going to use the data any
     * further.
     * 
     * XXX We prefer to truncate a header at the last line boundary before the
     * header size limit. If this would undershoot the limit by more than
     * 10%, we truncate between line boundaries to avoid losing too much
     * text. This "unkind cut" may result in syntax errors and may trigger
     * warnings from down-stream MTAs.
     * 
     * If Milter is enabled, pad a short header record with a dummy record so
     * that a header record can safely be overwritten by a pointer record.
     * This simplifies header modification enormously.
     */
    for (line = start; line; line = next_line) {
	next_line = split_at(line, '\n');
	line_len = next_line ? next_line - 1 - line : strlen(line);
	if (line + line_len > start + var_header_limit) {
	    if (line - start > 0.9 * var_header_limit)	/* nice cut */
		break;
	    start[var_header_limit] = 0;	/* unkind cut */
	    next_line = 0;
	}
	if (line == start) {
	    cleanup_out_string(state, REC_TYPE_NORM, line);
	    if ((state->milters || cleanup_milters)
		&& line_len < REC_TYPE_PTR_PAYL_SIZE)
		rec_pad(state->dst, REC_TYPE_DTXT,
			REC_TYPE_PTR_PAYL_SIZE - line_len);
	} else if (IS_SPACE_TAB(*line)) {
	    cleanup_out_string(state, REC_TYPE_NORM, line);
	} else {
	    cleanup_out_format(state, REC_TYPE_NORM, "\t%s", line);
	}
    }
}
