/*++
/* NAME
/*	postdrop 1
/* SUMMARY
/*	Postfix mail posting utility
/* SYNOPSIS
/*	\fBpostdrop\fR [\fB-rv\fR] [\fB-c \fIconfig_dir\fR]
/* DESCRIPTION
/*	The \fBpostdrop\fR command creates a file in the \fBmaildrop\fR
/*	directory and copies its standard input to the file.
/*
/*	Options:
/* .IP \fB-c \fIconfig_dir\fR
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory. See also the
/*	MAIL_CONFIG environment setting below.
/* .IP \fB-r\fR
/*	Use a Postfix-internal protocol for reading the message from
/*	standard input, and for reporting status information on standard
/*	output. This is currently the only supported method.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* SECURITY
/* .ad
/* .fi
/*	The command is designed to run with set-group ID privileges, so
/*	that it can write to the \fBmaildrop\fR queue directory and so that
/*	it can connect to Postfix daemon processes.
/* DIAGNOSTICS
/*	Fatal errors: malformed input, I/O error, out of memory. Problems
/*	are logged to \fBsyslogd\fR(8) and to the standard error stream.
/*	When the input is incomplete, or when the process receives a HUP,
/*	INT, QUIT or TERM signal, the queue file is deleted.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP MAIL_CONFIG
/*	Directory with the \fBmain.cf\fR file. In order to avoid exploitation
/*	of set-group ID privileges, it is not possible to specify arbitrary
/*	directory names.
/*
/*	A non-standard directory is allowed only if the name is listed in the
/*	standard \fBmain.cf\fR file, in the \fBalternate_config_directories\fR
/*	configuration parameter value.
/*
/*	Only the superuser is allowed to specify arbitrary directory names.
/* FILES
/*	/var/spool/postfix, mail queue
/*	/etc/postfix, configuration files
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	See the Postfix \fBmain.cf\fR file for syntax details and for
/*	default values. Use the \fBpostfix reload\fR command after a
/*	configuration change.
/* .IP \fBimport_environment\fR
/*	List of names of environment parameters that can be imported
/*	from non-Postfix processes.
/* .IP \fBqueue_directory\fR
/*	Top-level directory of the Postfix queue. This is also the root
/*	directory of Postfix daemons that run chrooted.
/* SEE ALSO
/*	sendmail(1) compatibility interface
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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>			/* remove() */
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>
#include <msg_vstream.h>
#include <msg_syslog.h>
#include <argv.h>
#include <iostuff.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_task.h>
#include <clean_env.h>
#include <mail_stream.h>
#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>

/* Application-specific. */

 /*
  * WARNING WARNING WARNING
  * 
  * This software is designed to run set-gid. In order to avoid exploitation of
  * privilege, this software should not run any external commands, nor should
  * it take any information from the user unless that information can be
  * properly sanitized. To get an idea of how much information a process can
  * inherit from a potentially hostile user, examine all the members of the
  * process structure (typically, in /usr/include/sys/proc.h): the current
  * directory, open files, timers, signals, environment, command line, umask,
  * and so on.
  */

 /*
  * Queue file name. Global, so that the cleanup routine can find it when
  * called by the run-time error handler.
  */
static char *postdrop_path;

/* postdrop_cleanup - callback for the runtime error handler */

static void postdrop_cleanup(void)
{

    /*
     * This is the fatal error handler. Don't try to do anything fancy.
     */
    if (postdrop_path) {
	if (remove(postdrop_path))
	    msg_warn("uid=%ld: remove %s: %m", (long) getuid(), postdrop_path);
	else if (msg_verbose)
	    msg_info("remove %s", postdrop_path);
	postdrop_path = 0;
    }
}

/* postdrop_sig - catch signal and clean up */

static void postdrop_sig(int sig)
{

    /*
     * Assume atomic signal() updates, even when emulated with sigaction().
     */
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN
	&& signal(SIGINT, SIG_IGN) != SIG_IGN
	&& signal(SIGQUIT, SIG_IGN) != SIG_IGN
	&& signal(SIGTERM, SIG_IGN) != SIG_IGN) {
	postdrop_cleanup();
	exit(sig);
    }
}

/* main - the main program */

int     main(int argc, char **argv)
{
    struct stat st;
    int     fd;
    int     c;
    VSTRING *buf;
    int     status;
    MAIL_STREAM *dst;
    int     rec_type;
    static char *segment_info[] = {
	REC_TYPE_POST_ENVELOPE, REC_TYPE_POST_CONTENT, REC_TYPE_POST_EXTRACT, ""
    };
    char  **expected;
    uid_t   uid = getuid();
    ARGV   *import_env;

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
     * Set up logging. Censor the process name: it is provided by the user.
     */
    argv[0] = "postdrop";
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(mail_task(argv[0]), LOG_PID, LOG_FACILITY);
    set_mail_conf_str(VAR_PROCNAME, var_procname = mystrdup(argv[0]));

    /*
     * Parse JCL. This program is set-gid and must sanitize all command-line
     * arguments. The configuration directory argument is validated by the
     * mail configuration read routine. Don't do complex things until we have
     * completed initializations.
     */
    while ((c = GETOPT(argc, argv, "c:rv")) > 0) {
	switch (c) {
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'r':				/* forward compatibility */
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    msg_fatal("usage: %s [-c config_dir] [-v]", argv[0]);
	}
    }

    /*
     * Read the global configuration file and extract configuration
     * information. Some claim that the user should supply the working
     * directory instead. That might be OK, given that this command needs
     * write permission in a subdirectory called "maildrop". However we still
     * need to reliably detect incomplete input, and so we must perform
     * record-level I/O. With that, we should also take the opportunity to
     * perform some sanity checks on the input.
     */
    mail_conf_read();

    /*
     * Stop run-away process accidents by limiting the queue file size. This
     * is not a defense against DOS attack.
     */
    if (var_message_limit > 0 && get_file_limit() > var_message_limit)
	set_file_limit((off_t) var_message_limit);

    /*
     * Strip the environment so we don't have to trust the C library.
     */
    import_env = argv_split(var_import_environ, ", \t\r\n");
    clean_env(import_env->argv);
    argv_free(import_env);

    if (chdir(var_queue_dir))
	msg_fatal("chdir %s: %m", var_queue_dir);
    if (msg_verbose)
	msg_info("chdir %s", var_queue_dir);

    /*
     * Set up signal handlers and a runtime error handler so that we can
     * clean up incomplete output.
     */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);

    signal(SIGHUP, postdrop_sig);
    signal(SIGINT, postdrop_sig);
    signal(SIGQUIT, postdrop_sig);
    signal(SIGTERM, postdrop_sig);
    msg_cleanup(postdrop_cleanup);

    /* End of initializations. */

    /*
     * Create queue file. mail_stream_file() never fails. Send the queue ID
     * to the caller. Stash away a copy of the queue file name so we can
     * clean up in case of a fatal error or an interrupt.
     */
    dst = mail_stream_file(MAIL_QUEUE_MAILDROP, MAIL_CLASS_PUBLIC,
			   var_pickup_service, 0444);
    attr_print(VSTREAM_OUT, ATTR_FLAG_NONE,
	       ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, dst->id,
	       ATTR_TYPE_END);
    vstream_fflush(VSTREAM_OUT);
    postdrop_path = mystrdup(VSTREAM_PATH(dst->stream));

    /*
     * Copy stdin to file. The format is checked so that we can recognize
     * incomplete input and cancel the operation. With the sanity checks
     * applied here, the pickup daemon could skip format checks and pass a
     * file descriptor to the cleanup daemon. These are by no means all
     * sanity checks - the cleanup service and queue manager services will
     * reject messages that lack required information.
     * 
     * If something goes wrong, slurp up the input before responding to the
     * client, otherwise the client will give up after detecting SIGPIPE.
     */
    vstream_control(VSTREAM_IN, VSTREAM_CTL_PATH, "stdin", VSTREAM_CTL_END);
    buf = vstring_alloc(100);
    expected = segment_info;
    for (;;) {
	rec_type = rec_get(VSTREAM_IN, buf, var_line_limit);
	if (rec_type == REC_TYPE_EOF) {		/* request cancelled */
	    mail_stream_cleanup(dst);
	    if (remove(postdrop_path))
		msg_warn("uid=%ld: remove %s: %m", (long) getuid(), postdrop_path);
	    else if (msg_verbose)
		msg_info("remove %s", postdrop_path);
	    myfree(postdrop_path);
	    postdrop_path = 0;
	    exit(0);
	}
	if (rec_type == REC_TYPE_ERROR)
	    msg_fatal("uid=%ld: malformed input", (long) uid);
	if (strchr(*expected, rec_type) == 0)
	    msg_fatal("uid=%ld: unexpected record type: %d", (long) uid, rec_type);
	if (rec_type == **expected)
	    expected++;
	if (REC_PUT_BUF(dst->stream, rec_type, buf) < 0) {
	    while ((rec_type = rec_get(VSTREAM_IN, buf, var_line_limit)) > 0
		   && rec_type != REC_TYPE_END)
		 /* void */ ;
	    break;
	}
	if (rec_type == REC_TYPE_END)
	    break;
    }
    vstring_free(buf);

    /*
     * Finish the file.
     */
    if ((status = mail_stream_finish(dst, (VSTRING *) 0)) != 0) {
	postdrop_cleanup();
	msg_warn("uid=%ld: %m", (long) uid);
    }

    /*
     * Disable deletion on fatal error before reporting success, so the file
     * will not be deleted after we have taken responsibility for delivery.
     */
    if (postdrop_path) {
	myfree(postdrop_path);
	postdrop_path = 0;
    }

    /*
     * Send the completion status to the caller and terminate.
     */
    attr_print(VSTREAM_OUT, ATTR_FLAG_NONE,
	       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, status,
	       ATTR_TYPE_STR, MAIL_ATTR_WHY, "",
	       ATTR_TYPE_END);
    vstream_fflush(VSTREAM_OUT);
    exit(status);
}
