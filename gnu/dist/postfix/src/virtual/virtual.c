/*++
/* NAME
/*	virtual 8
/* SUMMARY
/*	Postfix virtual domain mail delivery agent
/* SYNOPSIS
/*	\fBvirtual\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBvirtual\fR delivery agent is designed for virtual mail
/*	hosting services. Originally based on the Postfix local delivery
/*	agent, this agent looks up recipients with map lookups of their
/*	full recipient address, instead of using hard-coded unix password
/*	file lookups of the address local part only.
/*
/*	This delivery agent only delivers mail.  Other features such as
/*	mail forwarding, out-of-office notifications, etc., must be
/*	configured via virtual_alias maps or via similar lookup mechanisms.
/* MAILBOX LOCATION
/* .ad
/* .fi
/*	The mailbox location is controlled by the \fBvirtual_mailbox_base\fR
/*	and \fBvirtual_mailbox_maps\fR configuration parameters (see below).
/*	The \fBvirtual_mailbox_maps\fR table is indexed by the full recipient
/*	address.
/*
/*	The mailbox pathname is constructed as follows:
/*
/* .ti +2
/*	\fB$virtual_mailbox_base/$virtual_mailbox_maps(\fIrecipient\fB)\fR
/*
/*	where \fIrecipient\fR is the full recipient address.
/* UNIX MAILBOX FORMAT
/* .ad
/* .fi
/*	When the mailbox location does not end in \fB/\fR, the message
/*	is delivered in UNIX mailbox format.   This format stores multiple
/*	messages in one textfile.
/*
/*	The \fBvirtual\fR delivery agent prepends a "\fBFrom \fIsender
/*	time_stamp\fR" envelope header to each message, prepends a
/*	\fBDelivered-To:\fR message header with the envelope recipient
/*	address,
/*	prepends an \fBX-Original-To:\fR header with the recipient address as
/*	given to Postfix,
/*	prepends a \fBReturn-Path:\fR message header with the
/*	envelope sender address, prepends a \fB>\fR character to lines
/*	beginning with "\fBFrom \fR", and appends an empty line.
/*
/*	The mailbox is locked for exclusive access while delivery is in
/*	progress. In case of problems, an attempt is made to truncate the
/*	mailbox to its original length.
/* QMAIL MAILDIR FORMAT
/* .ad
/* .fi
/*	When the mailbox location ends in \fB/\fR, the message is delivered
/*	in qmail \fBmaildir\fR format. This format stores one message per file.
/*
/*	The \fBvirtual\fR delivery agent daemon prepends a \fBDelivered-To:\fR
/*	message header with the final envelope recipient address,
/*	prepends an \fBX-Original-To:\fR header with the recipient address as
/*	given to Postfix, and prepends a
/*	\fBReturn-Path:\fR message header with the envelope sender address.
/*
/*	By definition, \fBmaildir\fR format does not require file locking
/*	during mail delivery or retrieval.
/* MAILBOX OWNERSHIP
/* .ad
/* .fi
/*	Mailbox ownership is controlled by the \fBvirtual_uid_maps\fR
/*	and \fBvirtual_gid_maps\fR lookup tables, which are indexed
/*	with the full recipient address. Each table provides
/*	a string with the numerical user and group ID, respectively.
/*
/*	The \fBvirtual_minimum_uid\fR parameter imposes a lower bound on
/*	numerical user ID values that may be specified in any
/*	\fBvirtual_uid_maps\fR.
/* SECURITY
/* .ad
/* .fi
/*	The virtual delivery agent is not security sensitive, provided
/*	that the lookup tables with recipient user/group ID information are
/*	adequately protected. This program is not designed to run chrooted.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/* DIAGNOSTICS
/*	Mail bounces when the recipient has no mailbox or when the
/*	recipient is over disk quota. In all other cases, mail for
/*	an existing recipient is deferred and a warning is logged.
/*
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*	Corrupted message files are marked so that the queue
/*	manager can move them to the \fBcorrupt\fR queue afterwards.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces and of other trouble.
/* BUGS
/*	This delivery agent silently ignores address extensions.
/*
/*	Postfix should have lookup tables that can return multiple result
/*	attributes. In order to avoid the inconvenience of maintaining
/*	three tables, use an LDAP or MYSQL database.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Mailbox delivery
/* .ad
/* .fi
/* .IP \fBvirtual_mailbox_base\fR
/*	Specifies a path that is prepended to all mailbox or maildir paths.
/*	This is a safety measure to ensure that an out of control map in
/*	\fBvirtual_mailbox_maps\fR doesn't litter the filesystem with mailboxes.
/*	While it could be set to "/", this setting isn't recommended.
/* .IP \fBvirtual_mailbox_maps\fR
/*	Recipients are looked up in these maps to determine the path to
/*	their mailbox or maildir. If the returned path ends in a slash
/*	("/"), maildir-style delivery is carried out, otherwise the
/*	path is assumed to specify a UNIX-style mailbox file.
/*
/*	While searching a lookup table, an address extension
/*	(\fIuser+foo@domain.tld\fR) is ignored.
/*
/*	In a lookup table, specify a left-hand side of \fI@domain.tld\fR
/*	to match any user in the specified domain that does not have a
/*	specific \fIuser@domain.tld\fR entry.
/*
/*	Note that \fBvirtual_mailbox_base\fR is unconditionally prepended
/*	to this path.
/*
/*	For security reasons, regular expression maps are allowed but
/*	regular expression substitution of $1 etc. is disallowed,
/*	because that would open a security hole.
/*
/*	For security reasons, proxied table lookup is not allowed,
/*	because that would open a security hole.
/* .IP \fBvirtual_mailbox_domains\fR
/*	The list of domains that should be delivered via the Postfix virtual
/*	delivery agent. This uses the same syntax as the \fBmydestination\fR
/*	configuration parameter.
/* .IP \fBvirtual_minimum_uid\fR
/*	Specifies a minimum uid that will be accepted as a return from
/*	a \fBvirtual_uid_maps\fR lookup.
/*	Returned values less than this will be rejected, and the message
/*	will be deferred.
/* .IP \fBvirtual_uid_maps\fR
/*	Recipients are looked up in these maps to determine the user ID to be
/*	used when writing to the target mailbox.
/*
/*	While searching a lookup table, an address extension
/*	(\fIuser+foo@domain.tld\fR) is ignored.
/*
/*	In a lookup table, specify a left-hand side of \fI@domain.tld\fR
/*	to match any user in the specified domain that does not have a
/*	specific \fIuser@domain.tld\fR entry.
/*
/*	For security reasons, regular expression maps are allowed but
/*	regular expression substitution of $1 etc. is disallowed,
/*	because that would open a security hole.
/*
/*	For security reasons, proxied table lookup is not allowed,
/*	because that would open a security hole.
/* .IP \fBvirtual_gid_maps\fR
/*	Recipients are looked up in these maps to determine the group ID to be
/*	used when writing to the target mailbox.
/*
/*	While searching a lookup table, an address extension
/*	(\fIuser+foo@domain.tld\fR) is ignored.
/*
/*	In a lookup table, specify a left-hand side of \fI@domain.tld\fR
/*	to match any user in the specified domain that does not have a
/*	specific \fIuser@domain.tld\fR entry.
/*
/*	For security reasons, regular expression maps are allowed but
/*	regular expression substitution of $1 etc. is disallowed,
/*	because that would open a security hole.
/*
/*	For security reasons, proxied table lookup is not allowed,
/*	because that would open a security hole.
/* .SH "Locking controls"
/* .ad
/* .fi
/* .IP \fBvirtual_mailbox_lock\fR
/*	How to lock UNIX-style mailboxes: one or more of \fBflock\fR,
/*	\fBfcntl\fR or \fBdotlock\fR. The \fBdotlock\fR method requires
/*	that the recipient UID or GID has write access to the parent
/*	directory of the mailbox file.
/*
/*	This setting is ignored with \fBmaildir\fR style delivery,
/*	because such deliveries are safe without explicit locks.
/*
/*	Use the command \fBpostconf -l\fR to find out what locking methods
/*	are available on your system.
/* .IP \fBdeliver_lock_attempts\fR
/*	Limit the number of attempts to acquire an exclusive lock
/*	on a UNIX-style mailbox file.
/* .IP \fBdeliver_lock_delay\fR
/*	Time (default: seconds) between successive attempts to acquire
/*	an exclusive lock on a UNIX-style mailbox file. The actual delay
/*	is slightly randomized.
/* .IP \fBstale_lock_time\fR
/*	Limit the time after which a stale lockfile is removed (applicable
/*	to UNIX-style mailboxes only).
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBvirtual_destination_concurrency_limit\fR
/*	Limit the number of parallel deliveries to the same domain
/*	via the \fBvirtual\fR delivery agent.
/*	The default limit is taken from the
/*	\fBdefault_destination_concurrency_limit\fR parameter.
/*	The limit is enforced by the Postfix queue manager.
/* .IP \fBvirtual_destination_recipient_limit\fR
/*	Limit the number of recipients per message delivery
/*	via the \fBvirtual\fR delivery agent.
/*	The default limit is taken from the
/*	\fBdefault_destination_recipient_limit\fR parameter.
/*	The limit is enforced by the Postfix queue manager.
/* .IP \fBvirtual_mailbox_limit\fR
/*	The maximal size in bytes of a mailbox or maildir file.
/*	Set to zero to disable the limit.
/* HISTORY
/* .ad
/* .fi
/*	This agent was originally based on the Postfix local delivery
/*	agent. Modifications mainly consisted of removing code that either
/*	was not applicable or that was not safe in this context: aliases,
/*	~user/.forward files, delivery to "|command" or to /file/name.
/*
/*	The \fBDelivered-To:\fR header appears in the \fBqmail\fR system
/*	by Daniel Bernstein.
/*
/*	The \fBmaildir\fR structure appears in the \fBqmail\fR system
/*	by Daniel Bernstein.
/* SEE ALSO
/*	regexp_table(5) POSIX regular expression table format
/*	pcre_table(5) Perl Compatible Regular Expression table format
/*	bounce(8) non-delivery status reports
/*	syslogd(8) system logging
/*	qmgr(8) queue manager
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
/*	Andrew McNamara
/*	andrewm@connect.com.au
/*	connect.com.au Pty. Ltd.
/*	Level 3, 213 Miller St
/*	North Sydney 2060, NSW, Australia
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#ifdef USE_PATHS_H
#include <paths.h>			/* XXX mail_spool_dir dependency */
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <iostuff.h>
#include <set_eugid.h>
#include <dict.h>

/* Global library. */

#include <mail_queue.h>
#include <recipient_list.h>
#include <deliver_request.h>
#include <deliver_completed.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <virtual8_maps.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "virtual.h"

 /*
  * Tunable parameters.
  */
char   *var_virt_mailbox_maps;
char   *var_virt_uid_maps;
char   *var_virt_gid_maps;
int     var_virt_minimum_uid;
char   *var_virt_mailbox_base;
char   *var_virt_mailbox_lock;
int     var_virt_mailbox_limit;
char   *var_mail_spool_dir;		/* XXX dependency fix */

 /*
  * Mappings.
  */
MAPS   *virtual_mailbox_maps;
MAPS   *virtual_uid_maps;
MAPS   *virtual_gid_maps;

 /*
  * Bit masks.
  */
int     virtual_mbox_lock_mask;

/* local_deliver - deliver message with extreme prejudice */

static int local_deliver(DELIVER_REQUEST *rqst, char *service)
{
    char   *myname = "local_deliver";
    RECIPIENT *rcpt_end = rqst->rcpt_list.info + rqst->rcpt_list.len;
    RECIPIENT *rcpt;
    int     rcpt_stat;
    int     msg_stat;
    LOCAL_STATE state;
    USER_ATTR usr_attr;

    if (msg_verbose)
	msg_info("local_deliver: %s from %s", rqst->queue_id, rqst->sender);

    /*
     * Initialize the delivery attributes that are not recipient specific.
     */
    state.level = 0;
    deliver_attr_init(&state.msg_attr);
    state.msg_attr.queue_name = rqst->queue_name;
    state.msg_attr.queue_id = rqst->queue_id;
    state.msg_attr.fp = rqst->fp;
    state.msg_attr.offset = rqst->data_offset;
    state.msg_attr.sender = rqst->sender;
    state.msg_attr.relay = service;
    state.msg_attr.arrival_time = rqst->arrival_time;
    RESET_USER_ATTR(usr_attr, state.level);
    state.request = rqst;

    /*
     * Iterate over each recipient named in the delivery request. When the
     * mail delivery status for a given recipient is definite (i.e. bounced
     * or delivered), update the message queue file and cross off the
     * recipient. Update the per-message delivery status.
     */
    for (msg_stat = 0, rcpt = rqst->rcpt_list.info; rcpt < rcpt_end; rcpt++) {
	state.msg_attr.orig_rcpt = rcpt->orig_addr;
	state.msg_attr.recipient = rcpt->address;
	rcpt_stat = deliver_recipient(state, usr_attr);
	if (rcpt_stat == 0)
	    deliver_completed(state.msg_attr.fp, rcpt->offset);
	msg_stat |= rcpt_stat;
    }

    return (msg_stat);
}

/* local_service - perform service for client */

static void local_service(VSTREAM *stream, char *service, char **argv)
{
    DELIVER_REQUEST *request;
    int     status;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * that is dedicated to local mail delivery service. What we see below is
     * a little protocol to (1) tell the client that we are ready, (2) read a
     * delivery request from the client, and (3) report the completion status
     * of that request.
     */
    if ((request = deliver_request_read(stream)) != 0) {
	status = local_deliver(request, service);
	deliver_request_done(stream, request, status);
    }
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* post_init - post-jail initialization */

static void post_init(char *unused_name, char **unused_argv)
{

    /*
     * Drop privileges most of the time.
     */
    set_eugid(var_owner_uid, var_owner_gid);

    virtual_mailbox_maps =
	virtual8_maps_create(VAR_VIRT_MAILBOX_MAPS, var_virt_mailbox_maps,
			     DICT_FLAG_LOCK | DICT_FLAG_PARANOID);

    virtual_uid_maps =
	virtual8_maps_create(VAR_VIRT_UID_MAPS, var_virt_uid_maps,
			     DICT_FLAG_LOCK | DICT_FLAG_PARANOID);

    virtual_gid_maps =
	virtual8_maps_create(VAR_VIRT_GID_MAPS, var_virt_gid_maps,
			     DICT_FLAG_LOCK | DICT_FLAG_PARANOID);

    virtual_mbox_lock_mask = mbox_lock_mask(var_virt_mailbox_lock);
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{

    /*
     * Reset the file size limit from the message size limit to the mailbox
     * size limit.
     * 
     * We can't have mailbox size limit smaller than the message size limit,
     * because that prohibits the delivery agent from updating the queue
     * file.
     */
    if (var_virt_mailbox_limit) {
	if (var_virt_mailbox_limit < var_message_limit)
	    msg_fatal("main.cf configuration error: %s is smaller than %s",
		      VAR_VIRT_MAILBOX_LIMIT, VAR_MESSAGE_LIMIT);
	set_file_limit(var_virt_mailbox_limit);
    }
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_VIRT_MINUID, DEF_VIRT_MINUID, &var_virt_minimum_uid, 1, 0,
	VAR_VIRT_MAILBOX_LIMIT, DEF_VIRT_MAILBOX_LIMIT, &var_virt_mailbox_limit, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_MAIL_SPOOL_DIR, DEF_MAIL_SPOOL_DIR, &var_mail_spool_dir, 0, 0,
	VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps, 0, 0,
	VAR_VIRT_UID_MAPS, DEF_VIRT_UID_MAPS, &var_virt_uid_maps, 0, 0,
	VAR_VIRT_GID_MAPS, DEF_VIRT_GID_MAPS, &var_virt_gid_maps, 0, 0,
	VAR_VIRT_MAILBOX_BASE, DEF_VIRT_MAILBOX_BASE, &var_virt_mailbox_base, 1, 0,
	VAR_VIRT_MAILBOX_LOCK, DEF_VIRT_MAILBOX_LOCK, &var_virt_mailbox_lock, 1, 0,
	0,
    };

    single_server_main(argc, argv, local_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       MAIL_SERVER_POST_INIT, post_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
