/*++
/* NAME
/*	pipe 8
/* SUMMARY
/*	Postfix delivery to external command
/* SYNOPSIS
/*	\fBpipe\fR [generic Postfix daemon options] command_attributes...
/* DESCRIPTION
/*	The \fBpipe\fR daemon processes requests from the Postfix queue
/*	manager to deliver messages to external commands.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	Message attributes such as sender address, recipient address and
/*	next-hop host name can be specified as command-line macros that are
/*	expanded before the external command is executed.
/*
/*	The \fBpipe\fR daemon updates queue files and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery problem reports are sent
/*	to the \fBbounce\fR(8) or \fBdefer\fR(8) daemon as appropriate.
/* SINGLE-RECIPIENT DELIVERY
/* .ad
/* .fi
/*	Some external commands cannot handle more than one recipient
/*	per delivery request. Examples of such transports are pagers,
/*	fax machines, and so on.
/*
/*	To prevent Postfix from sending multiple recipients per delivery
/*	request, specify
/*
/* .ti +4
/*	\fItransport\fB_destination_recipient_limit = 1\fR
/*
/*	in the Postfix \fBmain.cf\fR file, where \fItransport\fR
/*	is the name in the first column of the Postfix \fBmaster.cf\fR
/*	entry for the pipe-based delivery transport.
/* COMMAND ATTRIBUTE SYNTAX
/* .ad
/* .fi
/*	The external command attributes are given in the \fBmaster.cf\fR
/*	file at the end of a service definition.  The syntax is as follows:
/* .IP "\fBflags=BFRhqu.>\fR (optional)"
/*	Optional message processing flags. By default, a message is
/*	copied unchanged.
/* .RS
/* .IP \fBB\fR
/*	Append a blank line at the end of each message. This is required
/*	by some mail user agents that recognize "\fBFrom \fR" lines only
/*	when preceded by a blank line.
/* .IP \fBF\fR
/*	Prepend a "\fBFrom \fIsender time_stamp\fR" envelope header to
/*	the message content.
/*	This is expected by, for example, \fBUUCP\fR software.
/* .IP \fBR\fR
/*	Prepend a \fBReturn-Path:\fR message header with the envelope sender
/*	address.
/* .IP \fBh\fR
/*	Fold the command-line \fB$recipient\fR domain name and \fB$nexthop\fR
/*	host name to lower case.
/*	This is recommended for delivery via \fBUUCP\fR.
/* .IP \fBq\fR
/*	Quote white space and other special characters in the command-line
/*	\fB$sender\fR and \fB$recipient\fR address localparts (text to the
/*	left of the right-most \fB@\fR character), according to an 8-bit
/*	transparent version of RFC 822.
/*	This is recommended for delivery via \fBUUCP\fR or \fBBSMTP\fR.
/* .sp
/*	The result is compatible with the address parsing of command-line
/*	recipients by the Postfix \fBsendmail\fR mail submission command.
/* .sp
/*	The \fBq\fR flag affects only entire addresses, not the partial
/*	address information from the \fB$user\fR, \fB$extension\fR or
/*	\fB$mailbox\fR command-line macros.
/* .IP \fBu\fR
/*	Fold the command-line \fB$recipient\fR address localpart (text to
/*	the left of the right-most \fB@\fR character) to lower case.
/*	This is recommended for delivery via \fBUUCP\fR.
/* .IP \fB.\fR
/*	Prepend \fB.\fR to lines starting with "\fB.\fR". This is needed
/*	by, for example, \fBBSMTP\fR software.
/* .IP \fB>\fR
/*	Prepend \fB>\fR to lines starting with "\fBFrom \fR". This is expected
/*	by, for example, \fBUUCP\fR software.
/* .RE
/* .IP "\fBuser\fR=\fIusername\fR (required)"
/* .IP "\fBuser\fR=\fIusername\fR:\fIgroupname\fR"
/*	The external command is executed with the rights of the
/*	specified \fIusername\fR.  The software refuses to execute
/*	commands with root privileges, or with the privileges of the
/*	mail system owner. If \fIgroupname\fR is specified, the
/*	corresponding group ID is used instead of the group ID of
/*	\fIusername\fR.
/* .IP "\fBeol=string\fR (default: \fB\en\fR)"
/*	The output record delimiter. Typically one would use either
/*	\fB\er\en\fR or \fB\en\fR. The usual C-style backslash escape
/*	sequences are recognized: \fB\ea \eb \ef \en \er \et \ev
/*	\e\fIoctal\fR and \fB\e\e\fR.
/* .IP "\fBsize\fR=\fIsize_limit\fR (optional)"
/*	Messages greater in size than this limit (in bytes) will be bounced
/*	back to the sender.
/* .IP "\fBargv\fR=\fIcommand\fR... (required)"
/*	The command to be executed. This must be specified as the
/*	last command attribute.
/*	The command is executed directly, i.e. without interpretation of
/*	shell meta characters by a shell command interpreter.
/* .sp
/*	In the command argument vector, the following macros are recognized
/*	and replaced with corresponding information from the Postfix queue
/*	manager delivery request:
/* .RS
/* .IP \fB${\fBextension\fR}\fR
/*	This macro expands to the extension part of a recipient address.
/*	For example, with an address \fIuser+foo@domain\fR the extension is
/*	\fIfoo\fR.
/* .sp
/*	A command-line argument that contains \fB${\fBextension\fR}\fR expands
/*	into as many command-line arguments as there are recipients.
/* .sp
/*	This information is modified by the \fBu\fR flag for case folding.
/* .IP \fB${\fBmailbox\fR}\fR
/*	This macro expands to the complete local part of a recipient address.
/*	For example, with an address \fIuser+foo@domain\fR the mailbox is
/*	\fIuser+foo\fR.
/* .sp
/*	A command-line argument that contains \fB${\fBmailbox\fR}\fR
/*	expands into as many command-line arguments as there are recipients.
/* .sp
/*	This information is modified by the \fBu\fR flag for case folding.
/* .IP \fB${\fBnexthop\fR}\fR
/*	This macro expands to the next-hop hostname.
/* .sp
/*	This information is modified by the \fBh\fR flag for case folding.
/* .IP \fB${\fBrecipient\fR}\fR
/*	This macro expands to the complete recipient address.
/* .sp
/*	A command-line argument that contains \fB${\fBrecipient\fR}\fR
/*	expands into as many command-line arguments as there are recipients.
/* .sp
/*	This information is modified by the \fBhqu\fR flags for quoting
/*	and case folding.
/* .IP \fB${\fBsender\fR}\fR
/*	This macro expands to the envelope sender address.
/* .sp
/*	This information is modified by the \fBq\fR flag for quoting.
/* .IP \fB${\fBsize\fR}\fR
/*	This macro expands to Postfix's idea of the message size, which
/*	is an approximation of the size of the message as delivered.
/* .IP \fB${\fBuser\fR}\fR
/*	This macro expands to the username part of a recipient address.
/*	For example, with an address \fIuser+foo@domain\fR the username
/*	part is \fIuser\fR.
/* .sp
/*	A command-line argument that contains \fB${\fBuser\fR}\fR expands
/*	into as many command-line arguments as there are recipients.
/* .sp
/*	This information is modified by the \fBu\fR flag for case folding.
/* .RE
/* .PP
/*	In addition to the form ${\fIname\fR}, the forms $\fIname\fR and
/*	$(\fIname\fR) are also recognized.  Specify \fB$$\fR where a single
/*	\fB$\fR is wanted.
/* DIAGNOSTICS
/*	Command exit status codes are expected to
/*	follow the conventions defined in <\fBsysexits.h\fR>.
/*
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*	Corrupted message files are marked so that the queue manager
/*	can move them to the \fBcorrupt\fR queue for further inspection.
/* SECURITY
/* .fi
/* .ad
/*	This program needs a dual personality 1) to access the private
/*	Postfix queue and IPC mechanisms, and 2) to execute external
/*	commands as the specified user. It is therefore security sensitive.
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
/* .IP \fBexport_environment\fR
/*	List of names of environment parameters that can be exported
/*	to non-Postfix processes.
/* .IP \fBmail_owner\fR
/*	The process privileges used while not running an external command.
/* .SH "Resource controls"
/* .ad
/* .fi
/*	In the text below, \fItransport\fR is the first field in a
/*	\fBmaster.cf\fR entry.
/* .IP \fItransport\fB_destination_concurrency_limit\fR
/*	Limit the number of parallel deliveries to the same destination,
/*	for delivery via the named \fItransport\fR. The default limit is
/*	taken from the \fBdefault_destination_concurrency_limit\fR parameter.
/*	The limit is enforced by the Postfix queue manager.
/* .IP \fItransport\fB_destination_recipient_limit\fR
/*	Limit the number of recipients per message delivery, for delivery
/*	via the named \fItransport\fR. The default limit is taken from
/*	the \fBdefault_destination_recipient_limit\fR parameter.
/*	The limit is enforced by the Postfix queue manager.
/* .IP \fItransport\fB_time_limit\fR
/*	Limit the time for delivery to external command, for delivery via
/*	the named \fBtransport\fR. The default limit is taken from the
/*	\fBcommand_time_limit\fR parameter.
/*	The limit is enforced by the Postfix queue manager.
/* SEE ALSO
/*	bounce(8) non-delivery status reports
/*	master(8) process manager
/*	qmgr(8) queue manager
/*	syslogd(8) system logging
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
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <argv.h>
#include <htable.h>
#include <dict.h>
#include <iostuff.h>
#include <mymalloc.h>
#include <mac_parse.h>
#include <set_eugid.h>
#include <split_at.h>
#include <stringops.h>

/* Global library. */

#include <recipient_list.h>
#include <deliver_request.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <bounce.h>
#include <defer.h>
#include <deliver_completed.h>
#include <sent.h>
#include <pipe_command.h>
#include <mail_copy.h>
#include <mail_addr.h>
#include <canon_addr.h>
#include <split_addr.h>
#include <off_cvt.h>
#include <quote_822_local.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

 /*
  * The mini symbol table name and keys used for expanding macros in
  * command-line arguments.
  */
#define PIPE_DICT_TABLE		"pipe_command"	/* table name */
#define PIPE_DICT_NEXTHOP	"nexthop"	/* key */
#define PIPE_DICT_RCPT		"recipient"	/* key */
#define PIPE_DICT_SENDER	"sender"/* key */
#define PIPE_DICT_USER		"user"	/* key */
#define PIPE_DICT_EXTENSION	"extension"	/* key */
#define PIPE_DICT_MAILBOX	"mailbox"	/* key */
#define PIPE_DICT_SIZE		"size"	/* key */

 /*
  * Flags used to pass back the type of special parameter found by
  * parse_callback.
  */
#define PIPE_FLAG_RCPT		(1<<0)
#define PIPE_FLAG_USER		(1<<1)
#define PIPE_FLAG_EXTENSION	(1<<2)
#define PIPE_FLAG_MAILBOX	(1<<3)
#define PIPE_FLAG_SIZE		(1<<4)

 /*
  * Additional flags. These are colocated with mail_copy() flags. Allow some
  * space for extension of the mail_copy() interface.
  */
#define PIPE_OPT_FOLD_USER	(1<<16)
#define PIPE_OPT_FOLD_HOST	(1<<17)
#define PIPE_OPT_QUOTE_LOCAL	(1<<18)

#define PIPE_OPT_FOLD_FLAGS	(PIPE_OPT_FOLD_USER | PIPE_OPT_FOLD_HOST)

 /*
  * Tunable parameters. Values are taken from the config file, after
  * prepending the service name to _name, and so on.
  */
int     var_command_maxtime;		/* system-wide */

 /*
  * For convenience. Instead of passing around lists of parameters, bundle
  * them up in convenient structures.
  */

 /*
  * Structure for service-specific configuration parameters.
  */
typedef struct {
    int     time_limit;			/* per-service time limit */
} PIPE_PARAMS;

 /*
  * Structure for command-line parameters.
  */
typedef struct {
    char  **command;			/* argument vector */
    uid_t   uid;			/* command privileges */
    gid_t   gid;			/* command privileges */
    int     flags;			/* mail_copy() flags */
    VSTRING *eol;			/* output record delimiter */
    off_t   size_limit;			/* max size in bytes we will accept */
} PIPE_ATTR;

 /*
  * Silly little macros.
  */
#define STR	vstring_str

/* parse_callback - callback for mac_parse() */

static int parse_callback(int type, VSTRING *buf, char *context)
{
    int    *expand_flag = (int *) context;

    /*
     * See if this command-line argument references a special macro.
     */
    if (type == MAC_PARSE_VARNAME) {
	if (strcmp(vstring_str(buf), PIPE_DICT_RCPT) == 0)
	    *expand_flag |= PIPE_FLAG_RCPT;
	else if (strcmp(vstring_str(buf), PIPE_DICT_USER) == 0)
	    *expand_flag |= PIPE_FLAG_USER;
	else if (strcmp(vstring_str(buf), PIPE_DICT_EXTENSION) == 0)
	    *expand_flag |= PIPE_FLAG_EXTENSION;
	else if (strcmp(vstring_str(buf), PIPE_DICT_MAILBOX) == 0)
	    *expand_flag |= PIPE_FLAG_MAILBOX;
    }
    return (0);
}

/* morph_recipient - morph a recipient address */

static void morph_recipient(VSTRING *buf, const char *address, int flags)
{
    char   *cp;

    /*
     * Quote the recipient address as appropriate.
     */
    if (flags & PIPE_OPT_QUOTE_LOCAL)
	quote_822_local(buf, address);
    else
	vstring_strcpy(buf, address);

    /*
     * Fold the recipient address as appropriate.
     */
    switch (flags & PIPE_OPT_FOLD_FLAGS) {
    case PIPE_OPT_FOLD_HOST:
	if ((cp = strrchr(STR(buf), '@')) != 0)
	    lowercase(cp + 1);
	break;
    case PIPE_OPT_FOLD_USER:
	if ((cp = strrchr(STR(buf), '@')) != 0) {
	    *cp = 0;
	    lowercase(STR(buf));
	    *cp = '@';
	    break;
	}
    case PIPE_OPT_FOLD_USER | PIPE_OPT_FOLD_HOST:
	lowercase(STR(buf));
	break;
    }
}

/* expand_argv - expand macros in the argument vector */

static ARGV *expand_argv(char **argv, RECIPIENT_LIST *rcpt_list, int flags)
{
    VSTRING *buf = vstring_alloc(100);
    ARGV   *result;
    char  **cpp;
    int     expand_flag;
    int     i;
    char   *ext;

    /*
     * This appears to be simple operation (replace $name by its expansion).
     * However, it becomes complex because a command-line argument that
     * references $recipient must expand to as many command-line arguments as
     * there are recipients (that's wat programs called by sendmail expect).
     * So we parse each command-line argument, and depending on what we find,
     * we either expand the argument just once, or we expand it once for each
     * recipient. In either case we end up parsing the command-line argument
     * twice. The amount of CPU time wasted will be negligible.
     * 
     * Note: we can't use recursive macro expansion here, because recursion
     * would screw up mail addresses that contain $ characters.
     */
#define NO	0

    result = argv_alloc(1);
    for (cpp = argv; *cpp; cpp++) {
	expand_flag = 0;
	mac_parse(*cpp, parse_callback, (char *) &expand_flag);
	if (expand_flag == 0) {			/* no $recipient etc. */
	    argv_add(result, dict_eval(PIPE_DICT_TABLE, *cpp, NO), ARGV_END);
	} else {				/* contains $recipient etc. */
	    for (i = 0; i < rcpt_list->len; i++) {

		/*
		 * This argument contains $recipient.
		 */
		if (expand_flag & PIPE_FLAG_RCPT) {
		    morph_recipient(buf, rcpt_list->info[i].address, flags);
		    dict_update(PIPE_DICT_TABLE, PIPE_DICT_RCPT, STR(buf));
		}

		/*
		 * This argument contains $user. Extract the plain user name.
		 * Either anything to the left of the extension delimiter or,
		 * in absence of the latter, anything to the left of the
		 * rightmost @.
		 * 
		 * Beware: if the user name is blank (e.g. +user@host), the
		 * argument is suppressed. This is necessary to allow for
		 * cyrus bulletin-board (global mailbox) delivery. XXX But,
		 * skipping empty user parts will also prevent other
		 * expansions of this specific command-line argument.
		 */
		if (expand_flag & PIPE_FLAG_USER) {
		    morph_recipient(buf, rcpt_list->info[i].address,
				    flags & PIPE_OPT_FOLD_FLAGS);
		    if (split_at_right(STR(buf), '@') == 0)
			msg_warn("no @ in recipient address: %s",
				 rcpt_list->info[i].address);
		    if (*var_rcpt_delim)
			split_addr(STR(buf), *var_rcpt_delim);
		    if (*STR(buf) == 0)
			continue;
		    dict_update(PIPE_DICT_TABLE, PIPE_DICT_USER, STR(buf));
		}

		/*
		 * This argument contains $extension. Extract the recipient
		 * extension: anything between the leftmost extension
		 * delimiter and the rightmost @. The extension may be blank.
		 */
		if (expand_flag & PIPE_FLAG_EXTENSION) {
		    morph_recipient(buf, rcpt_list->info[i].address,
				    flags & PIPE_OPT_FOLD_FLAGS);
		    if (split_at_right(STR(buf), '@') == 0)
			msg_warn("no @ in recipient address: %s",
				 rcpt_list->info[i].address);
		    if (*var_rcpt_delim == 0
		      || (ext = split_addr(STR(buf), *var_rcpt_delim)) == 0)
			ext = "";		/* insert null arg */
		    dict_update(PIPE_DICT_TABLE, PIPE_DICT_EXTENSION, ext);
		}

		/*
		 * This argument contains $mailbox. Extract the mailbox name:
		 * anything to the left of the rightmost @.
		 */
		if (expand_flag & PIPE_FLAG_MAILBOX) {
		    morph_recipient(buf, rcpt_list->info[i].address,
				    flags & PIPE_OPT_FOLD_FLAGS);
		    if (split_at_right(STR(buf), '@') == 0)
			msg_warn("no @ in recipient address: %s",
				 rcpt_list->info[i].address);
		    dict_update(PIPE_DICT_TABLE, PIPE_DICT_MAILBOX, STR(buf));
		}

		/*
		 * Done.
		 */
		argv_add(result, dict_eval(PIPE_DICT_TABLE, *cpp, NO), ARGV_END);
	    }
	}
    }
    argv_terminate(result);
    vstring_free(buf);
    return (result);
}

/* get_service_params - get service-name dependent config information */

static void get_service_params(PIPE_PARAMS *config, char *service)
{
    char   *myname = "get_service_params";

    /*
     * Figure out the command time limit for this transport.
     */
    config->time_limit =
	get_mail_conf_int2(service, "_time_limit", var_command_maxtime, 1, 0);

    /*
     * Give the poor tester a clue of what is going on.
     */
    if (msg_verbose)
	msg_info("%s: time_limit %d", myname, config->time_limit);
}

/* get_service_attr - get command-line attributes */

static void get_service_attr(PIPE_ATTR *attr, char **argv)
{
    char   *myname = "get_service_attr";
    struct passwd *pwd;
    struct group *grp;
    char   *user;			/* user name */
    char   *group;			/* group name */
    char   *size;			/* max message size */
    char   *cp;

    /*
     * Initialize.
     */
    user = 0;
    group = 0;
    attr->command = 0;
    attr->flags = 0;
    attr->eol = vstring_strcpy(vstring_alloc(1), "\n");
    attr->size_limit = 0;

    /*
     * Iterate over the command-line attribute list.
     */
    for ( /* void */ ; *argv != 0; argv++) {

	/*
	 * flags=stuff
	 */
	if (strncasecmp("flags=", *argv, sizeof("flags=") - 1) == 0) {
	    for (cp = *argv + sizeof("flags=") - 1; *cp; cp++) {
		switch (*cp) {
		case 'B':
		    attr->flags |= MAIL_COPY_BLANK;
		    break;
		case 'F':
		    attr->flags |= MAIL_COPY_FROM;
		    break;
		case '.':
		    attr->flags |= MAIL_COPY_DOT;
		    break;
		case '>':
		    attr->flags |= MAIL_COPY_QUOTE;
		    break;
		case 'R':
		    attr->flags |= MAIL_COPY_RETURN_PATH;
		    break;
		case 'h':
		    attr->flags |= PIPE_OPT_FOLD_HOST;
		    break;
		case 'q':
		    attr->flags |= PIPE_OPT_QUOTE_LOCAL;
		    break;
		case 'u':
		    attr->flags |= PIPE_OPT_FOLD_USER;
		    break;
		default:
		    msg_fatal("unknown flag: %c (ignored)", *cp);
		    break;
		}
	    }
	}

	/*
	 * user=username[:groupname]
	 */
	else if (strncasecmp("user=", *argv, sizeof("user=") - 1) == 0) {
	    user = *argv + sizeof("user=") - 1;
	    if ((group = split_at(user, ':')) != 0)	/* XXX clobbers argv */
		if (*group == 0)
		    group = 0;
	    if ((pwd = getpwnam(user)) == 0)
		msg_fatal("%s: unknown username: %s", myname, user);
	    attr->uid = pwd->pw_uid;
	    if (group != 0) {
		if ((grp = getgrnam(group)) == 0)
		    msg_fatal("%s: unknown group: %s", myname, group);
		attr->gid = grp->gr_gid;
	    } else {
		attr->gid = pwd->pw_gid;
	    }
	}

	/*
	 * eol=string
	 */
	else if (strncasecmp("eol=", *argv, sizeof("eol=") - 1) == 0) {
	    unescape(attr->eol, *argv + sizeof("eol=") - 1);
	}

	/*
	 * size=max_message_size (in bytes)
	 */
	else if (strncasecmp("size=", *argv, sizeof("size=") - 1) == 0) {
	    size = *argv + sizeof("size=") - 1;
	    if ((attr->size_limit = off_cvt_string(size)) < 0)
		msg_fatal("%s: bad size= value: %s", myname, size);
	}

	/*
	 * argv=command...
	 */
	else if (strncasecmp("argv=", *argv, sizeof("argv=") - 1) == 0) {
	    *argv += sizeof("argv=") - 1;	/* XXX clobbers argv */
	    attr->command = argv;
	    break;
	}

	/*
	 * Bad.
	 */
	else
	    msg_fatal("unknown attribute name: %s", *argv);
    }

    /*
     * Sanity checks. Verify that every member has an acceptable value.
     */
    if (user == 0)
	msg_fatal("missing user= attribute");
    if (attr->command == 0)
	msg_fatal("missing argv= attribute");
    if (attr->uid == 0)
	msg_fatal("request to deliver as root");
    if (attr->uid == var_owner_uid)
	msg_fatal("request to deliver as mail system owner");
    if (attr->gid == 0)
	msg_fatal("request to use privileged group id %ld", (long) attr->gid);
    if (attr->gid == var_owner_gid)
	msg_fatal("request to use mail system owner group id %ld", (long) attr->gid);

    /*
     * Give the poor tester a clue of what is going on.
     */
    if (msg_verbose)
	msg_info("%s: uid %ld, gid %ld, flags %d, size %ld",
		 myname, (long) attr->uid, (long) attr->gid,
		 attr->flags, (long) attr->size_limit);
}

/* eval_command_status - do something with command completion status */

static int eval_command_status(int command_status, char *service,
			             DELIVER_REQUEST *request, VSTREAM *src,
			               char *why)
{
    RECIPIENT *rcpt;
    int     status;
    int     result = 0;
    int     n;

    /*
     * Depending on the result, bounce or defer the message, and mark the
     * recipient as done where appropriate.
     */
    switch (command_status) {
    case PIPE_STAT_OK:
	for (n = 0; n < request->rcpt_list.len; n++) {
	    rcpt = request->rcpt_list.info + n;
	    sent(request->queue_id, rcpt->address, service,
		 request->arrival_time, "%s", request->nexthop);
	    if (request->flags & DEL_REQ_FLAG_SUCCESS)
		deliver_completed(src, rcpt->offset);
	}
	break;
    case PIPE_STAT_BOUNCE:
	for (n = 0; n < request->rcpt_list.len; n++) {
	    rcpt = request->rcpt_list.info + n;
	    status = bounce_append(BOUNCE_FLAG_KEEP,
				   request->queue_id, rcpt->address,
				 service, request->arrival_time, "%s", why);
	    if (status == 0)
		deliver_completed(src, rcpt->offset);
	    result |= status;
	}
	break;
    case PIPE_STAT_DEFER:
	for (n = 0; n < request->rcpt_list.len; n++) {
	    rcpt = request->rcpt_list.info + n;
	    result |= defer_append(BOUNCE_FLAG_KEEP,
				   request->queue_id, rcpt->address,
				 service, request->arrival_time, "%s", why);
	}
	break;
    case PIPE_STAT_CORRUPT:
	result |= DEL_STAT_DEFER;
	break;
    default:
	msg_panic("eval_command_status: bad status %d", command_status);
	/* NOTREACHED */
    }
    return (result);
}

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(DELIVER_REQUEST *request, char *service, char **argv)
{
    char   *myname = "deliver_message";
    static PIPE_PARAMS conf;
    static PIPE_ATTR attr;
    RECIPIENT_LIST *rcpt_list = &request->rcpt_list;
    VSTRING *why = vstring_alloc(100);
    VSTRING *buf;
    ARGV   *expanded_argv = 0;
    int     deliver_status;
    int     command_status;
    ARGV   *export_env;

#define DELIVER_MSG_CLEANUP() { \
	vstring_free(why); \
	if (expanded_argv) argv_free(expanded_argv); \
    }

    if (msg_verbose)
	msg_info("%s: from <%s>", myname, request->sender);

    /*
     * First of all, replace an empty sender address by the mailer daemon
     * address. The resolver already fixes empty recipient addresses.
     * 
     * XXX Should sender and recipient be transformed into external (i.e.
     * quoted) form? Problem is that the quoting rules are transport
     * specific. Such information must evidently not be hard coded into
     * Postfix, but would have to be provided in the form of lookup tables.
     */
    if (request->sender[0] == 0) {
	buf = vstring_alloc(100);
	canon_addr_internal(buf, MAIL_ADDR_MAIL_DAEMON);
	myfree(request->sender);
	request->sender = vstring_export(buf);
    }

    /*
     * Sanity checks. The get_service_params() and get_service_attr()
     * routines also do some sanity checks. Look up service attributes and
     * config information only once. This is safe since the information comes
     * from a trusted source, not from the delivery request.
     */
    if (request->nexthop[0] == 0)
	msg_fatal("empty nexthop hostname");
    if (rcpt_list->len <= 0)
	msg_fatal("recipient count: %d", rcpt_list->len);
    if (attr.command == 0) {
	get_service_params(&conf, service);
	get_service_attr(&attr, argv);
    }

    /*
     * Check that this agent accepts messages this large.
     */
    if (attr.size_limit != 0 && request->data_size > attr.size_limit) {
	if (msg_verbose)
	    msg_info("%s: too big: size_limit = %ld, request->data_size = %ld",
		     myname, (long) attr.size_limit, request->data_size);

	deliver_status = eval_command_status(PIPE_STAT_BOUNCE, service,
				 request, request->fp, "message too large");
	DELIVER_MSG_CLEANUP();
	return (deliver_status);
    }

    /*
     * Deliver. Set the nexthop and sender variables, and expand the command
     * argument vector. Recipients will be expanded on the fly. XXX Rewrite
     * envelope and header addresses according to transport-specific
     * rewriting rules.
     */
    if (vstream_fseek(request->fp, request->data_offset, SEEK_SET) < 0)
	msg_fatal("seek queue file %s: %m", VSTREAM_PATH(request->fp));

    buf = vstring_alloc(10);
    if (attr.flags & PIPE_OPT_QUOTE_LOCAL) {
	quote_822_local(buf, request->sender);
	dict_update(PIPE_DICT_TABLE, PIPE_DICT_SENDER, STR(buf));
    } else
	dict_update(PIPE_DICT_TABLE, PIPE_DICT_SENDER, request->sender);
    if (attr.flags & PIPE_OPT_FOLD_HOST) {
	vstring_strcpy(buf, request->nexthop);
	lowercase(STR(buf));
	dict_update(PIPE_DICT_TABLE, PIPE_DICT_NEXTHOP, STR(buf));
    } else
	dict_update(PIPE_DICT_TABLE, PIPE_DICT_NEXTHOP, request->nexthop);
    vstring_sprintf(buf, "%ld", (long) request->data_size);
    dict_update(PIPE_DICT_TABLE, PIPE_DICT_SIZE, STR(buf));
    vstring_free(buf);
    expanded_argv = expand_argv(attr.command, rcpt_list, attr.flags);
    export_env = argv_split(var_export_environ, ", \t\r\n");

    command_status = pipe_command(request->fp, why,
				  PIPE_CMD_UID, attr.uid,
				  PIPE_CMD_GID, attr.gid,
				  PIPE_CMD_SENDER, request->sender,
				  PIPE_CMD_COPY_FLAGS, attr.flags,
				  PIPE_CMD_ARGV, expanded_argv->argv,
				  PIPE_CMD_TIME_LIMIT, conf.time_limit,
				  PIPE_CMD_EOL, STR(attr.eol),
				  PIPE_CMD_EXPORT, export_env->argv,
				  PIPE_CMD_END);
    argv_free(export_env);

    deliver_status = eval_command_status(command_status, service, request,
					 request->fp, vstring_str(why));

    /*
     * Clean up.
     */
    DELIVER_MSG_CLEANUP();

    return (deliver_status);
}

/* pipe_service - perform service for client */

static void pipe_service(VSTREAM *client_stream, char *service, char **argv)
{
    DELIVER_REQUEST *request;
    int     status;

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to delivery via external command. What we see below is a
     * little protocol to (1) tell the queue manager that we are ready, (2)
     * read a request from the queue manager, and (3) report the completion
     * status of that request. All connection-management stuff is handled by
     * the common code in single_server.c.
     */
    if ((request = deliver_request_read(client_stream)) != 0) {
	status = deliver_message(request, service, argv);
	deliver_request_done(client_stream, request, status);
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

/* drop_privileges - drop privileges most of the time */

static void drop_privileges(char *unused_name, char **unused_argv)
{
    set_eugid(var_owner_uid, var_owner_gid);
}

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_COMMAND_MAXTIME, DEF_COMMAND_MAXTIME, &var_command_maxtime, 1, 0,
	0,
    };

    single_server_main(argc, argv, pipe_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_POST_INIT, drop_privileges,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
