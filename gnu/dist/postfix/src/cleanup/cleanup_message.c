/*++
/* NAME
/*	cleanup_message 3
/* SUMMARY
/*	process message segment
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_message(state, type, buf, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	char	*buf;
/*	int	len;
/* DESCRIPTION
/*	This module processes message content records and copies the
/*	result to the queue file.  It validates the input, rewrites
/*	sender/recipient addresses to canonical form, inserts missing
/*	message headers, and extracts information from message headers
/*	to be used later when generating the extracted output segment.
/*	This routine absorbs but does not emit the content to extracted
/*	boundary record.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP type
/*	Record type.
/* .IP buf
/*	Record content.
/* .IP len
/*	Record content length.
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
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <split_at.h>
#include <mymalloc.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <tok822.h>
#include <header_opts.h>
#include <quote_822_local.h>
#include <mail_params.h>
#include <mail_date.h>
#include <mail_addr.h>
#include <is_header.h>
#include <ext_prop.h>

/* Application-specific. */

#include "cleanup.h"

static void cleanup_message_header(CLEANUP_STATE *, int, char *, int);
static void cleanup_message_body(CLEANUP_STATE *, int, char *, int);

/* cleanup_out_header - output one header as a bunch of records */

static void cleanup_out_header(CLEANUP_STATE *state)
{
    char   *start = vstring_str(state->header_buf);
    char   *line;
    char   *next_line;

    /*
     * Prepend a tab to continued header lines that went through the address
     * rewriting machinery. See cleanup_fold_header(state) below for the form
     * of such header lines. NB: This code destroys the header. We could try
     * to avoid clobbering it, but we're not going to use the data any
     * further.
     */
    for (line = start; line; line = next_line) {
	next_line = split_at(line, '\n');
	if (line == start || ISSPACE(*line)) {
	    cleanup_out_string(state, REC_TYPE_NORM, line);
	} else {
	    cleanup_out_format(state, REC_TYPE_NORM, "\t%s", line);
	}
    }
}

/* cleanup_fold_header - wrap address list header */

static void cleanup_fold_header(CLEANUP_STATE *state)
{
    char   *start_line = vstring_str(state->header_buf);
    char   *end_line;
    char   *next_line;
    char   *line;

    /*
     * A rewritten address list contains one address per line. The code below
     * replaces newlines by spaces, to fit as many addresses on a line as
     * possible (without rearranging the order of addresses). Prepending
     * white space to the beginning of lines is delegated to the output
     * routine.
     */
    for (line = start_line; line != 0; line = next_line) {
	end_line = line + strcspn(line, "\n");
	if (line > start_line) {
	    if (end_line - start_line < 70) {	/* TAB counts as one */
		line[-1] = ' ';
	    } else {
		start_line = line;
	    }
	}
	next_line = *end_line ? end_line + 1 : 0;
    }
    cleanup_out_header(state);
}

/* cleanup_extract_internal - save unquoted copy of extracted address */

static char *cleanup_extract_internal(VSTRING *buffer, TOK822 *addr)
{

    /*
     * A little routine to stash away a copy of an address that we extracted
     * from a message header line.
     */
    tok822_internalize(buffer, addr->head, TOK822_STR_DEFL);
    return (mystrdup(vstring_str(buffer)));
}

/* cleanup_rewrite_sender - sender address rewriting */

static void cleanup_rewrite_sender(CLEANUP_STATE *state, HEADER_OPTS *hdr_opts)
{
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;

    if (msg_verbose)
	msg_info("rewrite_sender: %s", hdr_opts->name);

    /*
     * Parse the header line, rewrite each address found, save copies of
     * sender addresses, and regenerate the header line. Finally, pipe the
     * result through the header line folding routine.
     */
    tree = tok822_parse(vstring_str(state->header_buf)
			+ strlen(hdr_opts->name) + 1);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	cleanup_rewrite_tree(*tpp);
	if (cleanup_send_canon_maps)
	    cleanup_map11_tree(state, *tpp, cleanup_send_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps)
	    cleanup_map11_tree(state, *tpp, cleanup_comm_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_masq_domains)
	    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	if (hdr_opts->type == HDR_FROM && state->from == 0)
	    state->from = cleanup_extract_internal(state->header_buf, *tpp);
	if (hdr_opts->type == HDR_RESENT_FROM && state->resent_from == 0)
	    state->resent_from =
		cleanup_extract_internal(state->header_buf, *tpp);
    }
    vstring_sprintf(state->header_buf, "%s: ", hdr_opts->name);
    tok822_externalize(state->header_buf, tree, TOK822_STR_HEAD);
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0)
	cleanup_fold_header(state);
}

/* cleanup_rewrite_recip - recipient address rewriting */

static void cleanup_rewrite_recip(CLEANUP_STATE *state, HEADER_OPTS *hdr_opts)
{
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    ARGV   *rcpt;

    if (msg_verbose)
	msg_info("rewrite_recip: %s", hdr_opts->name);

    /*
     * Parse the header line, rewrite each address found, save copies of
     * recipient addresses, and regenerate the header line. Finally, pipe the
     * result through the header line folding routine.
     */
    tree = tok822_parse(vstring_str(state->header_buf)
			+ strlen(hdr_opts->name) + 1);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	cleanup_rewrite_tree(*tpp);
	if (cleanup_rcpt_canon_maps)
	    cleanup_map11_tree(state, *tpp, cleanup_rcpt_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps)
	    cleanup_map11_tree(state, *tpp, cleanup_comm_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (state->recip == 0 && (hdr_opts->flags & HDR_OPT_EXTRACT) != 0) {
	    rcpt = (hdr_opts->flags & HDR_OPT_RR) ?
		state->resent_recip : state->recipients;
	    if (rcpt->argc < var_extra_rcpt_limit) {
		tok822_internalize(state->temp1, tpp[0]->head, TOK822_STR_DEFL);
		argv_add(rcpt, vstring_str(state->temp1), (char *) 0);
	    }
	}
	if (cleanup_masq_domains)
	    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	if (hdr_opts->type == HDR_RETURN_RECEIPT_TO && !state->return_receipt)
	    state->return_receipt =
		cleanup_extract_internal(state->header_buf, *tpp);
	if (hdr_opts->type == HDR_ERRORS_TO && !state->errors_to)
	    state->errors_to =
		cleanup_extract_internal(state->header_buf, *tpp);
    }
    vstring_sprintf(state->header_buf, "%s: ", hdr_opts->name);
    tok822_externalize(state->header_buf, tree, TOK822_STR_HEAD);
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0)
	cleanup_fold_header(state);
}

/* cleanup_header - process one complete header line */

static void cleanup_header(CLEANUP_STATE *state)
{
    char   *myname = "cleanup_header";
    HEADER_OPTS *hdr_opts;

    if (msg_verbose)
	msg_info("%s: '%s'", myname, vstring_str(state->header_buf));

    if ((state->flags & CLEANUP_FLAG_FILTER) && cleanup_header_checks) {
	char   *header = vstring_str(state->header_buf);
	const char *value;

	if ((value = maps_find(cleanup_header_checks, header, 0)) != 0) {
	    if (strcasecmp(value, "REJECT") == 0) {
		msg_info("%s: reject: header %.200s; from=<%s> to=<%s>",
			 state->queue_id, header, state->sender,
			 state->recip ? state->recip : "unknown");
		state->errs |= CLEANUP_STAT_CONT;
	    } else if (strcasecmp(value, "IGNORE") == 0) {
		return;
	    }
	}
    }

    /*
     * If this is an "unknown" header, just copy it to the output without
     * even bothering to fold long lines. XXX Should split header lines that
     * do not fit a REC_TYPE_NORM record.
     */
    if ((hdr_opts = header_opts_find(vstring_str(state->header_buf))) == 0) {
	cleanup_out_header(state);
    }

    /*
     * Known header. Remember that we have seen at least one. Find out what
     * we should do with this header: delete, count, rewrite. Note that we
     * should examine headers even when they will be deleted from the output,
     * because the addresses in those headers might be needed elsewhere.
     */
    else {
	state->headers_seen |= (1 << hdr_opts->type);
	if (hdr_opts->type == HDR_MESSAGE_ID)
	    msg_info("%s: message-id=%s", state->queue_id,
	       vstring_str(state->header_buf) + strlen(hdr_opts->name) + 2);
	if (hdr_opts->type == HDR_RESENT_MESSAGE_ID)
	    msg_info("%s: resent-message-id=%s", state->queue_id,
	       vstring_str(state->header_buf) + strlen(hdr_opts->name) + 2);
	if (hdr_opts->type == HDR_RECEIVED)
	    if (++state->hop_count >= var_hopcount_limit)
		state->errs |= CLEANUP_STAT_HOPS;
	if (CLEANUP_OUT_OK(state)) {
	    if (hdr_opts->flags & HDR_OPT_RR)
		state->resent = "Resent-";
	    if (hdr_opts->flags & HDR_OPT_SENDER) {
		cleanup_rewrite_sender(state, hdr_opts);
	    } else if (hdr_opts->flags & HDR_OPT_RECIP) {
		cleanup_rewrite_recip(state, hdr_opts);
	    } else if ((hdr_opts->flags & HDR_OPT_DROP) == 0) {
		cleanup_out_header(state);
	    }
	}
    }
}

/* cleanup_missing_headers - insert missing message headers */

static void cleanup_missing_headers(CLEANUP_STATE *state)
{
    char    time_stamp[1024];		/* XXX locale dependent? */
    struct tm *tp;
    TOK822 *token;

    /*
     * Add a missing (Resent-)Message-Id: header. The message ID gives the
     * time in GMT units, plus the local queue ID.
     */
    if ((state->headers_seen & (1 << (state->resent[0] ?
			   HDR_RESENT_MESSAGE_ID : HDR_MESSAGE_ID))) == 0) {
	tp = gmtime(&state->time);
	strftime(time_stamp, sizeof(time_stamp), "%Y%m%d%H%M%S", tp);
	cleanup_out_format(state, REC_TYPE_NORM, "%sMessage-Id: <%s.%s@%s>",
		state->resent, time_stamp, state->queue_id, var_myhostname);
	msg_info("%s: %smessage-id=<%s.%s@%s>",
		 state->queue_id, *state->resent ? "resent-" : "",
		 time_stamp, state->queue_id, var_myhostname);
    }

    /*
     * Add a missing (Resent-)Date: header. The date is in local time units,
     * with the GMT offset at the end.
     */
    if ((state->headers_seen & (1 << (state->resent[0] ?
				      HDR_RESENT_DATE : HDR_DATE))) == 0) {
	cleanup_out_format(state, REC_TYPE_NORM, "%sDate: %s",
			   state->resent, mail_date(state->time));
    }

    /*
     * Add a missing (Resent-)From: header.
     */
    if ((state->headers_seen & (1 << (state->resent[0] ?
				      HDR_RESENT_FROM : HDR_FROM))) == 0) {
	quote_822_local(state->temp1, *state->sender ?
			state->sender : MAIL_ADDR_MAIL_DAEMON);
	vstring_sprintf(state->temp2, "%sFrom: %s",
			state->resent, vstring_str(state->temp1));
	if (*state->sender && state->fullname && *state->fullname) {
	    vstring_sprintf(state->temp1, "(%s)", state->fullname);
	    token = tok822_parse(vstring_str(state->temp1));
	    vstring_strcat(state->temp2, " ");
	    tok822_externalize(state->temp2, token, TOK822_STR_NONE);
	    tok822_free_tree(token);
	}
	CLEANUP_OUT_BUF(state, REC_TYPE_NORM, state->temp2);
    }

    /*
     * Add a missing destination header.
     */
#define VISIBLE_RCPT	((1 << HDR_TO) | (1 << HDR_RESENT_TO) \
			| (1 << HDR_CC) | (1 << HDR_RESENT_CC))

    if ((state->headers_seen & VISIBLE_RCPT) == 0)
	cleanup_out_format(state, REC_TYPE_NORM, "%s", var_rcpt_witheld);
}

/* cleanup_message - initialize message content segment */

void    cleanup_message(CLEANUP_STATE *state, int type, char *buf, int len)
{
    char   *myname = "cleanup_message";

    /*
     * Write a dummy start-of-content segment marker. We'll update it with
     * real file offset information after reaching the end of the message
     * content.
     */
    if ((state->mesg_offset = vstream_ftell(state->dst)) < 0)
	msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
    cleanup_out_format(state, REC_TYPE_MESG, REC_TYPE_MESG_FORMAT, 0L);
    if ((state->data_offset = vstream_ftell(state->dst)) < 0)
	msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);

    /*
     * Pass control to the header processing routine.
     */
    state->action = cleanup_message_header;
    cleanup_message_header(state, type, buf, len);
}

/* cleanup_message_header - process message content, header */

static void cleanup_message_header(CLEANUP_STATE *state, int type, char *buf, int len)
{
    char   *myname = "cleanup_message_header";

    /*
     * Sanity check.
     */
    if (strchr(REC_TYPE_CONTENT, type) == 0) {
	msg_warn("%s: %s: unexpected record type %d",
		 state->queue_id, myname, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
    }

    /*
     * First, deal with header information that we have accumulated from
     * previous input records. A whole record that starts with whitespace is
     * a continuation of previous data.
     * 
     * XXX Silently switch to body processing when some message header requires
     * an unreasonable amount of storage, or when a message header record
     * does not fit in a REC_TYPE_NORM type record.
     */
    if (VSTRING_LEN(state->header_buf) > 0) {
	if ((VSTRING_LEN(state->header_buf) >= var_header_limit
	     || type == REC_TYPE_CONT)) {
	    state->errs |= CLEANUP_STAT_HOVFL;
	} else if (type == REC_TYPE_NORM && ISSPACE(*buf)) {
	    VSTRING_ADDCH(state->header_buf, '\n');
	    vstring_strcat(state->header_buf, buf);
	    return;
	} else {
	     /* Body record or end of message segment. */ ;
	}

	/*
	 * No more input to append to this saved header. Do output processing
	 * and reset the saved header buffer.
	 */
	VSTRING_TERMINATE(state->header_buf);
	cleanup_header(state);
	VSTRING_RESET(state->header_buf);
    }

    /*
     * Switch to body processing if this is not a header or if the saved
     * header would require an unreasonable amount of storage. Generate
     * missing headers. Add one blank line when the message headers are
     * immediately followed by a non-empty message body.
     */
    if (((state->errs & CLEANUP_STAT_HOVFL)
	 || type != REC_TYPE_NORM
	 || !is_header(buf))) {
	cleanup_missing_headers(state);
	if (type != REC_TYPE_XTRA && *buf)	/* output blank line */
	    cleanup_out_string(state, REC_TYPE_NORM, "");
	state->action = cleanup_message_body;
	cleanup_message_body(state, type, buf, len);
    }

    /*
     * Save this header record until we know that the header is complete.
     */
    else {
	vstring_strcpy(state->header_buf, buf);
    }
}

/* cleanup_message_body - process message segment, body */

static void cleanup_message_body(CLEANUP_STATE *state, int type, char *buf, int len)
{
    char   *myname = "cleanup_message_body";

    /*
     * Copy body record to the output.
     */
    if (type == REC_TYPE_NORM || type == REC_TYPE_CONT) {

	/*
	 * Crude message body content filter for emergencies. This code has
	 * several problems: it sees one line at a time, and therefore does
	 * not recognize multi-line MIME headers in the body; it looks at
	 * long lines only in chunks of line_length_limit (2048) characters;
	 * it is easily bypassed with encodings and with multi-line tricks.
	 */
	if ((state->flags & CLEANUP_FLAG_FILTER) && cleanup_body_checks) {
	    const char *value;

	    if ((value = maps_find(cleanup_body_checks, buf, 0)) != 0) {
		if (strcasecmp(value, "REJECT") == 0) {
		    msg_info("%s: reject: body %.200s; from=<%s> to=<%s>",
			     state->queue_id, buf, state->sender,
			     state->recip ? state->recip : "unknown");
		    state->errs |= CLEANUP_STAT_CONT;
		} else if (strcasecmp(value, "IGNORE") == 0) {
		    return;
		}
	    }
	}
	cleanup_out(state, type, buf, len);
    }

    /*
     * If we have reached the end of the message content segment, record the
     * current file position so we can compute the message size lateron.
     */
    else if (type == REC_TYPE_XTRA) {
	if ((state->xtra_offset = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	state->action = cleanup_extracted;
    }

    /*
     * This should never happen.
     */
    else {
	msg_warn("%s: unexpected record type: %d", myname, type);
	state->errs |= CLEANUP_STAT_BAD;
    }
}
