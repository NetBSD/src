/*++
/* NAME
/*	cleanup 8
/* SUMMARY
/*	canonicalize and enqueue Postfix message
/* SYNOPSIS
/*	\fBcleanup\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBcleanup\fR daemon processes inbound mail, inserts it
/*	into the \fBincoming\fR mail queue, and informs the queue
/*	manager of its arrival.
/*
/*	The \fBcleanup\fR daemon always performs the following transformations:
/* .IP \(bu
/*	Insert missing message headers: (\fBResent-\fR) \fBFrom:\fR,
/*	\fBTo:\fR, \fBMessage-Id:\fR, and \fBDate:\fR.
/* .IP \(bu
/*	Extract envelope recipient addresses from (\fBResent-\fR) \fBTo:\fR,
/*	\fBCc:\fR and \fBBcc:\fR message headers when no recipients are
/*	specified in the message envelope.
/* .IP \(bu
/*	Transform envelope and header addresses to the standard
/*	\fIuser@fully-qualified-domain\fR form that is expected by other
/*	Postfix programs.
/*	This task is delegated to the \fBtrivial-rewrite\fR(8) daemon.
/* .IP \(bu
/*	Eliminate duplicate envelope recipient addresses.
/* .PP
/*	The following address transformations are optional:
/* .IP \(bu
/*	Optionally, rewrite all envelope and header addresses according
/*	to the mappings specified in the \fBcanonical\fR(5) lookup tables.
/* .IP \(bu
/*	Optionally, masquerade envelope sender addresses and message
/*	header addresses (i.e. strip host or domain information below
/*	all domains listed in the \fBmasquerade_domains\fR parameter,
/*	except for user names listed in \fBmasquerade_exceptions\fR).
/*	Address masquerading does not affect envelope recipients.
/* .IP \(bu
/*	Optionally, expand envelope recipients according to information
/*	found in the \fBvirtual\fR(5) lookup tables.
/* .PP
/*	The \fBcleanup\fR daemon performs sanity checks on the content of
/*	each message. When it finds a problem, by default it returns a
/*	diagnostic status to the client, and leaves it up to the client
/*	to deal with the problem. Alternatively, the client can request
/*	the \fBcleanup\fR daemon to bounce the message back to the sender
/*	in case of trouble.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* BUGS
/*	Table-driven rewriting rules make it hard to express \fBif then
/*	else\fR and other logical relationships.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Content filtering
/* .IP \fBbody_checks\fR
/*	Lookup tables with content filters for message body lines.
/*	These filters see physical lines one at a time, in chunks of
/*	at most line_length_limit bytes.
/* .IP \fBheader_checks\fR
/*	Lookup tables with content filters for message header lines.
/*	These filters see logical headers one at a time, including headers
/*	that span multiple lines.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalways_bcc\fR
/*	Address to send a copy of each message that enters the system.
/* .IP \fBhopcount_limit\fR
/*	Limit the number of \fBReceived:\fR message headers.
/* .IP \fBrecipients_witheld_header\fR
/*	The header line that is inserted when no recipients were
/*	specified in (Resent-)To: or (Resent-)Cc: message headers.
/* .SH "Address transformations"
/* .ad
/* .fi
/* .IP \fBempty_address_recipient\fR
/*	The destination for undeliverable mail from <>. This
/*	substitution is done before all other address rewriting.
/* .IP \fBcanonical_maps\fR
/*	Address mapping lookup table for sender and recipient addresses
/*	in envelopes and headers.
/* .IP \fBrecipient_canonical_maps\fR
/*	Address mapping lookup table for envelope and header recipient
/*	addresses.
/* .IP \fBsender_canonical_maps\fR
/*	Address mapping lookup table for envelope and header sender
/*	addresses.
/* .IP \fBmasquerade_domains\fR
/*	List of domains that hide their subdomain structure.
/* .IP \fBmasquerade_exceptions\fR
/*	List of user names that are not subject to address masquerading.
/* .IP \fBvirtual_maps\fR
/*	Address mapping lookup table for envelope recipient addresses.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBduplicate_filter_limit\fR
/*	Limit the number of envelope recipients that are remembered.
/* .IP \fBheader_size_limit\fR
/*	Limit the amount of memory in bytes used to process a message header.
/* SEE ALSO
/*	canonical(5) canonical address lookup table format
/*	qmgr(8) queue manager daemon
/*	syslogd(8) system logging
/*	trivial-rewrite(8) address rewriting
/*	virtual(5) virtual address lookup table format
/* FILES
/*	/etc/postfix/canonical*, canonical mapping table
/*	/etc/postfix/virtual*, virtual mapping table
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
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <dict.h>

/* Global library. */

#include <mail_conf.h>
#include <cleanup_user.h>
#include <mail_queue.h>
#include <mail_proto.h>
#include <opened.h>
#include <bounce.h>
#include <mail_params.h>
#include <mail_stream.h>
#include <mail_addr.h>
#include <ext_prop.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "cleanup.h"

 /*
  * Global state: any queue files that we have open, so that the error
  * handler can clean up in case of trouble.
  */
char   *cleanup_path;			/* queue file name */

 /*
  * Tunable parameters.
  */
int     var_hopcount_limit;		/* max mailer hop count */
int     var_header_limit;		/* max header length */
char   *var_canonical_maps;		/* common canonical maps */
char   *var_send_canon_maps;		/* sender canonical maps */
char   *var_rcpt_canon_maps;		/* recipient canonical maps */
char   *var_virtual_maps;		/* virtual maps */
char   *var_masq_domains;		/* masquerade domains */
char   *var_masq_exceptions;		/* users not masqueraded */
char   *var_header_checks;		/* any header checks */
char   *var_body_checks;		/* any body checks */
int     var_dup_filter_limit;		/* recipient dup filter */
char   *var_empty_addr;			/* destination of bounced bounces */
int     var_delay_warn_time;		/* delay that triggers warning */
char   *var_prop_extension;		/* propagate unmatched extension */
char   *var_always_bcc;
char   *var_rcpt_witheld;		/* recipients not disclosed */

 /*
  * Mappings.
  */
MAPS   *cleanup_comm_canon_maps;
MAPS   *cleanup_send_canon_maps;
MAPS   *cleanup_rcpt_canon_maps;
MAPS   *cleanup_header_checks;
MAPS   *cleanup_body_checks;
MAPS   *cleanup_virtual_maps;
ARGV   *cleanup_masq_domains;

 /*
  * Address extension propagation restrictions.
  */
int     cleanup_ext_prop_mask;

/* cleanup_service - process one request to inject a message into the queue */

static void cleanup_service(VSTREAM *src, char *unused_service, char **argv)
{
    char   *junk;
    static char *log_queues[] = {
	MAIL_QUEUE_DEFER,
	MAIL_QUEUE_BOUNCE,
	0,
    };
    char  **cpp;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Initialize.
     */
    cleanup_state_alloc();
    cleanup_src = src;

    /*
     * Open the queue file. Send the queue ID to the client so they can use
     * it for logging purposes. For example, the SMTP server sends the queue
     * id to the SMTP client after completion of the DATA command; and when
     * the local delivery agent forwards a message, it logs the new queue id
     * together with the old one. All this is done to make it easier for mail
     * admins to follow a message while it hops from machine to machine.
     * 
     * Save the queue file name, so that the runtime error handler can clean up
     * in case of problems.
     */
    cleanup_handle = mail_stream_file(MAIL_QUEUE_INCOMING,
				      MAIL_CLASS_PUBLIC, MAIL_SERVICE_QUEUE);
    cleanup_dst = cleanup_handle->stream;
    cleanup_path = mystrdup(VSTREAM_PATH(cleanup_dst));
    cleanup_queue_id = mystrdup(cleanup_handle->id);
    if (msg_verbose)
	msg_info("cleanup_service: open %s", cleanup_path);

    /*
     * If there is a time to get rid of spurious bounce/defer log files, this
     * is it. The down side is that this costs performance for every message,
     * while the probability of spurious bounce/defer log files is quite low.
     * Perhaps we should put the queue file ID inside the defer and bounce
     * files, so that the bounce and defer daemons can figure out if a file
     * is a left-over from a previous message instance. For now, we play safe
     * and check each time a new queue file is created.
     */
    for (cpp = log_queues; *cpp; cpp++) {
	if (mail_queue_remove(*cpp, cleanup_queue_id) == 0)
	    msg_warn("%s: removed spurious %s log", *cpp, cleanup_queue_id);
	else if (errno != ENOENT)
	    msg_fatal("%s: remove %s log: %m", *cpp, cleanup_queue_id);
    }

    /*
     * Send the queue id to the client. Read client processing options. If we
     * can't read the client processing options we can pretty much forget
     * about the whole operation.
     */
    mail_print(cleanup_src, "%s", cleanup_queue_id);
    if (mail_scan(src, "%d", &cleanup_flags) != 1) {
	cleanup_errs |= CLEANUP_STAT_BAD;
	cleanup_flags = 0;
    }

    /*
     * If the client requests us to do the bouncing in case of problems,
     * throw away the input only in case of real show-stopper errors, such as
     * unrecognizable data (which should never happen) or insufficient space
     * for the queue file (which will happen occasionally). Otherwise,
     * discard input after any lethal error. See the CLEANUP_OUT_OK()
     * definition.
     */
    if (cleanup_flags & CLEANUP_FLAG_BOUNCE) {
	cleanup_err_mask =
	    (CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE | CLEANUP_STAT_SIZE);
    } else {
	cleanup_err_mask = CLEANUP_STAT_LETHAL;
    }

    /*
     * First, copy the envelope records to the queue file. Then, copy the
     * message content (headers and body). Finally, attach any information
     * extracted from message headers.
     */
    cleanup_envelope();
    if (CLEANUP_OUT_OK())
	cleanup_message();
    if (CLEANUP_OUT_OK())
	cleanup_extracted();

    /*
     * Now that we have captured the entire message, see if there are any
     * other errors. For example, if the message needs to be bounced for lack
     * of recipients. We want to turn on the execute bits on a file only when
     * we want the queue manager to process it.
     */
    if (cleanup_recip == 0)
	cleanup_errs |= CLEANUP_STAT_RCPT;

    /*
     * If there are no errors, be very picky about queue file write errors
     * because we are about to tell the sender that it can throw away its
     * copy of the message.
     */
    if ((cleanup_errs & CLEANUP_STAT_LETHAL) == 0)
	cleanup_errs |= mail_stream_finish(cleanup_handle);
    else
	mail_stream_cleanup(cleanup_handle);
    cleanup_handle = 0;
    cleanup_dst = 0;

    /*
     * If there was an error, remove the queue file, after optionally
     * bouncing it. An incomplete message should never be bounced: it was
     * canceled by the client, and may not even have an address to bounce to.
     * That last test is redundant but we keep it just for robustness.
     * 
     * If we are responsible for bouncing a message, we must must report success
     * to the client unless the bounce message file could not be written
     * (which is just as bad as not being able to write the message queue
     * file in the first place).
     * 
     * Do not log the arrival of a message that will be bounced by the client.
     * 
     * XXX CLEANUP_STAT_LETHAL masks errors that are not directly fatal (e.g.,
     * header buffer overflow is normally allowed to happen), but that can
     * indirectly become a problem (e.g., no recipients were extracted from
     * message headers because we could not process all the message headers).
     * However, cleanup_strerror() prioritizes errors so that it can report
     * the cause (e.g., header buffer overflow), which is more useful.
     * Amazing.
     */
#define CAN_BOUNCE() \
	((cleanup_errs & (CLEANUP_STAT_BAD | CLEANUP_STAT_WRITE)) == 0 \
	    && cleanup_sender != 0 \
	    && (cleanup_flags & CLEANUP_FLAG_BOUNCE) != 0)

    if (cleanup_errs & CLEANUP_STAT_LETHAL) {
	if (CAN_BOUNCE()) {
	    if (bounce_append(BOUNCE_FLAG_CLEAN,
			      cleanup_queue_id, cleanup_recip ?
			      cleanup_recip : "", "cleanup", cleanup_time,
			      "Message rejected: %s",
			      cleanup_strerror(cleanup_errs)) == 0
		&& bounce_flush(BOUNCE_FLAG_CLEAN,
				MAIL_QUEUE_INCOMING,
				cleanup_queue_id, cleanup_sender) == 0) {
		cleanup_errs = 0;
	    } else {
		msg_warn("%s: bounce message failure", cleanup_queue_id);
		cleanup_errs = CLEANUP_STAT_WRITE;
	    }
	}
	if (REMOVE(cleanup_path))
	    msg_warn("remove %s: %m", cleanup_path);
    }

    /*
     * Report the completion status back to the client. Order of operations
     * matters very much: make sure that our queue file will not be deleted
     * by the error handler AFTER we have taken responsibility for delivery.
     * Better to deliver twice than to lose mail.
     */
    junk = cleanup_path;
    cleanup_path = 0;				/* don't delete upon error */
    mail_print(cleanup_src, "%d",		/* we're committed now */
	       cleanup_errs & CLEANUP_STAT_LETHAL);
    if (msg_verbose)
	msg_info("cleanup_service: status %d", cleanup_errs);
    myfree(junk);

    /*
     * Cleanup internal state. This is simply complementary to the
     * initializations at the beginning of this routine.
     */
    cleanup_state_free();
}

/* cleanup_all - callback for the runtime error handler */

static void cleanup_all(void)
{
    if (cleanup_path && REMOVE(cleanup_path))
	msg_warn("cleanup_all: remove %s: %m", cleanup_path);
}

/* cleanup_sig - cleanup after signal */

static void cleanup_sig(int sig)
{
    cleanup_all();
    exit(sig);
}

/* pre_jail_init - initialize before entering the chroot jail */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    if (*var_canonical_maps)
	cleanup_comm_canon_maps =
	maps_create(VAR_CANONICAL_MAPS, var_canonical_maps, DICT_FLAG_LOCK);
    if (*var_send_canon_maps)
	cleanup_send_canon_maps =
	    maps_create(VAR_SEND_CANON_MAPS, var_send_canon_maps,
			DICT_FLAG_LOCK);
    if (*var_rcpt_canon_maps)
	cleanup_rcpt_canon_maps =
	    maps_create(VAR_RCPT_CANON_MAPS, var_rcpt_canon_maps,
			DICT_FLAG_LOCK);
    if (*var_virtual_maps)
	cleanup_virtual_maps = maps_create(VAR_VIRTUAL_MAPS, var_virtual_maps,
					   DICT_FLAG_LOCK);
    if (*var_masq_domains)
	cleanup_masq_domains = argv_split(var_masq_domains, " ,\t\r\n");
    if (*var_header_checks)
	cleanup_header_checks =
	    maps_create(VAR_HEADER_CHECKS, var_header_checks, DICT_FLAG_LOCK);
    if (*var_body_checks)
	cleanup_body_checks =
	    maps_create(VAR_BODY_CHECKS, var_body_checks, DICT_FLAG_LOCK);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* post_jail_init - initialize after entering the chroot jail */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Optionally set the file size resource limit. XXX This limits the
     * message content to somewhat less than requested, because the total
     * queue file size also includes envelope information. Unless people set
     * really low limit, the difference is going to matter only when a queue
     * file has lots of recipients.
     */
    if (var_message_limit > 0)
	set_file_limit((off_t) var_message_limit);

    /*
     * Control how unmatched extensions are propagated.
     */
    cleanup_ext_prop_mask = ext_prop_mask(var_prop_extension);
}

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_HOPCOUNT_LIMIT, DEF_HOPCOUNT_LIMIT, &var_hopcount_limit, 1, 0,
	VAR_HEADER_LIMIT, DEF_HEADER_LIMIT, &var_header_limit, 1, 0,
	VAR_DUP_FILTER_LIMIT, DEF_DUP_FILTER_LIMIT, &var_dup_filter_limit, 0, 0,
	VAR_DELAY_WARN_TIME, DEF_DELAY_WARN_TIME, &var_delay_warn_time, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
	VAR_SEND_CANON_MAPS, DEF_SEND_CANON_MAPS, &var_send_canon_maps, 0, 0,
	VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
	VAR_VIRTUAL_MAPS, DEF_VIRTUAL_MAPS, &var_virtual_maps, 0, 0,
	VAR_MASQ_DOMAINS, DEF_MASQ_DOMAINS, &var_masq_domains, 0, 0,
	VAR_EMPTY_ADDR, DEF_EMPTY_ADDR, &var_empty_addr, 1, 0,
	VAR_MASQ_EXCEPTIONS, DEF_MASQ_EXCEPTIONS, &var_masq_exceptions, 0, 0,
	VAR_HEADER_CHECKS, DEF_HEADER_CHECKS, &var_header_checks, 0, 0,
	VAR_BODY_CHECKS, DEF_BODY_CHECKS, &var_body_checks, 0, 0,
	VAR_PROP_EXTENSION, DEF_PROP_EXTENSION, &var_prop_extension, 0, 0,
	VAR_ALWAYS_BCC, DEF_ALWAYS_BCC, &var_always_bcc, 0, 0,
	VAR_RCPT_WITHELD, DEF_RCPT_WITHELD, &var_rcpt_witheld, 1, 0,
	0,
    };

    /*
     * Clean up an incomplete queue file in case of a fatal run-time error,
     * or after receiving SIGTERM from the master at shutdown time.
     */
    signal(SIGTERM, cleanup_sig);
    msg_cleanup(cleanup_all);

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, cleanup_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
