/*	$NetBSD: msg_syslog.c,v 1.1.1.1.2.2 2009/09/15 06:04:00 snj Exp $	*/

/*++
/* NAME
/*	msg_syslog 3
/* SUMMARY
/*	direct diagnostics to syslog daemon
/* SYNOPSIS
/*	#include <msg_syslog.h>
/*
/*	void	msg_syslog_init(progname, log_opt, facility)
/*	const char *progname;
/*	int	log_opt;
/*	int	facility;
/*
/*	int     msg_syslog_facility(facility_name)
/*	const char *facility_name;
/* DESCRIPTION
/*	This module implements support to report msg(3) diagnostics
/*	via the syslog daemon.
/*
/*	msg_syslog_init() is a wrapper around the openlog(3) routine
/*	that directs subsequent msg(3) output to the syslog daemon.
/*
/*	msg_syslog_facility() is a helper routine that overrides the
/*	logging facility that is specified with msg_syslog_init().
/*	The result is zero in case of an unknown facility name.
/* SEE ALSO
/*	syslog(3) syslog library
/*	msg(3)	diagnostics module
/* BUGS
/*	Output records are truncated to 2000 characters. This is done in
/*	order to defend against a buffer overflow problem in some
/*	implementations of the syslog() library routine.
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

/* System libraries. */

#include <sys_defs.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

/* Application-specific. */

#include "vstring.h"
#include "stringops.h"
#include "msg.h"
#include "msg_output.h"
#include "msg_syslog.h"
#include "safe.h"

 /*
  * Stay a little below the 2048-byte limit of older syslog()
  * implementations.
  */
#define MSG_SYSLOG_RECLEN	2000

struct facility_list {
    const char *name;
    int     facility;
};

static struct facility_list facility_list[] = {
#ifdef LOG_AUTH
    "auth", LOG_AUTH,
#endif
#ifdef LOG_AUTHPRIV
    "authpriv", LOG_AUTHPRIV,
#endif
#ifdef LOG_CRON
    "cron", LOG_CRON,
#endif
#ifdef LOG_DAEMON
    "daemon", LOG_DAEMON,
#endif
#ifdef LOG_FTP
    "ftp", LOG_FTP,
#endif
#ifdef LOG_KERN
    "kern", LOG_KERN,
#endif
#ifdef LOG_LPR
    "lpr", LOG_LPR,
#endif
#ifdef LOG_MAIL
    "mail", LOG_MAIL,
#endif
#ifdef LOG_NEWS
    "news", LOG_NEWS,
#endif
#ifdef LOG_SECURITY
    "security", LOG_SECURITY,
#endif
#ifdef LOG_SYSLOG
    "syslog", LOG_SYSLOG,
#endif
#ifdef LOG_USER
    "user", LOG_USER,
#endif
#ifdef LOG_UUCP
    "uucp", LOG_UUCP,
#endif
#ifdef LOG_LOCAL0
    "local0", LOG_LOCAL0,
#endif
#ifdef LOG_LOCAL1
    "local1", LOG_LOCAL1,
#endif
#ifdef LOG_LOCAL2
    "local2", LOG_LOCAL2,
#endif
#ifdef LOG_LOCAL3
    "local3", LOG_LOCAL3,
#endif
#ifdef LOG_LOCAL4
    "local4", LOG_LOCAL4,
#endif
#ifdef LOG_LOCAL5
    "local5", LOG_LOCAL5,
#endif
#ifdef LOG_LOCAL6
    "local6", LOG_LOCAL6,
#endif
#ifdef LOG_LOCAL7
    "local7", LOG_LOCAL7,
#endif
    0,
};

static int syslog_facility;

/* msg_syslog_print - log info to syslog daemon */

static void msg_syslog_print(int level, const char *text)
{
    static int log_level[] = {
	LOG_INFO, LOG_WARNING, LOG_ERR, LOG_CRIT, LOG_CRIT,
    };
    static char *severity_name[] = {
	"info", "warning", "error", "fatal", "panic",
    };

    if (level < 0 || level >= (int) (sizeof(log_level) / sizeof(log_level[0])))
	msg_panic("msg_syslog_print: invalid severity level: %d", level);

    if (level == MSG_INFO) {
	syslog(syslog_facility | log_level[level], "%.*s",
	       (int) MSG_SYSLOG_RECLEN, text);
    } else {
	syslog(syslog_facility | log_level[level], "%s: %.*s",
	       severity_name[level], (int) MSG_SYSLOG_RECLEN, text);
    }
}

/* msg_syslog_init - initialize */

void    msg_syslog_init(const char *name, int logopt, int facility)
{
    static int first_call = 1;

    /*
     * XXX If this program is set-gid, then TZ must not be trusted. This
     * scrubbing code is in the wrong place.
     */
    if (unsafe())
	putenv("TZ=UTC");
    tzset();
    openlog(name, LOG_NDELAY | logopt, facility);
    if (first_call) {
	first_call = 0;
	msg_output(msg_syslog_print);
    }
}

/* msg_syslog_facility - set logging facility by name */

int     msg_syslog_facility(const char *facility_name)
{
    struct facility_list *fnp;

    for (fnp = facility_list; fnp->name; ++fnp) {
	if (!strcmp(fnp->name, facility_name)) {
	    syslog_facility = fnp->facility;
	    return (1);
	}
    }
    return 0;
}

#ifdef TEST

 /*
  * Proof-of-concept program to test the syslogging diagnostics interface
  * 
  * Usage: msg_syslog_test text...
  */

int     main(int argc, char **argv)
{
    VSTRING *vp = vstring_alloc(256);

    msg_syslog_init(argv[0], LOG_PID, LOG_MAIL);
    if (argc < 2)
	msg_error("usage: %s text to be logged", argv[0]);
    while (--argc && *++argv) {
	vstring_strcat(vp, *argv);
	if (argv[1])
	    vstring_strcat(vp, " ");
    }
    msg_warn("static text");
    msg_warn("dynamic text: >%s<", vstring_str(vp));
    msg_warn("dynamic numeric: >%d<", 42);
    msg_warn("error text: >%m<");
    msg_warn("dynamic: >%s<: error: >%m<", vstring_str(vp));
    vstring_free(vp);
    return (0);
}

#endif
