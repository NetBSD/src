/*++
/* NAME
/*	qmgr_message 3
/* SUMMARY
/*	in-core message structures
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	int	qmgr_message_count;
/*	int	qmgr_recipient_count;
/*
/*	QMGR_MESSAGE *qmgr_message_alloc(class, name, qflags)
/*	const char *class;
/*	const char *name;
/*	int	qflags;
/*
/*	QMGR_MESSAGE *qmgr_message_realloc(message)
/*	QMGR_MESSAGE *message;
/*
/*	void	qmgr_message_free(message)
/*	QMGR_MESSAGE *message;
/*
/*	void	qmgr_message_update_warn(message)
/*	QMGR_MESSAGE *message;
/* DESCRIPTION
/*	This module performs en-gross operations on queue messages.
/*
/*	qmgr_message_count is a global counter for the total number
/*	of in-core message structures (i.e. the total size of the
/*	`active' message queue).
/*
/*	qmgr_recipient_count is a global counter for the total number
/*	of in-core recipient structures (i.e. the sum of all recipients
/*	in all in-core message structures).
/*
/*	qmgr_message_alloc() creates an in-core message structure
/*	with sender and recipient information taken from the named queue
/*	file. A null result means the queue file could not be read or
/*	that the queue file contained incorrect information. A result
/*	QMGR_MESSAGE_LOCKED means delivery must be deferred. The number
/*	of recipients read from a queue file is limited by the global
/*	var_qmgr_rcpt_limit configuration parameter. When the limit
/*	is reached, the \fIrcpt_offset\fR structure member is set to
/*	the position where the read was terminated. Recipients are
/*	run through the resolver, and are assigned to destination
/*	queues. Recipients that cannot be assigned are deferred or
/*	bounced. Mail that has bounced twice is silently absorbed.
/*
/*	qmgr_message_realloc() resumes reading recipients from the queue
/*	file, and updates the recipient list and \fIrcpt_offset\fR message
/*	structure members. A null result means that the file could not be
/*	read or that the file contained incorrect information. Recipient
/*	limit imposed this time is based on the position of the message
/*	job(s) on corresponding transport job list(s). It's considered
/*	an error to call this when the recipient slots can't be allocated.
/*
/*	qmgr_message_free() destroys an in-core message structure and makes
/*	the resources available for reuse. It is an error to destroy
/*	a message structure that is still referenced by queue entry structures.
/*
/*	qmgr_message_update_warn() takes a closed message, opens it, updates
/*	the warning field, and closes it again.
/* DIAGNOSTICS
/*	Warnings: malformed message file. Fatal errors: out of memory.
/* SEE ALSO
/*	envelope(3) message envelope parser
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
/*	Scheduler enhancements:
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>			/* sscanf() */
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <split_at.h>
#include <valid_hostname.h>
#include <argv.h>
#include <stringops.h>
#include <myflock.h>
#include <sane_time.h>

/* Global library. */

#include <dict.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <canon_addr.h>
#include <record.h>
#include <rec_type.h>
#include <sent.h>
#include <deliver_completed.h>
#include <opened.h>
#include <verp_sender.h>
#include <mail_proto.h>

/* Client stubs. */

#include <resolve_clnt.h>

/* Application-specific. */

#include "qmgr.h"

int     qmgr_message_count;
int     qmgr_recipient_count;

/* qmgr_message_create - create in-core message structure */

static QMGR_MESSAGE *qmgr_message_create(const char *queue_name,
				           const char *queue_id, int qflags)
{
    QMGR_MESSAGE *message;

    message = (QMGR_MESSAGE *) mymalloc(sizeof(QMGR_MESSAGE));
    qmgr_message_count++;
    message->flags = 0;
    message->qflags = qflags;
    message->fp = 0;
    message->refcount = 0;
    message->single_rcpt = 0;
    message->arrival_time = 0;
    message->queued_time = sane_time();
    message->data_offset = 0;
    message->queue_id = mystrdup(queue_id);
    message->queue_name = mystrdup(queue_name);
    message->encoding = 0;
    message->sender = 0;
    message->errors_to = 0;
    message->return_receipt = 0;
    message->filter_xport = 0;
    message->inspect_xport = 0;
    message->data_size = 0;
    message->warn_offset = 0;
    message->warn_time = 0;
    message->rcpt_offset = 0;
    message->verp_delims = 0;
    message->unread_offset = 0;
    qmgr_rcpt_list_init(&message->rcpt_list);
    message->rcpt_count = 0;
    message->rcpt_limit = var_qmgr_msg_rcpt_limit;
    message->rcpt_unread = 0;
    QMGR_LIST_INIT(message->job_list);
    return (message);
}

/* qmgr_message_close - close queue file */

static void qmgr_message_close(QMGR_MESSAGE *message)
{
    vstream_fclose(message->fp);
    message->fp = 0;
}

/* qmgr_message_open - open queue file */

static int qmgr_message_open(QMGR_MESSAGE *message)
{

    /*
     * Sanity check.
     */
    if (message->fp)
	msg_panic("%s: queue file is open", message->queue_id);

    /*
     * Open this queue file. Skip files that we cannot open. Back off when
     * the system appears to be running out of resources.
     */
    if ((message->fp = mail_queue_open(message->queue_name,
				       message->queue_id,
				       O_RDWR, 0)) == 0) {
	if (errno != ENOENT)
	    msg_fatal("open %s %s: %m", message->queue_name, message->queue_id);
	msg_warn("open %s %s: %m", message->queue_name, message->queue_id);
	return (-1);
    }
    return (0);
}

/* qmgr_message_oldstyle_scan - extract required information from an old style queue file */

static void qmgr_message_oldstyle_scan(QMGR_MESSAGE *message)
{
    VSTRING *buf;
    long    orig_offset,
            curr_offset,
            extra_offset;
    int     rec_type;
    char   *start;

    /*
     * Initialize. No early returns or we have a memory leak.
     */
    buf = vstring_alloc(100);
    if ((orig_offset = vstream_ftell(message->fp)) < 0)
	msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));

    /*
     * Rewind to the very beginning to make sure we see all records.
     */
    if (vstream_fseek(message->fp, 0, SEEK_SET) < 0)
	msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));

    /*
     * Scan through the old style queue file. Count the total number of
     * recipients and find the data/extra sections offsets. Note that the new
     * queue files require that data_size equals extra_offset - data_offset,
     * so we set data_size to this as well and ignore the size record itself
     * completely.
     */
    message->rcpt_unread = 0;
    do {
	if ((curr_offset = vstream_ftell(message->fp)) < 0)
	    msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));
	rec_type = rec_get(message->fp, buf, 0);
	start = vstring_str(buf);
	if (rec_type == REC_TYPE_DONE || rec_type == REC_TYPE_RCPT) {
	    message->rcpt_unread++;
	} else if (rec_type == REC_TYPE_MESG) {
	    if ((message->data_offset = vstream_ftell(message->fp)) < 0)
		msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));
	    if ((extra_offset = atol(start)) <= curr_offset)
		msg_fatal("bad extra offset %s file %s",
			  start, VSTREAM_PATH(message->fp));
	    if (vstream_fseek(message->fp, extra_offset, SEEK_SET) < 0)
		msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
	    message->data_size = extra_offset - message->data_offset;
	}
    } while (rec_type > 0 && rec_type != REC_TYPE_END);

    /*
     * Clean up.
     */
    if (vstream_fseek(message->fp, orig_offset, SEEK_SET) < 0)
	msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
    vstring_free(buf);

    /*
     * Sanity checks. Verify that all required information was found,
     * including the queue file end marker.
     */
    if (message->data_offset == 0 || rec_type != REC_TYPE_END)
	msg_fatal("%s: envelope records out of order", message->queue_id);
}

/* qmgr_message_read - read envelope records */

static int qmgr_message_read(QMGR_MESSAGE *message)
{
    VSTRING *buf;
    long    extra_offset;
    int     rec_type;
    long    curr_offset;
    long    save_offset = message->rcpt_offset;	/* save a flag */
    char   *start;
    int     recipient_limit;
    const char *error_text;
    char   *name;
    char   *value;
    char   *orig_rcpt = 0;

    /*
     * Initialize. No early returns or we have a memory leak.
     */
    buf = vstring_alloc(100);

    /*
     * If we re-open this file, skip over on-file recipient records that we
     * already looked at, and refill the in-core recipient address list.
     * 
     * For the first time, the message recipient limit is calculated from the
     * global recipient limit. This is to avoid reading little recipients
     * when the active queue is near empty. When the queue becomes full, only
     * the necessary amount is read in core. Such priming is necessary
     * because there are no message jobs yet.
     * 
     * For the next time, the recipient limit is based solely on the message
     * jobs' positions in the job lists and/or job stacks.
     */
    if (message->rcpt_offset) {
	if (message->rcpt_list.len)
	    msg_panic("%s: recipient list not empty on recipient reload", message->queue_id);
	if (vstream_fseek(message->fp, message->rcpt_offset, SEEK_SET) < 0)
	    msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
	message->rcpt_offset = 0;
	recipient_limit = message->rcpt_limit - message->rcpt_count;
    } else {
	recipient_limit = var_qmgr_rcpt_limit - qmgr_recipient_count;
	if (recipient_limit < message->rcpt_limit)
	    recipient_limit = message->rcpt_limit;
    }
    if (recipient_limit <= 0)
	msg_panic("%s: no recipient slots available", message->queue_id);

    /*
     * Read envelope records. XXX Rely on the front-end programs to enforce
     * record size limits. Read up to recipient_limit recipients from the
     * queue file, to protect against memory exhaustion. Recipient records
     * may appear before or after the message content, so we keep reading
     * from the queue file until we have enough recipients (rcpt_offset != 0)
     * and until we know where the message content starts (data_offset != 0).
     * 
     * Note that the total recipient count record is accurate only for fresh
     * queue files. After some of the recipients are marked as done and the
     * queue file is deferred, it can be used as upper bound estimate only.
     * Fortunately, this poses no major problem on the scheduling algorithm,
     * as the only impact is that the already deferred messages are not
     * chosen by qmgr_job_candidate() as often as they could.
     */
    do {
	if ((curr_offset = vstream_ftell(message->fp)) < 0)
	    msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));
	if (curr_offset == message->data_offset && curr_offset > 0) {
	    extra_offset = curr_offset + message->data_size;
	    if (extra_offset <= curr_offset)
		msg_fatal("bad extra offset %ld file %s",
			  extra_offset, VSTREAM_PATH(message->fp));
	    if (vstream_fseek(message->fp, extra_offset, SEEK_SET) < 0)
		msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
	    curr_offset = extra_offset;
	}
	rec_type = rec_get(message->fp, buf, 0);
	start = vstring_str(buf);
	if (rec_type == REC_TYPE_SIZE) {
	    if (message->data_size == 0) {
		switch (sscanf(start, "%ld %ld %d", &message->data_size,
			    &message->data_offset, &message->rcpt_unread)) {
		case 1:

		    /*
		     * Gather data_size, data_offset and rcpt_unread values
		     * from the old style queue file.
		     */
		    qmgr_message_oldstyle_scan(message);
		    break;
		case 3:

		    /*
		     * No extra work for new style queue files.
		     */
		    break;
		default:
		    msg_fatal("%s: weird size record", message->queue_id);
		    break;
		}
	    }
	} else if (rec_type == REC_TYPE_TIME) {
	    if (message->arrival_time == 0)
		message->arrival_time = atol(start);
	} else if (rec_type == REC_TYPE_FILT) {
	    if (message->filter_xport != 0)
		myfree(message->filter_xport);
	    message->filter_xport = mystrdup(start);
	} else if (rec_type == REC_TYPE_INSP) {
	    if (message->inspect_xport != 0)
		myfree(message->inspect_xport);
	    message->inspect_xport = mystrdup(start);
	} else if (rec_type == REC_TYPE_FROM) {
	    if (message->sender == 0) {
		message->sender = mystrdup(start);
		opened(message->queue_id, message->sender,
		       message->data_size, message->rcpt_unread,
		       "queue %s", message->queue_name);
	    }
	} else if (rec_type == REC_TYPE_DONE) {
	    if (curr_offset > message->unread_offset) {
		message->unread_offset = curr_offset;
		message->rcpt_unread--;
	    }
	} else if (rec_type == REC_TYPE_RCPT) {
	    if (message->rcpt_list.len < recipient_limit) {
		message->rcpt_unread--;
		qmgr_rcpt_list_add(&message->rcpt_list, curr_offset,
				   orig_rcpt ? orig_rcpt : "unknown", start);
		if (orig_rcpt) {
		    myfree(orig_rcpt);
		    orig_rcpt = 0;
		}
		if (message->rcpt_list.len >= recipient_limit) {
		    if ((message->rcpt_offset = vstream_ftell(message->fp)) < 0)
			msg_fatal("vstream_ftell %s: %m",
				  VSTREAM_PATH(message->fp));
		    if (message->data_offset != 0
			&& message->errors_to != 0
			&& message->return_receipt != 0)
			break;
		}
	    }
	} else if (rec_type == REC_TYPE_ATTR) {
	    if ((error_text = split_nameval(start, &name, &value)) != 0) {
		msg_warn("%s: bad attribute: %s: %.200s",
			 message->queue_id, error_text, start);
		break;
	    }
	    /* Allow extra segment to override envelope segment info. */
	    if (strcmp(name, MAIL_ATTR_ENCODING) == 0) {
		if (message->encoding != 0)
		    myfree(message->encoding);
		message->encoding = mystrdup(value);
	    }
	} else if (rec_type == REC_TYPE_ERTO) {
	    if (message->errors_to == 0) {
		message->errors_to = mystrdup(start);
		if (message->data_offset != 0
		    && message->rcpt_offset != 0
		    && message->return_receipt != 0)
		    break;
	    }
	} else if (rec_type == REC_TYPE_RRTO) {
	    if (message->return_receipt == 0) {
		message->return_receipt = mystrdup(start);
		if (message->data_offset != 0
		    && message->rcpt_offset != 0
		    && message->errors_to != 0)
		    break;
	    }
	} else if (rec_type == REC_TYPE_WARN) {
	    if (message->warn_offset == 0) {
		message->warn_offset = curr_offset;
		message->warn_time = atol(start);
	    }
	} else if (rec_type == REC_TYPE_VERP) {
	    if (message->verp_delims == 0) {
		if (verp_delims_verify(start) != 0) {
		    msg_warn("%s: bad VERP record content: \"%s\"",
			     message->queue_id, start);
		} else {
		    message->single_rcpt = 1;
		    message->verp_delims = mystrdup(start);
		}
	    }
	}
	if (orig_rcpt != 0) {
	    if (rec_type != REC_TYPE_DONE)
		msg_warn("%s: out-of-order original recipient record <%.200s>",
			 message->queue_id, start);
	    myfree(orig_rcpt);
	    orig_rcpt = 0;
	}
	if (rec_type == REC_TYPE_ORCP)
	    orig_rcpt = mystrdup(start);
    } while (rec_type > 0 && rec_type != REC_TYPE_END);

    /*
     * Grr.
     */
    if (orig_rcpt != 0) {
	msg_warn("%s: out-of-order original recipient <%.200s>",
		 message->queue_id, start);
	myfree(orig_rcpt);
    }

    /*
     * Avoid clumsiness elsewhere in the program. When sending data across an
     * IPC channel, sending an empty string is more convenient than sending a
     * null pointer.
     */
    if (message->errors_to == 0)
	message->errors_to = mystrdup("");
    if (message->return_receipt == 0)
	message->return_receipt = mystrdup("");
    if (message->encoding == 0)
	message->encoding = mystrdup(MAIL_ATTR_ENC_NONE);

    /*
     * Clean up.
     */
    vstring_free(buf);

    /*
     * Sanity checks. Verify that all required information was found,
     * including the queue file end marker.
     */
    if (message->rcpt_unread < 0
	|| (message->rcpt_offset == 0 && message->rcpt_unread != 0)) {
	msg_warn("%s: rcpt count mismatch (%d)",
		 message->queue_id, message->rcpt_unread);
	message->rcpt_unread = 0;
    }
    if (message->arrival_time == 0
	|| message->sender == 0
	|| message->data_offset == 0
	|| (message->rcpt_offset == 0 && rec_type != REC_TYPE_END)) {
	msg_warn("%s: envelope records out of order", message->queue_id);
	message->rcpt_offset = save_offset;	/* restore flag */
	message->rcpt_unread += message->rcpt_list.len;
	qmgr_rcpt_list_free(&message->rcpt_list);
	qmgr_rcpt_list_init(&message->rcpt_list);
	return (-1);
    } else {
	return (0);
    }
}

/* qmgr_message_update_warn - update the time of next delay warning */

void    qmgr_message_update_warn(QMGR_MESSAGE *message)
{

    /*
     * XXX eventually this should let us schedule multiple warnings, right
     * now it just allows for one.
     */
    if (qmgr_message_open(message)
	|| vstream_fseek(message->fp, message->warn_offset, SEEK_SET) < 0
    || rec_fprintf(message->fp, REC_TYPE_WARN, REC_TYPE_WARN_FORMAT, 0L) < 0
	|| vstream_fflush(message->fp))
	msg_fatal("update queue file %s: %m", VSTREAM_PATH(message->fp));
    qmgr_message_close(message);
}

/* qmgr_message_sort_compare - compare recipient information */

static int qmgr_message_sort_compare(const void *p1, const void *p2)
{
    QMGR_RCPT *rcpt1 = (QMGR_RCPT *) p1;
    QMGR_RCPT *rcpt2 = (QMGR_RCPT *) p2;
    QMGR_QUEUE *queue1;
    QMGR_QUEUE *queue2;
    char   *at1;
    char   *at2;
    int     result;

    /*
     * Compare most significant to least significant recipient attributes.
     * The comparison function must be transitive, so NULL values need to be
     * assigned an ordinal (we set NULL last).
     */

    queue1 = rcpt1->queue;
    queue2 = rcpt2->queue;
    if (queue1 != 0 && queue2 == 0)
	return (-1);
    if (queue1 == 0 && queue2 != 0)
	return (1);
    if (queue1 != 0 && queue2 != 0) {

	/*
	 * Compare message transport.
	 */
	if ((result = strcmp(queue1->transport->name,
			     queue2->transport->name)) != 0)
	    return (result);

	/*
	 * Compare (already lowercased) next-hop hostname.
	 */
	if ((result = strcmp(queue1->name, queue2->name)) != 0)
	    return (result);
    }

    /*
     * Compare recipient domain.
     */
    at1 = strrchr(rcpt1->address, '@');
    at2 = strrchr(rcpt2->address, '@');
    if (at1 == 0 && at2 != 0)
	return (1);
    if (at1 != 0 && at2 == 0)
	return (-1);
    if (at1 != 0 && at2 != 0
	&& (result = strcasecmp(at1, at2)) != 0)
	return (result);

    /*
     * Compare recipient address.
     */
    return (strcmp(rcpt1->address, rcpt2->address));
}

/* qmgr_message_sort - sort message recipient addresses by domain */

static void qmgr_message_sort(QMGR_MESSAGE *message)
{
    qsort((char *) message->rcpt_list.info, message->rcpt_list.len,
	  sizeof(message->rcpt_list.info[0]), qmgr_message_sort_compare);
    if (msg_verbose) {
	QMGR_RCPT_LIST list = message->rcpt_list;
	QMGR_RCPT *rcpt;

	msg_info("start sorted recipient list");
	for (rcpt = list.info; rcpt < list.info + list.len; rcpt++)
	    msg_info("qmgr_message_sort: %s", rcpt->address);
	msg_info("end sorted recipient list");
    }
}

/* qmgr_message_resolve - resolve recipients */

static void qmgr_message_resolve(QMGR_MESSAGE *message)
{
    static ARGV *defer_xport_argv;
    QMGR_RCPT_LIST list = message->rcpt_list;
    QMGR_RCPT *recipient;
    QMGR_TRANSPORT *transport = 0;
    QMGR_QUEUE *queue = 0;
    RESOLVE_REPLY reply;
    char   *at;
    char  **cpp;
    char   *nexthop;
    int     len;

#define STREQ(x,y)	(strcmp(x,y) == 0)
#define STR		vstring_str
#define LEN		VSTRING_LEN
#define UPDATE(ptr,new)	{ myfree(ptr); ptr = mystrdup(new); }

    resolve_clnt_init(&reply);
    for (recipient = list.info; recipient < list.info + list.len; recipient++) {

	/*
	 * Resolve the destination to (transport, nexthop, address). The
	 * result address may differ from the one specified by the sender.
	 */
	if (var_sender_routing == 0) {
	    resolve_clnt_query(recipient->address, &reply);
	    if (reply.flags & RESOLVE_FLAG_FAIL) {
		qmgr_defer_recipient(message, recipient,
				     "address resolver failure");
		continue;
	    }
	    if (reply.flags & RESOLVE_FLAG_ERROR) {
		qmgr_bounce_recipient(message, recipient,
				      "bad address syntax: \"%s\"",
				      recipient->address);
		continue;
	    }
	} else {
	    resolve_clnt_query(message->sender, &reply);
	    if (reply.flags & RESOLVE_FLAG_FAIL) {
		qmgr_defer_recipient(message, recipient,
				     "address resolver failure");
		continue;
	    }
	    if (reply.flags & RESOLVE_FLAG_ERROR) {
		qmgr_bounce_recipient(message, recipient,
				      "bad address syntax: \"%s\"",
				      message->sender);
		continue;
	    }
	    vstring_strcpy(reply.recipient, recipient->address);
	}
	if (message->filter_xport) {
	    vstring_strcpy(reply.transport, message->filter_xport);
	    if ((nexthop = split_at(STR(reply.transport), ':')) == 0
		|| *nexthop == 0)
		nexthop = var_myhostname;
	    vstring_strcpy(reply.nexthop, nexthop);
	} else {
	    if (!STREQ(recipient->address, STR(reply.recipient)))
		UPDATE(recipient->address, STR(reply.recipient));
	}
	if (recipient->address[0] == 0) {
	    qmgr_bounce_recipient(message, recipient,
				  "null recipient address");
	    continue;
	}

	/*
	 * XXX The nexthop destination is also used as lookup key for the
	 * per-destination queue. Fold the nexthop to lower case so that we
	 * don't have multiple queues for the same site.
	 */
	lowercase(STR(reply.nexthop));

	/*
	 * Bounce recipient addresses that start with `-'. External commands
	 * may misinterpret such addresses as command-line options.
	 * 
	 * In theory I could say people should always carefully set up their
	 * master.cf pipe mailer entries with `--' before the first
	 * non-option argument, but mistakes will happen regardless.
	 * 
	 * Therefore the protection is put in place here, in the queue manager,
	 * where it cannot be bypassed.
	 */
	if (var_allow_min_user == 0 && recipient->address[0] == '-') {
	    qmgr_bounce_recipient(message, recipient,
				  "invalid recipient syntax: \"%s\"",
				  recipient->address);
	    continue;
	}

	/*
	 * Queues are identified by the transport name and by the next-hop
	 * hostname. When the delivery agent accepts only one recipient per
	 * delivery, give each recipient its own queue, so that deliveries to
	 * different recipients of the same message can happen in parallel.
	 * This also has the benefit that one bad recipient cannot interfere
	 * with deliveries to other recipients. XXX Should split the address
	 * on the recipient delimiter if one is defined, but doing a proper
	 * job requires knowledge of local aliases. Yuck! I don't want to
	 * duplicate delivery-agent specific knowledge in the queue manager.
	 * 
	 * XXX The nexthop field is overloaded to serve as destination and as
	 * queue name. Should have separate fields for queue name and for
	 * destination, so that we don't have to make a special case for the
	 * error delivery agent (where nexthop is arbitrary text). See also:
	 * qmgr_deliver.c.
	 */
	at = strrchr(STR(reply.recipient), '@');
	len = (at ? (at - STR(reply.recipient)) : strlen(STR(reply.recipient)));

	/*
	 * Look up or instantiate the proper transport. We're working a
	 * little ahead, doing queue management stuff that used to be done
	 * way down.
	 */
	if (transport == 0 || !STREQ(transport->name, STR(reply.transport))) {
	    if ((transport = qmgr_transport_find(STR(reply.transport))) == 0)
		transport = qmgr_transport_create(STR(reply.transport));
	    queue = 0;
	}
	if (strcmp(transport->name, MAIL_SERVICE_ERROR) != 0
	    && transport->recipient_limit == 1) {
	    VSTRING_SPACE(reply.nexthop, len + 2);
	    memmove(STR(reply.nexthop) + len + 1, STR(reply.nexthop),
		    LEN(reply.nexthop) + 1);
	    memcpy(STR(reply.nexthop), STR(reply.recipient), len);
	    STR(reply.nexthop)[len] = '@';
	    lowercase(STR(reply.nexthop));
	}

	/*
	 * Discard mail to the local double bounce address here, so this
	 * system can run without a local delivery agent. They'd still have
	 * to configure something for mail directed to the local postmaster,
	 * though, but that is an RFC requirement anyway.
	 * 
	 * XXX This lookup should be done in the resolver, and the mail should
	 * be directed to a general-purpose null delivery agent.
	 */
	if (reply.flags & RESOLVE_CLASS_LOCAL) {
	    if (strncasecmp(STR(reply.recipient), var_double_bounce_sender,
			    len) == 0
		&& !var_double_bounce_sender[len]) {
		sent(message->queue_id, recipient->orig_rcpt,
		     recipient->address, "none", message->arrival_time,
		     "discarded");
		deliver_completed(message->fp, recipient->offset);
		msg_warn("%s: undeliverable postmaster notification discarded",
			 message->queue_id);
		continue;
	    }
	}

	/*
	 * Optionally defer deliveries over specific transports, unless the
	 * restriction is lifted temporarily.
	 */
	if (*var_defer_xports && (message->qflags & QMGR_FLUSH_DEAD) == 0) {
	    if (defer_xport_argv == 0)
		defer_xport_argv = argv_split(var_defer_xports, " \t\r\n,");
	    for (cpp = defer_xport_argv->argv; *cpp; cpp++)
		if (strcmp(*cpp, STR(reply.transport)) == 0)
		    break;
	    if (*cpp) {
		qmgr_defer_recipient(message, recipient, "deferred transport");
		continue;
	    }
	}

	/*
	 * XXX Gross hack alert. We want to group recipients by transport and
	 * by next-hop hostname, in order to minimize the number of network
	 * transactions. However, it would be wasteful to have an in-memory
	 * resolver reply structure for each in-core recipient. Instead, we
	 * bind each recipient to an in-core queue instance which is needed
	 * anyway. That gives all information needed for recipient grouping.
	 */
#if 0

	/*
	 * Look up or instantiate the proper transport.
	 */
	if (transport == 0 || !STREQ(transport->name, STR(reply.transport))) {
	    if ((transport = qmgr_transport_find(STR(reply.transport))) == 0)
		transport = qmgr_transport_create(STR(reply.transport));
	    queue = 0;
	}
#endif

	/*
	 * This transport is dead. Defer delivery to this recipient.
	 */
	if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) != 0) {
	    qmgr_defer_recipient(message, recipient, transport->reason);
	    continue;
	}

	/*
	 * This transport is alive. Find or instantiate a queue for this
	 * recipient.
	 */
	if (queue == 0 || !STREQ(queue->name, STR(reply.nexthop))) {
	    if ((queue = qmgr_queue_find(transport, STR(reply.nexthop))) == 0)
		queue = qmgr_queue_create(transport, STR(reply.nexthop));
	}

	/*
	 * This queue is dead. Defer delivery to this recipient.
	 */
	if (queue->window == 0) {
	    qmgr_defer_recipient(message, recipient, queue->reason);
	    continue;
	}

	/*
	 * This queue is alive. Bind this recipient to this queue instance.
	 */
	recipient->queue = queue;
    }
    resolve_clnt_free(&reply);
}

/* qmgr_message_assign - assign recipients to specific delivery requests */

static void qmgr_message_assign(QMGR_MESSAGE *message)
{
    QMGR_RCPT_LIST list = message->rcpt_list;
    QMGR_RCPT *recipient;
    QMGR_ENTRY *entry = 0;
    QMGR_QUEUE *queue;
    QMGR_JOB *job = 0;
    QMGR_PEER *peer = 0;

    /*
     * Try to bundle as many recipients in a delivery request as we can. When
     * the recipient resolves to the same site and transport as the previous
     * recipient, do not create a new queue entry, just move that recipient
     * to the recipient list of the existing queue entry. All this provided
     * that we do not exceed the transport-specific limit on the number of
     * recipients per transaction. Skip recipients with a dead transport or
     * destination.
     */
#define LIMIT_OK(limit, count) ((limit) == 0 || ((count) < (limit)))

    for (recipient = list.info; recipient < list.info + list.len; recipient++) {
	if ((queue = recipient->queue) != 0) {
	    if (message->single_rcpt || entry == 0 || entry->queue != queue
		|| !LIMIT_OK(queue->transport->recipient_limit,
			     entry->rcpt_list.len)) {

		/*
		 * Lookup or instantiate the message job if necessary.
		 */
		if (job == 0 || queue->transport != job->transport) {
		    job = qmgr_job_obtain(message, queue->transport);
		    peer = 0;
		}

		/*
		 * Lookup or instantiate job peer if necessary.
		 */
		if (peer == 0 || queue != peer->queue) {
		    if ((peer = qmgr_peer_find(job, queue)) == 0)
			peer = qmgr_peer_create(job, queue);
		}

		/*
		 * Create new peer entry.
		 */
		entry = qmgr_entry_create(peer, message);
		job->read_entries++;
	    }

	    /*
	     * Add the recipient to the current entry and increase all those
	     * recipient counters accordingly.
	     */
	    qmgr_rcpt_list_add(&entry->rcpt_list, recipient->offset,
			       recipient->orig_rcpt, recipient->address);
	    job->rcpt_count++;
	    message->rcpt_count++;
	    qmgr_recipient_count++;
	}
    }

    /*
     * Release the message recipient list and reinitialize it for the next
     * time.
     */
    qmgr_rcpt_list_free(&message->rcpt_list);
    qmgr_rcpt_list_init(&message->rcpt_list);

    /*
     * Note that even if qmgr_job_obtain() reset the job candidate cache of
     * all transports to which we assigned new recipients, this message may
     * have other jobs which we didn't touch at all this time. But the number
     * of unread recipients affecting the candidate selection might have
     * changed considerably, so we must invalidate the caches if it might be
     * of some use.
     */
    for (job = message->job_list.next; job; job = job->message_peers.next)
	if (job->selected_entries < job->read_entries
	    && job->blocker_tag != job->transport->blocker_tag)
	    job->transport->candidate_cache_current = 0;
}

/* qmgr_message_move_limits - recycle unused recipient slots */

static void qmgr_message_move_limits(QMGR_MESSAGE *message)
{
    QMGR_JOB *job;

    for (job = message->job_list.next; job; job = job->message_peers.next)
	qmgr_job_move_limits(job);
}

/* qmgr_message_free - release memory for in-core message structure */

void    qmgr_message_free(QMGR_MESSAGE *message)
{
    QMGR_JOB *job;

    if (message->refcount != 0)
	msg_panic("qmgr_message_free: reference len: %d", message->refcount);
    if (message->fp)
	msg_panic("qmgr_message_free: queue file is open");
    while ((job = message->job_list.next) != 0)
	qmgr_job_free(job);
    myfree(message->queue_id);
    myfree(message->queue_name);
    if (message->encoding)
	myfree(message->encoding);
    if (message->sender)
	myfree(message->sender);
    if (message->verp_delims)
	myfree(message->verp_delims);
    if (message->errors_to)
	myfree(message->errors_to);
    if (message->return_receipt)
	myfree(message->return_receipt);
    if (message->filter_xport)
	myfree(message->filter_xport);
    if (message->inspect_xport)
	myfree(message->inspect_xport);
    qmgr_rcpt_list_free(&message->rcpt_list);
    qmgr_message_count--;
    myfree((char *) message);
}

/* qmgr_message_alloc - create in-core message structure */

QMGR_MESSAGE *qmgr_message_alloc(const char *queue_name, const char *queue_id,
				         int qflags)
{
    char   *myname = "qmgr_message_alloc";
    QMGR_MESSAGE *message;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, queue_name, queue_id);

    /*
     * Create an in-core message structure.
     */
    message = qmgr_message_create(queue_name, queue_id, qflags);

    /*
     * Extract message envelope information: time of arrival, sender address,
     * recipient addresses. Skip files with malformed envelope information.
     */
#define QMGR_LOCK_MODE (MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT)

    if (qmgr_message_open(message) < 0) {
	qmgr_message_free(message);
	return (0);
    }
    if (myflock(vstream_fileno(message->fp), INTERNAL_LOCK, QMGR_LOCK_MODE) < 0) {
	msg_info("%s: skipped, still being delivered", queue_id);
	qmgr_message_close(message);
	qmgr_message_free(message);
	return (QMGR_MESSAGE_LOCKED);
    }
    if (qmgr_message_read(message) < 0) {
	qmgr_message_close(message);
	qmgr_message_free(message);
	return (0);
    } else {

	/*
	 * Reset the defer log. This code should not be here, but we must
	 * reset the defer log *after* acquiring the exclusive lock on the
	 * queue file and *before* resolving new recipients. Since all those
	 * operations are encapsulated so nicely by this routine, the defer
	 * log reset has to be done here as well.
	 */
	if (mail_queue_remove(MAIL_QUEUE_DEFER, queue_id) && errno != ENOENT)
	    msg_fatal("%s: %s: remove %s %s: %m", myname,
		      queue_id, MAIL_QUEUE_DEFER, queue_id);
	qmgr_message_sort(message);
	qmgr_message_resolve(message);
	qmgr_message_sort(message);
	qmgr_message_assign(message);
	qmgr_message_close(message);
	if (message->rcpt_offset == 0)
	    qmgr_message_move_limits(message);
	return (message);
    }
}

/* qmgr_message_realloc - refresh in-core message structure */

QMGR_MESSAGE *qmgr_message_realloc(QMGR_MESSAGE *message)
{
    char   *myname = "qmgr_message_realloc";

    /*
     * Sanity checks.
     */
    if (message->rcpt_offset <= 0)
	msg_panic("%s: invalid offset: %ld", myname, message->rcpt_offset);
    if (msg_verbose)
	msg_info("%s: %s %s offset %ld", myname, message->queue_name,
		 message->queue_id, message->rcpt_offset);

    /*
     * Extract recipient addresses. Skip files with malformed envelope
     * information.
     */
    if (qmgr_message_open(message) < 0)
	return (0);
    if (qmgr_message_read(message) < 0) {
	qmgr_message_close(message);
	return (0);
    } else {
	qmgr_message_sort(message);
	qmgr_message_resolve(message);
	qmgr_message_sort(message);
	qmgr_message_assign(message);
	qmgr_message_close(message);
	if (message->rcpt_offset == 0)
	    qmgr_message_move_limits(message);
	return (message);
    }
}
