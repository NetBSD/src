/*++
/* NAME
/*	cleanup_message 3
/* SUMMARY
/*	process message segment
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_message(void)
/* DESCRIPTION
/*	This module processes message content segments.
/*	While copying records from input to output, it validates
/*	the input, rewrites sender/recipient addresses to canonical
/*	form, inserts missing message headers, and extracts information
/*	from message headers to be used later when generating the extracted
/*	output segment.
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

/* cleanup_out_header - output one header as a bunch of records */

static void cleanup_out_header(void)
{
    char   *start = vstring_str(cleanup_header_buf);
    char   *line;
    char   *next_line;

    /*
     * Prepend a tab to continued header lines that went through the address
     * rewriting machinery. See cleanup_fold_header() below for the form of
     * such header lines. NB: This code destroys the header. We could try to
     * avoid clobbering it, but we're not going to use the data any further.
     */
    for (line = start; line; line = next_line) {
	next_line = split_at(line, '\n');
	if (line == start || ISSPACE(*line)) {
	    cleanup_out_string(REC_TYPE_NORM, line);
	} else {
	    cleanup_out_format(REC_TYPE_NORM, "\t%s", line);
	}
    }
}

/* cleanup_fold_header - wrap address list header */

static void cleanup_fold_header(void)
{
    char   *start_line = vstring_str(cleanup_header_buf);
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
    cleanup_out_header();
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

static void cleanup_rewrite_sender(HEADER_OPTS *hdr_opts)
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
    tree = tok822_parse(vstring_str(cleanup_header_buf)
			+ strlen(hdr_opts->name) + 1);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	cleanup_rewrite_tree(*tpp);
	if (cleanup_send_canon_maps)
	    cleanup_map11_tree(*tpp, cleanup_send_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps)
	    cleanup_map11_tree(*tpp, cleanup_comm_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_masq_domains)
	    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	if (hdr_opts->type == HDR_FROM && cleanup_from == 0)
	    cleanup_from = cleanup_extract_internal(cleanup_header_buf, *tpp);
	if (hdr_opts->type == HDR_RESENT_FROM && cleanup_resent_from == 0)
	    cleanup_resent_from =
		cleanup_extract_internal(cleanup_header_buf, *tpp);
    }
    vstring_sprintf(cleanup_header_buf, "%s: ", hdr_opts->name);
    tok822_externalize(cleanup_header_buf, tree, TOK822_STR_HEAD);
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0)
	cleanup_fold_header();
}

/* cleanup_rewrite_recip - recipient address rewriting */

static void cleanup_rewrite_recip(HEADER_OPTS *hdr_opts)
{
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;

    if (msg_verbose)
	msg_info("rewrite_recip: %s", hdr_opts->name);

    /*
     * Parse the header line, rewrite each address found, save copies of
     * recipient addresses, and regenerate the header line. Finally, pipe the
     * result through the header line folding routine.
     */
    tree = tok822_parse(vstring_str(cleanup_header_buf)
			+ strlen(hdr_opts->name) + 1);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	cleanup_rewrite_tree(*tpp);
	if (cleanup_rcpt_canon_maps)
	    cleanup_map11_tree(*tpp, cleanup_rcpt_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps)
	    cleanup_map11_tree(*tpp, cleanup_comm_canon_maps,
			       cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	tok822_internalize(cleanup_temp1, tpp[0]->head, TOK822_STR_DEFL);
	if (cleanup_recip == 0 && (hdr_opts->flags & HDR_OPT_EXTRACT) != 0)
	    argv_add((hdr_opts->flags & HDR_OPT_RR) ?
		     cleanup_resent_recip : cleanup_recipients,
		     vstring_str(cleanup_temp1), (char *) 0);
	if (cleanup_masq_domains)
	    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	if (hdr_opts->type == HDR_RETURN_RECEIPT_TO && !cleanup_return_receipt)
	    cleanup_return_receipt =
		cleanup_extract_internal(cleanup_header_buf, *tpp);
	if (hdr_opts->type == HDR_ERRORS_TO && !cleanup_errors_to)
	    cleanup_errors_to =
		cleanup_extract_internal(cleanup_header_buf, *tpp);
    }
    vstring_sprintf(cleanup_header_buf, "%s: ", hdr_opts->name);
    tok822_externalize(cleanup_header_buf, tree, TOK822_STR_HEAD);
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0)
	cleanup_fold_header();
}

/* cleanup_header - process one complete header line */

static void cleanup_header(void)
{
    char   *myname = "cleanup_header";
    HEADER_OPTS *hdr_opts;

    if (msg_verbose)
	msg_info("%s: '%s'", myname, vstring_str(cleanup_header_buf));

    if ((cleanup_flags & CLEANUP_FLAG_FILTER) && cleanup_header_checks) {
	char   *header = vstring_str(cleanup_header_buf);
	const char *value;

	if ((value = maps_find(cleanup_header_checks, header, 0)) != 0) {
	    if (strcasecmp(value, "REJECT") == 0) {
		msg_warn("%s: reject: header %.200s", cleanup_queue_id, header);
		cleanup_errs |= CLEANUP_STAT_CONT;
	    }
	}
    }

    /*
     * If this is an "unknown" header, just copy it to the output without
     * even bothering to fold long lines. XXX Should split header lines that
     * do not fit a REC_TYPE_NORM record.
     */
    if ((hdr_opts = header_opts_find(vstring_str(cleanup_header_buf))) == 0) {
	cleanup_out_header();
    }

    /*
     * Known header. Remember that we have seen at least one. Find out what
     * we should do with this header: delete, count, rewrite. Note that we
     * should examine headers even when they will be deleted from the output,
     * because the addresses in those headers might be needed elsewhere.
     */
    else {
	cleanup_headers_seen |= (1 << hdr_opts->type);
	if (hdr_opts->type == HDR_MESSAGE_ID)
	    msg_info("%s: message-id=%s", cleanup_queue_id,
	      vstring_str(cleanup_header_buf) + strlen(hdr_opts->name) + 2);
	if (hdr_opts->type == HDR_RESENT_MESSAGE_ID)
	    msg_info("%s: resent-message-id=%s", cleanup_queue_id,
	      vstring_str(cleanup_header_buf) + strlen(hdr_opts->name) + 2);
	if (hdr_opts->type == HDR_RECEIVED)
	    if (++cleanup_hop_count >= var_hopcount_limit)
		cleanup_errs |= CLEANUP_STAT_HOPS;
	if (CLEANUP_OUT_OK()) {
	    if (hdr_opts->flags & HDR_OPT_RR)
		cleanup_resent = "Resent-";
	    if (hdr_opts->flags & HDR_OPT_SENDER) {
		cleanup_rewrite_sender(hdr_opts);
	    } else if (hdr_opts->flags & HDR_OPT_RECIP) {
		cleanup_rewrite_recip(hdr_opts);
	    } else if ((hdr_opts->flags & HDR_OPT_DROP) == 0) {
		cleanup_out_header();
	    }
	}
    }
}

/* cleanup_missing_headers - insert missing message headers */

static void cleanup_missing_headers(void)
{
    char    time_stamp[1024];		/* XXX locale dependent? */
    struct tm *tp;
    TOK822 *token;
    char   *from;

    /*
     * Add a missing (Resent-)Message-Id: header. The message ID gives the
     * time in GMT units, plus the local queue ID.
     */
    if ((cleanup_headers_seen & (1 << (cleanup_resent[0] ?
			   HDR_RESENT_MESSAGE_ID : HDR_MESSAGE_ID))) == 0) {
	tp = gmtime(&cleanup_time);
	strftime(time_stamp, sizeof(time_stamp), "%Y%m%d%H%M%S", tp);
	cleanup_out_format(REC_TYPE_NORM, "%sMessage-Id: <%s.%s@%s>",
	      cleanup_resent, time_stamp, cleanup_queue_id, var_myhostname);
	msg_info("%s: %smessage-id=<%s.%s@%s>",
		 cleanup_queue_id, *cleanup_resent ? "resent-" : "",
		 time_stamp, cleanup_queue_id, var_myhostname);
    }

    /*
     * Add a missing (Resent-)Date: header. The date is in local time units,
     * with the GMT offset at the end.
     */
    if ((cleanup_headers_seen & (1 << (cleanup_resent[0] ?
				       HDR_RESENT_DATE : HDR_DATE))) == 0) {
	cleanup_out_format(REC_TYPE_NORM, "%sDate: %s",
			   cleanup_resent, mail_date(cleanup_time));
    }

    /*
     * Add a missing (Resent-)From: header. If a (Resent-)From: header is
     * present, see if we need a (Resent-)Sender: header.
     */
#define NOT_SPECIAL_SENDER(addr) (*(addr) != 0 \
	   && strcasecmp(addr, mail_addr_double_bounce()) != 0)

    if ((cleanup_headers_seen & (1 << (cleanup_resent[0] ?
				       HDR_RESENT_FROM : HDR_FROM))) == 0) {
	quote_822_local(cleanup_temp1, cleanup_sender);
	vstring_sprintf(cleanup_temp2, "%sFrom: %s",
			cleanup_resent, vstring_str(cleanup_temp1));
	if (cleanup_fullname && *cleanup_fullname) {
	    vstring_strcat(cleanup_temp2, " (");
	    token = tok822_alloc(TOK822_COMMENT, cleanup_fullname);
	    tok822_externalize(cleanup_temp2, token, TOK822_STR_NONE);
	    tok822_free(token);
	    vstring_strcat(cleanup_temp2, ")");
	}
	CLEANUP_OUT_BUF(REC_TYPE_NORM, cleanup_temp2);
    } else if ((cleanup_headers_seen & (1 << (cleanup_resent[0] ?
				      HDR_RESENT_SENDER : HDR_SENDER))) == 0
	       && NOT_SPECIAL_SENDER(cleanup_sender)) {
	from = (cleanup_resent[0] ? cleanup_resent_from : cleanup_from);
	if (from == 0 || strcasecmp(cleanup_sender, from) != 0) {
	    quote_822_local(cleanup_temp1, cleanup_sender);
	    cleanup_out_format(REC_TYPE_NORM, "%sSender: %s",
			       cleanup_resent, vstring_str(cleanup_temp1));
	}
    }

    /*
     * Add a missing destination header.
     */
#define VISIBLE_RCPT    ((1 << HDR_TO) | (1 << HDR_RESENT_TO) | \
			    (1 << HDR_CC) | (1 << HDR_RESENT_CC))

    if ((cleanup_headers_seen & VISIBLE_RCPT) == 0)
	cleanup_out_format(REC_TYPE_NORM, "%s", var_rcpt_witheld);
}

/* cleanup_message - process message content segment */

void    cleanup_message(void)
{
    char   *myname = "cleanup_message";
    long    mesg_offset;
    long    data_offset;
    long    xtra_offset;
    int     in_header;
    char   *start;
    int     type = 0;

    /*
     * Write a dummy start-of-content segment marker. We'll update it with
     * real file offset information after reaching the end of the message
     * content.
     */
    if ((mesg_offset = vstream_ftell(cleanup_dst)) < 0)
	msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
    cleanup_out_format(REC_TYPE_MESG, REC_TYPE_MESG_FORMAT, 0L);
    if ((data_offset = vstream_ftell(cleanup_dst)) < 0)
	msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);

    /*
     * An unannounced end-of-input condition most likely means that the
     * client did not want to send this message after all. Don't complain,
     * just stop generating any further output.
     * 
     * XXX Rely on the front-end programs to enforce record size limits.
     */
    in_header = 1;

    while (CLEANUP_OUT_OK()) {

	if ((type = rec_get(cleanup_src, cleanup_inbuf, 0)) < 0) {
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    break;
	}
	if (strchr(REC_TYPE_CONTENT, type) == 0) {
	    msg_warn("%s: %s: unexpected record type %d",
		     cleanup_queue_id, myname, type);
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    break;
	}
	start = vstring_str(cleanup_inbuf);

	/*
	 * First, deal with header information that we have accumulated from
	 * previous input records. A whole record that starts with whitespace
	 * is a continuation of previous data.
	 * 
	 * XXX Silently switch to body processing when some message header
	 * requires an unreasonable amount of storage, or when a message
	 * header record does not fit in a REC_TYPE_NORM type record.
	 */
	if (VSTRING_LEN(cleanup_header_buf) > 0) {
	    if ((VSTRING_LEN(cleanup_header_buf) >= var_header_limit
		 || type == REC_TYPE_CONT)) {
		cleanup_errs |= CLEANUP_STAT_HOVFL;
	    } else if (type == REC_TYPE_NORM && ISSPACE(*start)) {
		VSTRING_ADDCH(cleanup_header_buf, '\n');
		vstring_strcat(cleanup_header_buf, start);
		continue;
	    }

	    /*
	     * No more input to append to this saved header. Do output
	     * processing and reset the saved header buffer.
	     */
	    VSTRING_TERMINATE(cleanup_header_buf);
	    cleanup_header();
	    VSTRING_RESET(cleanup_header_buf);
	}

	/*
	 * Switch to body processing if we didn't read a header or if the
	 * saved header requires an unreasonable amount of storage. Generate
	 * missing headers. Add one blank line when the message headers are
	 * immediately followed by a non-empty message body.
	 */
	if (in_header
	    && ((cleanup_errs & CLEANUP_STAT_HOVFL)
		|| type != REC_TYPE_NORM
		|| !is_header(start))) {
	    in_header = 0;
	    cleanup_missing_headers();
	    if (type != REC_TYPE_XTRA && *start)/* output blank line */
		cleanup_out_string(REC_TYPE_NORM, "");
	}

	/*
	 * If this is a header record, save it until we know that the header
	 * is complete. If this is a body record, copy it to the output
	 * immediately.
	 */
	if (type == REC_TYPE_NORM || type == REC_TYPE_CONT) {
	    if (in_header) {
		vstring_strcpy(cleanup_header_buf, start);
	    } else {

		/*
		 * Crude message body content filter for emergencies. This
		 * code has several problems: it sees one line at a time, and
		 * therefore does not recognize multi-line MIME headers in
		 * the body; it looks at long lines only in chunks of
		 * line_length_limit (2048) characters; it is easily bypassed
		 * with encodings and with multi-line tricks.
		 */
		if ((cleanup_flags & CLEANUP_FLAG_FILTER)
		    && cleanup_body_checks) {
		    char   *body = vstring_str(cleanup_inbuf);
		    const char *value;

		    if ((value = maps_find(cleanup_body_checks, body, 0)) != 0) {
			if (strcasecmp(value, "REJECT") == 0) {
			    msg_warn("%s: reject: body %.200s",
				     cleanup_queue_id, body);
			    cleanup_errs |= CLEANUP_STAT_CONT;
			}
		    }
		}
		CLEANUP_OUT_BUF(type, cleanup_inbuf);
	    }
	}

	/*
	 * If we have reached the end of the message content segment, update
	 * the start-of-content marker, now that we know how large the
	 * message content segment is, and update the content size indicator
	 * at the beginning of the message envelope segment. vstream_fseek()
	 * implicitly flushes the stream, which may fail for various reasons.
	 */
	else if (type == REC_TYPE_XTRA) {
	    if ((xtra_offset = vstream_ftell(cleanup_dst)) < 0)
		msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	    if (vstream_fseek(cleanup_dst, mesg_offset, SEEK_SET) < 0) {
		msg_warn("%s: write queue file: %m", cleanup_queue_id);
		if (errno == EFBIG)
		    cleanup_errs |= CLEANUP_STAT_SIZE;
		else
		    cleanup_errs |= CLEANUP_STAT_WRITE;
		break;
	    }
	    cleanup_out_format(REC_TYPE_MESG, REC_TYPE_MESG_FORMAT, xtra_offset);
	    if (vstream_fseek(cleanup_dst, 0L, SEEK_SET) < 0)
		msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
	    cleanup_out_format(REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
			       xtra_offset - data_offset);
	    if (vstream_fseek(cleanup_dst, xtra_offset, SEEK_SET) < 0)
		msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
	    break;
	}

	/*
	 * This should never happen.
	 */
	else {
	    msg_panic("%s: unexpected record type: %d", myname, type);
	}
    }

    /*
     * Keep reading in case of problems, so that the sender is ready to
     * receive our status report.
     */
    if (CLEANUP_OUT_OK() == 0)
	if (type >= 0)
	    cleanup_skip();
}
