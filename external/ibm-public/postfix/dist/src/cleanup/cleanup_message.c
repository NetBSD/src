/*	$NetBSD: cleanup_message.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

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
/*	const char *buf;
/*	ssize_t	len;
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
#include <stringops.h>
#include <nvtable.h>

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
#include <mail_proto.h>
#include <mime_state.h>
#include <lex_822.h>
#include <dsn_util.h>
#include <conv_time.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_fold_header - wrap address list header */

static void cleanup_fold_header(CLEANUP_STATE *state, VSTRING *header_buf)
{
    char   *start_line = vstring_str(header_buf);
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
    cleanup_out_header(state, header_buf);
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

static void cleanup_rewrite_sender(CLEANUP_STATE *state,
				           const HEADER_OPTS *hdr_opts,
				           VSTRING *header_buf)
{
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    int     did_rewrite = 0;

    if (msg_verbose)
	msg_info("rewrite_sender: %s", hdr_opts->name);

    /*
     * Parse the header line, rewrite each address found, and regenerate the
     * header line. Finally, pipe the result through the header line folding
     * routine.
     */
    tree = tok822_parse_limit(vstring_str(header_buf)
			      + strlen(hdr_opts->name) + 1,
			      var_token_limit);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	did_rewrite |= cleanup_rewrite_tree(state->hdr_rewrite_context, *tpp);
	if (state->flags & CLEANUP_FLAG_MAP_OK) {
	    if (cleanup_send_canon_maps
		&& (cleanup_send_canon_flags & CLEANUP_CANON_FLAG_HDR_FROM))
		did_rewrite |=
		    cleanup_map11_tree(state, *tpp, cleanup_send_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_comm_canon_maps
		&& (cleanup_comm_canon_flags & CLEANUP_CANON_FLAG_HDR_FROM))
		did_rewrite |=
		    cleanup_map11_tree(state, *tpp, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_masq_domains
		&& (cleanup_masq_flags & CLEANUP_MASQ_FLAG_HDR_FROM))
		did_rewrite |=
		    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	}
    }
    if (did_rewrite) {
	vstring_truncate(header_buf, strlen(hdr_opts->name));
	vstring_strcat(header_buf, ": ");
	tok822_externalize(header_buf, tree, TOK822_STR_HEAD);
    }
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0) {
	if (did_rewrite)
	    cleanup_fold_header(state, header_buf);
	else
	    cleanup_out_header(state, header_buf);
    }
}

/* cleanup_rewrite_recip - recipient address rewriting */

static void cleanup_rewrite_recip(CLEANUP_STATE *state,
				          const HEADER_OPTS *hdr_opts,
				          VSTRING *header_buf)
{
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    int     did_rewrite = 0;

    if (msg_verbose)
	msg_info("rewrite_recip: %s", hdr_opts->name);

    /*
     * Parse the header line, rewrite each address found, and regenerate the
     * header line. Finally, pipe the result through the header line folding
     * routine.
     */
    tree = tok822_parse_limit(vstring_str(header_buf)
			      + strlen(hdr_opts->name) + 1,
			      var_token_limit);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	did_rewrite |= cleanup_rewrite_tree(state->hdr_rewrite_context, *tpp);
	if (state->flags & CLEANUP_FLAG_MAP_OK) {
	    if (cleanup_rcpt_canon_maps
		&& (cleanup_rcpt_canon_flags & CLEANUP_CANON_FLAG_HDR_RCPT))
		did_rewrite |=
		    cleanup_map11_tree(state, *tpp, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_comm_canon_maps
		&& (cleanup_comm_canon_flags & CLEANUP_CANON_FLAG_HDR_RCPT))
		did_rewrite |=
		    cleanup_map11_tree(state, *tpp, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_masq_domains
		&& (cleanup_masq_flags & CLEANUP_MASQ_FLAG_HDR_RCPT))
		did_rewrite |=
		    cleanup_masquerade_tree(*tpp, cleanup_masq_domains);
	}
    }
    if (did_rewrite) {
	vstring_truncate(header_buf, strlen(hdr_opts->name));
	vstring_strcat(header_buf, ": ");
	tok822_externalize(header_buf, tree, TOK822_STR_HEAD);
    }
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    if ((hdr_opts->flags & HDR_OPT_DROP) == 0) {
	if (did_rewrite)
	    cleanup_fold_header(state, header_buf);
	else
	    cleanup_out_header(state, header_buf);
    }
}

/* cleanup_act_log - log action with context */

static void cleanup_act_log(CLEANUP_STATE *state,
			            const char *action, const char *class,
			            const char *content, const char *text)
{
    const char *attr;

    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_ORIGIN)) == 0)
	attr = "unknown";
    vstring_sprintf(state->temp1, "%s: %s: %s %.200s from %s;",
		    state->queue_id, action, class, content, attr);
    if (state->sender)
	vstring_sprintf_append(state->temp1, " from=<%s>", state->sender);
    if (state->recip)
	vstring_sprintf_append(state->temp1, " to=<%s>", state->recip);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_PROTO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " proto=%s", attr);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_HELO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " helo=<%s>", attr);
    if (text && *text)
	vstring_sprintf_append(state->temp1, ": %s", text);
    msg_info("%s", vstring_str(state->temp1));
}

#define CLEANUP_ACT_CTXT_HEADER	"header"
#define CLEANUP_ACT_CTXT_BODY	"body"
#define CLEANUP_ACT_CTXT_ANY	"content"

/* cleanup_act - act upon a header/body match */

static const char *cleanup_act(CLEANUP_STATE *state, char *context,
			               const char *buf, const char *value,
			               const char *map_class)
{
    const char *optional_text = value + strcspn(value, " \t");
    int     command_len = optional_text - value;

#ifdef DELAY_ACTION
    int     defer_delay;

#endif

    while (*optional_text && ISSPACE(*optional_text))
	optional_text++;

#define STREQUAL(x,y,l) (strncasecmp((x), (y), (l)) == 0 && (y)[l] == 0)
#define CLEANUP_ACT_DROP 0

    /*
     * CLEANUP_STAT_CONT and CLEANUP_STAT_DEFER both update the reason
     * attribute, but CLEANUP_STAT_DEFER takes precedence. It terminates
     * queue record processing, and prevents bounces from being sent.
     */
    if (STREQUAL(value, "REJECT", command_len)) {
	const CLEANUP_STAT_DETAIL *detail;

	if (state->reason)
	    myfree(state->reason);
	if (*optional_text) {
	    state->reason = dsn_prepend("5.7.1", optional_text);
	    if (*state->reason != '4' && *state->reason != '5') {
		msg_warn("bad DSN action in %s -- need 4.x.x or 5.x.x",
			 optional_text);
		*state->reason = '4';
	    }
	} else {
	    detail = cleanup_stat_detail(CLEANUP_STAT_CONT);
	    state->reason = dsn_prepend(detail->dsn, detail->text);
	}
	if (*state->reason == '4')
	    state->errs |= CLEANUP_STAT_DEFER;
	else
	    state->errs |= CLEANUP_STAT_CONT;
	state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	cleanup_act_log(state, "reject", context, buf, state->reason);
	return (buf);
    }
    if (STREQUAL(value, "WARN", command_len)) {
	cleanup_act_log(state, "warning", context, buf, optional_text);
	return (buf);
    }
    if (STREQUAL(value, "FILTER", command_len)) {
	if (*optional_text == 0) {
	    msg_warn("missing FILTER command argument in %s map", map_class);
	} else if (strchr(optional_text, ':') == 0) {
	    msg_warn("bad FILTER command %s in %s -- "
		     "need transport:destination",
		     optional_text, map_class);
	} else {
	    if (state->filter)
		myfree(state->filter);
	    state->filter = mystrdup(optional_text);
	    cleanup_act_log(state, "filter", context, buf, optional_text);
	}
	return (buf);
    }
    if (STREQUAL(value, "DISCARD", command_len)) {
	cleanup_act_log(state, "discard", context, buf, optional_text);
	state->flags |= CLEANUP_FLAG_DISCARD;
	state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	return (buf);
    }
    if (STREQUAL(value, "HOLD", command_len)) {
	if ((state->flags & (CLEANUP_FLAG_HOLD | CLEANUP_FLAG_DISCARD)) == 0) {
	    cleanup_act_log(state, "hold", context, buf, optional_text);
	    state->flags |= CLEANUP_FLAG_HOLD;
	}
	return (buf);
    }

    /*
     * The DELAY feature is disabled because it has too many problems. 1) It
     * does not work on some remote file systems; 2) mail will be delivered
     * anyway with "sendmail -q" etc.; 3) while the mail is queued it bogs
     * down the deferred queue scan with huge amounts of useless disk I/O
     * operations.
     */
#ifdef DELAY_ACTION
    if (STREQUAL(value, "DELAY", command_len)) {
	if ((state->flags & (CLEANUP_FLAG_HOLD | CLEANUP_FLAG_DISCARD)) == 0) {
	    if (*optional_text == 0) {
		msg_warn("missing DELAY argument in %s map", map_class);
	    } else if (conv_time(optional_text, &defer_delay, 's') == 0) {
		msg_warn("ignoring bad DELAY argument %s in %s map",
			 optional_text, map_class);
	    } else {
		cleanup_act_log(state, "delay", context, buf, optional_text);
		state->defer_delay = defer_delay;
	    }
	}
	return (buf);
    }
#endif
    if (STREQUAL(value, "PREPEND", command_len)) {
	if (*optional_text == 0) {
	    msg_warn("PREPEND action without text in %s map", map_class);
	} else if (strcmp(context, CLEANUP_ACT_CTXT_HEADER) == 0
		   && !is_header(optional_text)) {
	    msg_warn("bad PREPEND header text \"%s\" in %s map -- "
		     "need \"headername: headervalue\"",
		     optional_text, map_class);
	} else {
	    cleanup_act_log(state, "prepend", context, buf, optional_text);
	    cleanup_out_string(state, REC_TYPE_NORM, optional_text);
	}
	return (buf);
    }
    if (STREQUAL(value, "REPLACE", command_len)) {
	if (*optional_text == 0) {
	    msg_warn("REPLACE action without text in %s map", map_class);
	    return (buf);
	} else if (strcmp(context, CLEANUP_ACT_CTXT_HEADER) == 0
		   && !is_header(optional_text)) {
	    msg_warn("bad REPLACE header text \"%s\" in %s map -- "
		     "need \"headername: headervalue\"",
		     optional_text, map_class);
	    return (buf);
	} else {
	    cleanup_act_log(state, "replace", context, buf, optional_text);
	    return (mystrdup(optional_text));
	}
    }
    if (STREQUAL(value, "REDIRECT", command_len)) {
	if (strchr(optional_text, '@') == 0) {
	    msg_warn("bad REDIRECT target \"%s\" in %s map -- "
		     "need user@domain",
		     optional_text, map_class);
	} else {
	    if (state->redirect)
		myfree(state->redirect);
	    state->redirect = mystrdup(optional_text);
	    cleanup_act_log(state, "redirect", context, buf, optional_text);
	    state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	}
	return (buf);
    }
    /* Allow and ignore optional text after the action. */

    if (STREQUAL(value, "IGNORE", command_len))
	return (CLEANUP_ACT_DROP);

    if (STREQUAL(value, "DUNNO", command_len))	/* preferred */
	return (buf);

    if (STREQUAL(value, "OK", command_len))	/* compat */
	return (buf);

    msg_warn("unknown command in %s map: %s", map_class, value);
    return (buf);
}

/* cleanup_header_callback - process one complete header line */

static void cleanup_header_callback(void *context, int header_class,
				            const HEADER_OPTS *hdr_opts,
				            VSTRING *header_buf,
				            off_t unused_offset)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    const char *myname = "cleanup_header_callback";
    char   *hdrval;
    struct code_map {
	const char *name;
	const char *encoding;
    };
    static struct code_map code_map[] = {	/* RFC 2045 */
	"7bit", MAIL_ATTR_ENC_7BIT,
	"8bit", MAIL_ATTR_ENC_8BIT,
	"binary", MAIL_ATTR_ENC_8BIT,	/* XXX Violation */
	"quoted-printable", MAIL_ATTR_ENC_7BIT,
	"base64", MAIL_ATTR_ENC_7BIT,
	0,
    };
    struct code_map *cmp;
    MAPS   *checks;
    const char *map_class;

    if (msg_verbose)
	msg_info("%s: '%.200s'", myname, vstring_str(header_buf));

    /*
     * Crude header filtering. This stops malware that isn't sophisticated
     * enough to use fancy header encodings.
     */
#define CHECK(class, maps, var_name) \
	(header_class == class && (map_class = var_name, checks = maps) != 0)

    if (hdr_opts && (hdr_opts->flags & HDR_OPT_MIME))
	header_class = MIME_HDR_MULTIPART;

    if ((state->flags & CLEANUP_FLAG_FILTER)
	&& (CHECK(MIME_HDR_PRIMARY, cleanup_header_checks, VAR_HEADER_CHECKS)
    || CHECK(MIME_HDR_MULTIPART, cleanup_mimehdr_checks, VAR_MIMEHDR_CHECKS)
    || CHECK(MIME_HDR_NESTED, cleanup_nesthdr_checks, VAR_NESTHDR_CHECKS))) {
	char   *header = vstring_str(header_buf);
	const char *value;

	if ((value = maps_find(checks, header, 0)) != 0) {
	    const char *result;

	    if ((result = cleanup_act(state, CLEANUP_ACT_CTXT_HEADER,
				      header, value, map_class))
		== CLEANUP_ACT_DROP) {
		return;
	    } else if (result != header) {
		vstring_strcpy(header_buf, result);
		hdr_opts = header_opts_find(result);
		myfree((char *) result);
	    }
	}
    }

    /*
     * If this is an "unknown" header, just copy it to the output without
     * even bothering to fold long lines. cleanup_out() will split long
     * headers that do not fit a REC_TYPE_NORM record.
     */
    if (hdr_opts == 0) {
	cleanup_out_header(state, header_buf);
	return;
    }

    /*
     * Allow 8-bit type info to override 7-bit type info. XXX Should reuse
     * the effort that went into MIME header parsing.
     */
    hdrval = vstring_str(header_buf) + strlen(hdr_opts->name) + 1;
    while (ISSPACE(*hdrval))
	hdrval++;
    /* trimblanks(hdrval, 0)[0] = 0; */
    if (var_auto_8bit_enc_hdr
	&& hdr_opts->type == HDR_CONTENT_TRANSFER_ENCODING) {
	for (cmp = code_map; cmp->name != 0; cmp++) {
	    if (strcasecmp(hdrval, cmp->name) == 0) {
		if (strcasecmp(cmp->encoding, MAIL_ATTR_ENC_8BIT) == 0)
		    nvtable_update(state->attr, MAIL_ATTR_ENCODING,
				   cmp->encoding);
		break;
	    }
	}
    }

    /*
     * Copy attachment etc. header blocks without further inspection.
     */
    if (header_class != MIME_HDR_PRIMARY) {
	cleanup_out_header(state, header_buf);
	return;
    }

    /*
     * Known header. Remember that we have seen at least one. Find out what
     * we should do with this header: delete, count, rewrite. Note that we
     * should examine headers even when they will be deleted from the output,
     * because the addresses in those headers might be needed elsewhere.
     * 
     * XXX 2821: Return-path breakage.
     * 
     * RFC 821 specifies: When the receiver-SMTP makes the "final delivery" of a
     * message it inserts at the beginning of the mail data a return path
     * line.  The return path line preserves the information in the
     * <reverse-path> from the MAIL command.  Here, final delivery means the
     * message leaves the SMTP world.  Normally, this would mean it has been
     * delivered to the destination user, but in some cases it may be further
     * processed and transmitted by another mail system.
     * 
     * And that is what Postfix implements. Delivery agents prepend
     * Return-Path:. In order to avoid cluttering up the message with
     * possibly inconsistent Return-Path: information (the sender can change
     * as the result of mail forwarding or mailing list delivery), Postfix
     * removes any existing Return-Path: headers.
     * 
     * RFC 2821 Section 4.4 specifies:    A message-originating SMTP system
     * SHOULD NOT send a message that already contains a Return-path header.
     * SMTP servers performing a relay function MUST NOT inspect the message
     * data, and especially not to the extent needed to determine if
     * Return-path headers are present. SMTP servers making final delivery
     * MAY remove Return-path headers before adding their own.
     */
    else {
	state->headers_seen |= (1 << hdr_opts->type);
	if (hdr_opts->type == HDR_MESSAGE_ID)
	    msg_info("%s: message-id=%s", state->queue_id, hdrval);
	if (hdr_opts->type == HDR_RESENT_MESSAGE_ID)
	    msg_info("%s: resent-message-id=%s", state->queue_id, hdrval);
	if (hdr_opts->type == HDR_RECEIVED)
	    if (++state->hop_count >= var_hopcount_limit)
		state->errs |= CLEANUP_STAT_HOPS;
	if (CLEANUP_OUT_OK(state)) {
	    if (hdr_opts->flags & HDR_OPT_RR)
		state->resent = "Resent-";
	    if ((hdr_opts->flags & HDR_OPT_SENDER)
		&& state->hdr_rewrite_context) {
		cleanup_rewrite_sender(state, hdr_opts, header_buf);
	    } else if ((hdr_opts->flags & HDR_OPT_RECIP)
		       && state->hdr_rewrite_context) {
		cleanup_rewrite_recip(state, hdr_opts, header_buf);
	    } else if ((hdr_opts->flags & HDR_OPT_DROP) == 0) {
		cleanup_out_header(state, header_buf);
	    }
	}
    }
}

/* cleanup_header_done_callback - insert missing message headers */

static void cleanup_header_done_callback(void *context)
{
    const char *myname = "cleanup_header_done_callback";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    char    time_stamp[1024];		/* XXX locale dependent? */
    struct tm *tp;
    TOK822 *token;
    time_t  tv;

    /*
     * XXX Workaround: when we reach the end of headers, mime_state_update()
     * may execute up to three call-backs before returning to the caller:
     * head_out(), head_end(), and body_out() or body_end(). As long as
     * call-backs don't return a result, each call-back has to check for
     * itself if the previous call-back experienced a problem.
     */
    if (CLEANUP_OUT_OK(state) == 0)
	return;

    /*
     * Add a missing (Resent-)Message-Id: header. The message ID gives the
     * time in GMT units, plus the local queue ID.
     * 
     * XXX Message-Id is not a required message header (RFC 822 and RFC 2822).
     * 
     * XXX It is the queue ID non-inode bits that prevent messages from getting
     * the same Message-Id within the same second.
     * 
     * XXX An arbitrary amount of time may pass between the start of the mail
     * transaction and the creation of a queue file. Since we guarantee queue
     * ID uniqueness only within a second, we must ensure that the time in
     * the message ID matches the queue ID creation time, as long as we use
     * the queue ID in the message ID.
     * 
     * XXX We log a dummy name=value record so that we (hopefully) don't break
     * compatibility with existing logfile analyzers, and so that we don't
     * complicate future code that wants to log more name=value attributes.
     */
    if ((state->hdr_rewrite_context || var_always_add_hdrs)
	&& (state->headers_seen & (1 << (state->resent[0] ?
			   HDR_RESENT_MESSAGE_ID : HDR_MESSAGE_ID))) == 0) {
	tv = state->handle->ctime.tv_sec;
	tp = gmtime(&tv);
	strftime(time_stamp, sizeof(time_stamp), "%Y%m%d%H%M%S", tp);
	cleanup_out_format(state, REC_TYPE_NORM, "%sMessage-Id: <%s.%s@%s>",
		state->resent, time_stamp, state->queue_id, var_myhostname);
	msg_info("%s: %smessage-id=<%s.%s@%s>",
		 state->queue_id, *state->resent ? "resent-" : "",
		 time_stamp, state->queue_id, var_myhostname);
	state->headers_seen |= (1 << (state->resent[0] ?
				   HDR_RESENT_MESSAGE_ID : HDR_MESSAGE_ID));
    }
    if ((state->headers_seen & (1 << HDR_MESSAGE_ID)) == 0)
	msg_info("%s: message-id=<>", state->queue_id);

    /*
     * Add a missing (Resent-)Date: header. The date is in local time units,
     * with the GMT offset at the end.
     */
    if ((state->hdr_rewrite_context || var_always_add_hdrs)
	&& (state->headers_seen & (1 << (state->resent[0] ?
				       HDR_RESENT_DATE : HDR_DATE))) == 0) {
	cleanup_out_format(state, REC_TYPE_NORM, "%sDate: %s",
		      state->resent, mail_date(state->arrival_time.tv_sec));
    }

    /*
     * Add a missing (Resent-)From: header.
     */
    if ((state->hdr_rewrite_context || var_always_add_hdrs)
	&& (state->headers_seen & (1 << (state->resent[0] ?
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
     * XXX 2821: Appendix B: The return address in the MAIL command SHOULD,
     * if possible, be derived from the system's identity for the submitting
     * (local) user, and the "From:" header field otherwise. If there is a
     * system identity available, it SHOULD also be copied to the Sender
     * header field if it is different from the address in the From header
     * field.  (Any Sender field that was already there SHOULD be removed.)
     * Similar wording appears in RFC 2822 section 3.6.2.
     * 
     * Postfix presently does not insert a Sender: header if envelope and From:
     * address differ. Older Postfix versions assumed that the envelope
     * sender address specifies the system identity and inserted Sender:
     * whenever envelope and From: differed. This was wrong with relayed
     * mail, and was often not even desirable with original submissions.
     * 
     * XXX 2822 Section 3.6.2, as well as RFC 822 Section 4.1: FROM headers can
     * contain multiple addresses. If this is the case, then a Sender: header
     * must be provided with a single address.
     * 
     * Postfix does not count the number of addresses in a From: header
     * (although doing so is trivial, once the address is parsed).
     */

    /*
     * Add a missing destination header.
     */
#define VISIBLE_RCPT	((1 << HDR_TO) | (1 << HDR_RESENT_TO) \
			| (1 << HDR_CC) | (1 << HDR_RESENT_CC))

    if ((state->hdr_rewrite_context || var_always_add_hdrs)
	&& (state->headers_seen & VISIBLE_RCPT) == 0 && *var_rcpt_witheld) {
	if (!is_header(var_rcpt_witheld)) {
	    msg_warn("bad %s header text \"%s\" -- "
		     "need \"headername: headervalue\"",
		     VAR_RCPT_WITHELD, var_rcpt_witheld);
	} else {
	    cleanup_out_format(state, REC_TYPE_NORM, "%s", var_rcpt_witheld);
	}
    }

    /*
     * Place a dummy PTR record right after the last header so that we can
     * append headers without having to worry about clobbering the
     * end-of-content marker.
     */
    if (state->milters || cleanup_milters) {
	if ((state->append_hdr_pt_offset = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT, 0L);
	if ((state->append_hdr_pt_target = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	state->body_offset = state->append_hdr_pt_target;
    }
}

/* cleanup_body_callback - output one body record */

static void cleanup_body_callback(void *context, int type,
				          const char *buf, ssize_t len,
				          off_t offset)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;

    /*
     * XXX Workaround: when we reach the end of headers, mime_state_update()
     * may execute up to three call-backs before returning to the caller:
     * head_out(), head_end(), and body_out() or body_end(). As long as
     * call-backs don't return a result, each call-back has to check for
     * itself if the previous call-back experienced a problem.
     */
    if (CLEANUP_OUT_OK(state) == 0)
	return;

    /*
     * Crude message body content filter for emergencies. This code has
     * several problems: it sees one line at a time; it looks at long lines
     * only in chunks of line_length_limit (2048) characters; it is easily
     * bypassed with encodings and other tricks.
     */
    if ((state->flags & CLEANUP_FLAG_FILTER)
	&& cleanup_body_checks
	&& (var_body_check_len == 0 || offset < var_body_check_len)) {
	const char *value;

	if ((value = maps_find(cleanup_body_checks, buf, 0)) != 0) {
	    const char *result;

	    if ((result = cleanup_act(state, CLEANUP_ACT_CTXT_BODY,
				      buf, value, VAR_BODY_CHECKS))
		== CLEANUP_ACT_DROP) {
		return;
	    } else if (result != buf) {
		cleanup_out(state, type, result, strlen(result));
		myfree((char *) result);
		return;
	    }
	}
    }
    cleanup_out(state, type, buf, len);
}

/* cleanup_message_headerbody - process message content, header and body */

static void cleanup_message_headerbody(CLEANUP_STATE *state, int type,
				               const char *buf, ssize_t len)
{
    const char *myname = "cleanup_message_headerbody";
    const MIME_STATE_DETAIL *detail;
    const char *cp;
    char   *dst;

    /*
     * Reject unwanted characters.
     * 
     * XXX Possible optimization: simplify the loop when the "reject" set
     * contains only one character.
     */
    if ((state->flags & CLEANUP_FLAG_FILTER) && cleanup_reject_chars) {
	for (cp = buf; cp < buf + len; cp++) {
	    if (memchr(vstring_str(cleanup_reject_chars),
		       *(const unsigned char *) cp,
		       VSTRING_LEN(cleanup_reject_chars))) {
		cleanup_act(state, CLEANUP_ACT_CTXT_ANY,
			    buf, "REJECT disallowed character",
			    "character reject");
		return;
	    }
	}
    }

    /*
     * Strip unwanted characters. Don't overwrite the input.
     * 
     * XXX Possible optimization: simplify the loop when the "strip" set
     * contains only one character.
     * 
     * XXX Possible optimization: copy the input only if we really have to.
     */
    if ((state->flags & CLEANUP_FLAG_FILTER) && cleanup_strip_chars) {
	VSTRING_RESET(state->stripped_buf);
	VSTRING_SPACE(state->stripped_buf, len + 1);
	dst = vstring_str(state->stripped_buf);
	for (cp = buf; cp < buf + len; cp++)
	    if (!memchr(vstring_str(cleanup_strip_chars),
			*(const unsigned char *) cp,
			VSTRING_LEN(cleanup_strip_chars)))
		*dst++ = *cp;
	*dst = 0;
	buf = vstring_str(state->stripped_buf);
	len = dst - buf;
    }

    /*
     * Copy text record to the output.
     */
    if (type == REC_TYPE_NORM || type == REC_TYPE_CONT) {
	state->mime_errs = mime_state_update(state->mime_state, type, buf, len);
    }

    /*
     * If we have reached the end of the message content segment, record the
     * current file position so we can compute the message size lateron.
     */
    else if (type == REC_TYPE_XTRA) {
	state->mime_errs = mime_state_update(state->mime_state, type, buf, len);
	if (state->milters || cleanup_milters)
	    /* Make room for body modification. */
	    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT, 0L);
	/* Ignore header truncation after primary message headers. */
	state->mime_errs &= ~MIME_ERR_TRUNC_HEADER;
	if (state->mime_errs && state->reason == 0) {
	    state->errs |= CLEANUP_STAT_CONT;
	    detail = mime_state_detail(state->mime_errs);
	    state->reason = dsn_prepend(detail->dsn, detail->text);
	}
	state->mime_state = mime_state_free(state->mime_state);
	if ((state->xtra_offset = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	state->cont_length = state->xtra_offset - state->data_offset;
	state->action = cleanup_extracted;
    }

    /*
     * This should never happen.
     */
    else {
	msg_warn("%s: message rejected: "
	      "unexpected record type %d in message content", myname, type);
	state->errs |= CLEANUP_STAT_BAD;
    }
}

/* cleanup_mime_error_callback - error report call-back routine */

static void cleanup_mime_error_callback(void *context, int err_code,
				              const char *text, ssize_t len)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    const char *origin;

    /*
     * Message header too large errors are handled after the end of the
     * primary message headers.
     */
    if ((err_code & ~MIME_ERR_TRUNC_HEADER) != 0) {
	if ((origin = nvtable_find(state->attr, MAIL_ATTR_LOG_ORIGIN)) == 0)
	    origin = MAIL_ATTR_ORG_NONE;
#define TEXT_LEN (len < 100 ? (int) len : 100)
	msg_info("%s: reject: mime-error %s: %.*s from %s; from=<%s> to=<%s>",
		 state->queue_id, mime_state_error(err_code), TEXT_LEN, text,
	    origin, state->sender, state->recip ? state->recip : "unknown");
    }
}

/* cleanup_message - initialize message content segment */

void    cleanup_message(CLEANUP_STATE *state, int type, const char *buf, ssize_t len)
{
    const char *myname = "cleanup_message";
    int     mime_options;

    /*
     * Write the start-of-content segment marker.
     */
    cleanup_out_string(state, REC_TYPE_MESG, "");
    if ((state->data_offset = vstream_ftell(state->dst)) < 0)
	msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);

    /*
     * Set up MIME processing options, if any. MIME_OPT_DISABLE_MIME disables
     * special processing of Content-Type: headers, and thus, causes all text
     * after the primary headers to be treated as the message body.
     */
    mime_options = 0;
    if (var_disable_mime_input) {
	mime_options |= MIME_OPT_DISABLE_MIME;
    } else {
	/* Turn off content checks if bouncing or forwarding mail. */
	if (state->flags & CLEANUP_FLAG_FILTER) {
	    if (var_strict_8bitmime || var_strict_7bit_hdrs)
		mime_options |= MIME_OPT_REPORT_8BIT_IN_HEADER;
	    if (var_strict_8bitmime || var_strict_8bit_body)
		mime_options |= MIME_OPT_REPORT_8BIT_IN_7BIT_BODY;
	    if (var_strict_encoding)
		mime_options |= MIME_OPT_REPORT_ENCODING_DOMAIN;
	    if (var_strict_8bitmime || var_strict_7bit_hdrs
		|| var_strict_8bit_body || var_strict_encoding
		|| *var_header_checks || *var_mimehdr_checks
		|| *var_nesthdr_checks)
		mime_options |= MIME_OPT_REPORT_NESTING;
	}
    }
    state->mime_state = mime_state_alloc(mime_options,
					 cleanup_header_callback,
					 cleanup_header_done_callback,
					 cleanup_body_callback,
					 (MIME_STATE_ANY_END) 0,
					 cleanup_mime_error_callback,
					 (void *) state);

    /*
     * XXX Workaround: truncate a long message header so that we don't exceed
     * the default Sendmail libmilter request size limit of 65535.
     */
#define KLUDGE_HEADER_LIMIT	60000
    if ((cleanup_milters || state->milters)
	&& var_header_limit > KLUDGE_HEADER_LIMIT)
	var_header_limit = KLUDGE_HEADER_LIMIT;

    /*
     * Pass control to the header processing routine.
     */
    state->action = cleanup_message_headerbody;
    cleanup_message_headerbody(state, type, buf, len);
}
