/*	$NetBSD: postfix.c,v 1.2 2017/02/14 01:16:46 christos Exp $	*/

/*++
/* NAME
/*	postfix 1
/* SUMMARY
/*	Postfix control program
/* SYNOPSIS
/* .fi
/*	\fBpostfix\fR [\fB-Dv\fR] [\fB-c \fIconfig_dir\fR] \fIcommand\fR
/* DESCRIPTION
/*	This command is reserved for the superuser. To submit mail,
/*	use the Postfix \fBsendmail\fR(1) command.
/*
/*	The \fBpostfix\fR(1) command controls the operation of the Postfix
/*	mail system: start or stop the \fBmaster\fR(8) daemon, do a health
/*	check, and other maintenance.
/*
/*	By default, the \fBpostfix\fR(1) command sets up a standardized
/*	environment and runs the \fBpostfix-script\fR shell script
/*	to do the actual work.
/*
/*	However, when support for multiple Postfix instances is
/*	configured, \fBpostfix\fR(1) executes the command specified
/*	with the \fBmulti_instance_wrapper\fR configuration parameter.
/*	This command will execute the \fIcommand\fR for each
/*	applicable Postfix instance.
/*
/*	The following commands are implemented:
/* .IP \fBcheck\fR
/*	Warn about bad directory/file ownership or permissions,
/*	and create missing directories.
/* .IP \fBstart\fR
/*	Start the Postfix mail system. This also runs the configuration
/*	check described above.
/* .IP \fBstop\fR
/*	Stop the Postfix mail system in an orderly fashion. If
/*	possible, running processes are allowed to terminate at
/*	their earliest convenience.
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
/* .sp
/*	Warning: flushing undeliverable mail frequently will result in
/*	poor delivery performance of all other mail.
/* .IP \fBreload\fR
/*	Re-read configuration files. Running processes terminate at their
/*	earliest convenience.
/* .IP \fBstatus\fR
/*	Indicate if the Postfix mail system is currently running.
/* .IP "\fBset-permissions\fR [\fIname\fR=\fIvalue ...\fR]"
/*	Set the ownership and permissions of Postfix related files and
/*	directories, as specified in the \fBpostfix-files\fR file.
/* .sp
/*	Specify \fIname\fR=\fIvalue\fR to override and update specific
/*	main.cf configuration parameters. Use this, for example, to
/*	change the \fBmail_owner\fR or \fBsetgid_group\fR setting for an
/*	already installed Postfix system.
/* .sp
/*	This feature is available in Postfix 2.1 and later.  With
/*	Postfix 2.0 and earlier, use "\fB$config_directory/post-install
/*	set-permissions\fR".
/* .IP "\fBtls\fR \fIsubcommand\fR"
/*	Enable opportunistic TLS in the Postfix SMTP client or
/*	server, and manage Postfix SMTP server TLS private keys and
/*	certificates.  See postfix-tls(1) for documentation.
/* .sp
/*	This feature is available in Postfix 3.1 and later.
/* .IP "\fBupgrade-configuration\fR [\fIname\fR=\fIvalue ...\fR]"
/*	Update the \fBmain.cf\fR and \fBmaster.cf\fR files with information
/*	that Postfix needs in order to run: add or update services, and add
/*	or update configuration parameter settings.
/* .sp
/*	Specify \fIname\fR=\fIvalue\fR to override and update specific
/*	main.cf configuration parameters.
/* .sp
/*	This feature is available in Postfix 2.1 and later.  With
/*	Postfix 2.0 and earlier, use "\fB$config_directory/post-install
/*	upgrade-configuration\fR".
/* .PP
/*	The following options are implemented:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR and \fBmaster.cf\fR configuration files in
/*	the named directory instead of the default configuration directory.
/*	Use this to distinguish between multiple Postfix instances on the
/*	same host.
/*
/*	With Postfix 2.6 and later, this option forces the postfix(1)
/*	command to operate on the specified Postfix instance only.
/*	This behavior is inherited by postfix(1) commands that run
/*	as a descendant of the current process.
/* .IP "\fB-D\fR (with \fBpostfix start\fR only)"
/*	Run each Postfix daemon under control of a debugger as specified
/*	via the \fBdebugger_command\fR configuration parameter.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* ENVIRONMENT
/* .ad
/* .fi
/*	The \fBpostfix\fR(1) command exports the following environment
/*	variables before executing the \fBpostfix-script\fR file:
/* .IP \fBMAIL_CONFIG\fR
/*	This is set when the -c command-line option is present.
/*
/*	With Postfix 2.6 and later, this environment variable forces
/*	the postfix(1) command to operate on the specified Postfix
/*	instance only.  This behavior is inherited by postfix(1)
/*	commands that run as a descendant of the current process.
/* .IP \fBMAIL_VERBOSE\fR
/*	This is set when the -v command-line option is present.
/* .IP \fBMAIL_DEBUG\fR
/*	This is set when the -D command-line option is present.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR configuration parameters are
/*	exported as environment variables with the same names:
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBdaemon_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix support programs and daemon programs.
/* .IP "\fBhtml_directory (see 'postconf -d' output)\fR"
/*	The location of Postfix HTML files that describe how to build,
/*	configure or operate a specific Postfix subsystem or feature.
/* .IP "\fBmail_owner (postfix)\fR"
/*	The UNIX system account that owns the Postfix queue and most Postfix
/*	daemon processes.
/* .IP "\fBmailq_path (see 'postconf -d' output)\fR"
/*	Sendmail compatibility feature that specifies where the Postfix
/*	\fBmailq\fR(1) command is installed.
/* .IP "\fBmanpage_directory (see 'postconf -d' output)\fR"
/*	Where the Postfix manual pages are installed.
/* .IP "\fBnewaliases_path (see 'postconf -d' output)\fR"
/*	Sendmail compatibility feature that specifies the location of the
/*	\fBnewaliases\fR(1) command.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBreadme_directory (see 'postconf -d' output)\fR"
/*	The location of Postfix README files that describe how to build,
/*	configure or operate a specific Postfix subsystem or feature.
/* .IP "\fBsendmail_path (see 'postconf -d' output)\fR"
/*	A Sendmail compatibility feature that specifies the location of
/*	the Postfix \fBsendmail\fR(1) command.
/* .IP "\fBsetgid_group (postdrop)\fR"
/*	The group ownership of set-gid Postfix commands and of group-writable
/*	Postfix directories.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBdata_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix-writable data files (for example:
/*	caches, pseudo-random numbers).
/* .PP
/*	Available in Postfix version 3.0 and later:
/* .IP "\fBmeta_directory (see 'postconf -d' output)\fR"
/*	The location of non-executable files that are shared among
/*	multiple Postfix instances, such as postfix-files, dynamicmaps.cf,
/*	and the multi-instance template files main.cf.proto and master.cf.proto.
/* .IP "\fBshlib_directory (see 'postconf -d' output)\fR"
/*	The location of Postfix dynamically-linked libraries
/*	(libpostfix-*.so), and the default location of Postfix database
/*	plugins (postfix-*.so) that have a relative pathname in the
/*	dynamicmaps.cf file.
/* .PP
/*	Available in Postfix version 3.1 and later:
/* .IP "\fBopenssl_path (openssl)\fR"
/*	The location of the OpenSSL command line program \fBopenssl\fR(1).
/* .PP
/*	Other configuration parameters:
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment parameters that a Postfix process will
/*	import from a non-Postfix parent process.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBmulti_instance_directories (empty)\fR"
/*	An optional list of non-default Postfix configuration directories;
/*	these directories belong to additional Postfix instances that share
/*	the Postfix executable files and documentation with the default
/*	Postfix instance, and that are started, stopped, etc., together
/*	with the default Postfix instance.
/* .IP "\fBmulti_instance_wrapper (empty)\fR"
/*	The pathname of a multi-instance manager command that the
/*	\fBpostfix\fR(1) command invokes when the multi_instance_directories
/*	parameter value is non-empty.
/* .IP "\fBmulti_instance_group (empty)\fR"
/*	The optional instance group name of this Postfix instance.
/* .IP "\fBmulti_instance_name (empty)\fR"
/*	The optional instance name of this Postfix instance.
/* .IP "\fBmulti_instance_enable (no)\fR"
/*	Allow this Postfix instance to be started, stopped, etc., by a
/*	multi-instance manager.
/* FILES
/* .ad
/* .fi
/*	Prior to Postfix version 2.6, all of the following files
/*	were in \fB$config_directory\fR. Some files are now in
/*	\fB$daemon_directory\fR so that they can be shared among
/*	multiple instances that run the same Postfix version.
/*
/*	Use the command "\fBpostconf config_directory\fR" or
/*	"\fBpostconf daemon_directory\fR" to expand the names
/*	into their actual values.
/* .na
/* .nf
/*
/*	$config_directory/main.cf, Postfix configuration parameters
/*	$config_directory/master.cf, Postfix daemon processes
/*	$daemon_directory/postfix-files, file/directory permissions
/*	$daemon_directory/postfix-script, administrative commands
/*	$daemon_directory/post-install, post-installation configuration
/*	$daemon_directory/dynamicmaps.cf, plug-in database clients
/* SEE ALSO
/*	Commands:
/*	postalias(1), create/update/query alias database
/*	postcat(1), examine Postfix queue file
/*	postconf(1), Postfix configuration utility
/*	postfix(1), Postfix control program
/*	postfix-tls(1), Postfix TLS management
/*	postkick(1), trigger Postfix daemon
/*	postlock(1), Postfix-compatible locking
/*	postlog(1), Postfix-compatible logging
/*	postmap(1), Postfix lookup table manager
/*	postmulti(1), Postfix multi-instance manager
/*	postqueue(1), Postfix mail queue control
/*	postsuper(1), Postfix housekeeping
/*	mailq(1), Sendmail compatibility interface
/*	newaliases(1), Sendmail compatibility interface
/*	sendmail(1), Sendmail compatibility interface
/*
/*	Postfix configuration:
/*	bounce(5), Postfix bounce message templates
/*	master(5), Postfix master.cf file syntax
/*	postconf(5), Postfix main.cf file syntax
/*	postfix-wrapper(5), Postfix multi-instance API
/*
/*	Table-driven mechanisms:
/*	access(5), Postfix SMTP access control table
/*	aliases(5), Postfix alias database
/*	canonical(5), Postfix input address rewriting
/*	generic(5), Postfix output address rewriting
/*	header_checks(5), body_checks(5), Postfix content inspection
/*	relocated(5), Users that have moved
/*	transport(5), Postfix routing table
/*	virtual(5), Postfix virtual aliasing
/*
/*	Table lookup mechanisms:
/*	cidr_table(5), Associate CIDR pattern with value
/*	ldap_table(5), Postfix LDAP client
/*	lmdb_table(5), Postfix LMDB database driver
/*	memcache_table(5), Postfix memcache client
/*	mysql_table(5), Postfix MYSQL client
/*	nisplus_table(5), Postfix NIS+ client
/*	pcre_table(5), Associate PCRE pattern with value
/*	pgsql_table(5), Postfix PostgreSQL client
/*	regexp_table(5), Associate POSIX regexp pattern with value
/*	socketmap_table(5), Postfix socketmap client
/*	sqlite_table(5), Postfix SQLite database driver
/*	tcp_table(5), Postfix client-server table lookup
/*
/*	Daemon processes:
/*	anvil(8), Postfix connection/rate limiting
/*	bounce(8), defer(8), trace(8), Delivery status reports
/*	cleanup(8), canonicalize and enqueue message
/*	discard(8), Postfix discard delivery agent
/*	dnsblog(8), DNS black/whitelist logger
/*	error(8), Postfix error delivery agent
/*	flush(8), Postfix fast ETRN service
/*	local(8), Postfix local delivery agent
/*	master(8), Postfix master daemon
/*	oqmgr(8), old Postfix queue manager
/*	pickup(8), Postfix local mail pickup
/*	pipe(8), deliver mail to non-Postfix command
/*	postscreen(8), Postfix zombie blocker
/*	proxymap(8), Postfix lookup table proxy server
/*	qmgr(8), Postfix queue manager
/*	qmqpd(8), Postfix QMQP server
/*	scache(8), Postfix connection cache manager
/*	showq(8), list Postfix mail queue
/*	smtp(8), lmtp(8), Postfix SMTP+LMTP client
/*	smtpd(8), Postfix SMTP server
/*	spawn(8), run non-Postfix server
/*	tlsmgr(8), Postfix TLS cache and randomness manager
/*	tlsproxy(8), Postfix TLS proxy server
/*	trivial-rewrite(8), Postfix address rewriting
/*	verify(8), Postfix address verification
/*	virtual(8), Postfix virtual delivery agent
/*
/*	Other:
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	OVERVIEW, overview of Postfix commands and processes
/*	BASIC_CONFIGURATION_README, Postfix basic configuration
/*	ADDRESS_REWRITING_README, Postfix address rewriting
/*	SMTPD_ACCESS_README, SMTP relay/access control
/*	CONTENT_INSPECTION_README, Postfix content inspection
/*	QSHAPE_README, Postfix queue analysis
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	TLS support by:
/*	Lutz Jaenicke
/*	Brandenburg University of Technology
/*	Cottbus, Germany
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*
/*	SASL support originally by:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	LMTP support originally by:
/*	Philip A. Prindeville
/*	Mirapoint, Inc.
/*	USA.
/*
/*	Amos Gouaux
/*	University of Texas at Dallas
/*	P.O. Box 830688, MC34
/*	Richardson, TX 75083, USA
/*
/*	IPv6 support originally by:
/*	Mark Huizer, Eindhoven University, The Netherlands
/*	Jun-ichiro 'itojun' Hagino, KAME project, Japan
/*	The Linux PLD project
/*	Dean Strik, Eindhoven University, The Netherlands
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
#include <warn_stat.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_parm_split.h>

/* Additional installation parameters. */

static char *var_sendmail_path;
static char *var_mailq_path;
static char *var_newalias_path;
static char *var_manpage_dir;
static char *var_sample_dir;
static char *var_readme_dir;
static char *var_html_dir;

/* check_setenv - setenv() with extreme prejudice */

static void check_setenv(char *name, char *value)
{
#define CLOBBER 1
    if (setenv(name, value, CLOBBER) < 0)
	msg_fatal("setenv: %m");
}

MAIL_VERSION_STAMP_DECLARE;

/* main - run administrative script from controlled environment */

int     main(int argc, char **argv)
{
    char   *script;
    struct stat st;
    char   *slash;
    int     fd;
    int     ch;
    ARGV   *import_env;
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_SENDMAIL_PATH, DEF_SENDMAIL_PATH, &var_sendmail_path, 1, 0,
	VAR_MAILQ_PATH, DEF_MAILQ_PATH, &var_mailq_path, 1, 0,
	VAR_NEWALIAS_PATH, DEF_NEWALIAS_PATH, &var_newalias_path, 1, 0,
	VAR_MANPAGE_DIR, DEF_MANPAGE_DIR, &var_manpage_dir, 1, 0,
	VAR_SAMPLE_DIR, DEF_SAMPLE_DIR, &var_sample_dir, 1, 0,
	VAR_README_DIR, DEF_README_DIR, &var_readme_dir, 1, 0,
	VAR_HTML_DIR, DEF_HTML_DIR, &var_html_dir, 1, 0,
	0,
    };
    int     force_single_instance;
    ARGV   *my_argv;

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
	    msg_fatal("open /dev/null: %m");

    /*
     * Set up diagnostics. XXX What if stdin is the system console during
     * boot time? It seems a bad idea to log startup errors to the console.
     * This is UNIX, a system that can run without hand holding.
     */
    if ((slash = strrchr(argv[0], '/')) != 0 && slash[1])
	argv[0] = slash + 1;
    if (isatty(STDERR_FILENO))
	msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(argv[0], LOG_PID, LOG_FACILITY);

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

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
	    msg_fatal("usage: %s [-c config_dir] [-Dv] command", argv[0]);
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
    force_single_instance = (getenv(CONF_ENV_PATH) != 0);

    /*
     * Copy a bunch of configuration parameters into the environment for easy
     * access by the maintenance shell script.
     */
    mail_conf_read();
    get_mail_conf_str_table(str_table);

    /*
     * Alert the sysadmin that the backwards-compatible settings are still in
     * effect.
     */
    if (var_compat_level < CUR_COMPAT_LEVEL) {
	msg_info("Postfix is running with backwards-compatible default "
		 "settings");
	msg_info("See http://www.postfix.org/COMPATIBILITY_README.html "
		 "for details");
	msg_info("To disable backwards compatibility use \"postconf "
		 VAR_COMPAT_LEVEL "=%d\" and \"postfix reload\"",
		 CUR_COMPAT_LEVEL);
    }

    /*
     * Environment import filter, to enforce consistent behavior whether this
     * command is started by hand, or at system boot time. This is necessary
     * because some shell scripts use environment settings to override
     * main.cf settings.
     */
    import_env = mail_parm_split(VAR_IMPORT_ENVIRON, var_import_environ);
    clean_env(import_env->argv);
    argv_free(import_env);

    check_setenv("PATH", ROOT_PATH);		/* sys_defs.h */
    check_setenv(CONF_ENV_PATH, var_config_dir);/* mail_conf.h */

    check_setenv(VAR_COMMAND_DIR, var_command_dir);	/* main.cf */
    check_setenv(VAR_DAEMON_DIR, var_daemon_dir);	/* main.cf */
    check_setenv(VAR_DATA_DIR, var_data_dir);	/* main.cf */
    check_setenv(VAR_META_DIR, var_meta_dir);	/* main.cf */
    check_setenv(VAR_QUEUE_DIR, var_queue_dir);	/* main.cf */
    check_setenv(VAR_CONFIG_DIR, var_config_dir);	/* main.cf */
    check_setenv(VAR_SHLIB_DIR, var_shlib_dir);	/* main.cf */

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
    check_setenv(VAR_HTML_DIR, var_html_dir);	/* main.cf */

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
     * Run the management script.
     */
    if (force_single_instance
	|| argv_split(var_multi_conf_dirs, CHARS_COMMA_SP)->argc == 0) {
	script = concatenate(var_daemon_dir, "/postfix-script", (char *) 0);
	if (optind < 1)
	    msg_panic("bad optind value");
	argv[optind - 1] = script;
	execvp(script, argv + optind - 1);
	msg_fatal("%s: %m", script);
    }

    /*
     * Hand off control to a multi-instance manager.
     */
    else {
	if (*var_multi_wrapper == 0)
	    msg_fatal("multi-instance support is requested, but %s is empty",
		      VAR_MULTI_WRAPPER);
	my_argv = argv_split(var_multi_wrapper, CHARS_SPACE);
	do {
	    argv_add(my_argv, argv[optind], (char *) 0);
	} while (argv[optind++] != 0);
	execvp(my_argv->argv[0], my_argv->argv);
	msg_fatal("%s: %m", my_argv->argv[0]);
    }
}
