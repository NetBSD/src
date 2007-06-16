/*	$NetBSD: postkick.c,v 1.1.1.5.4.1 2007/06/16 17:00:41 snj Exp $	*/

/*++
/* NAME
/*	postkick 1
/* SUMMARY
/*	kick a Postfix service
/* SYNOPSIS
/* .fi
/*	\fBpostkick\fR [\fB-c \fIconfig_dir\fR] [\fB-v\fR]
/*	\fIclass service request\fR
/* DESCRIPTION
/*	The \fBpostkick\fR(1) command sends \fIrequest\fR to the
/*	specified \fIservice\fR over a local transport channel.
/*	This command makes Postfix private IPC accessible
/*	for use in, for example, shell scripts.
/*
/*	Options:
/* .IP "\fB-c\fR \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR configuration file in the named directory
/*	instead of the default configuration directory.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* .PP
/*	Arguments:
/* .IP \fIclass\fR
/*	Name of a class of local transport channel endpoints,
/*	either \fBpublic\fR (accessible by any local user) or
/*	\fBprivate\fR (administrative access only).
/* .IP \fIservice\fR
/*	The name of a local transport endpoint within the named class.
/* .IP \fIrequest\fR
/*	A string. The list of valid requests is service-specific.
/* DIAGNOSTICS
/*	Problems and transactions are logged to the standard error
/*	stream.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* .IP \fBMAIL_VERBOSE\fR
/*	Enable verbose logging for debugging purposes.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBapplication_event_drain_time (100s)\fR"
/*	How long the \fBpostkick\fR(1) command waits for a request to enter the
/*	server's input buffer before giving up.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* FILES
/*	/var/spool/postfix/private, private class endpoints
/*	/var/spool/postfix/public, public class endpoints
/* SEE ALSO
/*	qmgr(8), queue manager trigger protocol
/*	pickup(8), local pickup daemon
/*	postconf(5), configuration parameters
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
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <safe.h>
#include <events.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_conf.h>

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-c config_dir] [-v] class service request", myname);
}

MAIL_VERSION_STAMP_DECLARE;

int     main(int argc, char **argv)
{
    char   *class;
    char   *service;
    char   *request;
    int     fd;
    struct stat st;
    char   *slash;
    int     c;

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

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
     * Process environment options as early as we can.
     */
    if (safe_getenv(CONF_ENV_VERB))
	msg_verbose = 1;

    /*
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information.
     */
    if ((slash = strrchr(argv[0], '/')) != 0 && slash[1])
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    /*
     * Parse JCL.
     */
    while ((c = GETOPT(argc, argv, "c:v")) > 0) {
	switch (c) {
	default:
	    usage(argv[0]);
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    if (argc != optind + 3)
	usage(argv[0]);
    class = argv[optind];
    service = argv[optind + 1];
    request = argv[optind + 2];

    /*
     * Finish initializations.
     */
    mail_conf_read();
    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);

    /*
     * Kick the service.
     */
    if (mail_trigger(class, service, request, strlen(request)) < 0) {
	msg_warn("Cannot contact class %s service %s - perhaps the mail system is down",
		 class, service);
	exit(1);
    }

    /*
     * Problem: With triggers over full duplex (i.e. non-FIFO) channels, we
     * must avoid closing the channel before the server has received the
     * request. Otherwise some hostile kernel may throw away the request.
     * 
     * Solution: The trigger routine registers a read event handler that runs
     * when the server closes the channel. The event_drain() routine waits
     * for the event handler to run, but gives up when it takes too long.
     */
    else {
	event_drain(var_event_drain);
	exit(0);
    }
}
