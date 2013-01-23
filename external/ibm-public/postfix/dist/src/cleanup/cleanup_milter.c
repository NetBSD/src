/*	$NetBSD: cleanup_milter.c,v 1.1.1.5.2.2 2013/01/23 00:05:00 yamt Exp $	*/

/*++
/* NAME
/*	cleanup_milter 3
/* SUMMARY
/*	external mail filter support
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_milter_receive(state, count)
/*	CLEANUP_STATE *state;
/*	int	count;
/*
/*	void	cleanup_milter_inspect(state, milters)
/*	CLEANUP_STATE *state;
/*	MILTERS	*milters;
/*
/*	cleanup_milter_emul_mail(state, milters, sender)
/*	CLEANUP_STATE *state;
/*	MILTERS	*milters;
/*	const char *sender;
/*
/*	cleanup_milter_emul_rcpt(state, milters, recipient)
/*	CLEANUP_STATE *state;
/*	MILTERS	*milters;
/*	const char *recipient;
/*
/*	cleanup_milter_emul_data(state, milters)
/*	CLEANUP_STATE *state;
/*	MILTERS	*milters;
/* DESCRIPTION
/*	This module implements support for Sendmail-style mail
/*	filter (milter) applications, including in-place queue file
/*	modification.
/*
/*	cleanup_milter_receive() receives mail filter definitions,
/*	typically from an smtpd(8) server process, and registers
/*	local call-back functions for macro expansion and for queue
/*	file modification.
/*
/*	cleanup_milter_inspect() sends the current message headers
/*	and body to the mail filters that were received with
/*	cleanup_milter_receive(), or that are specified with the
/*	cleanup_milters configuration parameter.
/*
/*	cleanup_milter_emul_mail() emulates connect, helo and mail
/*	events for mail that does not arrive via the smtpd(8) server.
/*	The emulation pretends that mail arrives from localhost/127.0.0.1
/*	via ESMTP. Milters can reject emulated connect, helo, mail
/*	or data events, but not emulated rcpt events as described
/*	next.
/*
/*	cleanup_milter_emul_rcpt() emulates an rcpt event for mail
/*	that does not arrive via the smtpd(8) server. This reports
/*	a server configuration error condition when the milter
/*	rejects an emulated rcpt event.
/*
/*	cleanup_milter_emul_data() emulates a data event for mail
/*	that does not arrive via the smtpd(8) server.  It's OK for
/*	milters to reject emulated data events.
/* SEE ALSO
/*	milter(3) generic mail filter interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/*	Panic: interface violation.
/*	Warnings: I/O errors (state->errs is updated accordingly).
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
#include <sys/socket.h>			/* AF_INET */
#include <string.h>
#include <errno.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>

/* Global library. */

#include <off_cvt.h>
#include <dsn_mask.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <record.h>
#include <rec_attr_map.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <lex_822.h>
#include <is_header.h>
#include <quote_821_local.h>
#include <dsn_util.h>

/* Application-specific. */

#include <cleanup.h>

 /*
  * How Postfix 2.4 edits queue file information:
  * 
  * Mail filter applications (Milters) can send modification requests after
  * receiving the end of the message body.  Postfix implements these
  * modifications in the cleanup server, so that it can edit the queue file
  * in place. This avoids the temporary files that would be needed when
  * modifications were implemented in the SMTP server (Postfix normally does
  * not store the whole message in main memory). Once a Milter is done
  * editing, the queue file can be used as input for the next Milter, and so
  * on. Finally, the cleanup server changes file permissions, calls fsync(),
  * and waits for successful completion.
  * 
  * To implement in-place queue file edits, we need to introduce surprisingly
  * little change to the existing Postfix queue file structure.  All we need
  * is a way to mark a record as deleted, and to jump from one place in the
  * queue file to another. We could implement deleted records with jumps, but
  * marking is sometimes simpler.
  * 
  * Postfix does not store queue files as plain text files. Instead all
  * information is stored in records with an explicit type and length, for
  * sender, recipient, arrival time, and so on.  Even the content that makes
  * up the message header and body is stored as records with explicit types
  * and lengths.  This organization makes it very easy to mark a record as
  * deleted, and to introduce the pointer records that we will use to jump
  * from one place in a queue file to another place.
  * 
  * - Deleting a recipient is easiest - simply modify the record type into one
  * that is skipped by the software that delivers mail. We won't try to reuse
  * the deleted recipient for other purposes. When deleting a recipient, we
  * may need to delete multiple recipient records that result from virtual
  * alias expansion of the original recipient address.
  * 
  * - Replacing a header record involves pointer records. A record is replaced
  * by overwriting it with a forward pointer to space after the end of the
  * queue file, putting the new record there, followed by a reverse pointer
  * to the record that follows the replaced header. To simplify
  * implementation we follow a short header record with a filler record so
  * that we can always overwrite a header record with a pointer.
  * 
  * N.B. This is a major difference with Postfix version 2.3, which needed
  * complex code to save records that follow a short header, before it could
  * overwrite a short header record. This code contained two of the three
  * post-release bugs that were found with Postfix header editing.
  * 
  * - Inserting a header record is like replacing one, except that we also
  * relocate the record that is being overwritten by the forward pointer.
  * 
  * - Deleting a message header is simplest when we replace it by a "skip"
  * pointer to the information that follows the header. With a multi-line
  * header we need to update only the first line.
  * 
  * - Appending a recipient or header record involves pointer records as well.
  * To make this convenient, the queue file already contains dummy pointer
  * records at the locations where we want to append recipient or header
  * content. To append, change the dummy pointer into a forward pointer to
  * space after the end of a message, put the new recipient or header record
  * there, followed by a reverse pointer to the record that follows the
  * forward pointer.
  * 
  * - To append another header or recipient record, replace the reverse pointer
  * by a forward pointer to space after the end of a message, put the new
  * record there, followed by the value of the reverse pointer that we
  * replace. Thus, there is no one-to-one correspondence between forward and
  * backward pointers. Instead, there can be multiple forward pointers for
  * one reverse pointer.
  * 
  * - When a mail filter wants to replace an entire body, we overwrite existing
  * body records until we run out of space, and then write a pointer to space
  * after the end of the queue file, followed by more body content. There may
  * be multiple regions with body content; regions are connected by forward
  * pointers, and the last region ends with a pointer to the marker that ends
  * the message content segment. Body regions can be large and therefore they
  * are reused to avoid wasting space. Sendmail mail filters currently do not
  * replace individual body records, and that is a good thing.
  * 
  * Making queue file modifications safe:
  * 
  * Postfix queue files are segmented. The first segment is for envelope
  * records, the second for message header and body content, and the third
  * segment is for information that was extracted or generated from the
  * message header or body content.  Each segment is terminated by a marker
  * record. For now we don't want to change their location. That is, we want
  * to avoid moving the records that mark the start or end of a queue file
  * segment.
  * 
  * To ensure that we can always replace a header or body record by a pointer
  * record, without having to relocate a marker record, the cleanup server
  * places a dummy pointer record at the end of the recipients and at the end
  * of the message header. To support message body modifications, a dummy
  * pointer record is also placed at the end of the message content.
  * 
  * With all these changes in queue file organization, REC_TYPE_END is no longer
  * guaranteed to be the last record in a queue file. If an application were
  * to read beyond the REC_TYPE_END marker, it would go into an infinite
  * loop, because records after REC_TYPE_END alternate with reverse pointers
  * to the middle of the queue file. For robustness, the record reading
  * routine skips forward to the end-of-file position after reading the
  * REC_TYPE_END marker.
  */

/*#define msg_verbose	2*/

static void cleanup_milter_set_error(CLEANUP_STATE *, int);

#define STR(x)		vstring_str(x)
#define LEN(x)		VSTRING_LEN(x)

/* cleanup_milter_hbc_log - log post-milter header/body_checks action */

static void cleanup_milter_hbc_log(void *context, const char *action,
				        const char *where, const char *line,
				           const char *optional_text)
{
    const CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    const char *attr;

    vstring_sprintf(state->temp1, "%s: milter-%s-%s: %s %.60s from %s[%s];",
		    state->queue_id, where, action, where, line,
		    state->client_name, state->client_addr);
    if (state->sender)
	vstring_sprintf_append(state->temp1, " from=<%s>", state->sender);
    if (state->recip)
	vstring_sprintf_append(state->temp1, " to=<%s>", state->recip);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_PROTO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " proto=%s", attr);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_HELO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " helo=<%s>", attr);
    if (optional_text)
	vstring_sprintf_append(state->temp1, ": %s", optional_text);
    msg_info("%s", vstring_str(state->temp1));
}

/* cleanup_milter_header_prepend - prepend header to milter-generated header */

static void cleanup_milter_header_prepend(void *context, int rec_type,
			         const char *buf, ssize_t len, off_t offset)
{
    /* XXX save prepended header to buffer. */
    msg_warn("the milter_header/body_checks prepend action is not implemented");
}

/* cleanup_milter_hbc_extend - additional header/body_checks actions */

static char *cleanup_milter_hbc_extend(void *context, const char *command,
			             int cmd_len, const char *optional_text,
				         const char *where, const char *buf,
				               ssize_t buf_len, off_t offset)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    const char *map_class = VAR_MILT_HEAD_CHECKS;	/* XXX */

#define STREQUAL(x,y,l) (strncasecmp((x), (y), (l)) == 0 && (y)[l] == 0)

    /*
     * These are currently our mutually-exclusive ways of not receiving mail:
     * "reject" and "discard". Only these can be reported to the up-stream
     * Postfix libmilter code, because sending any reply there causes Postfix
     * libmilter to skip further "edit" requests. By way of safety net, each
     * of these must also reset CLEANUP_FLAG_FILTER_ALL.
     */
#define CLEANUP_MILTER_REJECTING_OR_DISCARDING_MESSAGE(state) \
    ((state->flags & CLEANUP_FLAG_DISCARD) || (state->errs & CLEANUP_STAT_CONT))

    /*
     * We log all header/body-checks actions here, because we know the
     * details of the message content that triggered the action. We report
     * detail-free milter-reply values (reject/discard, stored in the
     * milter_hbc_reply state member) to the Postfix libmilter code, so that
     * Postfix libmilter can stop sending requests.
     * 
     * We also set all applicable cleanup flags here, because there is no
     * guarantee that Postfix libmilter will propagate our own milter-reply
     * value to cleanup_milter_inspect() which calls cleanup_milter_apply().
     * The latter translates responses from Milter applications into cleanup
     * flags, and logs the response text. Postfix libmilter can convey only
     * one milter-reply value per email message, and that reply may even come
     * from outside Postfix.
     * 
     * To suppress redundant logging, cleanup_milter_apply() does nothing when
     * the milter-reply value matches the saved text in the milter_hbc_reply
     * state member. As we remember only one milter-reply value, we can't
     * report multiple milter-reply values per email message. We satisfy this
     * constraint, because we already clear the CLEANUP_FLAG_FILTER_ALL flags
     * to terminate further header inspection.
     */
    if ((state->flags & CLEANUP_FLAG_FILTER_ALL) == 0)
	return ((char *) buf);

    if (STREQUAL(command, "REJECT", cmd_len)) {
	const CLEANUP_STAT_DETAIL *detail;

	if (state->reason)
	    myfree(state->reason);
	detail = cleanup_stat_detail(CLEANUP_STAT_CONT);
	if (*optional_text) {
	    state->reason = dsn_prepend(detail->dsn, optional_text);
	    if (*state->reason != '4' && *state->reason != '5') {
		msg_warn("bad DSN action in %s -- need 4.x.x or 5.x.x",
			 optional_text);
		*state->reason = '4';
	    }
	} else {
	    state->reason = dsn_prepend(detail->dsn, detail->text);
	}
	if (*state->reason == '4')
	    state->errs |= CLEANUP_STAT_DEFER;
	else
	    state->errs |= CLEANUP_STAT_CONT;
	state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	cleanup_milter_hbc_log(context, "reject", where, buf, state->reason);
	vstring_sprintf(state->milter_hbc_reply, "%d %s",
			detail->smtp, state->reason);
	STR(state->milter_hbc_reply)[0] = *state->reason;
	return ((char *) buf);
    }
    if (STREQUAL(command, "FILTER", cmd_len)) {
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
	    cleanup_milter_hbc_log(context, "filter", where, buf,
				   optional_text);
	}
	return ((char *) buf);
    }
    if (STREQUAL(command, "DISCARD", cmd_len)) {
	cleanup_milter_hbc_log(context, "discard", where, buf, optional_text);
	vstring_strcpy(state->milter_hbc_reply, "D");
	state->flags |= CLEANUP_FLAG_DISCARD;
	state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	return ((char *) buf);
    }
    if (STREQUAL(command, "HOLD", cmd_len)) {
	if ((state->flags & (CLEANUP_FLAG_HOLD | CLEANUP_FLAG_DISCARD)) == 0) {
	    cleanup_milter_hbc_log(context, "hold", where, buf, optional_text);
	    state->flags |= CLEANUP_FLAG_HOLD;
	}
	return ((char *) buf);
    }
    if (STREQUAL(command, "REDIRECT", cmd_len)) {
	if (strchr(optional_text, '@') == 0) {
	    msg_warn("bad REDIRECT target \"%s\" in %s map -- "
		     "need user@domain",
		     optional_text, map_class);
	} else {
	    if (state->redirect)
		myfree(state->redirect);
	    state->redirect = mystrdup(optional_text);
	    cleanup_milter_hbc_log(context, "redirect", where, buf,
				   optional_text);
	    state->flags &= ~CLEANUP_FLAG_FILTER_ALL;
	}
	return ((char *) buf);
    }
    return ((char *) HBC_CHECKS_STAT_UNKNOWN);
}

/* cleanup_milter_header_checks - inspect Milter-generated header */

static int cleanup_milter_header_checks(CLEANUP_STATE *state, VSTRING *buf)
{
    char   *ret;

    /*
     * Milter application "add/insert/replace header" requests happen at the
     * end-of-message stage, therefore all the header operations are relative
     * to the primary message header.
     */
    ret = hbc_header_checks((void *) state, state->milter_hbc_checks,
			    MIME_HDR_PRIMARY, (HEADER_OPTS *) 0,
			    buf, (off_t) 0);
    if (ret == 0) {
	return (0);
    } else if (ret == HBC_CHECKS_STAT_ERROR) {
	msg_warn("%s: %s lookup error -- deferring delivery",
		 state->queue_id, VAR_MILT_HEAD_CHECKS);
	state->errs |= CLEANUP_STAT_WRITE;
	return (0);
    } else {
	if (ret != STR(buf)) {
	    vstring_strcpy(buf, ret);
	    myfree(ret);
	}
	return (1);
    }
}

/* cleanup_milter_hbc_add_meta_records - add REDIRECT or FILTER meta records */

static void cleanup_milter_hbc_add_meta_records(CLEANUP_STATE *state)
{
    const char *myname = "cleanup_milter_hbc_add_meta_records";
    off_t   reverse_ptr_offset;
    off_t   new_meta_offset;

    /*
     * Note: this code runs while the Milter infrastructure is being torn
     * down. For this reason we handle all I/O errors here on the spot,
     * instead of reporting them back through the Milter infrastructure.
     */

    /*
     * Sanity check.
     */
    if (state->append_meta_pt_offset < 0)
	msg_panic("%s: no meta append pointer location", myname);
    if (state->append_meta_pt_target < 0)
	msg_panic("%s: no meta append pointer target", myname);

    /*
     * Allocate space after the end of the queue file, and write the meta
     * record(s), followed by a reverse pointer record that points to the
     * target of the old "meta record append" pointer record. This reverse
     * pointer record becomes the new "meta record append" pointer record.
     * Although the new "meta record append" pointer record will never be
     * used, we update it here to make the code more similar to other code
     * that inserts/appends content, so that common code can be factored out
     * later.
     */
    if ((new_meta_offset = vstream_fseek(state->dst, (off_t) 0, SEEK_END)) < 0) {
	cleanup_milter_set_error(state, errno);
	return;
    }
    if (state->filter != 0)
	cleanup_out_string(state, REC_TYPE_FILT, state->filter);
    if (state->redirect != 0)
	cleanup_out_string(state, REC_TYPE_RDR, state->redirect);
    if ((reverse_ptr_offset = vstream_ftell(state->dst)) < 0) {
	msg_warn("%s: vstream_ftell file %s: %m", myname, cleanup_path);
	state->errs |= CLEANUP_STAT_WRITE;
	return;
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) state->append_meta_pt_target);

    /*
     * Pointer flipping: update the old "meta record append" pointer record
     * value with the location of the new meta record.
     */
    if (vstream_fseek(state->dst, state->append_meta_pt_offset, SEEK_SET) < 0) {
	cleanup_milter_set_error(state, errno);
	return;
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) new_meta_offset);

    /*
     * Update the in-memory "meta append" pointer record location with the
     * location of the reverse pointer record that follows the new meta
     * record. The target of the "meta append" pointer record does not
     * change; it's always the record that follows the dummy pointer record
     * that was written while Postfix received the message.
     */
    state->append_meta_pt_offset = reverse_ptr_offset;

    /*
     * Note: state->append_meta_pt_target never changes.
     */
}

/* cleanup_milter_header_checks_init - initialize post-Milter header checks */

static void cleanup_milter_header_checks_init(CLEANUP_STATE *state)
{
#define NO_NESTED_HDR_NAME	""
#define NO_NESTED_HDR_VALUE	""
#define NO_MIME_HDR_NAME	""
#define NO_MIME_HDR_VALUE	""

    static /* XXX not const */ HBC_CALL_BACKS call_backs = {
	cleanup_milter_hbc_log,
	cleanup_milter_header_prepend,
	cleanup_milter_hbc_extend,
    };

    state->milter_hbc_checks =
	hbc_header_checks_create(VAR_MILT_HEAD_CHECKS, var_milt_head_checks,
				 NO_MIME_HDR_NAME, NO_MIME_HDR_VALUE,
				 NO_NESTED_HDR_NAME, NO_NESTED_HDR_VALUE,
				 &call_backs);
    state->milter_hbc_reply = vstring_alloc(100);
    if (state->filter)
	myfree(state->filter);
    state->filter = 0;
    if (state->redirect)
	myfree(state->redirect);
    state->redirect = 0;
}

/* cleanup_milter_hbc_finish - finalize post-Milter header checks */

static void cleanup_milter_hbc_finish(CLEANUP_STATE *state)
{
    if (state->milter_hbc_checks)
	hbc_header_checks_free(state->milter_hbc_checks);
    state->milter_hbc_checks = 0;
    if (state->milter_hbc_reply)
	vstring_free(state->milter_hbc_reply);
    state->milter_hbc_reply = 0;
    if (CLEANUP_OUT_OK(state)
	&& !CLEANUP_MILTER_REJECTING_OR_DISCARDING_MESSAGE(state)
	&& (state->filter || state->redirect))
	cleanup_milter_hbc_add_meta_records(state);
}

 /*
  * Milter replies.
  */
#define CLEANUP_MILTER_SET_REASON(__state, __reason) do { \
	if ((__state)->reason) \
	    myfree((__state)->reason); \
	(__state)->reason = mystrdup(__reason); \
	if ((__state)->smtp_reply) { \
	    myfree((__state)->smtp_reply); \
	    (__state)->smtp_reply = 0; \
	} \
    } while (0)

#define CLEANUP_MILTER_SET_SMTP_REPLY(__state, __smtp_reply) do { \
	if ((__state)->reason) \
	    myfree((__state)->reason); \
	(__state)->reason = mystrdup(__smtp_reply + 4); \
	printable((__state)->reason, '_'); \
	if ((__state)->smtp_reply) \
	    myfree((__state)->smtp_reply); \
	(__state)->smtp_reply = mystrdup(__smtp_reply); \
    } while (0)

/* cleanup_milter_set_error - set error flag from errno */

static void cleanup_milter_set_error(CLEANUP_STATE *state, int err)
{
    if (err == EFBIG) {
	msg_warn("%s: queue file size limit exceeded", state->queue_id);
	state->errs |= CLEANUP_STAT_SIZE;
    } else {
	msg_warn("%s: write queue file: %m", state->queue_id);
	state->errs |= CLEANUP_STAT_WRITE;
    }
}

/* cleanup_milter_error - return dummy error description */

static const char *cleanup_milter_error(CLEANUP_STATE *state, int err)
{
    const char *myname = "cleanup_milter_error";
    const CLEANUP_STAT_DETAIL *dp;

    /*
     * For consistency with error reporting within the milter infrastructure,
     * content manipulation routines return a null pointer on success, and an
     * SMTP-like response on error.
     * 
     * However, when cleanup_milter_apply() receives this error response from
     * the milter infrastructure, it ignores the text since the appropriate
     * cleanup error flags were already set by cleanup_milter_set_error().
     * 
     * Specify a null error number when the "errno to error flag" mapping was
     * already done elsewhere, possibly outside this module.
     */
    if (err)
	cleanup_milter_set_error(state, err);
    else if (CLEANUP_OUT_OK(state))
	msg_panic("%s: missing errno to error flag mapping", myname);
    if (state->milter_err_text == 0)
	state->milter_err_text = vstring_alloc(50);
    dp = cleanup_stat_detail(state->errs);
    return (STR(vstring_sprintf(state->milter_err_text,
				"%d %s %s", dp->smtp, dp->dsn, dp->text)));
}

/* cleanup_add_header - append message header */

static const char *cleanup_add_header(void *context, const char *name,
				              const char *space,
				              const char *value)
{
    const char *myname = "cleanup_add_header";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    VSTRING *buf;
    off_t   reverse_ptr_offset;
    off_t   new_hdr_offset;

    /*
     * To simplify implementation, the cleanup server writes a dummy "header
     * append" pointer record after the last message header. We cache both
     * the location and the target of the current "header append" pointer
     * record.
     */
    if (state->append_hdr_pt_offset < 0)
	msg_panic("%s: no header append pointer location", myname);
    if (state->append_hdr_pt_target < 0)
	msg_panic("%s: no header append pointer target", myname);

    /*
     * Return early when Milter header checks request that this header record
     * be dropped.
     */
    buf = vstring_alloc(100);
    vstring_sprintf(buf, "%s:%s%s", name, space, value);
    if (state->milter_hbc_checks
	&& cleanup_milter_header_checks(state, buf) == 0) {
	vstring_free(buf);
	return (0);
    }

    /*
     * Allocate space after the end of the queue file, and write the header
     * record(s), followed by a reverse pointer record that points to the
     * target of the old "header append" pointer record. This reverse pointer
     * record becomes the new "header append" pointer record.
     */
    if ((new_hdr_offset = vstream_fseek(state->dst, (off_t) 0, SEEK_END)) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	vstring_free(buf);
	return (cleanup_milter_error(state, errno));
    }
    /* XXX emit prepended header, then clear it. */
    cleanup_out_header(state, buf);		/* Includes padding */
    vstring_free(buf);
    if ((reverse_ptr_offset = vstream_ftell(state->dst)) < 0) {
	msg_warn("%s: vstream_ftell file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) state->append_hdr_pt_target);

    /*
     * Pointer flipping: update the old "header append" pointer record value
     * with the location of the new header record.
     * 
     * XXX To avoid unnecessary seek operations when the new header immediately
     * follows the old append header pointer, write a null pointer or make
     * the record reading loop smarter. Making vstream_fseek() smarter does
     * not help, because it doesn't know if we're going to read or write
     * after a write+seek sequence.
     */
    if (vstream_fseek(state->dst, state->append_hdr_pt_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) new_hdr_offset);

    /*
     * Update the in-memory "header append" pointer record location with the
     * location of the reverse pointer record that follows the new header.
     * The target of the "header append" pointer record does not change; it's
     * always the record that follows the dummy pointer record that was
     * written while Postfix received the message.
     */
    state->append_hdr_pt_offset = reverse_ptr_offset;

    /*
     * In case of error while doing record output.
     */
    return (CLEANUP_OUT_OK(state) == 0 ? cleanup_milter_error(state, 0) :
	    state->milter_hbc_reply && LEN(state->milter_hbc_reply) ?
	    STR(state->milter_hbc_reply) : 0);

    /*
     * Note: state->append_hdr_pt_target never changes.
     */
}

/* cleanup_find_header_start - find specific header instance */

static off_t cleanup_find_header_start(CLEANUP_STATE *state, ssize_t index,
				               const char *header_label,
				               VSTRING *buf,
				               int *prec_type,
				               int allow_ptr_backup,
				               int skip_headers)
{
    const char *myname = "cleanup_find_header_start";
    off_t   curr_offset;		/* offset after found record */
    off_t   ptr_offset;			/* pointer to found record */
    VSTRING *ptr_buf = 0;
    int     rec_type = REC_TYPE_ERROR;
    int     last_type;
    ssize_t len;
    int     hdr_count = 0;

    if (msg_verbose)
	msg_info("%s: index %ld name \"%s\"",
	      myname, (long) index, header_label ? header_label : "(none)");

    /*
     * Sanity checks.
     */
    if (index < 1)
	msg_panic("%s: bad header index %ld", myname, (long) index);

    /*
     * Skip to the start of the message content, and read records until we
     * either find the specified header, or until we hit the end of the
     * headers.
     * 
     * The index specifies the header instance: 1 is the first one. The header
     * label specifies the header name. A null pointer matches any header.
     * 
     * When the specified header is not found, the result value is -1.
     * 
     * When the specified header is found, its first record is stored in the
     * caller-provided read buffer, and the result value is the queue file
     * offset of that record. The file read position is left at the start of
     * the next (non-filler) queue file record, which can be the remainder of
     * a multi-record header.
     * 
     * When a header is found and allow_ptr_backup is non-zero, then the result
     * is either the first record of that header, or it is the pointer record
     * that points to the first record of that header. In the latter case,
     * the file read position is undefined. Returning the pointer allows us
     * to do some optimizations when inserting text multiple times at the
     * same place.
     * 
     * XXX We can't use the MIME processor here. It not only buffers up the
     * input, it also reads the record that follows a complete header before
     * it invokes the header call-back action. This complicates the way that
     * we discover header offsets and boundaries. Worse is that the MIME
     * processor is unaware that multi-record message headers can have PTR
     * records in the middle.
     * 
     * XXX The draw-back of not using the MIME processor is that we have to
     * duplicate some of its logic here and in the routine that finds the end
     * of the header record. To minimize the duplication we define an ugly
     * macro that is used in all code that scans for header boundaries.
     * 
     * XXX Sendmail compatibility (based on Sendmail 8.13.6 measurements).
     * 
     * - When changing Received: header #1, we change the Received: header that
     * follows our own one; a request to change Received: header #0 is
     * silently treated as a request to change Received: header #1.
     * 
     * - When changing Date: header #1, we change the first Date: header; a
     * request to change Date: header #0 is silently treated as a request to
     * change Date: header #1.
     * 
     * Thus, header change requests are relative to the content as received,
     * that is, the content after our own Received: header. They can affect
     * only the headers that the MTA actually exposes to mail filter
     * applications.
     * 
     * - However, when inserting a header at position 0, the new header appears
     * before our own Received: header, and when inserting at position 1, the
     * new header appears after our own Received: header.
     * 
     * Thus, header insert operations are relative to the content as delivered,
     * that is, the content including our own Received: header.
     * 
     * None of the above is applicable after a Milter inserts a header before
     * our own Received: header. From then on, our own Received: header
     * becomes just like other headers.
     */
#define CLEANUP_FIND_HEADER_NOTFOUND	(-1)
#define CLEANUP_FIND_HEADER_IOERROR	(-2)

#define CLEANUP_FIND_HEADER_RETURN(offs) do { \
	if (ptr_buf) \
	    vstring_free(ptr_buf); \
	return (offs); \
    } while (0)

#define GET_NEXT_TEXT_OR_PTR_RECORD(rec_type, state, buf, curr_offset, quit) \
    if ((rec_type = rec_get_raw(state->dst, buf, 0, REC_FLAG_NONE)) < 0) { \
	msg_warn("%s: read file %s: %m", myname, cleanup_path); \
	cleanup_milter_set_error(state, errno); \
	do { quit; } while (0); \
    } \
    if (msg_verbose > 1) \
	msg_info("%s: read: %ld: %.*s", myname, (long) curr_offset, \
		 LEN(buf) > 30 ? 30 : (int) LEN(buf), STR(buf)); \
    if (rec_type == REC_TYPE_DTXT) \
	continue; \
    if (rec_type != REC_TYPE_NORM && rec_type != REC_TYPE_CONT \
	&& rec_type != REC_TYPE_PTR) \
	break;
    /* End of hairy macros. */

    if (vstream_fseek(state->dst, state->data_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	cleanup_milter_set_error(state, errno);
	CLEANUP_FIND_HEADER_RETURN(CLEANUP_FIND_HEADER_IOERROR);
    }
    for (ptr_offset = 0, last_type = 0; /* void */ ; /* void */ ) {
	if ((curr_offset = vstream_ftell(state->dst)) < 0) {
	    msg_warn("%s: vstream_ftell file %s: %m", myname, cleanup_path);
	    cleanup_milter_set_error(state, errno);
	    CLEANUP_FIND_HEADER_RETURN(CLEANUP_FIND_HEADER_IOERROR);
	}
	/* Don't follow the "append header" pointer. */
	if (curr_offset == state->append_hdr_pt_offset)
	    break;
	/* Caution: this macro terminates the loop at end-of-message. */
	/* Don't do complex processing while breaking out of this loop. */
	GET_NEXT_TEXT_OR_PTR_RECORD(rec_type, state, buf, curr_offset,
		   CLEANUP_FIND_HEADER_RETURN(CLEANUP_FIND_HEADER_IOERROR));
	/* Caution: don't assume ptr->header. This may be header-ptr->body. */
	if (rec_type == REC_TYPE_PTR) {
	    if (rec_goto(state->dst, STR(buf)) < 0) {
		msg_warn("%s: read file %s: %m", myname, cleanup_path);
		cleanup_milter_set_error(state, errno);
		CLEANUP_FIND_HEADER_RETURN(CLEANUP_FIND_HEADER_IOERROR);
	    }
	    /* Save PTR record, in case it points to the start of a header. */
	    if (allow_ptr_backup) {
		ptr_offset = curr_offset;
		if (ptr_buf == 0)
		    ptr_buf = vstring_alloc(100);
		vstring_strcpy(ptr_buf, STR(buf));
	    }
	    /* Don't update last_type; PTR can happen after REC_TYPE_CONT. */
	    continue;
	}
	/* The middle of a multi-record header. */
	else if (last_type == REC_TYPE_CONT || IS_SPACE_TAB(STR(buf)[0])) {
	    /* Reset the saved PTR record and update last_type. */
	}
	/* No more message headers. */
	else if ((len = is_header(STR(buf))) == 0) {
	    break;
	}
	/* This the start of a message header. */
	else if (hdr_count++ < skip_headers)
	     /* Reset the saved PTR record and update last_type. */ ;
	else if ((header_label == 0
		  || (strncasecmp(header_label, STR(buf), len) == 0
		      && (strlen(header_label) == len)))
		 && --index == 0) {
	    /* If we have a saved PTR record, it points to start of header. */
	    break;
	}
	ptr_offset = 0;
	last_type = rec_type;
    }

    /*
     * In case of failure, return negative start position.
     */
    if (index > 0) {
	curr_offset = CLEANUP_FIND_HEADER_NOTFOUND;
    } else {

	/*
	 * Skip over short-header padding, so that the file read pointer is
	 * always positioned at the first non-padding record after the header
	 * record. Insist on padding after short a header record, so that a
	 * short header record can safely be overwritten by a pointer record.
	 */
	if (LEN(buf) < REC_TYPE_PTR_PAYL_SIZE) {
	    VSTRING *rbuf = (ptr_offset ? buf :
			     (ptr_buf ? ptr_buf :
			      (ptr_buf = vstring_alloc(100))));
	    int     rval;

	    if ((rval = rec_get_raw(state->dst, rbuf, 0, REC_FLAG_NONE)) < 0) {
		cleanup_milter_set_error(state, errno);
		CLEANUP_FIND_HEADER_RETURN(CLEANUP_FIND_HEADER_IOERROR);
	    }
	    if (rval != REC_TYPE_DTXT)
		msg_panic("%s: short header without padding", myname);
	}

	/*
	 * Optionally return a pointer to the message header, instead of the
	 * start of the message header itself. In that case the file read
	 * position is undefined (actually it is at the first non-padding
	 * record that follows the message header record).
	 */
	if (ptr_offset != 0) {
	    rec_type = REC_TYPE_PTR;
	    curr_offset = ptr_offset;
	    vstring_strcpy(buf, STR(ptr_buf));
	}
	*prec_type = rec_type;
    }
    if (msg_verbose)
	msg_info("%s: index %ld name %s type %d offset %ld",
		 myname, (long) index, header_label ?
		 header_label : "(none)", rec_type, (long) curr_offset);

    CLEANUP_FIND_HEADER_RETURN(curr_offset);
}

/* cleanup_find_header_end - find end of header */

static off_t cleanup_find_header_end(CLEANUP_STATE *state,
				             VSTRING *rec_buf,
				             int last_type)
{
    const char *myname = "cleanup_find_header_end";
    off_t   read_offset;
    int     rec_type;

    /*
     * This routine is called immediately after cleanup_find_header_start().
     * rec_buf is the cleanup_find_header_start() result record; last_type is
     * the corresponding record type: REC_TYPE_PTR or REC_TYPE_NORM; the file
     * read position is at the first non-padding record after the result
     * header record.
     */
    for (;;) {
	if ((read_offset = vstream_ftell(state->dst)) < 0) {
	    msg_warn("%s: read file %s: %m", myname, cleanup_path);
	    cleanup_milter_error(state, errno);
	    return (-1);
	}
	/* Don't follow the "append header" pointer. */
	if (read_offset == state->append_hdr_pt_offset)
	    break;
	/* Caution: this macro terminates the loop at end-of-message. */
	/* Don't do complex processing while breaking out of this loop. */
	GET_NEXT_TEXT_OR_PTR_RECORD(rec_type, state, rec_buf, read_offset,
	/* Warning and errno->error mapping are done elsewhere. */
				    return (-1));
	if (rec_type == REC_TYPE_PTR) {
	    if (rec_goto(state->dst, STR(rec_buf)) < 0) {
		msg_warn("%s: read file %s: %m", myname, cleanup_path);
		cleanup_milter_error(state, errno);
		return (-1);
	    }
	    /* Don't update last_type; PTR may follow REC_TYPE_CONT. */
	    continue;
	}
	/* Start of header or message body. */
	if (last_type != REC_TYPE_CONT && !IS_SPACE_TAB(STR(rec_buf)[0]))
	    break;
	last_type = rec_type;
    }
    return (read_offset);
}

/* cleanup_patch_header - patch new header into an existing header */

static const char *cleanup_patch_header(CLEANUP_STATE *state,
					        const char *new_hdr_name,
					        const char *hdr_space,
					        const char *new_hdr_value,
					        off_t old_rec_offset,
					        int old_rec_type,
					        VSTRING *old_rec_buf,
					        off_t next_offset)
{
    const char *myname = "cleanup_patch_header";
    VSTRING *buf = vstring_alloc(100);
    off_t   new_hdr_offset;

#define CLEANUP_PATCH_HEADER_RETURN(ret) do { \
	vstring_free(buf); \
	return (ret); \
    } while (0)

    if (msg_verbose)
	msg_info("%s: \"%s\" \"%s\" at %ld",
		 myname, new_hdr_name, new_hdr_value, (long) old_rec_offset);

    /*
     * Allocate space after the end of the queue file for the new header and
     * optionally save an existing record to make room for a forward pointer
     * record. If the saved record was not a PTR record, follow the saved
     * record by a reverse pointer record that points to the record after the
     * original location of the saved record.
     * 
     * We update the queue file in a safe manner: save the new header and the
     * existing records after the end of the queue file, write the reverse
     * pointer, and only then overwrite the saved records with the forward
     * pointer to the new header.
     * 
     * old_rec_offset, old_rec_type, and old_rec_buf specify the record that we
     * are about to overwrite with a pointer record. If the record needs to
     * be saved (i.e. old_rec_type > 0), the buffer contains the data content
     * of exactly one PTR or text record.
     * 
     * next_offset specifies the record that follows the to-be-overwritten
     * record. It is ignored when the to-be-saved record is a pointer record.
     */

    /*
     * Return early when Milter header checks request that this header record
     * be dropped.
     */
    vstring_sprintf(buf, "%s:%s%s", new_hdr_name, hdr_space, new_hdr_value);
    if (state->milter_hbc_checks
	&& cleanup_milter_header_checks(state, buf) == 0)
	CLEANUP_PATCH_HEADER_RETURN(0);

    /*
     * Write the new header to a new location after the end of the queue
     * file.
     */
    if ((new_hdr_offset = vstream_fseek(state->dst, (off_t) 0, SEEK_END)) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	CLEANUP_PATCH_HEADER_RETURN(cleanup_milter_error(state, errno));
    }
    /* XXX emit prepended header, then clear it. */
    cleanup_out_header(state, buf);		/* Includes padding */
    if (msg_verbose > 1)
	msg_info("%s: %ld: write %.*s", myname, (long) new_hdr_offset,
		 LEN(buf) > 30 ? 30 : (int) LEN(buf), STR(buf));

    /*
     * Optionally, save the existing text record or pointer record that will
     * be overwritten with the forward pointer. Pad a short saved record to
     * ensure that it, too, can be overwritten by a pointer.
     */
    if (old_rec_type > 0) {
	CLEANUP_OUT_BUF(state, old_rec_type, old_rec_buf);
	if (LEN(old_rec_buf) < REC_TYPE_PTR_PAYL_SIZE)
	    rec_pad(state->dst, REC_TYPE_DTXT,
		    REC_TYPE_PTR_PAYL_SIZE - LEN(old_rec_buf));
	if (msg_verbose > 1)
	    msg_info("%s: write %.*s", myname, LEN(old_rec_buf) > 30 ?
		     30 : (int) LEN(old_rec_buf), STR(old_rec_buf));
    }

    /*
     * If the saved record wasn't a PTR record, write the reverse pointer
     * after the saved records. A reverse pointer value of -1 means we were
     * confused about what we were going to save.
     */
    if (old_rec_type != REC_TYPE_PTR) {
	if (next_offset < 0)
	    msg_panic("%s: bad reverse pointer %ld",
		      myname, (long) next_offset);
	cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
			   (long) next_offset);
	if (msg_verbose > 1)
	    msg_info("%s: write PTR %ld", myname, (long) next_offset);
    }

    /*
     * Write the forward pointer over the old record. Generally, a pointer
     * record will be shorter than a header record, so there will be a gap in
     * the queue file before the next record. In other words, we must always
     * follow pointer records otherwise we get out of sync with the data.
     */
    if (vstream_fseek(state->dst, old_rec_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	CLEANUP_PATCH_HEADER_RETURN(cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) new_hdr_offset);
    if (msg_verbose > 1)
	msg_info("%s: %ld: write PTR %ld", myname, (long) old_rec_offset,
		 (long) new_hdr_offset);

    /*
     * In case of error while doing record output.
     */
    CLEANUP_PATCH_HEADER_RETURN(
	       CLEANUP_OUT_OK(state) == 0 ? cleanup_milter_error(state, 0) :
		   state->milter_hbc_reply && LEN(state->milter_hbc_reply) ?
				STR(state->milter_hbc_reply) : 0);

    /*
     * Note: state->append_hdr_pt_target never changes.
     */
}

/* cleanup_ins_header - insert message header */

static const char *cleanup_ins_header(void *context, ssize_t index,
				              const char *new_hdr_name,
				              const char *hdr_space,
				              const char *new_hdr_value)
{
    const char *myname = "cleanup_ins_header";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    VSTRING *old_rec_buf = vstring_alloc(100);
    off_t   old_rec_offset;
    int     old_rec_type;
    off_t   next_offset;
    const char *ret;

#define CLEANUP_INS_HEADER_RETURN(ret) do { \
	vstring_free(old_rec_buf); \
	return (ret); \
    } while (0)

    if (msg_verbose)
	msg_info("%s: %ld \"%s\" \"%s\"",
		 myname, (long) index, new_hdr_name, new_hdr_value);

    /*
     * Look for a header at the specified position.
     * 
     * The lookup result may be a pointer record. This allows us to make some
     * optimization when multiple insert operations happen in the same place.
     * 
     * Index 1 is the top-most header.
     */
#define NO_HEADER_NAME	((char *) 0)
#define ALLOW_PTR_BACKUP	1
#define SKIP_ONE_HEADER		1
#define DONT_SKIP_HEADERS	0

    if (index < 1)
	index = 1;
    old_rec_offset = cleanup_find_header_start(state, index, NO_HEADER_NAME,
					       old_rec_buf, &old_rec_type,
					       ALLOW_PTR_BACKUP,
					       DONT_SKIP_HEADERS);
    if (old_rec_offset == CLEANUP_FIND_HEADER_IOERROR)
	/* Warning and errno->error mapping are done elsewhere. */
	CLEANUP_INS_HEADER_RETURN(cleanup_milter_error(state, 0));

    /*
     * If the header does not exist, simply append the header to the linked
     * list at the "header append" pointer record.
     */
    if (old_rec_offset < 0)
	CLEANUP_INS_HEADER_RETURN(cleanup_add_header(context, new_hdr_name,
						 hdr_space, new_hdr_value));

    /*
     * If the header does exist, save both the new and the existing header to
     * new storage at the end of the queue file, and link the new storage
     * with a forward and reverse pointer (don't write a reverse pointer if
     * we are starting with a pointer record).
     */
    if (old_rec_type == REC_TYPE_PTR) {
	next_offset = -1;
    } else {
	if ((next_offset = vstream_ftell(state->dst)) < 0) {
	    msg_warn("%s: read file %s: %m", myname, cleanup_path);
	    CLEANUP_INS_HEADER_RETURN(cleanup_milter_error(state, errno));
	}
    }
    ret = cleanup_patch_header(state, new_hdr_name, hdr_space, new_hdr_value,
			       old_rec_offset, old_rec_type,
			       old_rec_buf, next_offset);
    CLEANUP_INS_HEADER_RETURN(ret);
}

/* cleanup_upd_header - modify or append message header */

static const char *cleanup_upd_header(void *context, ssize_t index,
				              const char *new_hdr_name,
				              const char *hdr_space,
				              const char *new_hdr_value)
{
    const char *myname = "cleanup_upd_header";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    VSTRING *rec_buf;
    off_t   old_rec_offset;
    off_t   next_offset;
    int     last_type;
    const char *ret;

    if (msg_verbose)
	msg_info("%s: %ld \"%s\" \"%s\"",
		 myname, (long) index, new_hdr_name, new_hdr_value);

    /*
     * Sanity check.
     */
    if (*new_hdr_name == 0)
	msg_panic("%s: null header name", myname);

    /*
     * Find the header that is being modified.
     * 
     * The lookup result will never be a pointer record.
     * 
     * Index 1 is the first matching header instance.
     * 
     * XXX When a header is updated repeatedly we create jumps to jumps. To
     * eliminate this, rewrite the loop below so that we can start with the
     * pointer record that points to the header that's being edited.
     */
#define DONT_SAVE_RECORD	0
#define NO_PTR_BACKUP		0

#define CLEANUP_UPD_HEADER_RETURN(ret) do { \
	vstring_free(rec_buf); \
	return (ret); \
    } while (0)

    rec_buf = vstring_alloc(100);
    old_rec_offset = cleanup_find_header_start(state, index, new_hdr_name,
					       rec_buf, &last_type,
					       NO_PTR_BACKUP,
					       SKIP_ONE_HEADER);
    if (old_rec_offset == CLEANUP_FIND_HEADER_IOERROR)
	/* Warning and errno->error mapping are done elsewhere. */
	CLEANUP_UPD_HEADER_RETURN(cleanup_milter_error(state, 0));

    /*
     * If no old header is found, simply append the new header to the linked
     * list at the "header append" pointer record.
     */
    if (old_rec_offset < 0)
	CLEANUP_UPD_HEADER_RETURN(cleanup_add_header(context, new_hdr_name,
						 hdr_space, new_hdr_value));

    /*
     * If the old header is found, find the end of the old header, save the
     * new header to new storage at the end of the queue file, and link the
     * new storage with a forward and reverse pointer.
     */
    if ((next_offset = cleanup_find_header_end(state, rec_buf, last_type)) < 0)
	/* Warning and errno->error mapping are done elsewhere. */
	CLEANUP_UPD_HEADER_RETURN(cleanup_milter_error(state, 0));
    ret = cleanup_patch_header(state, new_hdr_name, hdr_space, new_hdr_value,
			       old_rec_offset, DONT_SAVE_RECORD,
			       (VSTRING *) 0, next_offset);
    CLEANUP_UPD_HEADER_RETURN(ret);
}

/* cleanup_del_header - delete message header */

static const char *cleanup_del_header(void *context, ssize_t index,
				              const char *hdr_name)
{
    const char *myname = "cleanup_del_header";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    VSTRING *rec_buf;
    off_t   header_offset;
    off_t   next_offset;
    int     last_type;

    if (msg_verbose)
	msg_info("%s: %ld \"%s\"", myname, (long) index, hdr_name);

    /*
     * Sanity check.
     */
    if (*hdr_name == 0)
	msg_panic("%s: null header name", myname);

    /*
     * Find the header that is being deleted.
     * 
     * The lookup result will never be a pointer record.
     * 
     * Index 1 is the first matching header instance.
     */
#define CLEANUP_DEL_HEADER_RETURN(ret) do { \
	vstring_free(rec_buf); \
	return (ret); \
    } while (0)

    rec_buf = vstring_alloc(100);
    header_offset = cleanup_find_header_start(state, index, hdr_name, rec_buf,
					      &last_type, NO_PTR_BACKUP,
					      SKIP_ONE_HEADER);
    if (header_offset == CLEANUP_FIND_HEADER_IOERROR)
	/* Warning and errno->error mapping are done elsewhere. */
	CLEANUP_DEL_HEADER_RETURN(cleanup_milter_error(state, 0));

    /*
     * Overwrite the beginning of the header record with a pointer to the
     * information that follows the header. We can't simply overwrite the
     * header with cleanup_out_header() and a special record type, because
     * there may be a PTR record in the middle of a multi-line header.
     */
    if (header_offset > 0) {
	if ((next_offset = cleanup_find_header_end(state, rec_buf, last_type)) < 0)
	    /* Warning and errno->error mapping are done elsewhere. */
	    CLEANUP_DEL_HEADER_RETURN(cleanup_milter_error(state, 0));
	/* Mark the header as deleted. */
	if (vstream_fseek(state->dst, header_offset, SEEK_SET) < 0) {
	    msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	    CLEANUP_DEL_HEADER_RETURN(cleanup_milter_error(state, errno));
	}
	rec_fprintf(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		    (long) next_offset);
    }
    vstring_free(rec_buf);

    /*
     * In case of error while doing record output.
     */
    return (CLEANUP_OUT_OK(state) ? 0 : cleanup_milter_error(state, 0));
}

/* cleanup_chg_from - replace sender address, ignore ESMTP arguments */

static const char *cleanup_chg_from(void *context, const char *ext_from,
				            const char *esmtp_args)
{
    const char *myname = "cleanup_chg_from";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    off_t   new_sender_offset;
    int     addr_count;
    TOK822 *tree;
    TOK822 *tp;
    VSTRING *int_sender_buf;

    if (msg_verbose)
	msg_info("%s: \"%s\" \"%s\"", myname, ext_from, esmtp_args);

    if (esmtp_args[0])
	msg_warn("%s: %s: ignoring ESMTP arguments \"%.100s\"",
		 state->queue_id, myname, esmtp_args);

    /*
     * The cleanup server remembers the location of the the original sender
     * address record (offset in sender_pt_offset) and the file offset of the
     * record that follows the sender address (offset in sender_pt_target).
     * Short original sender records are padded, so that they can safely be
     * overwritten with a pointer record to the new sender address record.
     */
    if (state->sender_pt_offset < 0)
	msg_panic("%s: no original sender record offset", myname);
    if (state->sender_pt_target < 0)
	msg_panic("%s: no post-sender record offset", myname);

    /*
     * Allocate space after the end of the queue file, and write the new
     * sender record, followed by a reverse pointer record that points to the
     * record that follows the original sender address record. No padding is
     * needed for a "new" short sender record, since the record is not meant
     * to be overwritten. When the "new" sender is replaced, we allocate a
     * new record at the end of the queue file.
     * 
     * We update the queue file in a safe manner: save the new sender after the
     * end of the queue file, write the reverse pointer, and only then
     * overwrite the old sender record with the forward pointer to the new
     * sender.
     */
    if ((new_sender_offset = vstream_fseek(state->dst, (off_t) 0, SEEK_END)) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }

    /*
     * Transform the address from external form to internal form. This also
     * removes the enclosing <>, if present.
     * 
     * XXX vstring_alloc() rejects zero-length requests.
     */
    int_sender_buf = vstring_alloc(strlen(ext_from) + 1);
    tree = tok822_parse(ext_from);
    for (addr_count = 0, tp = tree; tp != 0; tp = tp->next) {
	if (tp->type == TOK822_ADDR) {
	    if (addr_count == 0) {
		tok822_internalize(int_sender_buf, tp->head, TOK822_STR_DEFL);
		addr_count += 1;
	    } else {
		msg_warn("%s: Milter request to add multi-sender: \"%s\"",
			 state->queue_id, ext_from);
		break;
	    }
	}
    }
    tok822_free_tree(tree);
    cleanup_addr_sender(state, STR(int_sender_buf));
    vstring_free(int_sender_buf);
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) state->sender_pt_target);

    /*
     * Overwrite the original sender record with the pointer to the new
     * sender address record.
     */
    if (vstream_fseek(state->dst, state->sender_pt_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) new_sender_offset);

    /*
     * In case of error while doing record output.
     */
    return (CLEANUP_OUT_OK(state) ? 0 : cleanup_milter_error(state, 0));
}

/* cleanup_add_rcpt - append recipient address */

static const char *cleanup_add_rcpt(void *context, const char *ext_rcpt)
{
    const char *myname = "cleanup_add_rcpt";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    off_t   new_rcpt_offset;
    off_t   reverse_ptr_offset;
    int     addr_count;
    TOK822 *tree;
    TOK822 *tp;
    VSTRING *int_rcpt_buf;

    if (msg_verbose)
	msg_info("%s: \"%s\"", myname, ext_rcpt);

    /*
     * To simplify implementation, the cleanup server writes a dummy
     * "recipient append" pointer record after the last recipient. We cache
     * both the location and the target of the current "recipient append"
     * pointer record.
     */
    if (state->append_rcpt_pt_offset < 0)
	msg_panic("%s: no recipient append pointer location", myname);
    if (state->append_rcpt_pt_target < 0)
	msg_panic("%s: no recipient append pointer target", myname);

    /*
     * Allocate space after the end of the queue file, and write the
     * recipient record, followed by a reverse pointer record that points to
     * the target of the old "recipient append" pointer record. This reverse
     * pointer record becomes the new "recipient append" pointer record.
     * 
     * We update the queue file in a safe manner: save the new recipient after
     * the end of the queue file, write the reverse pointer, and only then
     * overwrite the old "recipient append" pointer with the forward pointer
     * to the new recipient.
     */
#define NO_DSN_ORCPT	((char *) 0)

    if ((new_rcpt_offset = vstream_fseek(state->dst, (off_t) 0, SEEK_END)) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }

    /*
     * Transform recipient from external form to internal form. This also
     * removes the enclosing <>, if present.
     * 
     * XXX vstring_alloc() rejects zero-length requests.
     */
    int_rcpt_buf = vstring_alloc(strlen(ext_rcpt) + 1);
    tree = tok822_parse(ext_rcpt);
    for (addr_count = 0, tp = tree; tp != 0; tp = tp->next) {
	if (tp->type == TOK822_ADDR) {
	    if (addr_count == 0) {
		tok822_internalize(int_rcpt_buf, tp->head, TOK822_STR_DEFL);
		addr_count += 1;
	    } else {
		msg_warn("%s: Milter request to add multi-recipient: \"%s\"",
			 state->queue_id, ext_rcpt);
		break;
	    }
	}
    }
    tok822_free_tree(tree);
    cleanup_addr_bcc_dsn(state, STR(int_rcpt_buf), NO_DSN_ORCPT, DEF_DSN_NOTIFY);
    vstring_free(int_rcpt_buf);
    if (addr_count == 0) {
	msg_warn("%s: ignoring attempt from Milter to add null recipient",
		 state->queue_id);
	return (CLEANUP_OUT_OK(state) ? 0 : cleanup_milter_error(state, 0));
    }
    if ((reverse_ptr_offset = vstream_ftell(state->dst)) < 0) {
	msg_warn("%s: vstream_ftell file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) state->append_rcpt_pt_target);

    /*
     * Pointer flipping: update the old "recipient append" pointer record
     * value to the location of the new recipient record.
     */
    if (vstream_fseek(state->dst, state->append_rcpt_pt_offset, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT,
		       (long) new_rcpt_offset);

    /*
     * Update the in-memory "recipient append" pointer record location with
     * the location of the reverse pointer record that follows the new
     * recipient. The target of the "recipient append" pointer record does
     * not change; it's always the record that follows the dummy pointer
     * record that was written while Postfix received the message.
     */
    state->append_rcpt_pt_offset = reverse_ptr_offset;

    /*
     * In case of error while doing record output.
     */
    return (CLEANUP_OUT_OK(state) ? 0 : cleanup_milter_error(state, 0));
}

/* cleanup_add_rcpt_par - append recipient address, ignore ESMTP arguments */

static const char *cleanup_add_rcpt_par(void *context, const char *ext_rcpt,
					        const char *esmtp_args)
{
    const char *myname = "cleanup_add_rcpt";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;

    if (esmtp_args[0])
	msg_warn("%s: %s: ignoring ESMTP arguments \"%.100s\"",
		 state->queue_id, myname, esmtp_args);
    return (cleanup_add_rcpt(context, ext_rcpt));
}

/* cleanup_del_rcpt - remove recipient and all its expansions */

static const char *cleanup_del_rcpt(void *context, const char *ext_rcpt)
{
    const char *myname = "cleanup_del_rcpt";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    off_t   curr_offset;
    VSTRING *buf;
    char   *attr_name;
    char   *attr_value;
    char   *dsn_orcpt = 0;		/* XXX for dup filter cleanup */
    int     dsn_notify = 0;		/* XXX for dup filter cleanup */
    char   *orig_rcpt = 0;
    char   *start;
    int     rec_type;
    int     junk;
    int     count = 0;
    TOK822 *tree;
    TOK822 *tp;
    VSTRING *int_rcpt_buf;
    int     addr_count;

    if (msg_verbose)
	msg_info("%s: \"%s\"", myname, ext_rcpt);

    /*
     * Virtual aliasing and other address rewriting happens after the mail
     * filter sees the envelope address. Therefore we must delete all
     * recipient records whose Postfix (not DSN) original recipient address
     * matches the specified address.
     * 
     * As the number of recipients may be very large we can't do an efficient
     * two-pass implementation (collect record offsets first, then mark
     * records as deleted). Instead we mark records as soon as we find them.
     * This is less efficient because we do (seek-write-read) for each marked
     * recipient, instead of (seek-write). It's unlikely that VSTREAMs will
     * be made smart enough to eliminate unnecessary I/O with small seeks.
     * 
     * XXX When Postfix original recipients are turned off, we have no option
     * but to match against the expanded and rewritten recipient address.
     * 
     * XXX Remove the (dsn_orcpt, dsn_notify, orcpt, recip) tuple from the
     * duplicate recipient filter. This requires that we maintain reference
     * counts.
     */
    if (vstream_fseek(state->dst, 0L, SEEK_SET) < 0) {
	msg_warn("%s: seek file %s: %m", myname, cleanup_path);
	return (cleanup_milter_error(state, errno));
    }
#define CLEANUP_DEL_RCPT_RETURN(ret) do { \
	if (orig_rcpt != 0)	\
	    myfree(orig_rcpt); \
	if (dsn_orcpt != 0) \
	    myfree(dsn_orcpt); \
	vstring_free(buf); \
	vstring_free(int_rcpt_buf); \
	return (ret); \
    } while (0)

    /*
     * Transform recipient from external form to internal form. This also
     * removes the enclosing <>, if present.
     * 
     * XXX vstring_alloc() rejects zero-length requests.
     */
    int_rcpt_buf = vstring_alloc(strlen(ext_rcpt) + 1);
    tree = tok822_parse(ext_rcpt);
    for (addr_count = 0, tp = tree; tp != 0; tp = tp->next) {
	if (tp->type == TOK822_ADDR) {
	    if (addr_count == 0) {
		tok822_internalize(int_rcpt_buf, tp->head, TOK822_STR_DEFL);
		addr_count += 1;
	    } else {
		msg_warn("%s: Milter request to drop multi-recipient: \"%s\"",
			 state->queue_id, ext_rcpt);
		break;
	    }
	}
    }
    tok822_free_tree(tree);

    buf = vstring_alloc(100);
    for (;;) {
	if (CLEANUP_OUT_OK(state) == 0)
	    /* Warning and errno->error mapping are done elsewhere. */
	    CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, 0));
	if ((curr_offset = vstream_ftell(state->dst)) < 0) {
	    msg_warn("%s: vstream_ftell file %s: %m", myname, cleanup_path);
	    CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
	}
	if ((rec_type = rec_get_raw(state->dst, buf, 0, REC_FLAG_NONE)) <= 0) {
	    msg_warn("%s: read file %s: %m", myname, cleanup_path);
	    CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
	}
	if (rec_type == REC_TYPE_END)
	    break;
	/* Skip over message content. */
	if (rec_type == REC_TYPE_MESG) {
	    if (vstream_fseek(state->dst, state->xtra_offset, SEEK_SET) < 0) {
		msg_warn("%s: seek file %s: %m", myname, cleanup_path);
		CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
	    }
	    continue;
	}
	start = STR(buf);
	if (rec_type == REC_TYPE_PTR) {
	    if (rec_goto(state->dst, start) < 0) {
		msg_warn("%s: seek file %s: %m", myname, cleanup_path);
		CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
	    }
	    continue;
	}
	/* Map attribute names to pseudo record type. */
	if (rec_type == REC_TYPE_ATTR) {
	    if (split_nameval(STR(buf), &attr_name, &attr_value) != 0
		|| *attr_value == 0)
		continue;
	    if ((junk = rec_attr_map(attr_name)) != 0) {
		start = attr_value;
		rec_type = junk;
	    }
	}
	switch (rec_type) {
	case REC_TYPE_DSN_ORCPT:		/* RCPT TO ORCPT parameter */
	    if (dsn_orcpt != 0)			/* can't happen */
		myfree(dsn_orcpt);
	    dsn_orcpt = mystrdup(start);
	    break;
	case REC_TYPE_DSN_NOTIFY:		/* RCPT TO NOTIFY parameter */
	    if (alldig(start) && (junk = atoi(start)) > 0
		&& DSN_NOTIFY_OK(junk))
		dsn_notify = junk;
	    else
		dsn_notify = 0;
	    break;
	case REC_TYPE_ORCP:			/* unmodified RCPT TO address */
	    if (orig_rcpt != 0)			/* can't happen */
		myfree(orig_rcpt);
	    orig_rcpt = mystrdup(start);
	    break;
	case REC_TYPE_RCPT:			/* rewritten RCPT TO address */
	    if (strcmp(orig_rcpt ? orig_rcpt : start, STR(int_rcpt_buf)) == 0) {
		if (vstream_fseek(state->dst, curr_offset, SEEK_SET) < 0) {
		    msg_warn("%s: seek file %s: %m", myname, cleanup_path);
		    CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
		}
		if (REC_PUT_BUF(state->dst, REC_TYPE_DRCP, buf) < 0) {
		    msg_warn("%s: write queue file: %m", state->queue_id);
		    CLEANUP_DEL_RCPT_RETURN(cleanup_milter_error(state, errno));
		}
		count++;
	    }
	    /* FALLTHROUGH */
	case REC_TYPE_DRCP:			/* canceled recipient */
	case REC_TYPE_DONE:			/* can't happen */
	    if (orig_rcpt != 0) {
		myfree(orig_rcpt);
		orig_rcpt = 0;
	    }
	    if (dsn_orcpt != 0) {
		myfree(dsn_orcpt);
		dsn_orcpt = 0;
	    }
	    dsn_notify = 0;
	    break;
	}
    }

    if (msg_verbose)
	msg_info("%s: deleted %d records for recipient \"%s\"",
		 myname, count, ext_rcpt);

    CLEANUP_DEL_RCPT_RETURN(0);
}

/* cleanup_repl_body - replace message body */

static const char *cleanup_repl_body(void *context, int cmd, VSTRING *buf)
{
    const char *myname = "cleanup_repl_body";
    CLEANUP_STATE *state = (CLEANUP_STATE *) context;
    static VSTRING empty;

    /*
     * XXX Sendmail compatibility: milters don't see the first body line, so
     * don't expect they will send one.
     */
    switch (cmd) {
    case MILTER_BODY_LINE:
	if (cleanup_body_edit_write(state, REC_TYPE_NORM, buf) < 0)
	    return (cleanup_milter_error(state, errno));
	break;
    case MILTER_BODY_START:
	VSTRING_RESET(&empty);
	if (cleanup_body_edit_start(state) < 0
	    || cleanup_body_edit_write(state, REC_TYPE_NORM, &empty) < 0)
	    return (cleanup_milter_error(state, errno));
	break;
    case MILTER_BODY_END:
	if (cleanup_body_edit_finish(state) < 0)
	    return (cleanup_milter_error(state, errno));
	break;
    default:
	msg_panic("%s: bad command: %d", myname, cmd);
    }
    return (CLEANUP_OUT_OK(state) ? 0 : cleanup_milter_error(state, errno));
}

/* cleanup_milter_eval - expand macro */

static const char *cleanup_milter_eval(const char *name, void *ptr)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) ptr;

    /*
     * Note: if we use XFORWARD attributes here, then consistency requires
     * that we forward all Sendmail macros via XFORWARD.
     */

    /*
     * Canonicalize the name.
     */
    if (*name != '{') {				/* } */
	vstring_sprintf(state->temp1, "{%s}", name);
	name = STR(state->temp1);
    }

    /*
     * System macros.
     */
    if (strcmp(name, S8_MAC_DAEMON_NAME) == 0)
	return (var_milt_daemon_name);
    if (strcmp(name, S8_MAC_V) == 0)
	return (var_milt_v);

    /*
     * Connect macros.
     */
#ifndef CLIENT_ATTR_UNKNOWN
#define CLIENT_ATTR_UNKNOWN "unknown"
#endif

    if (strcmp(name, S8_MAC__) == 0) {
	vstring_sprintf(state->temp1, "%s [%s]",
			state->reverse_name, state->client_addr);
	if (strcasecmp(state->client_name, state->reverse_name) != 0)
	    vstring_strcat(state->temp1, " (may be forged)");
	return (STR(state->temp1));
    }
    if (strcmp(name, S8_MAC_J) == 0)
	return (var_myhostname);
    if (strcmp(name, S8_MAC_CLIENT_ADDR) == 0)
	return (state->client_addr);
    if (strcmp(name, S8_MAC_CLIENT_NAME) == 0)
	return (state->client_name);
    if (strcmp(name, S8_MAC_CLIENT_PORT) == 0)
	return (state->client_port
		&& strcmp(state->client_port, CLIENT_ATTR_UNKNOWN) ?
		state->client_port : "0");
    if (strcmp(name, S8_MAC_CLIENT_PTR) == 0)
	return (state->reverse_name);

    /*
     * MAIL FROM macros.
     */
    if (strcmp(name, S8_MAC_I) == 0)
	return (state->queue_id);
#ifdef USE_SASL_AUTH
    if (strcmp(name, S8_MAC_AUTH_TYPE) == 0)
	return (nvtable_find(state->attr, MAIL_ATTR_SASL_METHOD));
    if (strcmp(name, S8_MAC_AUTH_AUTHEN) == 0)
	return (nvtable_find(state->attr, MAIL_ATTR_SASL_USERNAME));
    if (strcmp(name, S8_MAC_AUTH_AUTHOR) == 0)
	return (nvtable_find(state->attr, MAIL_ATTR_SASL_SENDER));
#endif
    if (strcmp(name, S8_MAC_MAIL_ADDR) == 0)
	return (state->milter_ext_from ? STR(state->milter_ext_from) : 0);

    /*
     * RCPT TO macros.
     */
    if (strcmp(name, S8_MAC_RCPT_ADDR) == 0)
	return (state->milter_ext_rcpt ? STR(state->milter_ext_rcpt) : 0);
    return (0);
}

/* cleanup_milter_receive - receive milter instances */

void    cleanup_milter_receive(CLEANUP_STATE *state, int count)
{
    if (state->milters)
	milter_free(state->milters);
    state->milters = milter_receive(state->src, count);
    if (state->milters == 0)
	msg_fatal("cleanup_milter_receive: milter receive failed");
    if (count <= 0)
	return;
    milter_macro_callback(state->milters, cleanup_milter_eval, (void *) state);
    milter_edit_callback(state->milters,
			 cleanup_add_header, cleanup_upd_header,
			 cleanup_ins_header, cleanup_del_header,
			 cleanup_chg_from, cleanup_add_rcpt,
			 cleanup_add_rcpt_par, cleanup_del_rcpt,
			 cleanup_repl_body, (void *) state);
}

/* cleanup_milter_apply - apply Milter reponse, non-zero if rejecting */

static const char *cleanup_milter_apply(CLEANUP_STATE *state, const char *event,
					        const char *resp)
{
    const char *myname = "cleanup_milter_apply";
    const char *action;
    const char *text;
    const char *attr;
    const char *ret = 0;

    if (msg_verbose)
	msg_info("%s: %s", myname, resp);

    /*
     * Don't process our own milter_header/body checks replies. See comments
     * in cleanup_milter_hbc_extend().
     */
    if (state->milter_hbc_reply &&
	strcmp(resp, STR(state->milter_hbc_reply)) == 0)
	return (0);

    /*
     * Don't process Milter replies that are redundant because header/body
     * checks already decided that we will not receive the message; or Milter
     * replies that would have conflicting effect with the outcome of
     * header/body checks (for example, header_checks "discard" action
     * followed by Milter "reject" reply). Logging both actions would look
     * silly.
     */
    if (CLEANUP_MILTER_REJECTING_OR_DISCARDING_MESSAGE(state)) {
	if (msg_verbose)
	    msg_info("%s: ignoring redundant or conflicting milter reply: %s",
		     state->queue_id, resp);
	return (0);
    }

    /*
     * Sanity check.
     */
    if (state->client_name == 0)
	msg_panic("%s: missing client info initialization", myname);

    /*
     * We don't report errors that were already reported by the content
     * editing call-back routines. See cleanup_milter_error() above.
     */
    if (CLEANUP_OUT_OK(state) == 0)
	return (0);
    switch (resp[0]) {
    case 'H':
	/* XXX Should log the reason here. */
	if (state->flags & CLEANUP_FLAG_HOLD)
	    return (0);
	state->flags |= CLEANUP_FLAG_HOLD;
	action = "milter-hold";
	text = "milter triggers HOLD action";
	break;
    case 'D':
	state->flags |= CLEANUP_FLAG_DISCARD;
	action = "milter-discard";
	text = "milter triggers DISCARD action";
	break;
    case 'S':
	/* XXX Can this happen after end-of-message? */
	state->flags |= CLEANUP_STAT_CONT;
	action = "milter-reject";
	text = cleanup_strerror(CLEANUP_STAT_CONT);
	break;

	/*
	 * Override permanent reject with temporary reject. This happens when
	 * the cleanup server has to bounce (hard reject) but is unable to
	 * store the message (soft reject). After a temporary reject we stop
	 * inspecting queue file records, so it can't be overruled by
	 * something else.
	 * 
	 * CLEANUP_STAT_CONT and CLEANUP_STAT_DEFER both update the reason
	 * attribute, but CLEANUP_STAT_DEFER takes precedence. It terminates
	 * queue record processing, and prevents bounces from being sent.
	 */
    case '4':
	CLEANUP_MILTER_SET_SMTP_REPLY(state, resp);
	ret = state->reason;
	state->errs |= CLEANUP_STAT_DEFER;
	action = "milter-reject";
	text = resp + 4;
	break;
    case '5':
	CLEANUP_MILTER_SET_SMTP_REPLY(state, resp);
	ret = state->reason;
	state->errs |= CLEANUP_STAT_CONT;
	action = "milter-reject";
	text = resp + 4;
	break;
    default:
	msg_panic("%s: unexpected mail filter reply: %s", myname, resp);
    }
    vstring_sprintf(state->temp1, "%s: %s: %s from %s[%s]: %s;",
		    state->queue_id, action, event, state->client_name,
		    state->client_addr, text);
    if (state->sender)
	vstring_sprintf_append(state->temp1, " from=<%s>", state->sender);
    if (state->recip)
	vstring_sprintf_append(state->temp1, " to=<%s>", state->recip);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_PROTO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " proto=%s", attr);
    if ((attr = nvtable_find(state->attr, MAIL_ATTR_LOG_HELO_NAME)) != 0)
	vstring_sprintf_append(state->temp1, " helo=<%s>", attr);
    msg_info("%s", vstring_str(state->temp1));

    return (ret);
}

/* cleanup_milter_client_init - initialize real or ersatz client info */

static void cleanup_milter_client_init(CLEANUP_STATE *state)
{
    const char *proto_attr;

    /*
     * Either the cleanup client specifies a name, address and protocol, or
     * we have a local submission and pretend localhost/127.0.0.1/AF_INET.
     */
#define NO_CLIENT_PORT	"0"

    state->client_name = nvtable_find(state->attr, MAIL_ATTR_ACT_CLIENT_NAME);
    state->reverse_name =
	nvtable_find(state->attr, MAIL_ATTR_ACT_REVERSE_CLIENT_NAME);
    state->client_addr = nvtable_find(state->attr, MAIL_ATTR_ACT_CLIENT_ADDR);
    state->client_port = nvtable_find(state->attr, MAIL_ATTR_ACT_CLIENT_PORT);
    proto_attr = nvtable_find(state->attr, MAIL_ATTR_ACT_CLIENT_AF);

    if (state->client_name == 0 || state->client_addr == 0 || proto_attr == 0
	|| !alldig(proto_attr)) {
	state->client_name = "localhost";
	state->client_addr = "127.0.0.1";
	state->client_af = AF_INET;
    } else
	state->client_af = atoi(proto_attr);
    if (state->reverse_name == 0)
	state->reverse_name = state->client_name;
    /* Compatibility with pre-2.5 queue files. */
    if (state->client_port == 0)
	state->client_port = NO_CLIENT_PORT;
}

/* cleanup_milter_inspect - run message through mail filter */

void    cleanup_milter_inspect(CLEANUP_STATE *state, MILTERS *milters)
{
    const char *myname = "cleanup_milter";
    const char *resp;

    if (msg_verbose)
	msg_info("enter %s", myname);

    /*
     * Initialize, in case we're called via smtpd(8).
     */
    if (state->client_name == 0)
	cleanup_milter_client_init(state);

    /*
     * Prologue: prepare for Milter header/body checks.
     */
    if (*var_milt_head_checks)
	cleanup_milter_header_checks_init(state);

    /*
     * Process mail filter replies. The reply format is verified by the mail
     * filter library.
     */
    if ((resp = milter_message(milters, state->handle->stream,
			       state->data_offset)) != 0)
	cleanup_milter_apply(state, "END-OF-MESSAGE", resp);

    /*
     * Epilogue: finalize Milter header/body checks.
     */
    if (*var_milt_head_checks)
	cleanup_milter_hbc_finish(state);

    if (msg_verbose)
	msg_info("leave %s", myname);
}

/* cleanup_milter_emul_mail - emulate connect/ehlo/mail event */

void    cleanup_milter_emul_mail(CLEANUP_STATE *state,
				         MILTERS *milters,
				         const char *addr)
{
    const char *resp;
    const char *helo;
    const char *argv[2];

    /*
     * Per-connection initialization.
     */
    milter_macro_callback(milters, cleanup_milter_eval, (void *) state);
    milter_edit_callback(milters,
			 cleanup_add_header, cleanup_upd_header,
			 cleanup_ins_header, cleanup_del_header,
			 cleanup_chg_from, cleanup_add_rcpt,
			 cleanup_add_rcpt_par, cleanup_del_rcpt,
			 cleanup_repl_body, (void *) state);
    if (state->client_name == 0)
	cleanup_milter_client_init(state);

    /*
     * Emulate SMTP events.
     */
    if ((resp = milter_conn_event(milters, state->client_name, state->client_addr,
			      state->client_port, state->client_af)) != 0) {
	cleanup_milter_apply(state, "CONNECT", resp);
	return;
    }
#define PRETEND_ESMTP	1

    if (CLEANUP_MILTER_OK(state)) {
	if ((helo = nvtable_find(state->attr, MAIL_ATTR_ACT_HELO_NAME)) == 0)
	    helo = state->client_name;
	if ((resp = milter_helo_event(milters, helo, PRETEND_ESMTP)) != 0) {
	    cleanup_milter_apply(state, "EHLO", resp);
	    return;
	}
    }
    if (CLEANUP_MILTER_OK(state)) {
	if (state->milter_ext_from == 0)
	    state->milter_ext_from = vstring_alloc(100);
	/* Sendmail 8.13 does not externalize the null address. */
	if (*addr)
	    quote_821_local(state->milter_ext_from, addr);
	else
	    vstring_strcpy(state->milter_ext_from, addr);
	argv[0] = STR(state->milter_ext_from);
	argv[1] = 0;
	if ((resp = milter_mail_event(milters, argv)) != 0) {
	    cleanup_milter_apply(state, "MAIL", resp);
	    return;
	}
    }
}

/* cleanup_milter_emul_rcpt - emulate rcpt event */

void    cleanup_milter_emul_rcpt(CLEANUP_STATE *state,
				         MILTERS *milters,
				         const char *addr)
{
    const char *myname = "cleanup_milter_emul_rcpt";
    const char *resp;
    const char *argv[2];

    /*
     * Sanity check.
     */
    if (state->client_name == 0)
	msg_panic("%s: missing client info initialization", myname);

    /*
     * CLEANUP_STAT_CONT and CLEANUP_STAT_DEFER both update the reason
     * attribute, but CLEANUP_STAT_DEFER takes precedence. It terminates
     * queue record processing, and prevents bounces from being sent.
     */
    if (state->milter_ext_rcpt == 0)
	state->milter_ext_rcpt = vstring_alloc(100);
    /* Sendmail 8.13 does not externalize the null address. */
    if (*addr)
	quote_821_local(state->milter_ext_rcpt, addr);
    else
	vstring_strcpy(state->milter_ext_rcpt, addr);
    argv[0] = STR(state->milter_ext_rcpt);
    argv[1] = 0;
    if ((resp = milter_rcpt_event(milters, MILTER_FLAG_NONE, argv)) != 0
	&& cleanup_milter_apply(state, "RCPT", resp) != 0) {
	msg_warn("%s: milter configuration error: can't reject recipient "
		 "in non-smtpd(8) submission", state->queue_id);
	msg_warn("%s: deferring delivery of this message", state->queue_id);
	CLEANUP_MILTER_SET_REASON(state, "4.3.5 Server configuration error");
	state->errs |= CLEANUP_STAT_DEFER;
    }
}

/* cleanup_milter_emul_data - emulate data event */

void    cleanup_milter_emul_data(CLEANUP_STATE *state, MILTERS *milters)
{
    const char *myname = "cleanup_milter_emul_data";
    const char *resp;

    /*
     * Sanity check.
     */
    if (state->client_name == 0)
	msg_panic("%s: missing client info initialization", myname);

    if ((resp = milter_data_event(milters)) != 0)
	cleanup_milter_apply(state, "DATA", resp);
}

#ifdef TEST

 /*
  * Queue file editing driver for regression tests. In this case it is OK to
  * report fatal errors after I/O errors.
  */
#include <stdio.h>
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <mail_addr.h>
#include <mail_version.h>

#undef msg_verbose

char   *cleanup_path;
VSTRING *cleanup_trace_path;
VSTRING *cleanup_strip_chars;
int     cleanup_comm_canon_flags;
MAPS   *cleanup_comm_canon_maps;
int     cleanup_ext_prop_mask;
ARGV   *cleanup_masq_domains;
int     cleanup_masq_flags;
MAPS   *cleanup_rcpt_bcc_maps;
int     cleanup_rcpt_canon_flags;
MAPS   *cleanup_rcpt_canon_maps;
MAPS   *cleanup_send_bcc_maps;
int     cleanup_send_canon_flags;
MAPS   *cleanup_send_canon_maps;
int     var_dup_filter_limit = DEF_DUP_FILTER_LIMIT;
char   *var_empty_addr = DEF_EMPTY_ADDR;
int     var_enable_orcpt = DEF_ENABLE_ORCPT;
MAPS   *cleanup_virt_alias_maps;
char   *var_milt_daemon_name = "host.example.com";
char   *var_milt_v = DEF_MILT_V;
MILTERS *cleanup_milters = (MILTERS *) ((char *) sizeof(*cleanup_milters));
char   *var_milt_head_checks = "";

/* Dummies to satisfy unused external references. */

int     cleanup_masquerade_internal(CLEANUP_STATE *state, VSTRING *addr, ARGV *masq_domains)
{
    msg_panic("cleanup_masquerade_internal dummy");
}

int     cleanup_rewrite_internal(const char *context, VSTRING *result,
				         const char *addr)
{
    vstring_strcpy(result, addr);
    return (0);
}

int     cleanup_map11_internal(CLEANUP_STATE *state, VSTRING *addr,
			               MAPS *maps, int propagate)
{
    msg_panic("cleanup_map11_internal dummy");
}

ARGV   *cleanup_map1n_internal(CLEANUP_STATE *state, const char *addr,
			               MAPS *maps, int propagate)
{
    msg_panic("cleanup_map1n_internal dummy");
}

void    cleanup_envelope(CLEANUP_STATE *state, int type, const char *buf,
			         ssize_t len)
{
    msg_panic("cleanup_envelope dummy");
}

static void usage(void)
{
    msg_warn("usage:");
    msg_warn("    verbose on|off");
    msg_warn("    open pathname");
    msg_warn("    close");
    msg_warn("    add_header index name [value]");
    msg_warn("    ins_header index name [value]");
    msg_warn("    upd_header index name [value]");
    msg_warn("    del_header index name");
    msg_warn("    chg_from addr parameters");
    msg_warn("    add_rcpt addr");
    msg_warn("    add_rcpt_par addr parameters");
    msg_warn("    del_rcpt addr");
    msg_warn("    replbody pathname");
    msg_warn("    header_checks type:name");
}

/* flatten_args - unparse partial command line */

static void flatten_args(VSTRING *buf, char **argv)
{
    char  **cpp;

    VSTRING_RESET(buf);
    for (cpp = argv; *cpp; cpp++) {
	vstring_strcat(buf, *cpp);
	if (cpp[1])
	    VSTRING_ADDCH(buf, ' ');
    }
    VSTRING_TERMINATE(buf);
}

/* open_queue_file - open an unedited queue file (all-zero dummy PTRs) */

static void open_queue_file(CLEANUP_STATE *state, const char *path)
{
    VSTRING *buf = vstring_alloc(100);
    off_t   curr_offset;
    int     rec_type;
    long    msg_seg_len;
    long    data_offset;
    long    rcpt_count;
    long    qmgr_opts;

    if (state->dst != 0) {
	msg_warn("closing %s", cleanup_path);
	vstream_fclose(state->dst);
	state->dst = 0;
	myfree(cleanup_path);
	cleanup_path = 0;
    }
    if ((state->dst = vstream_fopen(path, O_RDWR, 0)) == 0) {
	msg_warn("open %s: %m", path);
    } else {
	cleanup_path = mystrdup(path);
	for (;;) {
	    if ((curr_offset = vstream_ftell(state->dst)) < 0)
		msg_fatal("file %s: vstream_ftell: %m", cleanup_path);
	    if ((rec_type = rec_get_raw(state->dst, buf, 0, REC_FLAG_NONE)) < 0)
		msg_fatal("file %s: missing SIZE or PTR record", cleanup_path);
	    if (rec_type == REC_TYPE_SIZE) {
		if (sscanf(STR(buf), "%ld %ld %ld %ld",
			   &msg_seg_len, &data_offset,
			   &rcpt_count, &qmgr_opts) != 4)
		    msg_fatal("file %s: bad SIZE record: %s",
			      cleanup_path, STR(buf));
		state->data_offset = data_offset;
		state->xtra_offset = data_offset + msg_seg_len;
	    } else if (rec_type == REC_TYPE_FROM) {
		state->sender_pt_offset = curr_offset;
		if (LEN(buf) < REC_TYPE_PTR_PAYL_SIZE
		    && rec_get_raw(state->dst, buf, 0, REC_FLAG_NONE) != REC_TYPE_PTR)
		    msg_fatal("file %s: missing PTR record after short sender",
			      cleanup_path);
		if ((state->sender_pt_target = vstream_ftell(state->dst)) < 0)
		    msg_fatal("file %s: missing END record", cleanup_path);
	    } else if (rec_type == REC_TYPE_PTR) {
		if (state->data_offset < 0)
		    msg_fatal("file %s: missing SIZE record", cleanup_path);
		if (curr_offset < state->data_offset
		    || curr_offset > state->xtra_offset) {
		    if (state->append_rcpt_pt_offset < 0) {
			state->append_rcpt_pt_offset = curr_offset;
			if (atol(STR(buf)) != 0)
			    msg_fatal("file %s: bad dummy recipient PTR record: %s",
				      cleanup_path, STR(buf));
			if ((state->append_rcpt_pt_target =
			     vstream_ftell(state->dst)) < 0)
			    msg_fatal("file %s: vstream_ftell: %m", cleanup_path);
		    } else if (curr_offset > state->xtra_offset
			       && state->append_meta_pt_offset < 0) {
			state->append_meta_pt_offset = curr_offset;
			if (atol(STR(buf)) != 0)
			    msg_fatal("file %s: bad dummy meta PTR record: %s",
				      cleanup_path, STR(buf));
			if ((state->append_meta_pt_target =
			     vstream_ftell(state->dst)) < 0)
			    msg_fatal("file %s: vstream_ftell: %m", cleanup_path);
		    }
		} else {
		    if (state->append_hdr_pt_offset < 0) {
			state->append_hdr_pt_offset = curr_offset;
			if (atol(STR(buf)) != 0)
			    msg_fatal("file %s: bad dummy header PTR record: %s",
				      cleanup_path, STR(buf));
			if ((state->append_hdr_pt_target =
			     vstream_ftell(state->dst)) < 0)
			    msg_fatal("file %s: vstream_ftell: %m", cleanup_path);
		    }
		}
	    }
	    if (state->append_rcpt_pt_offset > 0
		&& state->append_hdr_pt_offset > 0
		&& (rec_type == REC_TYPE_END
		    || state->append_meta_pt_offset > 0))
		break;
	}
	if (msg_verbose) {
	    msg_info("append_rcpt_pt_offset %ld append_rcpt_pt_target %ld",
		     (long) state->append_rcpt_pt_offset,
		     (long) state->append_rcpt_pt_target);
	    msg_info("append_hdr_pt_offset %ld append_hdr_pt_target %ld",
		     (long) state->append_hdr_pt_offset,
		     (long) state->append_hdr_pt_target);
	}
    }
    vstring_free(buf);
}

static void close_queue_file(CLEANUP_STATE *state)
{
    (void) vstream_fclose(state->dst);
    state->dst = 0;
    myfree(cleanup_path);
    cleanup_path = 0;
}

int     main(int unused_argc, char **argv)
{
    VSTRING *inbuf = vstring_alloc(100);
    VSTRING *arg_buf = vstring_alloc(100);
    char   *bufp;
    int     istty = isatty(vstream_fileno(VSTREAM_IN));
    CLEANUP_STATE *state = cleanup_state_alloc((VSTREAM *) 0);

    state->queue_id = mystrdup("NOQUEUE");
    state->sender = mystrdup("sender");
    state->recip = mystrdup("recipient");
    state->client_name = "client_name";
    state->client_addr = "client_addr";
    state->flags |= CLEANUP_FLAG_FILTER_ALL;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    var_line_limit = DEF_LINE_LIMIT;
    var_header_limit = DEF_HEADER_LIMIT;

    for (;;) {
	ARGV   *argv;
	ssize_t index;
	const char *resp = 0;

	if (istty) {
	    vstream_printf("- ");
	    vstream_fflush(VSTREAM_OUT);
	}
	if (vstring_fgets_nonl(inbuf, VSTREAM_IN) == 0)
	    break;

	bufp = vstring_str(inbuf);
	if (!istty) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#' || *bufp == 0 || allspace(bufp))
	    continue;
	argv = argv_split(bufp, " ");
	if (argv->argc == 0) {
	    msg_warn("missing command");
	} else if (strcmp(argv->argv[0], "?") == 0) {
	    usage();
	} else if (strcmp(argv->argv[0], "verbose") == 0) {
	    if (argv->argc != 2) {
		msg_warn("bad verbose argument count: %d", argv->argc);
	    } else if (strcmp(argv->argv[1], "on") == 0) {
		msg_verbose = 2;
	    } else if (strcmp(argv->argv[1], "off") == 0) {
		msg_verbose = 0;
	    } else {
		msg_warn("bad verbose argument");
	    }
	} else if (strcmp(argv->argv[0], "open") == 0) {
	    if (state->dst != 0) {
		msg_info("closing %s", VSTREAM_PATH(state->dst));
		close_queue_file(state);
	    }
	    if (argv->argc != 2) {
		msg_warn("bad open argument count: %d", argv->argc);
	    } else {
		open_queue_file(state, argv->argv[1]);
	    }
	} else if (state->dst == 0) {
	    msg_warn("no open queue file");
	} else if (strcmp(argv->argv[0], "close") == 0) {
	    if (*var_milt_head_checks) {
		cleanup_milter_hbc_finish(state);
		var_milt_head_checks = "";
	    }
	    close_queue_file(state);
	} else if (state->milter_hbc_reply && LEN(state->milter_hbc_reply)) {
	    /* Postfix libmilter would skip further requests. */
	    msg_info("ignoring: %s %s %s", argv->argv[0],
		     argv->argc > 1 ? argv->argv[1] : "",
		     argv->argc > 2 ? argv->argv[2] : "");
	    continue;
	} else if (strcmp(argv->argv[0], "add_header") == 0) {
	    if (argv->argc < 2) {
		msg_warn("bad add_header argument count: %d", argv->argc);
	    } else {
		flatten_args(arg_buf, argv->argv + 2);
		resp = cleanup_add_header(state, argv->argv[1], " ", STR(arg_buf));
	    }
	} else if (strcmp(argv->argv[0], "ins_header") == 0) {
	    if (argv->argc < 3) {
		msg_warn("bad ins_header argument count: %d", argv->argc);
	    } else if ((index = atoi(argv->argv[1])) < 1) {
		msg_warn("bad ins_header index value");
	    } else {
		flatten_args(arg_buf, argv->argv + 3);
		resp = cleanup_ins_header(state, index, argv->argv[2], " ", STR(arg_buf));
	    }
	} else if (strcmp(argv->argv[0], "upd_header") == 0) {
	    if (argv->argc < 3) {
		msg_warn("bad upd_header argument count: %d", argv->argc);
	    } else if ((index = atoi(argv->argv[1])) < 1) {
		msg_warn("bad upd_header index value");
	    } else {
		flatten_args(arg_buf, argv->argv + 3);
		resp = cleanup_upd_header(state, index, argv->argv[2], " ", STR(arg_buf));
	    }
	} else if (strcmp(argv->argv[0], "del_header") == 0) {
	    if (argv->argc != 3) {
		msg_warn("bad del_header argument count: %d", argv->argc);
	    } else if ((index = atoi(argv->argv[1])) < 1) {
		msg_warn("bad del_header index value");
	    } else {
		cleanup_del_header(state, index, argv->argv[2]);
	    }
	} else if (strcmp(argv->argv[0], "chg_from") == 0) {
	    if (argv->argc != 3) {
		msg_warn("bad chg_from argument count: %d", argv->argc);
	    } else {
		cleanup_chg_from(state, argv->argv[1], argv->argv[2]);
	    }
	} else if (strcmp(argv->argv[0], "add_rcpt") == 0) {
	    if (argv->argc != 2) {
		msg_warn("bad add_rcpt argument count: %d", argv->argc);
	    } else {
		cleanup_add_rcpt(state, argv->argv[1]);
	    }
	} else if (strcmp(argv->argv[0], "add_rcpt_par") == 0) {
	    if (argv->argc != 3) {
		msg_warn("bad add_rcpt_par argument count: %d", argv->argc);
	    } else {
		cleanup_add_rcpt_par(state, argv->argv[1], argv->argv[2]);
	    }
	} else if (strcmp(argv->argv[0], "del_rcpt") == 0) {
	    if (argv->argc != 2) {
		msg_warn("bad del_rcpt argument count: %d", argv->argc);
	    } else {
		cleanup_del_rcpt(state, argv->argv[1]);
	    }
	} else if (strcmp(argv->argv[0], "replbody") == 0) {
	    if (argv->argc != 2) {
		msg_warn("bad replbody argument count: %d", argv->argc);
	    } else {
		VSTREAM *fp;
		VSTRING *buf;

		if ((fp = vstream_fopen(argv->argv[1], O_RDONLY, 0)) == 0) {
		    msg_warn("open %s file: %m", argv->argv[1]);
		} else {
		    buf = vstring_alloc(100);
		    cleanup_repl_body(state, MILTER_BODY_START, buf);
		    while (vstring_get_nonl(buf, fp) != VSTREAM_EOF)
			cleanup_repl_body(state, MILTER_BODY_LINE, buf);
		    cleanup_repl_body(state, MILTER_BODY_END, buf);
		    vstring_free(buf);
		    vstream_fclose(fp);
		}
	    }
	} else if (strcmp(argv->argv[0], "header_checks") == 0) {
	    if (argv->argc != 2) {
		msg_warn("bad header_checks argument count: %d", argv->argc);
	    } else if (*var_milt_head_checks) {
		msg_warn("can't change header checks");
	    } else {
		var_milt_head_checks = mystrdup(argv->argv[1]);
		cleanup_milter_header_checks_init(state);
	    }
	} else {
	    msg_warn("bad command: %s", argv->argv[0]);
	}
	argv_free(argv);
	if (resp)
	    cleanup_milter_apply(state, "END-OF-MESSAGE", resp);
    }
    vstring_free(inbuf);
    vstring_free(arg_buf);
    if (state->append_meta_pt_offset >= 0) {
	if (state->flags)
	    msg_info("flags = %s", cleanup_strflags(state->flags));
	if (state->errs)
	    msg_info("errs = %s", cleanup_strerror(state->errs));
    }
    cleanup_state_free(state);

    return (0);
}

#endif
