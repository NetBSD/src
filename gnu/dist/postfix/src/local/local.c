/*++
/* NAME
/*	local 8
/* SUMMARY
/*	Postfix local mail delivery
/* SYNOPSIS
/*	\fBlocal\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBlocal\fR daemon processes delivery requests from the
/*	Postfix queue manager to deliver mail to local recipients.
/*	Each delivery request specifies a queue file, a sender address,
/*	a domain or host to deliver to, and one or more recipients.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The \fBlocal\fR daemon updates queue files and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery status reports are sent
/*	to the \fBbounce\fR(8), \fBdefer\fR(8) or \fBtrace\fR(8) daemon as
/*	appropriate.
/* SYSTEM-WIDE AND USER-LEVEL ALIASING
/* .ad
/* .fi
/*	The system administrator can set up one or more system-wide
/*	\fBsendmail\fR-style alias databases.
/*	Users can have \fBsendmail\fR-style ~/.\fBforward\fR files.
/*	Mail for \fIname\fR is delivered to the alias \fIname\fR, to
/*	destinations in ~\fIname\fR/.\fBforward\fR, to the mailbox owned
/*	by the user \fIname\fR, or it is sent back as undeliverable.
/*
/*	The system administrator can specify a comma/space separated list
/*	of ~\fR/.\fBforward\fR like files through the \fBforward_path\fR
/*	configuration parameter. Upon delivery, the local delivery agent
/*	tries each pathname in the list until a file is found.
/*	The \fBforward_path\fR parameter is subject to interpolation of
/*	\fB$user\fR (recipient username), \fB$home\fR (recipient home
/*	directory), \fB$shell\fR (recipient shell), \fB$recipient\fR
/*	(complete recipient address), \fB$extension\fR (recipient address
/*	extension), \fB$domain\fR (recipient domain), \fBlocal\fR
/*	(entire recipient address localpart) and
/*	\fB$recipient_delimiter.\fR The forms \fI${name?value}\fR and
/*	\fI${name:value}\fR expand conditionally to \fIvalue\fR when
/*	\fI$name\fR is (is not) defined.
/*	Characters that may have special meaning to the shell or file system
/*	are replaced by underscores.  The list of acceptable characters
/*	is specified with the \fBforward_expansion_filter\fR configuration
/*	parameter.
/*
/*	An alias or ~/.\fBforward\fR file may list any combination of external
/*	commands, destination file names, \fB:include:\fR directives, or
/*	mail addresses.
/*	See \fBaliases\fR(5) for a precise description. Each line in a
/*	user's .\fBforward\fR file has the same syntax as the right-hand part
/*	of an alias.
/*
/*	When an address is found in its own alias expansion, delivery is
/*	made to the user instead. When a user is listed in the user's own
/*	~/.\fBforward\fR file, delivery is made to the user's mailbox instead.
/*	An empty ~/.\fBforward\fR file means do not forward mail.
/*
/*	In order to prevent the mail system from using up unreasonable
/*	amounts of memory, input records read from \fB:include:\fR or from
/*	~/.\fBforward\fR files are broken up into chunks of length
/*	\fBline_length_limit\fR.
/*
/*	While expanding aliases, ~/.\fBforward\fR files, and so on, the
/*	program attempts to avoid duplicate deliveries. The
/*	\fBduplicate_filter_limit\fR configuration parameter limits the
/*	number of remembered recipients.
/* MAIL FORWARDING
/* .ad
/* .fi
/*	For the sake of reliability, forwarded mail is re-submitted as
/*	a new message, so that each recipient has a separate on-file
/*	delivery status record.
/*
/*	In order to stop mail forwarding loops early, the software adds an
/*	optional
/*	\fBDelivered-To:\fR header with the final envelope recipient address. If
/*	mail arrives for a recipient that is already listed in a
/*	\fBDelivered-To:\fR header, the message is bounced.
/* MAILBOX DELIVERY
/* .ad
/* .fi
/*	The default per-user mailbox is a file in the UNIX mail spool
/*	directory (\fB/var/mail/\fIuser\fR or \fB/var/spool/mail/\fIuser\fR);
/*	the location can be specified with the \fBmail_spool_directory\fR
/*	configuration parameter. Specify a name ending in \fB/\fR for
/*	\fBqmail\fR-compatible \fBmaildir\fR delivery.
/*
/*	Alternatively, the per-user mailbox can be a file in the user's home
/*	directory with a name specified via the \fBhome_mailbox\fR
/*	configuration parameter. Specify a relative path name. Specify a name
/*	ending in \fB/\fR for \fBqmail\fR-compatible \fBmaildir\fR delivery.
/*
/*	Mailbox delivery can be delegated to an external command specified
/*	with the \fBmailbox_command\fR configuration parameter. The command
/*	executes with the privileges of the recipient user (exception: in
/*	case of delivery as root, the command executes with the privileges
/*	of \fBdefault_privs\fR).
/*
/*	Mailbox delivery can be delegated to alternative message transports
/*	specified in the \fBmaster.cf\fR file.
/*	The \fBmailbox_transport\fR configuration parameter specifies a
/*	message transport that is to be used for all local recipients,
/*	regardless of whether they are found in the UNIX passwd database.
/*	The \fBfallback_transport\fR parameter specifies a message transport
/*	for recipients that are not found in the UNIX passwd database.
/*
/*	In the case of UNIX-style mailbox delivery,
/*	the \fBlocal\fR daemon prepends a "\fBFrom \fIsender time_stamp\fR"
/*	envelope header to each message, prepends an
/*	\fBX-Original-To:\fR header with the recipient address as given to
/*	Postfix, prepends an
/*	optional \fBDelivered-To:\fR header
/*	with the final envelope recipient address, prepends a \fBReturn-Path:\fR
/*	header with the envelope sender address, prepends a \fB>\fR character
/*	to lines beginning with "\fBFrom \fR", and appends an empty line.
/*	The mailbox is locked for exclusive access while delivery is in
/*	progress. In case of problems, an attempt is made to truncate the
/*	mailbox to its original length.
/*
/*	In the case of \fBmaildir\fR delivery, the local daemon prepends
/*	an optional
/*	\fBDelivered-To:\fR header with the final envelope recipient address,
/*	prepends an
/*	\fBX-Original-To:\fR header with the recipient address as given to
/*	Postfix,
/*	and prepends a \fBReturn-Path:\fR header with the envelope sender
/*	address.
/* EXTERNAL COMMAND DELIVERY
/* .ad
/* .fi
/*	The \fBallow_mail_to_commands\fR configuration parameter restricts
/*	delivery to external commands. The default setting (\fBalias,
/*	forward\fR) forbids command destinations in \fB:include:\fR files.
/*
/*	The command is executed directly where possible. Assistance by the
/*	shell (\fB/bin/sh\fR on UNIX systems) is used only when the command
/*	contains shell magic characters, or when the command invokes a shell
/*	built-in command.
/*
/*	A limited amount of command output (standard output and standard
/*	error) is captured for inclusion with non-delivery status reports.
/*	A command is forcibly terminated if it does not complete within
/*	\fBcommand_time_limit\fR seconds.  Command exit status codes are
/*	expected to follow the conventions defined in <\fBsysexits.h\fR>.
/*
/*	A limited amount of message context is exported via environment
/*	variables. Characters that may have special meaning to the shell
/*	are replaced by underscores.  The list of acceptable characters
/*	is specified with the \fBcommand_expansion_filter\fR configuration
/*	parameter.
/* .IP \fBSHELL\fR
/*	The recipient user's login shell.
/* .IP \fBHOME\fR
/*	The recipient user's home directory.
/* .IP \fBUSER\fR
/*	The bare recipient name.
/* .IP \fBEXTENSION\fR
/*	The optional recipient address extension.
/* .IP \fBDOMAIN\fR
/*	The recipient address domain part.
/* .IP \fBLOGNAME\fR
/*	The bare recipient name.
/* .IP \fBLOCAL\fR
/*	The entire recipient address localpart (text to the left of the
/*	rightmost @ character).
/* .IP \fBRECIPIENT\fR
/*	The entire recipient address.
/* .IP \fBSENDER\fR
/*	The entire sender address.
/* .PP
/*	The \fBPATH\fR environment variable is always reset to a
/*	system-dependent default path, and environment variables
/*	whose names are blessed by the \fBexport_environment\fR
/*	configuration parameter are exported unchanged.
/*
/*	The current working directory is the mail queue directory.
/*
/*	The \fBlocal\fR daemon prepends a "\fBFrom \fIsender time_stamp\fR"
/*	envelope header to each message, prepends an
/*	\fBX-Original-To:\fR header with the recipient address as given to
/*	Postfix, prepends an
/*	optional \fBDelivered-To:\fR
/*	header with the final recipient envelope address, prepends a
/*	\fBReturn-Path:\fR header with the sender envelope address,
/*	and appends no empty line.
/* EXTERNAL FILE DELIVERY
/* .ad
/* .fi
/*	The delivery format depends on the destination filename syntax.
/*	The default is to use UNIX-style mailbox format.  Specify a name
/*	ending in \fB/\fR for \fBqmail\fR-compatible \fBmaildir\fR delivery.
/*
/*	The \fBallow_mail_to_files\fR configuration parameter restricts
/*	delivery to external files. The default setting (\fBalias,
/*	forward\fR) forbids file destinations in \fB:include:\fR files.
/*
/*	In the case of UNIX-style mailbox delivery,
/*	the \fBlocal\fR daemon prepends a "\fBFrom \fIsender time_stamp\fR"
/*	envelope header to each message, prepends an
/*	\fBX-Original-To:\fR header with the recipient address as given to
/*	Postfix, prepends an
/*	optional \fBDelivered-To:\fR
/*	header with the final recipient envelope address, prepends a \fB>\fR
/*	character to lines beginning with "\fBFrom \fR", and appends an
/*	empty line.
/*	The envelope sender address is available in the \fBReturn-Path:\fR
/*	header.
/*	When the destination is a regular file, it is locked for exclusive
/*	access while delivery is in progress. In case of problems, an attempt
/*	is made to truncate a regular file to its original length.
/*
/*	In the case of \fBmaildir\fR delivery, the local daemon prepends
/*	an optional
/*	\fBDelivered-To:\fR header with the final envelope recipient address,
/*	and prepends an
/*	\fBX-Original-To:\fR header with the recipient address as given to
/*	Postfix.
/*	The envelope sender address is available in the \fBReturn-Path:\fR
/*	header.
/* ADDRESS EXTENSION
/* .ad
/* .fi
/*	The optional \fBrecipient_delimiter\fR configuration parameter
/*	specifies how to separate address extensions from local recipient
/*	names.
/*
/*	For example, with "\fBrecipient_delimiter = +\fR", mail for
/*	\fIname\fR+\fIfoo\fR is delivered to the alias \fIname\fR+\fIfoo\fR
/*	or to the alias \fIname\fR, to the destinations listed in
/*	~\fIname\fR/.\fBforward\fR+\fIfoo\fR or in ~\fIname\fR/.\fBforward\fR,
/*	to the mailbox owned by the user \fIname\fR, or it is sent back as
/*	undeliverable.
/*
/*	In all cases the \fBlocal\fR daemon prepends an optional
/*	`\fBDelivered-To:\fR header line with the final recipient
/*	address.
/* DELIVERY RIGHTS
/* .ad
/* .fi
/*	Deliveries to external files and external commands are made with
/*	the rights of the receiving user on whose behalf the delivery is made.
/*	In the absence of a user context, the \fBlocal\fR daemon uses the
/*	owner rights of the \fB:include:\fR file or alias database.
/*	When those files are owned by the superuser, delivery is made with
/*	the rights specified with the \fBdefault_privs\fR configuration
/*	parameter.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*	Corrupted message files are marked so that the queue
/*	manager can move them to the \fBcorrupt\fR queue afterwards.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces and of other trouble.
/* BUGS
/*	For security reasons, the message delivery status of external commands
/*	or of external files is never checkpointed to file. As a result,
/*	the program may occasionally deliver more than once to a command or
/*	external file. Better safe than sorry.
/*
/*	Mutually-recursive aliases or ~/.\fBforward\fR files are not detected
/*	early.  The resulting mail forwarding loop is broken by the use of the
/*	\fBDelivered-To:\fR message header.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as local(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	postconf(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBbiff (yes)\fR"
/*	Whether or not to use the local biff service.
/* .IP "\fBexpand_owner_alias (no)\fR"
/*	When delivering to an alias "aliasname" that has an "owner-aliasname"
/*	companion alias, set the envelope sender address to the expansion
/*	of the "owner-aliasname" alias.
/* .IP "\fBowner_request_special (yes)\fR"
/*	Give special treatment to owner-listname and listname-request
/*	address localparts: don't don't split such addresses when the
/*	recipient_delimiter is set to "-".
/* .IP "\fBsun_mailtool_compatibility (no)\fR"
/*	Obsolete SUN mailtool compatibility feature.
/* DELIVERY METHOD CONTROLS
/* .ad
/* .fi
/*	The precedence of local(8) delivery methods from high to low is:
/*	aliases, .forward files, mailbox_transport, mailbox_command_maps,
/*	mailbox_command, home_mailbox, mail_spool_directory, fallback_transport
/*	and luser_relay.
/* .IP "\fBalias_maps (see 'postconf -d' output)\fR"
/*	The alias databases that are used for local(8) delivery.
/* .IP "\fBforward_path (see 'postconf -d' output)\fR"
/*	The local(8) delivery agent search list for finding a .forward
/*	file with user-specified delivery methods.
/* .IP "\fBmailbox_transport (empty)\fR"
/*	Optional message delivery transport that the local(8) delivery
/*	agent should use for mailbox delivery to all local recipients,
/*	whether or not they are found in the UNIX passwd database.
/* .IP "\fBmailbox_command_maps (empty)\fR"
/*	Optional lookup tables with per-recipient external commands to use
/*	for local(8) mailbox delivery.
/* .IP "\fBmailbox_command (empty)\fR"
/*	Optional external command that the local(8) delivery agent should
/*	use for mailbox delivery.
/* .IP "\fBhome_mailbox (empty)\fR"
/*	Optional pathname of a mailbox file relative to a local(8) user's
/*	home directory.
/* .IP "\fBmail_spool_directory (see 'postconf -d' output)\fR"
/*	The directory where local(8) UNIX-style mailboxes are kept.
/* .IP "\fBfallback_transport (empty)\fR"
/*	Optional message delivery transport that the local(8) delivery
/*	agent should use for names that are not found in the aliases(5)
/*	database or in the UNIX passwd database.
/* .IP "\fBluser_relay (empty)\fR"
/*	Optional catch-all destination for unknown local(8) recipients.
/* MAILBOX LOCKING CONTROLS
/* .ad
/* .fi
/* .IP "\fBdeliver_lock_attempts (20)\fR"
/*	The maximal number of attempts to acquire an exclusive lock on a
/*	mailbox file or bounce(8) logfile.
/* .IP "\fBdeliver_lock_delay (1s)\fR"
/*	The time between attempts to acquire an exclusive lock on a mailbox
/*	file or bounce(8) logfile.
/* .IP "\fBstale_lock_time (500s)\fR"
/*	The time after which a stale exclusive mailbox lockfile is removed.
/* .IP "\fBmailbox_delivery_lock (see 'postconf -d' output)\fR"
/*	How to lock a UNIX-style local(8) mailbox before attempting delivery.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBcommand_time_limit (1000s)\fR"
/*	Time limit for delivery to external commands.
/* .IP "\fBduplicate_filter_limit (1000)\fR"
/*	The maximal number of addresses remembered by the address
/*	duplicate filter for aliases(5) or virtual(5) alias expansion, or
/*	for showq(8) queue displays.
/* .IP "\fBlocal_destination_concurrency_limit (2)\fR"
/*	The maximal number of parallel deliveries via the local mail
/*	delivery transport to the same recipient (when
/*	"local_destination_recipient_limit = 1") or the maximal number of
/*	parallel deliveries to the same local domain (when
/*	"local_destination_recipient_limit > 1").
/* .IP "\fBlocal_destination_recipient_limit (1)\fR"
/*	The maximal number of recipients per message delivery via the
/*	local mail delivery transport.
/* .IP "\fBmailbox_size_limit (51200000)\fR"
/*	The maximal size of any local(8) individual mailbox or maildir
/*	file, or zero (no limit).
/* SECURITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBallow_mail_to_commands (alias, forward)\fR"
/*	Restrict local(8) mail delivery to external commands.
/* .IP "\fBallow_mail_to_files (alias, forward)\fR"
/*	Restrict local(8) mail delivery to external files.
/* .IP "\fBcommand_expansion_filter (see 'postconf -d' output)\fR"
/*	Restrict the characters that the local(8) delivery agent allows in
/*	$name expansions of $mailbox_command.
/* .IP "\fBdefault_privs (nobody)\fR"
/*	The default rights used by the local(8) delivery agent for delivery
/*	to external file or command.
/* .IP "\fBforward_expansion_filter (see 'postconf -d' output)\fR"
/*	Restrict the characters that the local(8) delivery agent allows in
/*	$name expansions of $forward_path.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBexport_environment (see 'postconf -d' output)\fR"
/*	The list of environment variables that a Postfix process will export
/*	to non-Postfix processes.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBlocal_command_shell (empty)\fR"
/*	Optional shell program for local(8) delivery to non-Postfix command.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process
/*	waits for the next service request before exiting.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of connection requests before a Postfix daemon
/*	process terminates.
/* .IP "\fBprepend_delivered_header (command, file, forward)\fR"
/*	The message delivery contexts where the Postfix local(8) delivery
/*	agent prepends a Delivered-To:  message header.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBpropagate_unmatched_extensions (canonical, virtual)\fR"
/*	What address lookup tables copy an address extension from the lookup
/*	key to the lookup result.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBrecipient_delimiter (empty)\fR"
/*	The separator between user names and address extensions (user+foo).
/* .IP "\fBrequire_home_directory (no)\fR"
/*	Whether or not a local(8) recipient's home directory must exist
/*	before mail delivery is attempted.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* FILES
/*	The following are examples; details differ between systems.
/*	$HOME/.forward, per-user aliasing
/*	/etc/aliases, sytem-wide alias database
/*	/var/spool/mail, system mailboxes
/* SEE ALSO
/*	qmgr(8), queue manager
/*	bounce(8), delivery status reports
/*	newaliases(1), create/update alias database
/*	postalias(1), create/update alias database
/*	aliases(5), format of alias database
/*	postconf(5), configuration parameters
/*	syslogd(8), system logging
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	The \fBDelivered-To:\fR message header appears in the \fBqmail\fR
/*	system by Daniel Bernstein.
/*
/*	The \fImaildir\fR structure appears in the \fBqmail\fR system
/*	by Daniel Bernstein.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <vstring.h>
#include <vstream.h>
#include <iostuff.h>
#include <name_mask.h>
#include <set_eugid.h>
#include <dict.h>

/* Global library. */

#include <recipient_list.h>
#include <deliver_request.h>
#include <deliver_completed.h>
#include <mail_params.h>
#include <mail_addr.h>
#include <mail_conf.h>
#include <been_here.h>
#include <mail_params.h>
#include <ext_prop.h>
#include <maps.h>
#include <flush_clnt.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "local.h"

 /*
  * Tunable parameters.
  */
char   *var_allow_commands;
char   *var_allow_files;
char   *var_alias_maps;
int     var_dup_filter_limit;
int     var_command_maxtime;
char   *var_home_mailbox;
char   *var_mailbox_command;
char   *var_mailbox_cmd_maps;
char   *var_rcpt_fdelim;
char   *var_local_cmd_shell;
char   *var_luser_relay;
int     var_biff;
char   *var_mail_spool_dir;
char   *var_mailbox_transport;
char   *var_fallback_transport;
char   *var_forward_path;
char   *var_cmd_exp_filter;
char   *var_fwd_exp_filter;
char   *var_prop_extension;
int     var_exp_own_alias;
char   *var_deliver_hdr;
int     var_stat_home_dir;
int     var_mailtool_compat;
char   *var_mailbox_lock;
int     var_mailbox_limit;

int     local_cmd_deliver_mask;
int     local_file_deliver_mask;
int     local_ext_prop_mask;
int     local_deliver_hdr_mask;
int     local_mbox_lock_mask;
MAPS   *alias_maps;

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
     * While messages are being delivered and while aliases or forward files
     * are being expanded, this attribute list is being changed constantly.
     * For this reason, the list is passed on by value (except when it is
     * being initialized :-), so that there is no need to undo attribute
     * changes made by lower-level routines. The alias/include/forward
     * expansion attribute list is part of a tree with self and parent
     * references (see the EXPAND_ATTR definitions). The user-specific
     * attributes are security sensitive, and are therefore kept separate.
     * All this results in a noticeable level of clumsiness, but passing
     * things around by value gives good protection against accidental change
     * by subroutines.
     */
    state.level = 0;
    deliver_attr_init(&state.msg_attr);
    state.msg_attr.queue_name = rqst->queue_name;
    state.msg_attr.queue_id = rqst->queue_id;
    state.msg_attr.fp = rqst->fp;
    state.msg_attr.offset = rqst->data_offset;
    state.msg_attr.encoding = rqst->encoding;
    state.msg_attr.sender = rqst->sender;
    state.msg_attr.relay = service;
    state.msg_attr.arrival_time = rqst->arrival_time;
    RESET_OWNER_ATTR(state.msg_attr, state.level);
    RESET_USER_ATTR(usr_attr, state.level);
    state.loop_info = delivered_init(state.msg_attr);	/* delivered-to */
    state.request = rqst;

    /*
     * Iterate over each recipient named in the delivery request. When the
     * mail delivery status for a given recipient is definite (i.e. bounced
     * or delivered), update the message queue file and cross off the
     * recipient. Update the per-message delivery status.
     */
    for (msg_stat = 0, rcpt = rqst->rcpt_list.info; rcpt < rcpt_end; rcpt++) {
	state.dup_filter = been_here_init(var_dup_filter_limit, BH_FLAG_FOLD);
	forward_init();
	state.msg_attr.orig_rcpt = rcpt->orig_addr;
	state.msg_attr.recipient = rcpt->address;
	state.msg_attr.rcpt_offset = rcpt->offset;
	rcpt_stat = deliver_recipient(state, usr_attr);
	rcpt_stat |= forward_finish(rqst, state.msg_attr, rcpt_stat);
	if (rcpt_stat == 0 && (rqst->flags & DEL_REQ_FLAG_SUCCESS))
	    deliver_completed(state.msg_attr.fp, rcpt->offset);
	been_here_free(state.dup_filter);
	msg_stat |= rcpt_stat;
    }

    /*
     * Clean up.
     */
    delivered_free(state.loop_info);

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

/* local_mask_init - initialize delivery restrictions */

static void local_mask_init(void)
{
    static NAME_MASK file_mask[] = {
	"alias", EXPAND_TYPE_ALIAS,
	"forward", EXPAND_TYPE_FWD,
	"include", EXPAND_TYPE_INCL,
	0,
    };
    static NAME_MASK command_mask[] = {
	"alias", EXPAND_TYPE_ALIAS,
	"forward", EXPAND_TYPE_FWD,
	"include", EXPAND_TYPE_INCL,
	0,
    };
    static NAME_MASK deliver_mask[] = {
	"command", DELIVER_HDR_CMD,
	"file", DELIVER_HDR_FILE,
	"forward", DELIVER_HDR_FWD,
	0,
    };

    local_file_deliver_mask = name_mask(VAR_ALLOW_FILES, file_mask,
					var_allow_files);
    local_cmd_deliver_mask = name_mask(VAR_ALLOW_COMMANDS, command_mask,
				       var_allow_commands);
    local_ext_prop_mask =
	ext_prop_mask(VAR_PROP_EXTENSION, var_prop_extension);
    local_deliver_hdr_mask = name_mask(VAR_DELIVER_HDR, deliver_mask,
				       var_deliver_hdr);
    local_mbox_lock_mask = mbox_lock_mask(var_mailbox_lock);
    if (var_mailtool_compat) {
	msg_warn("%s: deprecated parameter, use \"%s = dotlock\" instead",
		 VAR_MAILTOOL_COMPAT, VAR_MAILBOX_LOCK);
	local_mbox_lock_mask &= MBOX_DOT_LOCK;
    }
    if (local_mbox_lock_mask == 0)
	msg_fatal("parameter %s specifies no applicable mailbox locking method",
		  VAR_MAILBOX_LOCK);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
	exit(0);
    }
}

/* post_init - post-jail initialization */

static void post_init(char *unused_name, char **unused_argv)
{

    /*
     * Drop privileges most of the time, and set up delivery restrictions.
     */
    set_eugid(var_owner_uid, var_owner_gid);
    local_mask_init();
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{

    /*
     * Reset the file size limit from the message size limit to the mailbox
     * size limit. XXX This still isn't accurate because the file size limit
     * also affects delivery to command.
     * 
     * A file size limit protects the machine against runaway software errors.
     * It is not suitable to enforce mail quota, because users can get around
     * mail quota by delivering to /file/name or to |command.
     * 
     * We can't have mailbox size limit smaller than the message size limit,
     * because that prohibits the delivery agent from updating the queue
     * file.
     */
    if (var_mailbox_limit) {
	if (var_mailbox_limit < var_message_limit)
	    msg_fatal("main.cf configuration error: %s is smaller than %s",
		      VAR_MAILBOX_LIMIT, VAR_MESSAGE_LIMIT);
	set_file_limit(var_mailbox_limit);
    }
    alias_maps = maps_create("aliases", var_alias_maps,
			     DICT_FLAG_LOCK | DICT_FLAG_PARANOID);

    flush_init();
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_COMMAND_MAXTIME, DEF_COMMAND_MAXTIME, &var_command_maxtime, 1, 0,
	0,
    };
    static CONFIG_INT_TABLE int_table[] = {
	VAR_DUP_FILTER_LIMIT, DEF_DUP_FILTER_LIMIT, &var_dup_filter_limit, 0, 0,
	VAR_MAILBOX_LIMIT, DEF_MAILBOX_LIMIT, &var_mailbox_limit, 0, 0,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_ALIAS_MAPS, DEF_ALIAS_MAPS, &var_alias_maps, 0, 0,
	VAR_HOME_MAILBOX, DEF_HOME_MAILBOX, &var_home_mailbox, 0, 0,
	VAR_ALLOW_COMMANDS, DEF_ALLOW_COMMANDS, &var_allow_commands, 0, 0,
	VAR_ALLOW_FILES, DEF_ALLOW_FILES, &var_allow_files, 0, 0,
	VAR_LOCAL_CMD_SHELL, DEF_LOCAL_CMD_SHELL, &var_local_cmd_shell, 0, 0,
	VAR_MAIL_SPOOL_DIR, DEF_MAIL_SPOOL_DIR, &var_mail_spool_dir, 0, 0,
	VAR_MAILBOX_TRANSP, DEF_MAILBOX_TRANSP, &var_mailbox_transport, 0, 0,
	VAR_FALLBACK_TRANSP, DEF_FALLBACK_TRANSP, &var_fallback_transport, 0, 0,
	VAR_CMD_EXP_FILTER, DEF_CMD_EXP_FILTER, &var_cmd_exp_filter, 1, 0,
	VAR_FWD_EXP_FILTER, DEF_FWD_EXP_FILTER, &var_fwd_exp_filter, 1, 0,
	VAR_PROP_EXTENSION, DEF_PROP_EXTENSION, &var_prop_extension, 0, 0,
	VAR_DELIVER_HDR, DEF_DELIVER_HDR, &var_deliver_hdr, 0, 0,
	VAR_MAILBOX_LOCK, DEF_MAILBOX_LOCK, &var_mailbox_lock, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_BIFF, DEF_BIFF, &var_biff,
	VAR_EXP_OWN_ALIAS, DEF_EXP_OWN_ALIAS, &var_exp_own_alias,
	VAR_STAT_HOME_DIR, DEF_STAT_HOME_DIR, &var_stat_home_dir,
	VAR_MAILTOOL_COMPAT, DEF_MAILTOOL_COMPAT, &var_mailtool_compat,
	0,
    };

    /* Suppress $name expansion upon loading. */
    static CONFIG_RAW_TABLE raw_table[] = {
	VAR_FORWARD_PATH, DEF_FORWARD_PATH, &var_forward_path, 0, 0,
	VAR_MAILBOX_COMMAND, DEF_MAILBOX_COMMAND, &var_mailbox_command, 0, 0,
	VAR_MAILBOX_CMD_MAPS, DEF_MAILBOX_CMD_MAPS, &var_mailbox_cmd_maps, 0, 0,
	VAR_LUSER_RELAY, DEF_LUSER_RELAY, &var_luser_relay, 0, 0,
	0,
    };

    single_server_main(argc, argv, local_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_RAW_TABLE, raw_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_init,
		       MAIL_SERVER_POST_INIT, post_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
