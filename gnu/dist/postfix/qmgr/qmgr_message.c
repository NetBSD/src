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
/*	read or that the file contained incorrect information.
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <stdlib.h>
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
#include <stringops.h>

/* Global library. */

#include <dict.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <canon_addr.h>
#include <record.h>
#include <rec_type.h>
#include <sent.h>
#include <deliver_completed.h>
#include <mail_addr_find.h>
#include <opened.h>
#include <resolve_local.h>

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
    message->data_offset = 0;
    message->queue_id = mystrdup(queue_id);
    message->queue_name = mystrdup(queue_name);
    message->sender = 0;
    message->errors_to = 0;
    message->return_receipt = 0;
    message->data_size = 0;
    message->warn_offset = 0;
    message->warn_time = 0;
    message->rcpt_offset = 0;
    qmgr_rcpt_list_init(&message->rcpt_list);
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

/* qmgr_message_read - read envelope records */

static int qmgr_message_read(QMGR_MESSAGE *message)
{
    VSTRING *buf;
    long    extra_offset;
    int     rec_type;
    long    curr_offset;
    long    save_offset = message->rcpt_offset;	/* save a flag */
    char   *start;
    struct stat st;

    /*
     * Initialize. No early returns or we have a memory leak.
     */
    buf = vstring_alloc(100);

    /*
     * If we re-open this file, skip over on-file recipient records that we
     * already looked at, and reset the in-core recipient address list.
     */
    if (message->rcpt_offset) {
	if (message->rcpt_list.len)
	    msg_panic("%s: recipient list not empty on recipient reload", message->queue_id);
	if (vstream_fseek(message->fp, message->rcpt_offset, SEEK_SET) < 0)
	    msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
	message->rcpt_offset = 0;
    }

    /*
     * Read envelope records. XXX Rely on the front-end programs to enforce
     * record size limits. Read up to var_qmgr_rcpt_limit recipients from the
     * queue file, to protect against memory exhaustion. Recipient records
     * may appear before or after the message content, so we keep reading
     * from the queue file until we have enough recipients (rcpt_offset != 0)
     * and until we know where the message content starts (data_offset != 0).
     * 
     * When reading recipients from queue file, stop reading when we reach a
     * per-message in-core recipient limit rather than a global in-core
     * recipient limit. Use the global recipient limit only in order to stop
     * opening queue files. The purpose is to achieve equal delay for
     * messages with recipient counts up to var_qmgr_rcpt_limit recipients.
     * 
     * If we would read recipients up to a global recipient limit, the average
     * number of in-core recipients per message would asymptotically approach
     * (global recipient limit)/(active queue size limit), which gives equal
     * delay per recipient rather than equal delay per message.
     */
    do {
	if ((curr_offset = vstream_ftell(message->fp)) < 0)
	    msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));
	rec_type = rec_get(message->fp, buf, 0);
	start = vstring_str(buf);
	if (rec_type == REC_TYPE_SIZE) {
	    if (message->data_size == 0)
		message->data_size = atol(start);
	} else if (rec_type == REC_TYPE_TIME) {
	    if (message->arrival_time == 0)
		message->arrival_time = atol(start);
	} else if (rec_type == REC_TYPE_FROM) {
	    if (message->sender == 0) {
		message->sender = mystrdup(start);
		opened(message->queue_id, message->sender,
		       message->data_size, "queue %s", message->queue_name);
	    }
	} else if (rec_type == REC_TYPE_RCPT) {
#define FUDGE(x)	((x) * (var_qmgr_fudge / 100.0))
	    if (message->rcpt_list.len < FUDGE(var_qmgr_rcpt_limit)) {
		qmgr_rcpt_list_add(&message->rcpt_list, curr_offset, start);
		if (message->rcpt_list.len >= FUDGE(var_qmgr_rcpt_limit)) {
		    if ((message->rcpt_offset = vstream_ftell(message->fp)) < 0)
			msg_fatal("vstream_ftell %s: %m",
				  VSTREAM_PATH(message->fp));
		    if (message->data_offset != 0
			&& message->errors_to != 0
			&& message->return_receipt != 0)
			break;
		}
	    }
	} else if (rec_type == REC_TYPE_MESG) {
	    if ((message->data_offset = vstream_ftell(message->fp)) < 0)
		msg_fatal("vstream_ftell %s: %m", VSTREAM_PATH(message->fp));
	    if (message->rcpt_offset != 0
		&& message->errors_to != 0
		&& message->return_receipt != 0)
		break;
	    if ((extra_offset = atol(start)) < curr_offset) {
		msg_warn("bad extra offset %s file %s",
			 start, VSTREAM_PATH(message->fp));
		break;
	    }
	    if (vstream_fseek(message->fp, extra_offset, SEEK_SET) < 0)
		msg_fatal("seek file %s: %m", VSTREAM_PATH(message->fp));
	} else if (rec_type == REC_TYPE_ERTO) {
	    if (message->errors_to == 0)
		message->errors_to = mystrdup(start);
	} else if (rec_type == REC_TYPE_RRTO) {
	    if (message->return_receipt == 0)
		message->return_receipt = mystrdup(start);
	} else if (rec_type == REC_TYPE_WARN) {
	    if (message->warn_offset == 0) {
		message->warn_offset = curr_offset;
		message->warn_time = atol(start);
	    }
	}
    } while (rec_type > 0 && rec_type != REC_TYPE_END);

    /*
     * If there is no size record, use the queue file size instead.
     */
    if (message->data_size == 0) {
	if (fstat(vstream_fileno(message->fp), &st) < 0)
	    msg_fatal("fstat %s: %m", VSTREAM_PATH(message->fp));
	message->data_size = st.st_size;
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

    /*
     * Clean up.
     */
    vstring_free(buf);

    /*
     * Sanity checks. Verify that all required information was found,
     * including the queue file end marker.
     */
    if (message->arrival_time == 0
	|| message->sender == 0
	|| message->data_offset == 0
	|| (message->rcpt_offset == 0 && rec_type != REC_TYPE_END)) {
	msg_warn("%s: envelope records out of order", message->queue_id);
	message->rcpt_offset = save_offset;	/* restore flag */
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
     */
    if ((queue1 = rcpt1->queue) != 0 && (queue2 = rcpt2->queue) != 0) {

	/*
	 * Compare message transport.
	 */
	if ((result = strcasecmp(queue1->transport->name,
				 queue2->transport->name)) != 0)
	    return (result);

	/*
	 * Compare next-hop hostname.
	 */
	if ((result = strcasecmp(queue1->name, queue2->name)) != 0)
	    return (result);
    }

    /*
     * Compare recipient domain.
     */
    if ((at1 = strrchr(rcpt1->address, '@')) != 0
	&& (at2 = strrchr(rcpt2->address, '@')) != 0
	&& (result = strcasecmp(at1, at2)) != 0)
	return (result);

    /*
     * Compare recipient address.
     */
    return (strcasecmp(rcpt1->address, rcpt2->address));
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
    const char *newloc;
    char   *at;
    char  **cpp;
    char   *domain;
    const char *junk;

#define STREQ(x,y)	(strcasecmp(x,y) == 0)
#define STR		vstring_str
#define UPDATE(ptr,new)	{ myfree(ptr); ptr = mystrdup(new); }

    resolve_clnt_init(&reply);
    for (recipient = list.info; recipient < list.info + list.len; recipient++) {

	/*
	 * This may be a bit late in the game, but it is the most convenient
	 * place to scrutinize the destination address syntax. We have a
	 * complete queue file, so bouncing is easy. That luxury is not
	 * available to the cleanup service. The main issue is that we want
	 * to have this test in one place, instead of having to do this in
	 * every front-ent program.
	 */
	if ((at = strrchr(recipient->address, '@')) != 0
	    && (at + 1)[strspn(at + 1, "[]0123456789.")] != 0
	    && valid_hostname(at + 1) == 0) {
	    qmgr_bounce_recipient(message, recipient,
				  "bad host/domain syntax: \"%s\"", at + 1);
	    continue;
	}

	/*
	 * Resolve the destination to (transport, nexthop, address). The
	 * result address may differ from the one specified by the sender.
	 */
	resolve_clnt_query(recipient->address, &reply);
	if (!STREQ(recipient->address, STR(reply.recipient)))
	    UPDATE(recipient->address, STR(reply.recipient));

	/*
	 * XXX The nexthop destination is also used as lookup key for the
	 * per-destination queue. Fold the nexthop to lower case so that we
	 * don't have multiple queues for the same site.
	 */
	lowercase(STR(reply.nexthop));

	/*
	 * Bounce recipients that have moved. We do it here instead of in the
	 * local delivery agent. The benefit is that we can bounce mail for
	 * virtual addresses, not just local addresses only, and that there
	 * is no need to run a local delivery agent just for the sake of
	 * relocation notices. The downside is that this table has no effect
	 * on local alias expansion results, so that mail will have to make
	 * almost an entire iteration through the mail system.
	 */
#define IGNORE_ADDR_EXTENSION	((char **) 0)

	if (qmgr_relocated != 0) {
	    if ((newloc = mail_addr_find(qmgr_relocated, recipient->address,
					 IGNORE_ADDR_EXTENSION)) != 0) {
		qmgr_bounce_recipient(message, recipient,
				      "user has moved to %s", newloc);
		continue;
	    } else if (dict_errno != 0) {
		qmgr_defer_recipient(message, recipient->address,
				     "relocated map lookup failure");
		continue;
	    }
	}

	/*
	 * Bounce mail to non-existent users in virtual domains.
	 */
	if (qmgr_virtual != 0
	    && (at = strrchr(recipient->address, '@')) != 0
	    && !resolve_local(at + 1)) {
	    domain = lowercase(mystrdup(at + 1));
	    junk = maps_find(qmgr_virtual, domain, 0);
	    myfree(domain);
	    if (junk) {
		qmgr_bounce_recipient(message, recipient,
				"unknown user: \"%s\"", recipient->address);
		continue;
	    }
	}

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
	 * hostname. When the destination is local (no next hop), derive the
	 * queue name from the recipient name. XXX Should split the address
	 * on the recipient delimiter if one is defined, but doing a proper
	 * job requires knowledge of local aliases. Yuck! I don't want to
	 * duplicate delivery-agent specific knowledge in the queue manager.
	 */
	if ((at = strrchr(STR(reply.recipient), '@')) == 0
	    || resolve_local(at + 1)) {
#if 0
	    vstring_strcpy(reply.nexthop, STR(reply.recipient));
	    (void) split_at_right(STR(reply.nexthop), '@');
#endif
#if 0
	    if (*var_rcpt_delim)
		(void) split_addr(STR(reply.nexthop), *var_rcpt_delim);
#endif

	    /*
	     * Discard mail to the local double bounce address here, so this
	     * system can run without a local delivery agent. They'd still
	     * have to configure something for mail directed to the local
	     * postmaster, though, but that is an RFC requirement anyway.
	     */
	    if (strncasecmp(STR(reply.recipient), var_double_bounce_sender,
			    at - STR(reply.recipient)) == 0
		&& !var_double_bounce_sender[at - STR(reply.recipient)]) {
		sent(message->queue_id, recipient->address,
		     "none", message->arrival_time, "discarded");
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
	if (*var_defer_xports && (message->qflags & QMGR_SCAN_ALL) == 0) {
	    if (defer_xport_argv == 0)
		defer_xport_argv = argv_split(var_defer_xports, " \t\r\n,");
	    for (cpp = defer_xport_argv->argv; *cpp; cpp++)
		if (strcasecmp(*cpp, STR(reply.transport)) == 0)
		    break;
	    if (*cpp) {
		qmgr_defer_recipient(message, recipient->address,
				     "deferred transport");
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

	/*
	 * Look up or instantiate the proper transport.
	 */
	if (transport == 0 || !STREQ(transport->name, STR(reply.transport))) {
	    if ((transport = qmgr_transport_find(STR(reply.transport))) == 0)
		transport = qmgr_transport_create(STR(reply.transport));
	    queue = 0;
	}

	/*
	 * This transport is dead. Defer delivery to this recipient.
	 */
	if ((transport->flags & QMGR_TRANSPORT_STAT_DEAD) != 0) {
	    qmgr_defer_recipient(message, recipient->address, transport->reason);
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
	    qmgr_defer_recipient(message, recipient->address, queue->reason);
	    continue;
	}

	/*
	 * This queue is a hog. Defer this recipient until the queue drains.
	 * When a site accumulates a large backlog, Postfix will deliver a
	 * little chunk and hammer the disk as it defers the remainder of the
	 * backlog and searches the deferred queue for deliverable mail.
	 */
	if (var_qmgr_hog < 100) {
	    if (queue->todo_refcount + queue->busy_refcount
		> (var_qmgr_hog / 100.0)
		* (qmgr_recipient_count > 0.8 * var_qmgr_rcpt_limit ?
		   qmgr_message_count : var_qmgr_active_limit)) {
		qmgr_defer_recipient(message, recipient->address,
				     "site destination queue overflow");
		continue;
	    }
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
		|| !LIMIT_OK(entry->queue->transport->recipient_limit,
			     entry->rcpt_list.len)) {
		entry = qmgr_entry_create(queue, message);
	    }
	    qmgr_rcpt_list_add(&entry->rcpt_list, recipient->offset, recipient->address);
	    qmgr_recipient_count++;
	}
    }
    qmgr_rcpt_list_free(&message->rcpt_list);
    qmgr_rcpt_list_init(&message->rcpt_list);
}

/* qmgr_message_free - release memory for in-core message structure */

void    qmgr_message_free(QMGR_MESSAGE *message)
{
    if (message->refcount != 0)
	msg_panic("qmgr_message_free: reference len: %d", message->refcount);
    if (message->fp)
	msg_panic("qmgr_message_free: queue file is open");
    myfree(message->queue_id);
    myfree(message->queue_name);
    if (message->sender)
	myfree(message->sender);
    if (message->errors_to)
	myfree(message->errors_to);
    if (message->return_receipt)
	myfree(message->return_receipt);
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
#define QMGR_LOCK_MODE (MYFLOCK_EXCLUSIVE | MYFLOCK_NOWAIT)

    if (qmgr_message_open(message) < 0) {
	qmgr_message_free(message);
	return (0);
    }
    if (myflock(vstream_fileno(message->fp), QMGR_LOCK_MODE) < 0) {
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
	return (message);
    }
}
