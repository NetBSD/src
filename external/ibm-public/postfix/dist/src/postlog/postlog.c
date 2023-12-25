/*	$NetBSD: postlog.c,v 1.4.2.1 2023/12/25 12:43:33 martin Exp $	*/

/*++
/* NAME
/*	postlog 1
/* SUMMARY
/*	Postfix-compatible logging utility
/* SYNOPSIS
/* .fi
/* .ad
/*	\fBpostlog\fR [\fB-iv\fR] [\fB-c \fIconfig_dir\fR]
/*	[\fB-p \fIpriority\fR] [\fB-t \fItag\fR] [\fItext...\fR]
/* DESCRIPTION
/*	The \fBpostlog\fR(1) command implements a Postfix-compatible logging
/*	interface for use in, for example, shell scripts.
/*
/*	By default, \fBpostlog\fR(1) logs the \fItext\fR given on the command
/*	line as one record. If no \fItext\fR is specified on the command
/*	line, \fBpostlog\fR(1) reads from standard input and logs each input
/*	line as one record.
/*
/*	By default, logging is sent to \fBsyslogd\fR(8) or
/*	\fBpostlogd\fR(8); when the
/*	standard error stream is connected to a terminal, logging
/*	is sent there as well.
/*
/*	The following options are implemented:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR configuration file in the named directory
/*	instead of the default configuration directory.
/* .IP "\fB-i\fR (obsolete)"
/*	Include the process ID in the logging tag. This flag is ignored as
/*	of Postfix 3.4, where the PID is always included.
/* .IP "\fB-p \fIpriority\fR (default: \fBinfo\fR)"
/*	Specifies the logging severity: \fBinfo\fR, \fBwarn\fR,
/*	\fBerror\fR, \fBfatal\fR, or \fBpanic\fR. With Postfix 3.1
/*	and later, the program will pause for 1 second after reporting
/*	a \fBfatal\fR or \fBpanic\fR condition, just like other
/*	Postfix programs.
/* .IP "\fB-t \fItag\fR"
/*	Specifies the logging tag, that is, the identifying name that
/*	appears at the beginning of each logging record. A default tag
/*	is used when none is specified.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* SECURITY
/* .ad
/* .fi
/*	The \fBpostlog\fR(1) command is designed to run with
/*	set-groupid privileges, so that it can connect to the
/*	\fBpostlogd\fR(8) daemon process (Postfix 3.7 and later;
/*	earlier implementations of this command must not have
/*	set-groupid or set-userid permissions).
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP MAIL_CONFIG
/*	Directory with the \fBmain.cf\fR file.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment variables that a privileged Postfix
/*	process will import from a non-Postfix parent process, or name=value
/*	environment overrides.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .PP
/*	Available in Postfix 3.4 and later:
/* .IP "\fBmaillog_file (empty)\fR"
/*	The name of an optional logfile that is written by the Postfix
/*	\fBpostlogd\fR(8) service.
/* .IP "\fBpostlog_service_name (postlog)\fR"
/*	The name of the \fBpostlogd\fR(8) service entry in master.cf.
/* SEE ALSO
/*	postconf(5), configuration parameters
/*	postlogd(8), Postfix logging
/*	syslogd(8), system logging
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/*	The \fBpostlog\fR(1) command was introduced with Postfix
/*	version 3.4.
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_output.h>
#include <msg_vstream.h>
#include <warn_stat.h>
#include <clean_env.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>		/* XXX right place for LOG_FACILITY? */
#include <mail_version.h>
#include <mail_conf.h>
#include <mail_task.h>
#include <mail_parm_split.h>
#include <maillog_client.h>

/* Application-specific. */

 /*
  * WARNING WARNING WARNING
  * 
  * This software is designed to run set-gid. In order to avoid exploitation of
  * privilege, this software should not run any external commands, nor should
  * it take any information from the user, unless that information can be
  * properly sanitized. To get an idea of how much information a process can
  * inherit from a potentially hostile user, examine all the members of the
  * process structure (typically, in /usr/include/sys/proc.h): the current
  * directory, open files, timers, signals, environment, command line, umask,
  * and so on.
  */

 /*
  * Access lists.
  */
#if 0
char   *var_postlog_acl;

X static const CONFIG_STR_TABLE str_table[] = {
    X VAR_POSTLOG_ACL, DEF_POSTLOG_ACL, &var_postlog_acl, 0, 0,
    X 0,
X};

#endif

 /*
  * Support for the severity level mapping.
  */
struct level_table {
    char   *name;
    int     level;
};

static struct level_table level_table[] = {
    "info", MSG_INFO,
    "warn", MSG_WARN,
    "warning", MSG_WARN,
    "error", MSG_ERROR,
    "err", MSG_ERROR,
    "fatal", MSG_FATAL,
    "crit", MSG_FATAL,
    "panic", MSG_PANIC,
    0,
};

#define POSTLOG_CMD	"postlog"

/* level_map - lookup facility or severity value */

static int level_map(char *name)
{
    struct level_table *t;

    for (t = level_table; t->name; t++)
	if (strcasecmp(t->name, name) == 0)
	    return (t->level);
    msg_fatal("bad severity: \"%s\"", name);
}

/* log_argv - log the command line */

static void log_argv(int level, char **argv)
{
    VSTRING *buf = vstring_alloc(100);

    while (*argv) {
	vstring_strcat(buf, *argv++);
	if (*argv)
	    vstring_strcat(buf, " ");
    }
    msg_printf(level, "%s", vstring_str(buf));
    vstring_free(buf);
}

/* log_stream - log lines from a stream */

static void log_stream(int level, VSTREAM *fp)
{
    VSTRING *buf = vstring_alloc(100);

    while (vstring_get_nonl(buf, fp) != VSTREAM_EOF)
	msg_printf(level, "%s", vstring_str(buf));
    vstring_free(buf);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - logger */

int     main(int argc, char **argv)
{
    struct stat st;
    int     fd;
    int     ch;
    const char *tag;
    char   *unsanitized_tag;
    int     level = MSG_INFO;
    ARGV   *import_env;

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
     * Set up diagnostics. Censor the process name: it is provided by the
     * user.
     */
    argv[0] = POSTLOG_CMD;
    tag = mail_task(argv[0]);
    msg_vstream_init(tag, VSTREAM_ERR);
    maillog_client_init(tag, MAILLOG_CLIENT_FLAG_LOGWRITER_FALLBACK);

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    /*
     * Parse switches. This program is set-gid and must sanitize all
     * command-line parameters. The configuration directory argument is
     * validated by the mail configuration read routine. Don't do complex
     * things until we have completed initializations.
     */
    unsanitized_tag = 0;
    while ((ch = GETOPT(argc, argv, "c:ip:t:v")) > 0) {
	switch (ch) {
	default:
	    msg_fatal("usage: %s [-c config_dir] [-i] [-p priority] [-t tag] [-v] [text]", argv[0]);
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'i':
	    break;
	case 'p':
	    level = level_map(optarg);
	    break;
	case 't':
	    unsanitized_tag = optarg;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }

    /*
     * Process the main.cf file. This may change the syslog_name setting and
     * may require that mail_task() be re-evaluated.
     */
    mail_conf_read();
    /* Re-evaluate mail_task() after reading main.cf. */
    maillog_client_init(mail_task(POSTLOG_CMD), MAILLOG_CLIENT_FLAG_NONE);
#if 0
    mail_dict_init();				/* proxy, sql, ldap */
    get_mail_conf_str_table(str_table);
#endif

    /*
     * This program is designed to be set-gid, which makes it a potential
     * target for attack. Strip and optionally override the process
     * environment so that we don't have to trust the C library.
     */
    import_env = mail_parm_split(VAR_IMPORT_ENVIRON, var_import_environ);
    clean_env(import_env->argv);
    argv_free(import_env);

    /*
     * Sanitize the user-specified tag. The result depends on the value of
     * var_smtputf8_enable, therefore this code is after the mail_conf_read()
     * call.
     */
    if (unsanitized_tag != 0)
	tag = printable(unsanitized_tag, '?');

    /*
     * Re-initialize the logging, this time with the tag specified in main.cf
     * or on the command line.
     */
    if (isatty(STDERR_FILENO))
	msg_vstream_init(tag, VSTREAM_ERR);
    maillog_client_init(tag, MAILLOG_CLIENT_FLAG_LOGWRITER_FALLBACK);

#if 0
    uid_t   uid = getuid();

    if (uid != 0 && uid != var_owner_uid
	&& (errstr = check_user_acl_byuid(VAR_SHOWQ_ACL, var_showq_acl,
					  uid)) != 0)
	msg_fatal_status(EX_NOPERM,
			 "User %s(%ld) is not allowed to invoke 'postlog'",
			 errstr, (long) uid);
#endif

    /*
     * Log the command line or log lines from standard input.
     */
    if (argc > optind) {
	log_argv(level, argv + optind);
    } else {
	log_stream(level, VSTREAM_IN);
    }

    /*
     * Consistency with msg(3) functions.
     */
    if (level >= MSG_FATAL)
	sleep(1);
    exit(0);
}
