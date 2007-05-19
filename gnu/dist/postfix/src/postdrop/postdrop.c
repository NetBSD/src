/*	$NetBSD: postdrop.c,v 1.1.1.9 2007/05/19 16:28:26 heas Exp $	*/

/*++
/* NAME
/*	postdrop 1
/* SUMMARY
/*	Postfix mail posting utility
/* SYNOPSIS
/*	\fBpostdrop\fR [\fB-rv\fR] [\fB-c \fIconfig_dir\fR]
/* DESCRIPTION
/*	The \fBpostdrop\fR(1) command creates a file in the \fBmaildrop\fR
/*	directory and copies its standard input to the file.
/*
/*	Options:
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory. See also the
/*	MAIL_CONFIG environment setting below.
/* .IP \fB-r\fR
/*	Use a Postfix-internal protocol for reading the message from
/*	standard input, and for reporting status information on standard
/*	output. This is currently the only supported method.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose. As of Postfix 2.3,
/*	this option is available for the super-user only.
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
/*	of set-group ID privileges, a non-standard directory is allowed only
/*	if:
/* .RS
/* .IP \(bu
/*	The name is listed in the standard \fBmain.cf\fR file with the
/*	\fBalternate_config_directories\fR configuration parameter.
/* .IP \(bu
/*	The command is invoked by the super-user.
/* .RE
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBalternate_config_directories (empty)\fR"
/*	A list of non-default Postfix configuration directories that may
/*	be specified with "-c config_directory" on the command line, or
/*	via the MAIL_CONFIG environment parameter.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment parameters that a Postfix process will
/*	import from a non-Postfix parent process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .IP "\fBtrigger_timeout (10s)\fR"
/*	The time limit for sending a trigger to a Postfix daemon (for
/*	example, the \fBpickup\fR(8) or \fBqmgr\fR(8) daemon).
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBauthorized_submit_users (static:anyone)\fR"
/*	List of users who are authorized to submit mail with the \fBsendmail\fR(1)
/*	command (and with the privileged \fBpostdrop\fR(1) helper command).
/* FILES
/*	/var/spool/postfix/maildrop, maildrop queue
/* SEE ALSO
/*	sendmail(1), compatibility interface
/*	postconf(5), configuration parameters
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
#include <stringops.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_queue.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_conf.h>
#include <mail_task.h>
#include <clean_env.h>
#include <mail_stream.h>
#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>
#include <user_acl.h>
#include <rec_attr_map.h>

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
  * Local mail submission access list.
  */
char   *var_submit_acl;

static CONFIG_STR_TABLE str_table[] = {
    VAR_SUBMIT_ACL, DEF_SUBMIT_ACL, &var_submit_acl, 0, 0,
    0,
};

 /*
  * Queue file name. Global, so that the cleanup routine can find it when
  * called by the run-time error handler.
  */
static char *postdrop_path;

/* postdrop_sig - catch signal and clean up */

static void postdrop_sig(int sig)
{

    /*
     * This is the fatal error handler. Don't try to do anything fancy.
     * 
     * msg_vstream does not allocate memory, but msg_syslog may indirectly in
     * syslog(), so it should not be called from a user-triggered signal
     * handler.
     * 
     * Assume atomic signal() updates, even when emulated with sigaction(). We
     * use the in-kernel SIGINT handler address as an atomic variable to
     * prevent nested postdrop_sig() calls. For this reason, main() must
     * configure postdrop_sig() as SIGINT handler before other signal
     * handlers are allowed to invoke postdrop_sig().
     */
    if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	if (postdrop_path) {
	    (void) remove(postdrop_path);
	    postdrop_path = 0;
	}
	/* Future proofing. If you need exit() here then you broke Postfix. */
	if (sig)
	    _exit(sig);
    }
}

/* postdrop_cleanup - callback for the runtime error handler */

static void postdrop_cleanup(void)
{
    postdrop_sig(0);
}

MAIL_VERSION_STAMP_DECLARE;

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
    const char *error_text;
    char   *attr_name;
    char   *attr_value;
    const char *errstr;
    char   *junk;
    struct timeval start;
    int     saved_errno;

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
     * Set up logging. Censor the process name: it is provided by the user.
     */
    argv[0] = "postdrop";
    msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(mail_task("postdrop"), LOG_PID, LOG_FACILITY);
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
	    if (geteuid() == 0)
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
    if (strcmp(var_syslog_name, DEF_SYSLOG_NAME) != 0)
	msg_syslog_init(mail_task("postdrop"), LOG_PID, LOG_FACILITY);
    get_mail_conf_str_table(str_table);

    /*
     * Mail submission access control. Should this be in the user-land gate,
     * or in the daemon process?
     */
    if ((errstr = check_user_acl_byuid(var_submit_acl, uid)) != 0)
	msg_fatal("User %s(%ld) is not allowed to submit mail",
		  errstr, (long) uid);

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
     * 
     * postdrop_sig() uses the in-kernel SIGINT handler address as an atomic
     * variable to prevent nested postdrop_sig() calls. For this reason, the
     * SIGINT handler must be configured before other signal handlers are
     * allowed to invoke postdrop_sig().
     */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);

    signal(SIGINT, postdrop_sig);
    signal(SIGQUIT, postdrop_sig);
    signal(SIGTERM, postdrop_sig);
    if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
	signal(SIGHUP, postdrop_sig);
    msg_cleanup(postdrop_cleanup);

    /* End of initializations. */

    /*
     * Don't trust the caller's time information.
     */
    GETTIMEOFDAY(&start);

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
     * 
     * Allow attribute records if the attribute specifies the MIME body type
     * (sendmail -B).
     */
    vstream_control(VSTREAM_IN, VSTREAM_CTL_PATH, "stdin", VSTREAM_CTL_END);
    buf = vstring_alloc(100);
    expected = segment_info;
    /* Override time information from the untrusted caller. */
    rec_fprintf(dst->stream, REC_TYPE_TIME, REC_TYPE_TIME_FORMAT,
		REC_TYPE_TIME_ARG(start));
    for (;;) {
	/* Don't allow PTR records. */
	rec_type = rec_get_raw(VSTREAM_IN, buf, var_line_limit, REC_FLAG_NONE);
	if (rec_type == REC_TYPE_EOF) {		/* request cancelled */
	    mail_stream_cleanup(dst);
	    if (remove(postdrop_path))
		msg_warn("uid=%ld: remove %s: %m", (long) uid, postdrop_path);
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
	/* Override time information from the untrusted caller. */
	if (rec_type == REC_TYPE_TIME)
	    continue;
	if (rec_type == REC_TYPE_ATTR) {
	    if ((error_text = split_nameval(vstring_str(buf), &attr_name,
					    &attr_value)) != 0) {
		msg_warn("uid=%ld: ignoring malformed record: %s: %.200s",
			 (long) uid, error_text, vstring_str(buf));
		continue;
	    }
#define STREQ(x,y) (strcmp(x,y) == 0)

	    if ((STREQ(attr_name, MAIL_ATTR_ENCODING)
		 && (STREQ(attr_value, MAIL_ATTR_ENC_7BIT)
		     || STREQ(attr_value, MAIL_ATTR_ENC_8BIT)
		     || STREQ(attr_value, MAIL_ATTR_ENC_NONE)))
		|| STREQ(attr_name, MAIL_ATTR_DSN_ENVID)
		|| STREQ(attr_name, MAIL_ATTR_DSN_NOTIFY)
		|| rec_attr_map(attr_name)
		|| (STREQ(attr_name, MAIL_ATTR_RWR_CONTEXT)
		    && (STREQ(attr_value, MAIL_ATTR_RWR_LOCAL)
			|| STREQ(attr_value, MAIL_ATTR_RWR_REMOTE)))
		|| STREQ(attr_name, MAIL_ATTR_TRACE_FLAGS)) {	/* XXX */
		rec_fprintf(dst->stream, REC_TYPE_ATTR, "%s=%s",
			    attr_name, attr_value);
	    } else {
		msg_warn("uid=%ld: ignoring attribute record: %.200s=%.200s",
			 (long) uid, attr_name, attr_value);
	    }
	    continue;
	}
	if (REC_PUT_BUF(dst->stream, rec_type, buf) < 0) {
	    /* rec_get() errors must not clobber errno. */
	    saved_errno = errno;
	    while (rec_get_raw(VSTREAM_IN, buf, var_line_limit,
			       REC_FLAG_NONE) > 0)
		 /* void */ ;
	    errno = saved_errno;
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
	msg_warn("uid=%ld: %m", (long) uid);
	postdrop_cleanup();
    }

    /*
     * Disable deletion on fatal error before reporting success, so the file
     * will not be deleted after we have taken responsibility for delivery.
     */
    if (postdrop_path) {
	junk = postdrop_path;
	postdrop_path = 0;
	myfree(junk);
    }

    /*
     * Send the completion status to the caller and terminate.
     */
    attr_print(VSTREAM_OUT, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_STATUS, status,
	       ATTR_TYPE_STR, MAIL_ATTR_WHY, "",
	       ATTR_TYPE_END);
    vstream_fflush(VSTREAM_OUT);
    exit(status);
}
