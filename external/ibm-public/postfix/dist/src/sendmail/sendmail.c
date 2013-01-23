/*	$NetBSD: sendmail.c,v 1.1.1.1.10.1 2013/01/23 00:05:11 yamt Exp $	*/

/*++
/* NAME
/*	sendmail 1
/* SUMMARY
/*	Postfix to Sendmail compatibility interface
/* SYNOPSIS
/*	\fBsendmail\fR [\fIoption ...\fR] [\fIrecipient ...\fR]
/*
/*	\fBmailq\fR
/*	\fBsendmail -bp\fR
/*
/*	\fBnewaliases\fR
/*	\fBsendmail -I\fR
/* DESCRIPTION
/*	The Postfix \fBsendmail\fR(1) command implements the Postfix
/*	to Sendmail compatibility interface.
/*	For the sake of compatibility with existing applications, some
/*	Sendmail command-line options are recognized but silently ignored.
/*
/*	By default, Postfix \fBsendmail\fR(1) reads a message from
/*	standard input
/*	until EOF or until it reads a line with only a \fB.\fR character,
/*	and arranges for delivery.  Postfix \fBsendmail\fR(1) relies on the
/*	\fBpostdrop\fR(1) command to create a queue file in the \fBmaildrop\fR
/*	directory.
/*
/*	Specific command aliases are provided for other common modes of
/*	operation:
/* .IP \fBmailq\fR
/*	List the mail queue. Each entry shows the queue file ID, message
/*	size, arrival time, sender, and the recipients that still need to
/*	be delivered.  If mail could not be delivered upon the last attempt,
/*	the reason for failure is shown. The queue ID string is
/*	followed by an optional status character:
/* .RS
/* .IP \fB*\fR
/*	The message is in the \fBactive\fR queue, i.e. the message is
/*	selected for delivery.
/* .IP \fB!\fR
/*	The message is in the \fBhold\fR queue, i.e. no further delivery
/*	attempt will be made until the mail is taken off hold.
/* .RE
/* .IP
/*	This mode of operation is implemented by executing the
/*	\fBpostqueue\fR(1) command.
/* .IP \fBnewaliases\fR
/*	Initialize the alias database.  If no input file is specified (with
/*	the \fB-oA\fR option, see below), the program processes the file(s)
/*	specified with the \fBalias_database\fR configuration parameter.
/*	If no alias database type is specified, the program uses the type
/*	specified with the \fBdefault_database_type\fR configuration parameter.
/*	This mode of operation is implemented by running the \fBpostalias\fR(1)
/*	command.
/* .sp
/*	Note: it may take a minute or so before an alias database update
/*	becomes visible. Use the "\fBpostfix reload\fR" command to eliminate
/*	this delay.
/* .PP
/*	These and other features can be selected by specifying the
/*	appropriate combination of command-line options. Some features are
/*	controlled by parameters in the \fBmain.cf\fR configuration file.
/*
/*	The following options are recognized:
/* .IP "\fB-Am\fR (ignored)"
/* .IP "\fB-Ac\fR (ignored)"
/*	Postfix sendmail uses the same configuration file regardless of
/*	whether or not a message is an initial submission.
/* .IP "\fB-B \fIbody_type\fR"
/*	The message body MIME type: \fB7BIT\fR or \fB8BITMIME\fR.
/* .IP \fB-bd\fR
/*	Go into daemon mode. This mode of operation is implemented by
/*	executing the "\fBpostfix start\fR" command.
/* .IP "\fB-bh\fR (ignored)"
/* .IP "\fB-bH\fR (ignored)"
/*	Postfix has no persistent host status database.
/* .IP \fB-bi\fR
/*	Initialize alias database. See the \fBnewaliases\fR
/*	command above.
/* .IP \fB-bm\fR
/*	Read mail from standard input and arrange for delivery.
/*	This is the default mode of operation.
/* .IP \fB-bp\fR
/*	List the mail queue. See the \fBmailq\fR command above.
/* .IP \fB-bs\fR
/*	Stand-alone SMTP server mode. Read SMTP commands from
/*	standard input, and write responses to standard output.
/*	In stand-alone SMTP server mode, mail relaying and other
/*	access controls are disabled by default. To enable them,
/*	run the process as the \fBmail_owner\fR user.
/* .sp
/*	This mode of operation is implemented by running the
/*	\fBsmtpd\fR(8) daemon.
/* .IP \fB-bv\fR
/*	Do not collect or deliver a message. Instead, send an email
/*	report after verifying each recipient address.  This is useful
/*	for testing address rewriting and routing configurations.
/* .sp
/*	This feature is available in Postfix version 2.1 and later.
/* .IP "\fB-C \fIconfig_file\fR"
/* .IP "\fB-C \fIconfig_dir\fR"
/*	The path name of the Postfix \fBmain.cf\fR file, or of its
/*	parent directory. This information is ignored with Postfix
/*	versions before 2.3.
/*
/*	With all Postfix versions, you can specify a directory pathname
/*	with the MAIL_CONFIG environment variable to override the
/*	location of configuration files.
/* .IP "\fB-F \fIfull_name\fR
/*	Set the sender full name. This overrides the NAME environment
/*	variable, and is used only with messages that
/*	have no \fBFrom:\fR message header.
/* .IP "\fB-f \fIsender\fR"
/*	Set the envelope sender address. This is the address where
/*	delivery problems are sent to. With Postfix versions before 2.1, the
/*	\fBErrors-To:\fR message header overrides the error return address.
/* .IP \fB-G\fR
/*	Gateway (relay) submission, as opposed to initial user
/*	submission.  Either do not rewrite addresses at all, or
/*	update incomplete addresses with the domain information
/*	specified with \fBremote_header_rewrite_domain\fR.
/*
/*	This option is ignored before Postfix version 2.3.
/* .IP "\fB-h \fIhop_count\fR (ignored)"
/*	Hop count limit. Use the \fBhopcount_limit\fR configuration
/*	parameter instead.
/* .IP \fB-I\fR
/*	Initialize alias database. See the \fBnewaliases\fR
/*	command above.
/* .IP "\fB-i\fR"
/*	When reading a message from standard input, don\'t treat a line
/*	with only a \fB.\fR character as the end of input.
/* .IP "\fB-L \fIlabel\fR (ignored)"
/*	The logging label. Use the \fBsyslog_name\fR configuration
/*	parameter instead.
/* .IP "\fB-m\fR (ignored)"
/*	Backwards compatibility.
/* .IP "\fB-N \fIdsn\fR (default: 'delay, failure')"
/*	Delivery status notification control. Specify either a
/*	comma-separated list with one or more of \fBfailure\fR (send
/*	notification when delivery fails), \fBdelay\fR (send
/*	notification when delivery is delayed), or \fBsuccess\fR
/*	(send notification when the message is delivered); or specify
/*	\fBnever\fR (don't send any notifications at all).
/*
/*	This feature is available in Postfix 2.3 and later.
/* .IP "\fB-n\fR (ignored)"
/*	Backwards compatibility.
/* .IP "\fB-oA\fIalias_database\fR"
/*	Non-default alias database. Specify \fIpathname\fR or
/*	\fItype\fR:\fIpathname\fR. See \fBpostalias\fR(1) for
/*	details.
/* .IP "\fB-O \fIoption=value\fR (ignored)"
/*	Backwards compatibility.
/* .IP "\fB-o7\fR (ignored)"
/* .IP "\fB-o8\fR (ignored)"
/*	To send 8-bit or binary content, use an appropriate MIME encapsulation
/*	and specify the appropriate \fB-B\fR command-line option.
/* .IP "\fB-oi\fR"
/*	When reading a message from standard input, don\'t treat a line
/*	with only a \fB.\fR character as the end of input.
/* .IP "\fB-om\fR (ignored)"
/*	The sender is never eliminated from alias etc. expansions.
/* .IP "\fB-o \fIx value\fR (ignored)"
/*	Set option \fIx\fR to \fIvalue\fR. Use the equivalent
/*	configuration parameter in \fBmain.cf\fR instead.
/* .IP "\fB-r \fIsender\fR"
/*	Set the envelope sender address. This is the address where
/*	delivery problems are sent to. With Postfix versions before 2.1, the
/*	\fBErrors-To:\fR message header overrides the error return address.
/* .IP "\fB-R \fIreturn_limit\fR (ignored)"
/*	Limit the size of bounced mail. Use the \fBbounce_size_limit\fR
/*	configuration parameter instead.
/* .IP \fB-q\fR
/*	Attempt to deliver all queued mail. This is implemented by
/*	executing the \fBpostqueue\fR(1) command.
/*
/*	Warning: flushing undeliverable mail frequently will result in
/*	poor delivery performance of all other mail.
/* .IP "\fB-q\fIinterval\fR (ignored)"
/*	The interval between queue runs. Use the \fBqueue_run_delay\fR
/*	configuration parameter instead.
/* .IP \fB-qI\fIqueueid\fR
/*	Schedule immediate delivery of mail with the specified queue
/*	ID.  This option is implemented by executing the
/*	\fBpostqueue\fR(1) command, and is available with Postfix
/*	version 2.4 and later.
/* .IP \fB-qR\fIsite\fR
/*	Schedule immediate delivery of all mail that is queued for the named
/*	\fIsite\fR. This option accepts only \fIsite\fR names that are
/*	eligible for the "fast flush" service, and is implemented by
/*	executing the \fBpostqueue\fR(1) command.
/*	See \fBflush\fR(8) for more information about the "fast flush"
/*	service.
/* .IP \fB-qS\fIsite\fR
/*	This command is not implemented. Use the slower "\fBsendmail -q\fR"
/*	command instead.
/* .IP \fB-t\fR
/*	Extract recipients from message headers. These are added to any
/*	recipients specified on the command line.
/*
/*	With Postfix versions prior to 2.1, this option requires that
/*	no recipient addresses are specified on the command line.
/* .IP "\fB-U\fR (ignored)"
/*	Initial user submission.
/* .IP "\fB-V \fIenvid\fR"
/*	Specify the envelope ID for notification by servers that
/*	support DSN.
/*
/*	This feature is available in Postfix 2.3 and later.
/* .IP "\fB-XV\fR (Postfix 2.2 and earlier: \fB-V\fR)"
/*	Variable Envelope Return Path. Given an envelope sender address
/*	of the form \fIowner-listname\fR@\fIorigin\fR, each recipient
/*	\fIuser\fR@\fIdomain\fR receives mail with a personalized envelope
/*	sender address.
/* .sp
/*	By default, the personalized envelope sender address is
/*	\fIowner-listname\fB+\fIuser\fB=\fIdomain\fR@\fIorigin\fR. The default
/*	\fB+\fR and \fB=\fR characters are configurable with the
/*	\fBdefault_verp_delimiters\fR configuration parameter.
/* .IP "\fB-XV\fIxy\fR (Postfix 2.2 and earlier: \fB-V\fIxy\fR)"
/*	As \fB-XV\fR, but uses \fIx\fR and \fIy\fR as the VERP delimiter
/*	characters, instead of the characters specified with the
/*	\fBdefault_verp_delimiters\fR configuration parameter.
/* .IP \fB-v\fR
/*	Send an email report of the first delivery attempt (Postfix
/*	versions 2.1 and later). Mail delivery
/*	always happens in the background. When multiple \fB-v\fR
/*	options are given, enable verbose logging for debugging purposes.
/* .IP "\fB-X \fIlog_file\fR (ignored)"
/*	Log mailer traffic. Use the \fBdebug_peer_list\fR and
/*	\fBdebug_peer_level\fR configuration parameters instead.
/* SECURITY
/* .ad
/* .fi
/*	By design, this program is not set-user (or group) id. However,
/*	it must handle data from untrusted, possibly remote, users.
/*	Thus, the usual precautions need to be taken against malicious
/*	inputs.
/* DIAGNOSTICS
/*	Problems are logged to \fBsyslogd\fR(8) and to the standard error
/*	stream.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* .IP "\fBMAIL_VERBOSE\fR (value does not matter)"
/*	Enable verbose logging for debugging purposes.
/* .IP "\fBMAIL_DEBUG\fR (value does not matter)"
/*	Enable debugging with an external command, as specified with the
/*	\fBdebugger_command\fR configuration parameter.
/* .IP \fBNAME\fR
/*	The sender full name. This is used only with messages that
/*	have no \fBFrom:\fR message header. See also the \fB-F\fR
/*	option above.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/*	Available with Postfix 2.9 and later:
/* .IP "\fBsendmail_fix_line_endings (always)\fR"
/*	Controls how the Postfix sendmail command converts email message
/*	line endings from <CR><LF> into UNIX format (<LF>).
/* TROUBLE SHOOTING CONTROLS
/* .ad
/* .fi
/*	The DEBUG_README file gives examples of how to trouble shoot a
/*	Postfix system.
/* .IP "\fBdebugger_command (empty)\fR"
/*	The external command to execute when a Postfix daemon program is
/*	invoked with the -D option.
/* .IP "\fBdebug_peer_level (2)\fR"
/*	The increment in verbose logging level when a remote client or
/*	server matches a pattern in the debug_peer_list parameter.
/* .IP "\fBdebug_peer_list (empty)\fR"
/*	Optional list of remote client or server hostname or network
/*	address patterns that cause the verbose logging level to increase
/*	by the amount specified in $debug_peer_level.
/* ACCESS CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBauthorized_flush_users (static:anyone)\fR"
/*	List of users who are authorized to flush the queue.
/* .IP "\fBauthorized_mailq_users (static:anyone)\fR"
/*	List of users who are authorized to view the queue.
/* .IP "\fBauthorized_submit_users (static:anyone)\fR"
/*	List of users who are authorized to submit mail with the \fBsendmail\fR(1)
/*	command (and with the privileged \fBpostdrop\fR(1) helper command).
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBbounce_size_limit (50000)\fR"
/*	The maximal amount of original message text that is sent in a
/*	non-delivery notification.
/* .IP "\fBfork_attempts (5)\fR"
/*	The maximal number of attempts to fork() a child process.
/* .IP "\fBfork_delay (1s)\fR"
/*	The delay between attempts to fork() a child process.
/* .IP "\fBhopcount_limit (50)\fR"
/*	The maximal number of Received:  message headers that is allowed
/*	in the primary message headers.
/* .IP "\fBqueue_run_delay (300s)\fR"
/*	The time between deferred queue scans by the queue manager;
/*	prior to Postfix 2.4 the default value was 1000s.
/* FAST FLUSH CONTROLS
/* .ad
/* .fi
/*	The ETRN_README file describes configuration and operation
/*	details for the Postfix "fast flush" service.
/* .IP "\fBfast_flush_domains ($relay_domains)\fR"
/*	Optional list of destinations that are eligible for per-destination
/*	logfiles with mail that is queued to those destinations.
/* VERP CONTROLS
/* .ad
/* .fi
/*	The VERP_README file describes configuration and operation
/*	details of Postfix support for variable envelope return
/*	path addresses.
/* .IP "\fBdefault_verp_delimiters (+=)\fR"
/*	The two default VERP delimiter characters.
/* .IP "\fBverp_delimiter_filter (-=+)\fR"
/*	The characters Postfix accepts as VERP delimiter characters on the
/*	Postfix \fBsendmail\fR(1) command line and in SMTP commands.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBalias_database (see 'postconf -d' output)\fR"
/*	The alias databases for \fBlocal\fR(8) delivery that are updated with
/*	"\fBnewaliases\fR" or with "\fBsendmail -bi\fR".
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix support programs and daemon programs.
/* .IP "\fBdefault_database_type (see 'postconf -d' output)\fR"
/*	The default database type for use in \fBnewaliases\fR(1), \fBpostalias\fR(1)
/*	and \fBpostmap\fR(1) commands.
/* .IP "\fBdelay_warning_time (0h)\fR"
/*	The time after which the sender receives the message headers of
/*	mail that is still queued.
/* .IP "\fBenable_errors_to (no)\fR"
/*	Report mail delivery errors to the address specified with the
/*	non-standard Errors-To: message header, instead of the envelope
/*	sender address (this feature is removed with Postfix version 2.2, is
/*	turned off by default with Postfix version 2.1, and is always turned on
/*	with older Postfix versions).
/* .IP "\fBmail_owner (postfix)\fR"
/*	The UNIX system account that owns the Postfix queue and most Postfix
/*	daemon processes.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBremote_header_rewrite_domain (empty)\fR"
/*	Don't rewrite message headers from remote clients at all when
/*	this parameter is empty; otherwise, rewrite message headers and
/*	append the specified domain name to incomplete addresses.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* FILES
/*	/var/spool/postfix, mail queue
/*	/etc/postfix, configuration files
/* SEE ALSO
/*	pickup(8), mail pickup daemon
/*	qmgr(8), queue manager
/*	smtpd(8), SMTP server
/*	flush(8), fast flush service
/*	postsuper(1), queue maintenance
/*	postalias(1), create/update/query alias database
/*	postdrop(1), mail posting utility
/*	postfix(1), mail system control
/*	postqueue(1), mail queue control
/*	syslogd(8), system logging
/* README_FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	DEBUG_README, Postfix debugging howto
/*	ETRN_README, Postfix ETRN howto
/*	VERP_README, Postfix VERP howto
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
#include <unistd.h>
#include <string.h>
#include <stdio.h>			/* remove() */
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <sysexits.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <msg_syslog.h>
#include <vstring_vstream.h>
#include <username.h>
#include <fullname.h>
#include <argv.h>
#include <safe.h>
#include <iostuff.h>
#include <stringops.h>
#include <set_ugid.h>
#include <connect.h>
#include <split_at.h>
#include <name_code.h>
#include <warn_stat.h>

/* Global library. */

#include <mail_queue.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <mail_version.h>
#include <record.h>
#include <rec_type.h>
#include <rec_streamlf.h>
#include <mail_conf.h>
#include <cleanup_user.h>
#include <mail_task.h>
#include <mail_run.h>
#include <debug_process.h>
#include <tok822.h>
#include <mail_flush.h>
#include <mail_stream.h>
#include <verp_sender.h>
#include <deliver_request.h>
#include <mime_state.h>
#include <header_opts.h>
#include <user_acl.h>
#include <dsn_mask.h>

/* Application-specific. */

 /*
  * Modes of operation.
  */
#define SM_MODE_ENQUEUE		1	/* delivery mode */
#define SM_MODE_NEWALIAS	2	/* initialize alias database */
#define SM_MODE_MAILQ		3	/* list mail queue */
#define SM_MODE_DAEMON		4	/* daemon mode */
#define SM_MODE_USER		5	/* user (stand-alone) mode */
#define SM_MODE_FLUSHQ		6	/* user (stand-alone) mode */
#define SM_MODE_IGNORE		7	/* ignore this mode */

 /*
  * Flag parade. Flags 8-15 are reserved for delivery request trace flags.
  */
#define SM_FLAG_AEOF	(1<<0)		/* archaic EOF */
#define SM_FLAG_XRCPT	(1<<1)		/* extract recipients from headers */

#define SM_FLAG_DEFAULT	(SM_FLAG_AEOF)

 /*
  * VERP support.
  */
static char *verp_delims;

 /*
  * Callback context for extracting recipients.
  */
typedef struct SM_STATE {
    VSTREAM *dst;			/* output stream */
    ARGV   *recipients;			/* recipients from regular headers */
    ARGV   *resent_recip;		/* recipients from resent headers */
    int     resent;			/* resent flag */
    const char *saved_sender;		/* for error messages */
    uid_t   uid;			/* for error messages */
    VSTRING *temp;			/* scratch buffer */
} SM_STATE;

 /*
  * Mail submission ACL, line-end fixing.
  */
char   *var_submit_acl;
char   *var_sm_fix_eol;

static const CONFIG_STR_TABLE str_table[] = {
    VAR_SUBMIT_ACL, DEF_SUBMIT_ACL, &var_submit_acl, 0, 0,
    VAR_SM_FIX_EOL, DEF_SM_FIX_EOL, &var_sm_fix_eol, 1, 0,
    0,
};

 /*
  * Silly little macros (SLMs).
  */
#define STR	vstring_str

/* output_text - output partial or complete text line */

static void output_text(void *context, int rec_type, const char *buf, ssize_t len,
			        off_t unused_offset)
{
    SM_STATE *state = (SM_STATE *) context;

    if (rec_put(state->dst, rec_type, buf, len) < 0)
	msg_fatal_status(EX_TEMPFAIL,
			 "%s(%ld): error writing queue file: %m",
			 state->saved_sender, (long) state->uid);
}

/* output_header - output one message header */

static void output_header(void *context, int header_class,
			          const HEADER_OPTS *header_info,
			          VSTRING *buf, off_t offset)
{
    SM_STATE *state = (SM_STATE *) context;
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    ARGV   *rcpt;
    char   *start;
    char   *line;
    char   *next_line;
    ssize_t len;

    /*
     * Parse the header line, and save copies of recipient addresses in the
     * appropriate place.
     */
    if (header_class == MIME_HDR_PRIMARY
	&& header_info
	&& (header_info->flags & HDR_OPT_RECIP)
	&& (header_info->flags & HDR_OPT_EXTRACT)
	&& (state->resent == 0 || (header_info->flags & HDR_OPT_RR))) {
	if (header_info->flags & HDR_OPT_RR) {
	    rcpt = state->resent_recip;
	    if (state->resent == 0)
		state->resent = 1;
	} else
	    rcpt = state->recipients;
	tree = tok822_parse(STR(buf) + strlen(header_info->name) + 1);
	addr_list = tok822_grep(tree, TOK822_ADDR);
	for (tpp = addr_list; *tpp; tpp++) {
	    tok822_internalize(state->temp, tpp[0]->head, TOK822_STR_DEFL);
	    argv_add(rcpt, STR(state->temp), (char *) 0);
	}
	myfree((char *) addr_list);
	tok822_free_tree(tree);
    }

    /*
     * Pipe the unmodified message header through the header line folding
     * routine, and ensure that long lines are chopped appropriately.
     */
    for (line = start = STR(buf); line; line = next_line) {
	next_line = split_at(line, '\n');
	len = next_line ? next_line - line - 1 : strlen(line);
	do {
	    if (len > var_line_limit) {
		output_text(context, REC_TYPE_CONT, line, var_line_limit, offset);
		line += var_line_limit;
		len -= var_line_limit;
		offset += var_line_limit;
	    } else {
		output_text(context, REC_TYPE_NORM, line, len, offset);
		offset += len;
		break;
	    }
	} while (len > 0);
	offset += 1;
    }
}

/* enqueue - post one message */

static void enqueue(const int flags, const char *encoding,
		            const char *dsn_envid, int dsn_notify,
		            const char *rewrite_context, const char *sender,
		            const char *full_name, char **recipients)
{
    VSTRING *buf;
    VSTREAM *dst;
    char   *saved_sender;
    char  **cpp;
    int     type;
    char   *start;
    int     skip_from_;
    TOK822 *tree;
    TOK822 *tp;
    int     rcpt_count = 0;
    enum {
	STRIP_CR_DUNNO, STRIP_CR_DO, STRIP_CR_DONT, STRIP_CR_ERROR
    }       strip_cr;
    MAIL_STREAM *handle;
    VSTRING *postdrop_command;
    uid_t   uid = getuid();
    int     status;
    int     naddr;
    int     prev_type;
    MIME_STATE *mime_state = 0;
    SM_STATE state;
    int     mime_errs;
    const char *errstr;
    int     addr_count;
    int     level;
    static NAME_CODE sm_fix_eol_table[] = {
	SM_FIX_EOL_ALWAYS, STRIP_CR_DO,
	SM_FIX_EOL_STRICT, STRIP_CR_DUNNO,
	SM_FIX_EOL_NEVER, STRIP_CR_DONT,
	0, STRIP_CR_ERROR,
    };

    /*
     * Access control is enforced in the postdrop command. The code here
     * merely produces a more user-friendly interface.
     */
    if ((errstr = check_user_acl_byuid(var_submit_acl, uid)) != 0)
	msg_fatal_status(EX_NOPERM,
	  "User %s(%ld) is not allowed to submit mail", errstr, (long) uid);

    /*
     * Initialize.
     */
    buf = vstring_alloc(100);

    /*
     * Stop run-away process accidents by limiting the queue file size. This
     * is not a defense against DOS attack.
     */
    if (var_message_limit > 0 && get_file_limit() > var_message_limit)
	set_file_limit((off_t) var_message_limit);

    /*
     * The sender name is provided by the user. In principle, the mail pickup
     * service could deduce the sender name from queue file ownership, but:
     * pickup would not be able to run chrooted, and it may not be desirable
     * to use login names at all.
     */
    if (sender != 0) {
	VSTRING_RESET(buf);
	VSTRING_TERMINATE(buf);
	tree = tok822_parse(sender);
	for (naddr = 0, tp = tree; tp != 0; tp = tp->next)
	    if (tp->type == TOK822_ADDR && naddr++ == 0)
		tok822_internalize(buf, tp->head, TOK822_STR_DEFL);
	tok822_free_tree(tree);
	saved_sender = mystrdup(STR(buf));
	if (naddr > 1)
	    msg_warn("-f option specified malformed sender: %s", sender);
    } else {
	if ((sender = username()) == 0)
	    msg_fatal_status(EX_OSERR, "no login name found for user ID %lu",
			     (unsigned long) uid);
	saved_sender = mystrdup(sender);
    }

    /*
     * Let the postdrop command open the queue file for us, and sanity check
     * the content. XXX Make postdrop a manifest constant.
     */
    errno = 0;
    postdrop_command = vstring_alloc(1000);
    vstring_sprintf(postdrop_command, "%s/postdrop -r", var_command_dir);
    for (level = 0; level < msg_verbose; level++)
	vstring_strcat(postdrop_command, " -v");
    if ((handle = mail_stream_command(STR(postdrop_command))) == 0)
	msg_fatal_status(EX_UNAVAILABLE, "%s(%ld): unable to execute %s: %m",
			 saved_sender, (long) uid, STR(postdrop_command));
    vstring_free(postdrop_command);
    dst = handle->stream;

    /*
     * First, write envelope information to the output stream.
     * 
     * For sendmail compatibility, parse each command-line recipient as if it
     * were an RFC 822 message header; some MUAs specify comma-separated
     * recipient lists; and some MUAs even specify "word word <address>".
     * 
     * Sort-uniq-ing the recipient list is done after address canonicalization,
     * before recipients are written to queue file. That's cleaner than
     * having the queue manager nuke duplicate recipient status records.
     * 
     * XXX Should limit the size of envelope records.
     * 
     * With "sendmail -N", instead of a per-message NOTIFY record we store one
     * per recipient so that we can simplify the implementation somewhat.
     */
    if (dsn_envid)
	rec_fprintf(dst, REC_TYPE_ATTR, "%s=%s",
		    MAIL_ATTR_DSN_ENVID, dsn_envid);
    rec_fprintf(dst, REC_TYPE_ATTR, "%s=%s",
		MAIL_ATTR_RWR_CONTEXT, rewrite_context);
    if (full_name || (full_name = fullname()) != 0)
	rec_fputs(dst, REC_TYPE_FULL, full_name);
    rec_fputs(dst, REC_TYPE_FROM, saved_sender);
    if (verp_delims && *saved_sender == 0)
	msg_fatal_status(EX_USAGE,
		      "%s(%ld): -V option requires non-null sender address",
			 saved_sender, (long) uid);
    if (encoding)
	rec_fprintf(dst, REC_TYPE_ATTR, "%s=%s", MAIL_ATTR_ENCODING, encoding);
    if (DEL_REQ_TRACE_FLAGS(flags))
	rec_fprintf(dst, REC_TYPE_ATTR, "%s=%d", MAIL_ATTR_TRACE_FLAGS,
		    DEL_REQ_TRACE_FLAGS(flags));
    if (verp_delims)
	rec_fputs(dst, REC_TYPE_VERP, verp_delims);
    if (recipients) {
	for (cpp = recipients; *cpp != 0; cpp++) {
	    tree = tok822_parse(*cpp);
	    for (addr_count = 0, tp = tree; tp != 0; tp = tp->next) {
		if (tp->type == TOK822_ADDR) {
		    tok822_internalize(buf, tp->head, TOK822_STR_DEFL);
		    if (dsn_notify)
			rec_fprintf(dst, REC_TYPE_ATTR, "%s=%d",
				    MAIL_ATTR_DSN_NOTIFY, dsn_notify);
		    if (REC_PUT_BUF(dst, REC_TYPE_RCPT, buf) < 0)
			msg_fatal_status(EX_TEMPFAIL,
				    "%s(%ld): error writing queue file: %m",
					 saved_sender, (long) uid);
		    ++rcpt_count;
		    ++addr_count;
		}
	    }
	    tok822_free_tree(tree);
	    if (addr_count == 0) {
		if (rec_put(dst, REC_TYPE_RCPT, "", 0) < 0)
		    msg_fatal_status(EX_TEMPFAIL,
				     "%s(%ld): error writing queue file: %m",
				     saved_sender, (long) uid);
		++rcpt_count;
	    }
	}
    }

    /*
     * Append the message contents to the queue file. Write chunks of at most
     * 1kbyte. Internally, we use different record types for data ending in
     * LF and for data that doesn't, so we can actually be binary transparent
     * for local mail. Unfortunately, SMTP has no record continuation
     * convention, so there is no guarantee that arbitrary data will be
     * delivered intact via SMTP. Strip leading From_ lines. For the benefit
     * of UUCP environments, also get rid of leading >>>From_ lines.
     */
    rec_fputs(dst, REC_TYPE_MESG, "");
    if (DEL_REQ_TRACE_ONLY(flags) != 0) {
	if (flags & SM_FLAG_XRCPT)
	    msg_fatal_status(EX_USAGE, "%s(%ld): -t option cannot be used with -bv",
			     saved_sender, (long) uid);
	if (*saved_sender)
	    rec_fprintf(dst, REC_TYPE_NORM, "From: %s", saved_sender);
	rec_fprintf(dst, REC_TYPE_NORM, "Subject: probe");
	if (recipients) {
	    rec_fprintf(dst, REC_TYPE_CONT, "To:");
	    for (cpp = recipients; *cpp != 0; cpp++) {
		rec_fprintf(dst, REC_TYPE_NORM, "	%s%s",
			    *cpp, cpp[1] ? "," : "");
	    }
	}
    } else {

	/*
	 * Initialize the MIME processor and set up the callback context.
	 */
	if (flags & SM_FLAG_XRCPT) {
	    state.dst = dst;
	    state.recipients = argv_alloc(2);
	    state.resent_recip = argv_alloc(2);
	    state.resent = 0;
	    state.saved_sender = saved_sender;
	    state.uid = uid;
	    state.temp = vstring_alloc(10);
	    mime_state = mime_state_alloc(MIME_OPT_DISABLE_MIME
					  | MIME_OPT_REPORT_TRUNC_HEADER,
					  output_header,
					  (MIME_STATE_ANY_END) 0,
					  output_text,
					  (MIME_STATE_ANY_END) 0,
					  (MIME_STATE_ERR_PRINT) 0,
					  (void *) &state);
	}

	/*
	 * Process header/body lines.
	 */
	skip_from_ = 1;
	strip_cr = name_code(sm_fix_eol_table, NAME_CODE_FLAG_STRICT_CASE,
			     var_sm_fix_eol);
	if (strip_cr == STRIP_CR_ERROR)
	    msg_fatal_status(EX_USAGE,
		    "invalid %s value: %s", VAR_SM_FIX_EOL, var_sm_fix_eol);
	for (prev_type = 0; (type = rec_streamlf_get(VSTREAM_IN, buf, var_line_limit))
	     != REC_TYPE_EOF; prev_type = type) {
	    if (strip_cr == STRIP_CR_DUNNO && type == REC_TYPE_NORM) {
		if (VSTRING_LEN(buf) > 0 && vstring_end(buf)[-1] == '\r')
		    strip_cr = STRIP_CR_DO;
		else
		    strip_cr = STRIP_CR_DONT;
	    }
	    if (skip_from_) {
		if (type == REC_TYPE_NORM) {
		    start = STR(buf);
		    if (strncmp(start + strspn(start, ">"), "From ", 5) == 0)
			continue;
		}
		skip_from_ = 0;
	    }
	    if (strip_cr == STRIP_CR_DO && type == REC_TYPE_NORM)
		while (VSTRING_LEN(buf) > 0 && vstring_end(buf)[-1] == '\r')
		    vstring_truncate(buf, VSTRING_LEN(buf) - 1);
	    if ((flags & SM_FLAG_AEOF) && prev_type != REC_TYPE_CONT
		&& VSTRING_LEN(buf) == 1 && *STR(buf) == '.')
		break;
	    if (mime_state) {
		mime_errs = mime_state_update(mime_state, type, STR(buf),
					      VSTRING_LEN(buf));
		if (mime_errs)
		    msg_fatal_status(EX_DATAERR,
				"%s(%ld): unable to extract recipients: %s",
				     saved_sender, (long) uid,
				     mime_state_error(mime_errs));
	    } else {
		if (REC_PUT_BUF(dst, type, buf) < 0)
		    msg_fatal_status(EX_TEMPFAIL,
				     "%s(%ld): error writing queue file: %m",
				     saved_sender, (long) uid);
	    }
	}
    }

    /*
     * Finish MIME processing. We need a final mime_state_update() call in
     * order to flush text that is still buffered. That can happen when the
     * last line did not end in newline.
     */
    if (mime_state) {
	mime_errs = mime_state_update(mime_state, REC_TYPE_EOF, "", 0);
	if (mime_errs)
	    msg_fatal_status(EX_DATAERR,
			     "%s(%ld): unable to extract recipients: %s",
			     saved_sender, (long) uid,
			     mime_state_error(mime_errs));
	mime_state = mime_state_free(mime_state);
    }

    /*
     * Append recipient addresses that were extracted from message headers.
     */
    rec_fputs(dst, REC_TYPE_XTRA, "");
    if (flags & SM_FLAG_XRCPT) {
	for (cpp = state.resent ? state.resent_recip->argv :
	     state.recipients->argv; *cpp; cpp++) {
	    if (dsn_notify)
		rec_fprintf(dst, REC_TYPE_ATTR, "%s=%d",
			    MAIL_ATTR_DSN_NOTIFY, dsn_notify);
	    if (rec_put(dst, REC_TYPE_RCPT, *cpp, strlen(*cpp)) < 0)
		msg_fatal_status(EX_TEMPFAIL,
				 "%s(%ld): error writing queue file: %m",
				 saved_sender, (long) uid);
	    ++rcpt_count;
	}
	argv_free(state.recipients);
	argv_free(state.resent_recip);
	vstring_free(state.temp);
    }
    if (rcpt_count == 0)
	msg_fatal_status(EX_USAGE, (flags & SM_FLAG_XRCPT) ?
		 "%s(%ld): No recipient addresses found in message header" :
			 "Recipient addresses must be specified on"
			 " the command line or via the -t option",
			 saved_sender, (long) uid);

    /*
     * Identify the end of the queue file.
     */
    rec_fputs(dst, REC_TYPE_END, "");

    /*
     * Make sure that the message makes it to the file system. Once we have
     * terminated with successful exit status we cannot lose the message due
     * to "frivolous reasons". If all goes well, prevent the run-time error
     * handler from removing the file.
     */
    if (vstream_ferror(VSTREAM_IN))
	msg_fatal_status(EX_DATAERR, "%s(%ld): error reading input: %m",
			 saved_sender, (long) uid);
    if ((status = mail_stream_finish(handle, (VSTRING *) 0)) != 0)
	msg_fatal_status((status & CLEANUP_STAT_BAD) ? EX_SOFTWARE :
			 (status & CLEANUP_STAT_WRITE) ? EX_TEMPFAIL :
			 EX_UNAVAILABLE, "%s(%ld): %s", saved_sender,
			 (long) uid, cleanup_strerror(status));

    /*
     * Don't leave them in the dark.
     */
    if (DEL_REQ_TRACE_FLAGS(flags)) {
	vstream_printf("Mail Delivery Status Report will be mailed to <%s>.\n",
		       saved_sender);
	vstream_fflush(VSTREAM_OUT);
    }

    /*
     * Cleanup. Not really necessary as we're about to exit, but good for
     * debugging purposes.
     */
    vstring_free(buf);
    myfree(saved_sender);
}

/* tempfail - sanitize exit status after library run-time error */

static void tempfail(void)
{
    exit(EX_TEMPFAIL);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static char *full_name = 0;		/* sendmail -F */
    struct stat st;
    char   *slash;
    char   *sender = 0;			/* sendmail -f */
    int     c;
    int     fd;
    int     mode;
    ARGV   *ext_argv;
    int     debug_me = 0;
    int     err;
    int     n;
    int     flags = SM_FLAG_DEFAULT;
    char   *site_to_flush = 0;
    char   *id_to_flush = 0;
    char   *encoding = 0;
    char   *qtime = 0;
    const char *errstr;
    uid_t   uid;
    const char *rewrite_context = MAIL_ATTR_RWR_LOCAL;
    int     dsn_notify = 0;
    const char *dsn_envid = 0;
    int     saved_optind;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Be consistent with file permissions.
     */
    umask(022);

    /*
     * To minimize confusion, make sure that the standard file descriptors
     * are open before opening anything else. XXX Work around for 44BSD where
     * fstat can return EBADF on an open file descriptor.
     */
    for (fd = 0; fd < 3; fd++)
	if (fstat(fd, &st) == -1
	    && (close(fd), open("/dev/null", O_RDWR, 0)) != fd)
	    msg_fatal_status(EX_OSERR, "open /dev/null: %m");

    /*
     * The CDE desktop calendar manager leaks a parent file descriptor into
     * the child process. For the sake of sendmail compatibility we have to
     * close the file descriptor otherwise mail notification will hang.
     */
    for ( /* void */ ; fd < 100; fd++)
	(void) close(fd);

    /*
     * Process environment options as early as we can. We might be called
     * from a set-uid (set-gid) program, so be careful with importing
     * environment variables.
     */
    if (safe_getenv(CONF_ENV_VERB))
	msg_verbose = 1;
    if (safe_getenv(CONF_ENV_DEBUG))
	debug_me = 1;

    /*
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information. Set up signal handlers so that we
     * can clean up incomplete output.
     */
    if ((slash = strrchr(argv[0], '/')) != 0 && slash[1])
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_cleanup(tempfail);
    msg_syslog_init(mail_task("sendmail"), LOG_PID, LOG_FACILITY);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    /*
     * Some sites mistakenly install Postfix sendmail as set-uid root. Drop
     * set-uid privileges only when root, otherwise some systems will not
     * reset the saved set-userid, which would be a security vulnerability.
     */
    if (geteuid() == 0 && getuid() != 0) {
	msg_warn("the Postfix sendmail command has set-uid root file permissions");
	msg_warn("or the command is run from a set-uid root process");
	msg_warn("the Postfix sendmail command must be installed without set-uid root file permissions");
	set_ugid(getuid(), getgid());
    }

    /*
     * Further initialization. Load main.cf first, so that command-line
     * options can override main.cf settings. Pre-scan the argument list so
     * that we load the right main.cf file.
     */
#define GETOPT_LIST "A:B:C:F:GIL:N:O:R:UV:X:b:ce:f:h:imno:p:r:q:tvx"

    saved_optind = optind;
    while (argv[OPTIND] != 0) {
	if (strcmp(argv[OPTIND], "-q") == 0) {	/* not getopt compatible */
	    optind++;
	    continue;
	}
	if ((c = GETOPT(argc, argv, GETOPT_LIST)) <= 0)
	    break;
	if (c == 'C') {
	    VSTRING *buf = vstring_alloc(1);

	    if (setenv(CONF_ENV_PATH,
		   strcmp(sane_basename(buf, optarg), MAIN_CONF_FILE) == 0 ?
		       sane_dirname(buf, optarg) : optarg, 1) < 0)
		msg_fatal_status(EX_UNAVAILABLE, "out of memory");
	    vstring_free(buf);
	}
    }
    optind = saved_optind;
    mail_conf_read();
    if (strcmp(var_syslog_name, DEF_SYSLOG_NAME) != 0)
	msg_syslog_init(mail_task("sendmail"), LOG_PID, LOG_FACILITY);
    get_mail_conf_str_table(str_table);

    if (chdir(var_queue_dir))
	msg_fatal_status(EX_UNAVAILABLE, "chdir %s: %m", var_queue_dir);

    signal(SIGPIPE, SIG_IGN);

    /*
     * Optionally start the debugger on ourself. This must be done after
     * reading the global configuration file, because that file specifies
     * what debugger command to execute.
     */
    if (debug_me)
	debug_process();

    /*
     * The default mode of operation is determined by the process name. It
     * can, however, be changed via command-line options (for example,
     * "newaliases -bp" will show the mail queue).
     */
    if (strcmp(argv[0], "mailq") == 0) {
	mode = SM_MODE_MAILQ;
    } else if (strcmp(argv[0], "newaliases") == 0) {
	mode = SM_MODE_NEWALIAS;
    } else if (strcmp(argv[0], "smtpd") == 0) {
	mode = SM_MODE_DAEMON;
    } else {
	mode = SM_MODE_ENQUEUE;
    }

    /*
     * Parse JCL. Sendmail has been around for a long time, and has acquired
     * a large number of options in the course of time. Some options such as
     * -q are not parsable with GETOPT() and get special treatment.
     */
#define OPTIND  (optind > 0 ? optind : 1)

    while (argv[OPTIND] != 0) {
	if (strcmp(argv[OPTIND], "-q") == 0) {
	    if (mode == SM_MODE_DAEMON)
		msg_warn("ignoring -q option in daemon mode");
	    else
		mode = SM_MODE_FLUSHQ;
	    optind++;
	    continue;
	}
	if (strcmp(argv[OPTIND], "-V") == 0
	    && argv[OPTIND + 1] != 0 && strlen(argv[OPTIND + 1]) == 2) {
	    msg_warn("option -V is deprecated with Postfix 2.3; "
		     "specify -XV instead");
	    argv[OPTIND] = "-XV";
	}
	if (strncmp(argv[OPTIND], "-V", 2) == 0 && strlen(argv[OPTIND]) == 4) {
	    msg_warn("option %s is deprecated with Postfix 2.3; "
		     "specify -X%s instead",
		     argv[OPTIND], argv[OPTIND] + 1);
	    argv[OPTIND] = concatenate("-X", argv[OPTIND] + 1, (char *) 0);
	}
	if (strcmp(argv[OPTIND], "-XV") == 0) {
	    verp_delims = var_verp_delims;
	    optind++;
	    continue;
	}
	if ((c = GETOPT(argc, argv, GETOPT_LIST)) <= 0)
	    break;
	switch (c) {
	default:
	    if (msg_verbose)
		msg_info("-%c option ignored", c);
	    break;
	case 'n':
	    msg_fatal_status(EX_USAGE, "-%c option not supported", c);
	case 'B':
	    if (strcmp(optarg, "8BITMIME") == 0)/* RFC 1652 */
		encoding = MAIL_ATTR_ENC_8BIT;
	    else if (strcmp(optarg, "7BIT") == 0)	/* RFC 1652 */
		encoding = MAIL_ATTR_ENC_7BIT;
	    else
		msg_fatal_status(EX_USAGE, "-B option needs 8BITMIME or 7BIT");
	    break;
	case 'F':				/* full name */
	    full_name = optarg;
	    break;
	case 'G':				/* gateway submission */
	    rewrite_context = MAIL_ATTR_RWR_REMOTE;
	    break;
	case 'I':				/* newaliases */
	    mode = SM_MODE_NEWALIAS;
	    break;
	case 'N':
	    if ((dsn_notify = dsn_notify_mask(optarg)) == 0)
		msg_warn("bad -N option value -- ignored");
	    break;
	case 'V':				/* DSN, was: VERP */
	    if (strlen(optarg) > 100)
		msg_warn("too long -V option value -- ignored");
	    else if (!allprint(optarg))
		msg_warn("bad syntax in -V option value -- ignored");
	    else
		dsn_envid = optarg;
	    break;
	case 'X':
	    switch (*optarg) {
	    default:
		msg_fatal_status(EX_USAGE, "unsupported: -%c%c", c, *optarg);
	    case 'V':				/* VERP */
		if (verp_delims_verify(optarg + 1) != 0)
		    msg_fatal_status(EX_USAGE, "-V requires two characters from %s",
				     var_verp_filter);
		verp_delims = optarg + 1;
		break;
	    }
	    break;
	case 'b':
	    switch (*optarg) {
	    default:
		msg_fatal_status(EX_USAGE, "unsupported: -%c%c", c, *optarg);
	    case 'd':				/* daemon mode */
		if (mode == SM_MODE_FLUSHQ)
		    msg_warn("ignoring -q option in daemon mode");
		mode = SM_MODE_DAEMON;
		break;
	    case 'h':				/* print host status */
	    case 'H':				/* flush host status */
		mode = SM_MODE_IGNORE;
		break;
	    case 'i':				/* newaliases */
		mode = SM_MODE_NEWALIAS;
		break;
	    case 'm':				/* deliver mail */
		mode = SM_MODE_ENQUEUE;
		break;
	    case 'p':				/* mailq */
		mode = SM_MODE_MAILQ;
		break;
	    case 's':				/* stand-alone mode */
		mode = SM_MODE_USER;
		break;
	    case 'v':				/* expand recipients */
		flags |= DEL_REQ_FLAG_USR_VRFY;
		break;
	    }
	    break;
	case 'f':
	    sender = optarg;
	    break;
	case 'i':
	    flags &= ~SM_FLAG_AEOF;
	    break;
	case 'o':
	    switch (*optarg) {
	    default:
		if (msg_verbose)
		    msg_info("-%c%c option ignored", c, *optarg);
		break;
	    case 'A':
		if (optarg[1] == 0)
		    msg_fatal_status(EX_USAGE, "-oA requires pathname");
		myfree(var_alias_db_map);
		var_alias_db_map = mystrdup(optarg + 1);
		set_mail_conf_str(VAR_ALIAS_DB_MAP, var_alias_db_map);
		break;
	    case '7':
	    case '8':
		break;
	    case 'i':
		flags &= ~SM_FLAG_AEOF;
		break;
	    case 'm':
		break;
	    }
	    break;
	case 'r':				/* obsoleted by -f */
	    sender = optarg;
	    break;
	case 'q':
	    if (ISDIGIT(optarg[0])) {
		qtime = optarg;
	    } else if (optarg[0] == 'R') {
		site_to_flush = optarg + 1;
		if (*site_to_flush == 0)
		    msg_fatal_status(EX_USAGE, "specify: -qRsitename");
	    } else if (optarg[0] == 'I') {
		id_to_flush = optarg + 1;
		if (*id_to_flush == 0)
		    msg_fatal_status(EX_USAGE, "specify: -qIqueueid");
	    } else {
		msg_fatal_status(EX_USAGE, "-q%c is not implemented",
				 optarg[0]);
	    }
	    break;
	case 't':
	    flags |= SM_FLAG_XRCPT;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	case '?':
	    msg_fatal_status(EX_USAGE, "usage: %s [options]", argv[0]);
	}
    }

    /*
     * Look for conflicting options and arguments.
     */
    if ((flags & SM_FLAG_XRCPT) && mode != SM_MODE_ENQUEUE)
	msg_fatal_status(EX_USAGE, "-t can be used only in delivery mode");

    if (site_to_flush && mode != SM_MODE_ENQUEUE)
	msg_fatal_status(EX_USAGE, "-qR can be used only in delivery mode");

    if (id_to_flush && mode != SM_MODE_ENQUEUE)
	msg_fatal_status(EX_USAGE, "-qI can be used only in delivery mode");

    if (flags & DEL_REQ_FLAG_USR_VRFY) {
	if (flags & SM_FLAG_XRCPT)
	    msg_fatal_status(EX_USAGE, "-t option cannot be used with -bv");
	if (dsn_notify)
	    msg_fatal_status(EX_USAGE, "-N option cannot be used with -bv");
	if (msg_verbose == 1)
	    msg_fatal_status(EX_USAGE, "-v option cannot be used with -bv");
    }

    /*
     * The -v option plays double duty. One requests verbose delivery, more
     * than one requests verbose logging.
     */
    if (msg_verbose == 1 && mode == SM_MODE_ENQUEUE) {
	msg_verbose = 0;
	flags |= DEL_REQ_FLAG_RECORD;
    }

    /*
     * Start processing. Everything is delegated to external commands.
     */
    if (qtime && mode != SM_MODE_DAEMON)
	exit(0);
    switch (mode) {
    default:
	msg_panic("unknown operation mode: %d", mode);
	/* NOTREACHED */
    case SM_MODE_ENQUEUE:
	if (site_to_flush) {
	    if (argv[OPTIND])
		msg_fatal_status(EX_USAGE, "flush site requires no recipient");
	    ext_argv = argv_alloc(2);
	    argv_add(ext_argv, "postqueue", "-s", site_to_flush, (char *) 0);
	    for (n = 0; n < msg_verbose; n++)
		argv_add(ext_argv, "-v", (char *) 0);
	    argv_terminate(ext_argv);
	    mail_run_replace(var_command_dir, ext_argv->argv);
	    /* NOTREACHED */
	} else if (id_to_flush) {
	    if (argv[OPTIND])
		msg_fatal_status(EX_USAGE, "flush queue_id requires no recipient");
	    ext_argv = argv_alloc(2);
	    argv_add(ext_argv, "postqueue", "-i", id_to_flush, (char *) 0);
	    for (n = 0; n < msg_verbose; n++)
		argv_add(ext_argv, "-v", (char *) 0);
	    argv_terminate(ext_argv);
	    mail_run_replace(var_command_dir, ext_argv->argv);
	    /* NOTREACHED */
	} else {
	    enqueue(flags, encoding, dsn_envid, dsn_notify,
		    rewrite_context, sender, full_name, argv + OPTIND);
	    exit(0);
	    /* NOTREACHED */
	}
	break;
    case SM_MODE_MAILQ:
	if (argv[OPTIND])
	    msg_fatal_status(EX_USAGE,
			     "display queue mode requires no recipient");
	ext_argv = argv_alloc(2);
	argv_add(ext_argv, "postqueue", "-p", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(ext_argv, "-v", (char *) 0);
	argv_terminate(ext_argv);
	mail_run_replace(var_command_dir, ext_argv->argv);
	/* NOTREACHED */
    case SM_MODE_FLUSHQ:
	if (argv[OPTIND])
	    msg_fatal_status(EX_USAGE,
			     "flush queue mode requires no recipient");
	ext_argv = argv_alloc(2);
	argv_add(ext_argv, "postqueue", "-f", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(ext_argv, "-v", (char *) 0);
	argv_terminate(ext_argv);
	mail_run_replace(var_command_dir, ext_argv->argv);
	/* NOTREACHED */
    case SM_MODE_DAEMON:
	if (argv[OPTIND])
	    msg_fatal_status(EX_USAGE, "daemon mode requires no recipient");
	ext_argv = argv_alloc(2);
	argv_add(ext_argv, "postfix", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(ext_argv, "-v", (char *) 0);
	argv_add(ext_argv, "start", (char *) 0);
	argv_terminate(ext_argv);
	err = (mail_run_background(var_command_dir, ext_argv->argv) < 0);
	argv_free(ext_argv);
	exit(err);
	break;
    case SM_MODE_NEWALIAS:
	if (argv[OPTIND])
	    msg_fatal_status(EX_USAGE,
			 "alias initialization mode requires no recipient");
	if (*var_alias_db_map == 0)
	    return (0);
	ext_argv = argv_alloc(2);
	argv_add(ext_argv, "postalias", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(ext_argv, "-v", (char *) 0);
	argv_split_append(ext_argv, var_alias_db_map, ", \t\r\n");
	argv_terminate(ext_argv);
	mail_run_replace(var_command_dir, ext_argv->argv);
	/* NOTREACHED */
    case SM_MODE_USER:
	if (argv[OPTIND])
	    msg_fatal_status(EX_USAGE,
			     "stand-alone mode requires no recipient");
	/* The actual enforcement happens in the postdrop command. */
	if ((errstr = check_user_acl_byuid(var_submit_acl, uid = getuid())) != 0)
	    msg_fatal_status(EX_NOPERM,
			     "User %s(%ld) is not allowed to submit mail",
			     errstr, (long) uid);
	ext_argv = argv_alloc(2);
	argv_add(ext_argv, "smtpd", "-S", (char *) 0);
	for (n = 0; n < msg_verbose; n++)
	    argv_add(ext_argv, "-v", (char *) 0);
	argv_terminate(ext_argv);
	mail_run_replace(var_daemon_dir, ext_argv->argv);
	/* NOTREACHED */
    case SM_MODE_IGNORE:
	exit(0);
	/* NOTREACHED */
    }
}
