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
/*	be tried again at a later time. Delivery problem reports are sent
/*	to the \fBbounce\fR(8) or \fBdefer\fR(8) daemon as appropriate.
/* SYSTEM-WIDE AND USER-LEVEL ALIASING
/* .ad
/* .fi
/*	The system adminstrator can set up one or more system-wide
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
/*	\fBDelivered-To:\fR header with the envelope recipient address. If
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
/*	optional \fBDelivered-To:\fR header
/*	with the envelope recipient address, prepends a \fBReturn-Path:\fR
/*	header with the envelope sender address, prepends a \fB>\fR character
/*	to lines beginning with "\fBFrom \fR", and appends an empty line.
/*	The mailbox is locked for exclusive access while delivery is in
/*	progress. In case of problems, an attempt is made to truncate the
/*	mailbox to its original length.
/*
/*	In the case of \fBmaildir\fR delivery, the local daemon prepends
/*	an optional
/*	\fBDelivered-To:\fR header with the envelope recipient address
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
/* .PP
/*	The \fBPATH\fR environment variable is always reset to a
/*	system-dependent default path, and the \fBTZ\fR (time zone)
/*	environment variable is always passed on without change.
/*
/*	The current working directory is the mail queue directory.
/*
/*	The \fBlocal\fR daemon prepends a "\fBFrom \fIsender time_stamp\fR"
/*	envelope header to each message, prepends an
/*	optional \fBDelivered-To:\fR
/*	header with the recipient envelope address, prepends a
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
/*	optional \fBDelivered-To:\fR
/*	header with the recipient envelope address, prepends a \fB>\fR
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
/*	\fBDelivered-To:\fR header with the envelope recipient address.
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
/*	`\fBDelivered-To:\fR \fIname\fR+\fIfoo\fR' header line.
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
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values. Use the \fBpostfix reload\fR command after
/*	a configuration change.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBalias_maps\fR
/*	List of alias databases.
/* .IP \fBbiff\fR
/*	Enable or disable notification of new mail via the
/*	\fBcomsat\fR network service.
/* .IP \fBexpand_owner_alias\fR
/*	When delivering to an alias that has an owner- companion alias,
/*	set the envelope sender address to the right-hand side of the
/*	owner alias, instead using of the left-hand side address.
/* .IP \fBexport_environment\fR
/*	List of names of environment parameters that can be exported
/*	to non-Postfix processes.
/* .IP \fBforward_path\fR
/*	Search list for .forward files.  The names are subject to \fI$name\fR
/*	expansion.
/* .IP \fBlocal_command_shell\fR
/*	Shell to use for external command execution (for example,
/*	/some/where/smrsh -c).
/*	When a shell is specified, it is invoked even when the command
/*	contains no shell built-in commands or meta characters.
/* .IP \fBowner_request_special\fR
/*	Give special treatment to \fBowner-\fIxxx\fR and \fIxxx\fB-request\fR
/*	addresses.
/* .IP \fBprepend_delivered_header\fR
/*	Prepend an optional \fBDelivered-To:\fR header upon external
/*	forwarding, delivery to command or file. Specify zero or more of:
/*	\fBcommand, file, forward\fR. Turning off \fBDelivered-To:\fR when
/*	forwarding mail is not recommended.
/* .IP \fBrecipient_delimiter\fR
/*	Separator between username and address extension.
/* .IP \fBrequire_home_directory\fR
/*	Require that a recipient's home directory is accessible by the
/*	recipient before attempting delivery. Defer delivery otherwise.
/* .SH Mailbox delivery
/* .ad
/* .fi
/* .IP \fBfallback_transport\fR
/*	Message transport for recipients that are not found in the UNIX
/*	passwd database.
/*	This parameter overrides \fBluser_relay\fR.
/* .IP \fBhome_mailbox\fR
/*	Pathname of a mailbox relative to a user's home directory.
/*	Specify a path ending in \fB/\fR for maildir-style delivery.
/* .IP \fBluser_relay\fR
/*	Destination (\fI@domain\fR or \fIaddress\fR) for non-existent users.
/*	The \fIaddress\fR is subjected to \fI$name\fR expansion.
/* .IP \fBmail_spool_directory\fR
/*	Directory with UNIX-style mailboxes. The default pathname is system
/*	dependent.
/*	Specify a path ending in \fB/\fR for maildir-style delivery.
/* .IP \fBmailbox_command\fR
/*	External command to use for mailbox delivery. The command executes
/*	with the recipient privileges (exception: root). The string is subject
/*	to $name expansions.
/* .IP \fBmailbox_command_maps\fR
/*	Lookup tables with per-recipient external commands to use for mailbox
/*	delivery. Behavior is as with \fBmailbox_command\fR.
/* .IP \fBmailbox_transport\fR
/*	Message transport to use for mailbox delivery to all local
/*	recipients, whether or not they are found in the UNIX passwd database.
/*	This parameter overrides all other configuration parameters that
/*	control mailbox delivery, including \fBluser_relay\fR.
/* .SH "Locking controls"
/* .ad
/* .fi
/* .IP \fBdeliver_lock_attempts\fR
/*	Limit the number of attempts to acquire an exclusive lock
/*	on a mailbox or external file.
/* .IP \fBdeliver_lock_delay\fR
/*	Time in seconds between successive attempts to acquire
/*	an exclusive lock.
/* .IP \fBstale_lock_time\fR
/*	Limit the time after which a stale lock is removed.
/* .IP \fBmailbox_delivery_lock\fR
/*	What file locking method(s) to use when delivering to a UNIX-style
/*	mailbox.
/*	The default setting is system dependent.  For a list of available
/*	file locking methods, use the \fBpostconf -l\fR command.
/* .SH "Resource controls"
/* .ad
/* .fi
/* .IP \fBcommand_time_limit\fR
/*	Limit the amount of time for delivery to external command.
/* .IP \fBduplicate_filter_limit\fR
/*	Limit the size of the duplicate filter for results from
/*	alias etc. expansion.
/* .IP \fBline_length_limit\fR
/*	Limit the amount of memory used for processing a partial
/*	input line.
/* .IP \fBlocal_destination_concurrency_limit\fR
/*	Limit the number of parallel deliveries to the same user.
/*	The default limit is taken from the
/*	\fBdefault_destination_concurrency_limit\fR parameter.
/* .IP \fBlocal_destination_recipient_limit\fR
/*	Limit the number of recipients per message delivery.
/*	The default limit is taken from the
/*	\fBdefault_destination_recipient_limit\fR parameter.
/* .IP \fBmailbox_size_limit\fR
/*	Limit the size of a mailbox etc. file (any file that is
/*	written to upon delivery).
/*	Set to zero to disable the limit.
/* .SH "Security controls"
/* .ad
/* .fi
/* .IP \fBallow_mail_to_commands\fR
/*	Restrict the usage of mail delivery to external command.
/*	Specify zero or more of: \fBalias\fR, \fBforward\fR, \fBinclude\fR.
/* .IP \fBallow_mail_to_files\fR
/*	Restrict the usage of mail delivery to external file.
/*	Specify zero or more of: \fBalias\fR, \fBforward\fR, \fBinclude\fR.
/* .IP \fBcommand_expansion_filter\fR
/*	What characters are allowed to appear in $name expansions of
/*	mailbox_command. Illegal characters are replaced by underscores.
/* .IP \fBdefault_privs\fR
/*	Default rights for delivery to external file or command.
/* .IP \fBforward_expansion_filter\fR
/*	What characters are allowed to appear in $name expansions of
/*	forward_path. Illegal characters are replaced by underscores.
/* HISTORY
/* .ad
/* .fi
/*	The \fBDelivered-To:\fR header appears in the \fBqmail\fR system
/*	by Daniel Bernstein.
/*
/*	The \fImaildir\fR structure appears in the \fBqmail\fR system
/*	by Daniel Bernstein.
/* SEE ALSO
/*	aliases(5) format of alias database
/*	bounce(8) non-delivery status reports
/*	postalias(1) create/update alias database
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
	state.msg_attr.recipient = rcpt->address;
	rcpt_stat = deliver_recipient(state, usr_attr);
	rcpt_stat |= forward_finish(state.msg_attr, rcpt_stat);
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
    local_ext_prop_mask = ext_prop_mask(var_prop_extension);
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
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
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
