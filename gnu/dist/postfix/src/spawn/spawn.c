/*++
/* NAME
/*	spawn 8
/* SUMMARY
/*	Postfix external command spawner
/* SYNOPSIS
/*	\fBspawn\fR [generic Postfix daemon options] command_attributes...
/* DESCRIPTION
/*	The \fBspawn\fR daemon provides the Postfix equivalent of \fBinetd\fR.
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
/*	corresponding group ID is used instead of the group ID of
/*	of \fIusername\fR.
/* .IP "\fBargv\fR=\fIcommand\fR... (required)"
/*	The command to be executed. This must be specified as the
/*	last command attribute.
/*	The command is executed directly, i.e. without interpretation of
/*	shell meta characters by a shell command interpreter.
/* BUGS
/*	In order to enforce standard Postfix process resource controls,
/*	the \fBspawn\fR daemon runs only one external command at a time.
/*	As such, it presents a noticeable overhead by wasting precious
/*	process resources. The \fBspawn\fR daemon is expected to be
/*	replaced by a more structural solution.
/* DIAGNOSTICS
/*	The \fBspawn\fR daemon reports abnormal child exits.
/*	Problems are logged to \fBsyslogd\fR(8).
/* SECURITY
/* .fi
/* .ad
/*	This program needs root privilege in order to execute external
/*	commands as the specified user. It is therefore security sensitive.
/*	However the \fBspawn\fR daemon does not talk to the external command
/*	and thus is not vulnerable to data-driven attacks.
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
/* .SH Resource control
/* .ad
/* .fi
/* .IP \fIservice\fB_command_time_limit\fR
/*	The amount of time the command is allowed to run before it is
/*	killed with force. The \fIservice\fR name is the name of the entry
/*	in the \fBmaster.cf\fR file. The default time limit is given by the
/*	global \fBcommand_time_limit\fR configuration parameter.
/* SEE ALSO
/*	master(8) process manager
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
    char   *myname = "get_service_attr";
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
	get_mail_conf_int2(service, "_time_limit", var_command_maxtime, 1, 0);

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
    char   *myname = "spawn_service";
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

    single_server_main(argc, argv, spawn_service,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_POST_INIT, drop_privileges,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       0);
}
