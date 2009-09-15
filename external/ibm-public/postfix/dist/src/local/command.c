/*	$NetBSD: command.c,v 1.1.1.1.2.2 2009/09/15 06:02:56 snj Exp $	*/

/*++
/* NAME
/*	command 3
/* SUMMARY
/*	message delivery to shell command
/* SYNOPSIS
/*	#include "local.h"
/*
/*	int	deliver_command(state, usr_attr, command)
/*	LOCAL_STATE state;
/*	USER_ATTR exp_attr;
/*	const char *command;
/* DESCRIPTION
/*	deliver_command() runs a command with a message as standard
/*	input.  A limited amount of standard output and standard error
/*	output is captured for diagnostics purposes.
/*	Duplicate commands for the same recipient are suppressed.
/*	A limited amount of information is exported via the environment:
/*	HOME, SHELL, LOGNAME, USER, EXTENSION, DOMAIN, RECIPIENT (entire
/*	address) LOCAL (just the local part) and SENDER. The exported
/*	information is censored with var_cmd_filter.
/*
/*	Arguments:
/* .IP state
/*	The attributes that specify the message, recipient and more.
/*	Attributes describing the alias, include or forward expansion.
/*	A table with the results from expanding aliases or lists.
/* .IP usr_attr
/*	Attributes describing user rights and environment.
/* .IP command
/*	The shell command to be executed. If possible, the command is
/*	executed without actually invoking a shell. if the command is
/*	the mailbox_command, it is subjected to $name expansion.
/* DIAGNOSTICS
/*	deliver_command() returns non-zero when delivery should be
/*	tried again,
/* SEE ALSO
/*	mailbox(3) deliver to mailbox
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
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <mac_parse.h>

/* Global library. */

#include <defer.h>
#include <bounce.h>
#include <sent.h>
#include <been_here.h>
#include <mail_params.h>
#include <pipe_command.h>
#include <mail_copy.h>
#include <dsn_util.h>

/* Application-specific. */

#include "local.h"

/* deliver_command - deliver to shell command */

int     deliver_command(LOCAL_STATE state, USER_ATTR usr_attr, const char *command)
{
    const char *myname = "deliver_command";
    DSN_BUF *why = state.msg_attr.why;
    int     cmd_status;
    int     deliver_status;
    ARGV   *env;
    int     copy_flags;
    char  **cpp;
    char   *cp;
    ARGV   *export_env;
    VSTRING *exec_dir;
    int     expand_status;

    /*
     * Make verbose logging easier to understand.
     */
    state.level++;
    if (msg_verbose)
	MSG_LOG_STATE(myname, state);

    /*
     * DUPLICATE ELIMINATION
     * 
     * Skip this command if it was already delivered to as this user.
     */
    if (been_here(state.dup_filter, "command %s:%ld %s",
		  state.msg_attr.user, (long) usr_attr.uid, command))
	return (0);

    /*
     * Don't deliver a trace-only request.
     */
    if (DEL_REQ_TRACE_ONLY(state.request->flags)) {
	dsb_simple(why, "2.0.0", "delivers to command: %s", command);
	return (sent(BOUNCE_FLAGS(state.request),
		     SENT_ATTR(state.msg_attr)));
    }

    /*
     * DELIVERY RIGHTS
     * 
     * Choose a default uid and gid when none have been selected (i.e. values
     * are still zero).
     */
    if (usr_attr.uid == 0 && (usr_attr.uid = var_default_uid) == 0)
	msg_panic("privileged default user id");
    if (usr_attr.gid == 0 && (usr_attr.gid = var_default_gid) == 0)
	msg_panic("privileged default group id");

    /*
     * Deliver.
     */
    copy_flags = MAIL_COPY_FROM | MAIL_COPY_RETURN_PATH
	| MAIL_COPY_ORIG_RCPT;
    if (local_deliver_hdr_mask & DELIVER_HDR_CMD)
	copy_flags |= MAIL_COPY_DELIVERED;

    if (vstream_fseek(state.msg_attr.fp, state.msg_attr.offset, SEEK_SET) < 0)
	msg_fatal("%s: seek queue file %s: %m",
		  myname, VSTREAM_PATH(state.msg_attr.fp));

    /*
     * Pass additional environment information. XXX This should be
     * configurable. However, passing untrusted information via environment
     * parameters opens up a whole can of worms. Lesson from web servers:
     * don't let any network data even near a shell. It causes trouble.
     */
    env = argv_alloc(1);
    if (usr_attr.home)
	argv_add(env, "HOME", usr_attr.home, ARGV_END);
    argv_add(env,
	     "LOGNAME", state.msg_attr.user,
	     "USER", state.msg_attr.user,
	     "SENDER", state.msg_attr.sender,
	     "RECIPIENT", state.msg_attr.rcpt.address,
	     "LOCAL", state.msg_attr.local,
	     ARGV_END);
    if (usr_attr.shell)
	argv_add(env, "SHELL", usr_attr.shell, ARGV_END);
    if (state.msg_attr.domain)
	argv_add(env, "DOMAIN", state.msg_attr.domain, ARGV_END);
    if (state.msg_attr.extension)
	argv_add(env, "EXTENSION", state.msg_attr.extension, ARGV_END);
    if (state.msg_attr.rcpt.orig_addr && state.msg_attr.rcpt.orig_addr[0])
	argv_add(env, "ORIGINAL_RECIPIENT", state.msg_attr.rcpt.orig_addr,
		 ARGV_END);

#define EXPORT_REQUEST(name, value) \
	if ((value)[0]) argv_add(env, (name), (value), ARGV_END);

    EXPORT_REQUEST("CLIENT_HOSTNAME", state.msg_attr.request->client_name);
    EXPORT_REQUEST("CLIENT_ADDRESS", state.msg_attr.request->client_addr);
    EXPORT_REQUEST("CLIENT_HELO", state.msg_attr.request->client_helo);
    EXPORT_REQUEST("CLIENT_PROTOCOL", state.msg_attr.request->client_proto);
    EXPORT_REQUEST("SASL_METHOD", state.msg_attr.request->sasl_method);
    EXPORT_REQUEST("SASL_SENDER", state.msg_attr.request->sasl_sender);
    EXPORT_REQUEST("SASL_USERNAME", state.msg_attr.request->sasl_username);

    argv_terminate(env);

    /*
     * Censor out undesirable characters from exported data.
     */
    for (cpp = env->argv; *cpp; cpp += 2)
	for (cp = cpp[1]; *(cp += strspn(cp, var_cmd_exp_filter)) != 0;)
	    *cp++ = '_';

    /*
     * Evaluate the command execution directory. Defer delivery if expansion
     * fails.
     */
    export_env = argv_split(var_export_environ, ", \t\r\n");
    exec_dir = vstring_alloc(10);
    expand_status = local_expand(exec_dir, var_exec_directory,
				 &state, &usr_attr, var_exec_exp_filter);

    if (expand_status & MAC_PARSE_ERROR) {
	cmd_status = PIPE_STAT_DEFER;
	dsb_simple(why, "4.3.5", "mail system configuration error");
	msg_warn("bad parameter value syntax for %s: %s",
		 VAR_EXEC_DIRECTORY, var_exec_directory);
    } else {
	cmd_status = pipe_command(state.msg_attr.fp, why,
				  PIPE_CMD_UID, usr_attr.uid,
				  PIPE_CMD_GID, usr_attr.gid,
				  PIPE_CMD_COMMAND, command,
				  PIPE_CMD_COPY_FLAGS, copy_flags,
				  PIPE_CMD_SENDER, state.msg_attr.sender,
			  PIPE_CMD_ORIG_RCPT, state.msg_attr.rcpt.orig_addr,
			       PIPE_CMD_DELIVERED, state.msg_attr.delivered,
				  PIPE_CMD_TIME_LIMIT, var_command_maxtime,
				  PIPE_CMD_ENV, env->argv,
				  PIPE_CMD_EXPORT, export_env->argv,
				  PIPE_CMD_SHELL, var_local_cmd_shell,
				  PIPE_CMD_CWD, *STR(exec_dir) ?
				  STR(exec_dir) : (char *) 0,
				  PIPE_CMD_END);
    }
    vstring_free(exec_dir);
    argv_free(export_env);
    argv_free(env);

    /*
     * Depending on the result, bounce or defer the message.
     */
    switch (cmd_status) {
    case PIPE_STAT_OK:
	dsb_simple(why, "2.0.0", "delivered to command: %s", command);
	deliver_status = sent(BOUNCE_FLAGS(state.request),
			      SENT_ATTR(state.msg_attr));
	break;
    case PIPE_STAT_BOUNCE:
    case PIPE_STAT_DEFER:
	deliver_status =
	    (STR(why->status)[0] == '4' ?
	     defer_append : bounce_append)
	    (BOUNCE_FLAGS(state.request),
	     BOUNCE_ATTR(state.msg_attr));
	break;
    case PIPE_STAT_CORRUPT:
	deliver_status = DEL_STAT_DEFER;
	break;
    default:
	msg_panic("%s: bad status %d", myname, cmd_status);
	/* NOTREACHED */
    }

    return (deliver_status);
}
