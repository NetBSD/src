/*++
/* NAME
/*	postfix 1
/* SUMMARY
/*	Postfix control program
/* SYNOPSIS
/* .fi
/*	\fBpostfix\fR [\fB-c \fIconfig_dir\fR] [\fB-D\fR] [\fB-v\fR]
/*		\fIcommand\fR
/* DESCRIPTION
/*	This command is reserved for the super-user. To submit mail,
/*	use the Postfix \fBsendmail\fR command.
/*
/*	The \fBpostfix\fR command controls the operation of the Postfix
/*	mail system: start or stop the \fBmaster\fR daemon, do a health
/*	check, and other maintenance.
/*
/*	The \fBpostfix\fR command sets up a standardized environment and
/*	runs the \fBpostfix-script\fR shell script to do the actual work.
/*
/*	The following commands are implemented:
/* .IP \fBcheck\fR
/*	Validate the Postfix mail system configuration. Warn about bad
/*	directory/file ownership or permissions, and create missing
/*	directories.
/* .IP \fBstart\fR
/*	Start the Postfix mail system. This also runs the configuration
/*	check described above.
/* .IP \fBstop\fR
/*	Stop the Postfix mail system in an orderly fashion. Running processes
/*	are allowed to terminate at their earliest convenience.
/* .sp
/*	Note: in order to refresh the Postfix mail system after a
/*	configuration change, do not use the \fBstart\fR and \fBstop\fR
/*	commands in succession. Use the \fBreload\fR command instead.
/* .IP \fBabort\fR
/*	Stop the Postfix mail system abruptly. Running processes are
/*	signaled to stop immediately.
/* .IP \fBflush\fR
/*	Force delivery: attempt to deliver every message in the deferred
/*	mail queue. Normally, attempts to deliver delayed mail happen at
/*	regular intervals, the interval doubling after each failed attempt.
/* .IP \fBreload\fR
/*	Re-read configuration files. Running processes terminate at their
/*	earliest convenience.
/* .PP
/*	The following options are implemented:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR and \fBmaster.cf\fR configuration files in
/*	the named directory instead of the default configuration directory.
/*	Use this to distinguish between multiple Postfix instances on the
/*	same host.
/* .IP "\fB-D\fR (with \fBpostfix start\fR only)"
/*	Run each Postfix daemon under control of a debugger as specified
/*	via the \fBdebugger_command\fR configuration parameter.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* ENVIRONMENT
/* .ad
/* .fi
/*	The \fBpostfix\fR command sets the following environment
/*	variables:
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* .IP \fBMAIL_VERBOSE\fR
/*	This is set when the -v command-line option is present.
/* .IP \fBMAIL_DEBUG\fR
/*	This is set when the -D command-line option is present.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR configuration parameters are made
/*	available as process environment variables with the same names:
/* .IP \fBcommand_directory\fR
/*	Directory with Postfix administrative commands.
/* .IP \fBdaemon_directory\fR
/*	Directory with Postfix daemon programs.
/* .IP \fBconfig_directory\fR
/*	Directory with Postfix configuration files and with administrative
/*	shell scripts.
/* .IP \fBqueue_directory\fR
/*	The directory with Postfix queue files, with local inter-process
/*	communication endpoints, and with files needed for daemon programs
/*	that run in the optional chrooted environment.
/* .IP \fBmail_owner\fR
/*      The owner of Postfix queue files and of most Postfix processes.
/* .IP \fBsetgid_group\fR
/*      The group for mail submission and queue management commands.
/* .IP \fBsendmail_path
/*       The full pathname for the Postfix sendmail command.
/* .IP \fBnewaliases_path
/*       The full pathname for the Postfix newaliases command.
/* .IP \fBmailq_path
/*       The full pathname for the Postfix mailq command.
/* .IP \fBmanpage_directory
/*       The directory for the Postfix on-line manual pages.
/* .IP \fBsample_directory
/*       The directory for the Postfix sample configuration files.
/* .IP \fBreadme_directory
/*       The directory for the Postfix README files.
/* .SH Other configuration parameters
/* .ad
/* .fi
/* .IP \fBimport_environment\fR
/*	List of names of environment parameters that can be imported
/*	from non-Postfix processes.
/* FILES
/*	$\fBconfig_directory/postfix-script\fR, administrative commands
/*	$\fBconfig_directory/main.cf\fR, configuration parameters
/*	$\fBconfig_directory/master.cf\fR, Postfix daemon processes
/* SEE ALSO
/*	master(8) Postfix master program
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
#include <vstream.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif

/* Utility library. */

#include <msg.h>
#include <msg_vstream.h>
#include <msg_syslog.h>
#include <stringops.h>
#include <clean_env.h>
#include <argv.h>
#include <safe.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>

/* Additional installation parameters. */

static char *var_sendmail_path;
static char *var_mailq_path;
static char *var_newalias_path;
static char *var_manpage_dir;
static char *var_sample_dir;
static char *var_readme_dir;

/* check_setenv - setenv() with extreme prejudice */

static void check_setenv(char *name, char *value)
{
#define CLOBBER 1
    if (setenv(name, value, CLOBBER) < 0)
	msg_fatal("setenv: %m");
}

/* main - run administrative script from controlled environment */

int     main(int argc, char **argv)
{
    char   *script;
    struct stat st;
    char   *slash;
    int     fd;
    int     ch;
    ARGV   *import_env;
    static CONFIG_STR_TABLE str_table[] = {
	VAR_SENDMAIL_PATH, DEF_SENDMAIL_PATH, &var_sendmail_path, 1, 0,
	VAR_MAILQ_PATH, DEF_MAILQ_PATH, &var_mailq_path, 1, 0,
	VAR_NEWALIAS_PATH, DEF_NEWALIAS_PATH, &var_newalias_path, 1, 0,
	VAR_MANPAGE_DIR, DEF_MANPAGE_DIR, &var_manpage_dir, 1, 0,
	VAR_SAMPLE_DIR, DEF_SAMPLE_DIR, &var_sample_dir, 1, 0,
	VAR_README_DIR, DEF_README_DIR, &var_readme_dir, 1, 0,
	0,
    };

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
	    msg_fatal("open /dev/null: %m");

    /*
     * Set up diagnostics. XXX What if stdin is the system console during
     * boot time? It seems a bad idea to log startup errors to the console.
     * This is UNIX, a system that can run without hand holding.
     */
    if ((slash = strrchr(argv[0], '/')) != 0)
	argv[0] = slash + 1;
    if (isatty(STDERR_FILENO))
	msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(argv[0], LOG_PID, LOG_FACILITY);

    /*
     * The mail system must be run by the superuser so it can revoke
     * privileges for selected operations. That's right - it takes privileges
     * to toss privileges.
     */
    if (getuid() != 0) {
	msg_error("to submit mail, use the Postfix sendmail command");
	msg_fatal("the postfix command is reserved for the superuser");
    }
    if (unsafe() != 0)
	msg_fatal("the postfix command must not run as a set-uid process");

    /*
     * Parse switches.
     */
    while ((ch = GETOPT(argc, argv, "c:Dv")) > 0) {
	switch (ch) {
	default:
	    msg_fatal("usage: %s [-c config_dir] [-v] command", argv[0]);
	case 'c':
	    if (*optarg != '/')
		msg_fatal("-c requires absolute pathname");
	    check_setenv(CONF_ENV_PATH, optarg);
	    break;
	case 'D':
	    check_setenv(CONF_ENV_DEBUG, "");
	    break;
	case 'v':
	    msg_verbose++;
	    check_setenv(CONF_ENV_VERB, "");
	    break;
	}
    }

    /*
     * Copy a bunch of configuration parameters into the environment for easy
     * access by the maintenance shell script. XXX There should be a postconf
     * utility that makes config parameters easily accessible for shell
     * scripts.
     */
    mail_conf_read();
    get_mail_conf_str_table(str_table);

    /*
     * Environment import filter, to enforce consistent behavior whether this
     * command is started by hand, or at system boot time. This is necessary
     * because some shell scripts use environment settings to override
     * main.cf settings.
     */
    import_env = argv_split(var_import_environ, ", \t\r\n");
    clean_env(import_env->argv);
    argv_free(import_env);

    check_setenv("PATH", ROOT_PATH);		/* sys_defs.h */
    check_setenv(CONF_ENV_PATH, var_config_dir);/* mail_conf.h */

    check_setenv(VAR_COMMAND_DIR, var_command_dir);	/* main.cf */
    check_setenv(VAR_DAEMON_DIR, var_daemon_dir);	/* main.cf */
    check_setenv(VAR_QUEUE_DIR, var_queue_dir);	/* main.cf */
    check_setenv(VAR_CONFIG_DIR, var_config_dir);	/* main.cf */

    /*
     * Do we want to keep adding things here as shell scripts evolve?
     */
    check_setenv(VAR_MAIL_OWNER, var_mail_owner);	/* main.cf */
    check_setenv(VAR_SGID_GROUP, var_sgid_group);	/* main.cf */
    check_setenv(VAR_SENDMAIL_PATH, var_sendmail_path);	/* main.cf */
    check_setenv(VAR_MAILQ_PATH, var_mailq_path);	/* main.cf */
    check_setenv(VAR_NEWALIAS_PATH, var_newalias_path);	/* main.cf */
    check_setenv(VAR_MANPAGE_DIR, var_manpage_dir);	/* main.cf */
    check_setenv(VAR_SAMPLE_DIR, var_sample_dir);	/* main.cf */
    check_setenv(VAR_README_DIR, var_readme_dir);	/* main.cf */

    /*
     * Make sure these directories exist. Run the maintenance scripts with as
     * current directory the mail database.
     */
    if (chdir(var_command_dir))
	msg_fatal("chdir(%s): %m", var_command_dir);
    if (chdir(var_daemon_dir))
	msg_fatal("chdir(%s): %m", var_daemon_dir);
    if (chdir(var_queue_dir))
	msg_fatal("chdir(%s): %m", var_queue_dir);

    /*
     * Run the management script with as process name ourself.
     */
    script = concatenate(var_config_dir, "/postfix-script", (char *) 0);
    execvp(script, argv + optind - 1);
    msg_fatal("%s: %m", script);
}
