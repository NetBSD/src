/*++
/* NAME
/*	postlog 1
/* SUMMARY
/*	Postfix-compatible logging utility
/* SYNOPSIS
/* .fi
/*	\fBpostlog\fR [\fB-iv\fR] [\fB-c \fIconfig_dir\fR]
/*		[\fB-p \fIpriority\fB] [\fB-t \fItag\fR] [\fItext...\fR]
/* DESCRIPTION
/*	The \fBpostlog\fR command implements a Postfix-compatible logging
/*	interface for use in, for example, shell scripts.
/*
/*	By default, \fBpostlog\fR logs the \fItext\fR given on the command
/*	line as one record. If no \fItext\fR is specified on the command
/*	line, \fBpostlog\fR reads from standard input and logs each input
/*	line as one record.
/*
/*	Logging is sent to \fBsyslogd\fR(8); when the standard error stream
/*	is connected to a terminal, logging is sent there as well.
/*
/*	The following options are implemented:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR configuration file in the named directory
/*	instead of the default configuration directory.
/* .IP \fB-i\fR
/*	Include the process ID in the logging tag.
/* .IP "\fB-p \fIpriority\fR"
/*	Specifies the logging severity: \fBinfo\fR (default), \fBwarn\fR,
/*	\fBerror\fR, \fBfatal\fR, or \fBpanic\fR.
/* .IP "\fB-t \fItag\fR"
/*	Specifies the logging tag, that is, the identifying name that
/*	appears at the beginning of each logging record.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* SEE ALSO
/*	syslogd(8) syslog daemon.
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
#include <string.h>
#include <syslog.h>
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
#include <msg_syslog.h>

/* Global library. */

#include <mail_params.h>		/* XXX right place for LOG_FACILITY? */
#include <mail_conf.h>

/* Application-specific. */

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
    msg_text(level, vstring_str(buf));
    vstring_free(buf);
}

/* log_stream - log lines from a stream */

static void log_stream(int level, VSTREAM *fp)
{
    VSTRING *buf = vstring_alloc(100);

    while (vstring_get_nonl(buf, fp) != VSTREAM_EOF)
	msg_text(level, vstring_str(buf));
    vstring_free(buf);
}

/* main - logger */

int     main(int argc, char **argv)
{
    struct stat st;
    char   *slash;
    int     fd;
    int     ch;
    char   *tag;
    int     log_flags = 0;
    int     level = MSG_INFO;

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
     * Set up diagnostics.
     */
    if ((slash = strrchr(argv[0], '/')) != 0)
	tag = slash + 1;
    else
	tag = argv[0];
    if (isatty(STDERR_FILENO))
	msg_vstream_init(tag, VSTREAM_ERR);
    msg_syslog_init(tag, LOG_PID, LOG_FACILITY);

    /*
     * Parse switches.
     */
    while ((ch = GETOPT(argc, argv, "c:ip:t:v")) > 0) {
	switch (ch) {
	default:
	    msg_fatal("usage: %s [-c config_dir] [-i] [-p priority] [-t tag] [-v] [text]", tag);
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'i':
	    log_flags |= LOG_PID;
	    break;
	case 'p':
	    level = level_map(optarg);
	    break;
	case 't':
	    tag = optarg;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }

    /*
     * Re-initialize the logging, this time with the user-specified tag and
     * severity level.
     */
    if (isatty(STDERR_FILENO))
	msg_vstream_init(tag, VSTREAM_ERR);
    msg_syslog_init(tag, log_flags, LOG_FACILITY);

    /*
     * Process the main.cf file. This overrides any logging facility that was
     * specified with msg_syslog_init();
     */
    mail_conf_read();

    /*
     * Log the command line or log lines from standard input.
     */
    if (argc > optind) {
	log_argv(level, argv + optind);
    } else {
	log_stream(level, VSTREAM_IN);
    }
    exit(0);
}
