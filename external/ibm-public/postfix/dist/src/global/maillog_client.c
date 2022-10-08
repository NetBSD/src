/*	$NetBSD: maillog_client.c,v 1.3 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	maillog_client 3
/* SUMMARY
/*	choose between syslog client and postlog client
/* SYNOPSIS
/*	#include <maillog_client.h>
/*
/*	int	maillog_client_init(
/*	const char *progname,
/*	int	flags)
/* DESCRIPTION
/*	maillog_client_init() chooses between logging to the syslog
/*	service or to the internal postlog service.
/*
/*	maillog_client_init() may be called before configuration
/*	parameters are initialized. During this time, logging is
/*	controlled by the presence or absence of POSTLOG_SERVICE
/*	in the process environment (this is ignored if a program
/*	runs with set-uid or set-gid permissions).
/*
/*	maillog_client_init() may also be called after configuration
/*	parameters are initialized. During this time, logging is
/*	controlled by the "maillog_file" parameter value.
/*
/*	Arguments:
/* .IP progname
/*	The program name that will be prepended to logfile records.
/* .IP flags
/*	Specify one of the following:
/* .RS
/* .IP MAILLOG_CLIENT_FLAG_NONE
/*	No special processing.
/* .IP MAILLOG_CLIENT_FLAG_LOGWRITER_FALLBACK
/*	Try to fall back to writing the "maillog_file" directly,
/*	if logging to the internal postlog service is enabled, but
/*	the postlog service is unavailable. If the fallback fails,
/*	die with a fatal error.
/* .RE
/* ENVIRONMENT
/* .ad
/* .fi
/*	When logging to the internal postlog service is enabled,
/*	each process exports the following information, to help
/*	initialize the logging in a child process, before the child
/*	has initialized its configuration parameters.
/* .IP POSTLOG_SERVICE
/*	The pathname of the public postlog service endpoint, usually
/*	"$queue_directory/public/$postlog_service_name".
/* .IP POSTLOG_HOSTNAME
/*	The hostname to prepend to information that is sent to the
/*	internal postlog logging service, usually "$myhostname".
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/* .IP "maillog_file (empty)"
/*	The name of an optional logfile. If the value is empty, or
/*	unitialized and the process environment does not specify
/*	POSTLOG_SERVICE, the program will log to the syslog service
/*	instead.
/* .IP "myhostname (default: see postconf -d output)"
/*	The internet hostname of this mail system.
/* .IP "postlog_service_name (postlog)"
/*	The name of the internal postlog logging service.
/* SEE ALSO
/*	msg_syslog(3)   syslog client
/*	msg_logger(3)   internal logger
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this
/*	software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <logwriter.h>
#include <msg_logger.h>
#include <msg_syslog.h>
#include <safe.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_proto.h>
#include <maillog_client.h>
#include <msg.h>

 /*
  * Using logging to debug logging is painful.
  */
#define MAILLOG_CLIENT_DEBUG	0

 /*
  * Application-specific.
  */
static int maillog_client_flags;

#define POSTLOG_SERVICE_ENV	"POSTLOG_SERVICE"
#define POSTLOG_HOSTNAME_ENV	"POSTLOG_HOSTNAME"

/* maillog_client_logwriter_fallback - fall back to logfile writer or bust */

static void maillog_client_logwriter_fallback(const char *text)
{
    static int fallback_guard = 0;

    /*
     * Guard against recursive calls.
     * 
     * If an error happened before the maillog_file parameter was initialized,
     * or if maillog_file logging is disabled, then we cannot fall back to a
     * logfile. All we can do is to hope that stderr logging will bring out
     * the bad news.
     */
    if (fallback_guard == 0 && var_maillog_file && *var_maillog_file
	&& logwriter_one_shot(var_maillog_file, text, strlen(text)) < 0) {
	fallback_guard = 1;
	msg_fatal("logfile '%s' write error: %m", var_maillog_file);
    }
}

/* maillog_client_init - set up syslog or internal log client */

void    maillog_client_init(const char *progname, int flags)
{
    char   *import_service_path;
    char   *import_hostname;

    /*
     * Crucially, only one logger mode can be in effect at any time,
     * otherwise postlogd(8) may go into a loop.
     */
    enum {
	MAILLOG_CLIENT_MODE_SYSLOG, MAILLOG_CLIENT_MODE_POSTLOG,
    }       logger_mode;

    /*
     * Security: this code may run before the import_environment setting has
     * taken effect. It has to guard against privilege escalation attacks on
     * setgid programs, using malicious environment settings.
     * 
     * Import the postlog service name and hostname from the environment.
     * 
     * - These will be used and kept if the process has not yet initialized its
     * configuration parameters.
     * 
     * - These will be set or updated if the configuration enables postlog
     * logging.
     * 
     * - These will be removed if the configuration does not enable postlog
     * logging.
     */
    if ((import_service_path = safe_getenv(POSTLOG_SERVICE_ENV)) != 0
	&& *import_service_path == 0)
	import_service_path = 0;
    if ((import_hostname = safe_getenv(POSTLOG_HOSTNAME_ENV)) != 0
	&& *import_hostname == 0)
	import_hostname = 0;

#if MAILLOG_CLIENT_DEBUG
#define STRING_OR_NULL(s) ((s) ? (s) : "(null)")
    msg_syslog_init(progname, LOG_PID, LOG_FACILITY);
    msg_info("import_service_path=%s", STRING_OR_NULL(import_service_path));
    msg_info("import_hostname=%s", STRING_OR_NULL(import_hostname));
#endif

    /*
     * Before configuration parameters are initialized, the logging mode is
     * controlled by the presence or absence of POSTLOG_SERVICE in the
     * process environment. After configuration parameters are initialized,
     * the logging mode is controlled by the "maillog_file" parameter value.
     * 
     * The configured mode may change after a process is started. The
     * postlogd(8) server will proxy logging to syslogd where needed.
     */
    if (var_maillog_file ? *var_maillog_file == 0 : import_service_path == 0) {
	logger_mode = MAILLOG_CLIENT_MODE_SYSLOG;
    } else {
	/* var_maillog_file ? *var_maillog_file : import_service_path != 0 */
	logger_mode = MAILLOG_CLIENT_MODE_POSTLOG;
    }

    /*
     * Postlog logging is enabled. Update the 'progname' as that may have
     * changed since an earlier call, and update the environment settings if
     * they differ from configuration settings. This blends two code paths,
     * one code path where configuration parameters are initialized (the
     * preferred path), and one code path that uses imports from environment.
     */
    if (logger_mode == MAILLOG_CLIENT_MODE_POSTLOG) {
	char   *myhostname;
	char   *service_path;

	if (var_maillog_file && *var_maillog_file) {
	    ARGV   *good_prefixes = argv_split(var_maillog_file_pfxs,
					       CHARS_COMMA_SP);
	    char  **cpp;

	    for (cpp = good_prefixes->argv; /* see below */ ; cpp++) {
		if (*cpp == 0)
		    msg_fatal("%s value '%s' does not match any prefix in %s",
			      VAR_MAILLOG_FILE, var_maillog_file,
			      VAR_MAILLOG_FILE_PFXS);
		if (strncmp(var_maillog_file, *cpp, strlen(*cpp)) == 0)
		    break;
	    }
	    argv_free(good_prefixes);
	}
	if (var_myhostname && *var_myhostname) {
	    myhostname = var_myhostname;
	} else if ((myhostname = import_hostname) == 0) {
	    myhostname = "amnesiac";
	}
#if MAILLOG_CLIENT_DEBUG
	msg_info("myhostname=%s", STRING_OR_NULL(myhostname));
#endif
	if (var_postlog_service) {
	    service_path = concatenate(var_queue_dir, "/", MAIL_CLASS_PUBLIC,
				       "/", var_postlog_service, (char *) 0);
	} else {

	    /*
	     * var_postlog_service == 0, therefore var_maillog_file == 0.
	     * logger_mode == MAILLOG_CLIENT_MODE_POSTLOG && var_maillog_file ==
	     * 0, therefore import_service_path != 0.
	     */
	    service_path = import_service_path;
	}
	maillog_client_flags = flags;
	msg_logger_init(progname, myhostname, service_path,
			(flags & MAILLOG_CLIENT_FLAG_LOGWRITER_FALLBACK) ?
			maillog_client_logwriter_fallback :
			(MSG_LOGGER_FALLBACK_FN) 0);

	/*
	 * Export or update the exported postlog service pathname and the
	 * hostname, so that a child process can bootstrap postlog logging
	 * before it has processed main.cf and command-line options.
	 */
	if (import_service_path == 0
	    || strcmp(service_path, import_service_path) != 0) {
#if MAILLOG_CLIENT_DEBUG
	    msg_info("export %s=%s", POSTLOG_SERVICE_ENV, service_path);
#endif
	    if (setenv(POSTLOG_SERVICE_ENV, service_path, 1) < 0)
		msg_fatal("setenv: %m");
	}
	if (import_hostname == 0 || strcmp(myhostname, import_hostname) != 0) {
#if MAILLOG_CLIENT_DEBUG
	    msg_info("export %s=%s", POSTLOG_HOSTNAME_ENV, myhostname);
#endif
	    if (setenv(POSTLOG_HOSTNAME_ENV, myhostname, 1) < 0)
		msg_fatal("setenv: %m");
	}
	if (service_path != import_service_path)
	    myfree(service_path);
	msg_logger_control(CA_MSG_LOGGER_CTL_CONNECT_NOW,
			   CA_MSG_LOGGER_CTL_END);
    }

    /*
     * Postlog logging is disabled. Silence the msg_logger client, and remove
     * the environment settings that bootstrap postlog logging in a child
     * process.
     */
    else {
	msg_logger_control(CA_MSG_LOGGER_CTL_DISABLE, CA_MSG_LOGGER_CTL_END);
	if ((import_service_path && unsetenv(POSTLOG_SERVICE_ENV))
	    || (import_hostname && unsetenv(POSTLOG_HOSTNAME_ENV)))
	    msg_fatal("unsetenv: %m");
    }

    /*
     * Syslog logging is enabled. Update the 'progname' as that may have
     * changed since an earlier call.
     */
    if (logger_mode == MAILLOG_CLIENT_MODE_SYSLOG) {
	msg_syslog_init(progname, LOG_PID, LOG_FACILITY);
    }

    /*
     * Syslog logging is disabled, silence the syslog client.
     */
    else {
	msg_syslog_disable();
    }
}
