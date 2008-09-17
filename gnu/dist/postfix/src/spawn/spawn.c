/*++
/* NAME
/*	spawn 8
/* SUMMARY
/*	Postfix external command spawner
/* SYNOPSIS
/*	\fBspawn\fR [generic Postfix daemon options] command_attributes...
/* DESCRIPTION
/*	The \fBspawn\fR(8) daemon provides the Postfix equivalent
/*	of \fBinetd\fR.
/*	It listens on a port as specified in the Postfix \fBmaster.cf\fR file
/*	and spawns an external command whenever a connection is established.
/*	The connection can be made over local IPC (such as UNIX-domain
/*	sockets) or over non-local IPC (such as TCP sockets).
/*	The command\'s standard input, output and error streams are connected
/*	directly to the communication endpoint.
/*
/*	This daemon expects to be run from the \fBmaster\fR(8) process
/*	manager.
/* COMMAND ATTRIBUTE SYNTAX
/* .ad
/* .fi
/*	The external command attributes are given in the \fBmaster.cf\fR
/*	file at the end of a service definition.  The syntax is as follows:
/* .IP "\fBuser\fR=\fIusername\fR (required)"
/* .IP "\fBuser\fR=\fIusername\fR:\fIgroupname\fR"
/*	The external command is executed with the rights of the
/*	specified \fIusername\fR.  The software refuses to execute
/*	commands with root privileges, or with the privileges of the
/*	mail system owner. If \fIgroupname\fR is specified, the
/*	corresponding group ID is used instead of the group ID
/*	of \fIusername\fR.
/* .IP "\fBargv\fR=\fIcommand\fR... (required)"
/*	The command to be executed. This must be specified as the
/*	last command attribute.
/*	The command is executed directly, i.e. without interpretation of
/*	shell meta characters by a shell command interpreter.
/* BUGS
/*	In order to enforce standard Postfix process resource controls,
/*	the \fBspawn\fR(8) daemon runs only one external command at a time.
/*	As such, it presents a noticeable overhead by wasting precious
/*	process resources. The \fBspawn\fR(8) daemon is expected to be
/*	replaced by a more structural solution.
/* DIAGNOSTICS
/*	The \fBspawn\fR(8) daemon reports abnormal child exits.
/*	Problems are logged to \fBsyslogd\fR(8).
/* SECURITY
/* .fi
/* .ad
/*	This program needs root privilege in order to execute external
/*	commands as the specified user. It is therefore security sensitive.
/*	However the \fBspawn\fR(8) daemon does not talk to the external command
/*	and thus is not vulnerable to data-driven attacks.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically as \fBspawn\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/*
/*	In the text below, \fItransport\fR is the first field of the entry
/*	in the \fBmaster.cf\fR file.
/* RESOURCE AND RATE CONTROL
/* .ad
/* .fi
/* .IP "\fItransport\fB_time_limit ($command_time_limit)\fR"
/*	The amount of time the command is allowed to run before it is
/*	terminated.
/*
/*	Postfix 2.4 and later support a suffix that specifies the
/*	time unit: s (seconds), m (minutes), h (hours), d (days),
/*	w (weeks). The default time unit is seconds.
/* MISCELLANEOUS
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
/* .IP "\fBmail_owner (postfix)\fR"
/*	The UNIX system account that owns the Postfix queue and most Postfix
/*	daemon processes.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	postconf(5), configuration parameters
/*	master(8), process manager
/*	syslogd(8), system logging
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
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <argv.h>
#include <dict.h>
#include <mymalloc.h>
#include <spawn_command.h>
#include <split_at.h>
#include <timed_wait.h>
#include <set_eugid.h>

/* Global library. */

#include <mail_version.h>

/* Single server skeleton. */

#include <mail_params.h>
#include <mail_server.h>
#include <mail_conf.h>

/* Application-specific. */

 /*
  * Tunable parameters. Values are taken from the config file, after
  * prepending the service name to _name, and so on.
  */
int     var_command_maxtime;		/* system-wide */

 /*
  * For convenience. Instead of passing around lists of parameters, bundle
  * them up in convenient structures.
  */
typedef struct {
    char  **argv;			/* argument vector */
    uid_t   uid;			/* command privileges */
    gid_t   gid;			/* command privileges */
    int     time_limit;			/* per-service time limit */
} SPAWN_ATTR;

/* get_service_attr - get service attributes */

static void get_service_attr(SPAWN_ATTR *attr, char *service, char **argv)
{
    const char *myname = "get_service_attr";
    struct passwd *pwd;
    struct group *grp;
    char   *user;			/* user name */
    char   *group;			/* group name */

    /*
     * Initialize.
     */
    user = 0;
    group = 0;
    attr->argv = 0;

    /*
     * Figure out the command time limit for this transport.
     */
    attr->time_limit =
	get_mail_conf_time2(service, _MAXTIME, var_command_maxtime, 's', 1, 0);

    /*
     * Iterate over the command-line attribute list.
     */
    for ( /* void */ ; *argv != 0; argv++) {

	/*
	 * user=username[:groupname]
	 */
	if (strncasecmp("user=", *argv, sizeof("user=") - 1) == 0) {
	    user = *argv + sizeof("user=") - 1;
	    if ((group = split_at(user, ':')) != 0)	/* XXX clobbers argv */
		if (*group == 0)
		    group = 0;
	    if ((pwd = getpwnam(user)) == 0)
		msg_fatal("unknown user name: %s", user);
	    attr->uid = pwd->pw_uid;
	    if (group != 0) {
		if ((grp = getgrnam(group)) == 0)
		    msg_fatal("unknown group name: %s", group);
		attr->gid = grp->gr_gid;
	    } else {
		attr->gid = pwd->pw_gid;
	    }
	}

	/*
	 * argv=command...
	 */
	else if (strncasecmp("argv=", *argv, sizeof("argv=") - 1) == 0) {
	    *argv += sizeof("argv=") - 1;	/* XXX clobbers argv */
	    attr->argv = argv;
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
    if (attr->argv == 0)
	msg_fatal("missing argv= attribute");
    if (attr->uid == 0)
	msg_fatal("request to deliver as root");
    if (attr->uid == var_owner_uid)
	msg_fatal("request to deliver as mail system owner");
    if (attr->gid == 0)
	msg_fatal("request to use privileged group id %ld", (long) attr->gid);
    if (attr->gid == var_owner_gid)
	msg_fatal("request to use mail system owner group id %ld", (long) attr->gid);
    if (attr->uid == (uid_t) (-1))
	msg_fatal("user must not have user ID -1");
    if (attr->gid == (gid_t) (-1))
	msg_fatal("user must not have group ID -1");

    /*
     * Give the poor tester a clue of what is going on.
     */
    if (msg_verbose)
	msg_info("%s: uid %ld, gid %ld; time %d",
	      myname, (long) attr->uid, (long) attr->gid, attr->time_limit);
}

/* spawn_service - perform service for client */

static void spawn_service(VSTREAM *client_stream, char *service, char **argv)
{
    const char *myname = "spawn_service";
    static SPAWN_ATTR attr;
    WAIT_STATUS_T status;
    ARGV   *export_env;

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to running an external command.
     */
    if (msg_verbose)
	msg_info("%s: service=%s, command=%s...", myname, service, argv[0]);

    /*
     * Look up service attributes and config information only once. This is
     * safe since the information comes from a trusted source.
     */
    if (attr.argv == 0) {
	get_service_attr(&attr, service, argv);
    }

    /*
     * Execute the command.
     */
    export_env = argv_split(var_export_environ, ", \t\r\n");
    status = spawn_command(SPAWN_CMD_STDIN, vstream_fileno(client_stream),
			   SPAWN_CMD_STDOUT, vstream_fileno(client_stream),
			   SPAWN_CMD_STDERR, vstream_fileno(client_stream),
			   SPAWN_CMD_UID, attr.uid,
			   SPAWN_CMD_GID, attr.gid,
			   SPAWN_CMD_ARGV, attr.argv,
			   SPAWN_CMD_TIME_LIMIT, attr.time_limit,
			   SPAWN_CMD_EXPORT, export_env->argv,
			   SPAWN_CMD_END);
    argv_free(export_env);

    /*
     * Warn about unsuccessful completion.
     */
    if (!NORMAL_EXIT_STATUS(status)) {
	if (WIFEXITED(status))
	    msg_warn("command %s exit status %d",
		     attr.argv[0], WEXITSTATUS(status));
	if (WIFSIGNALED(status))
	    msg_warn("command %s killed by signal %d",
		     attr.argv[0], WTERMSIG(status));
    }
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

/* drop_privileges - drop privileges most of the time */

static void drop_privileges(char *unused_name, char **unused_argv)
{
    set_eugid(var_owner_uid, var_owner_gid);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_COMMAND_MAXTIME, DEF_COMMAND_MAXTIME, &var_command_maxtime, 1, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    single_server_main(argc, argv, spawn_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_POST_INIT, drop_privileges,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_PRIVILEGED,
		       0);
}
