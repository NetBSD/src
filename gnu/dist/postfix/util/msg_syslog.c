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
/* DESCRIPTION
/*	This module implements support to report msg(3) diagnostics
/*	via the syslog daemon.
/*
/*	msg_syslog_init() is a wrapper around the openlog(3) routine
/*	that directs subsequent msg(3) output to the syslog daemon.
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
#include <stdlib.h>		/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>

/* Application-specific. */

#include "vstring.h"
#include "stringops.h"
#include "msg.h"
#include "msg_output.h"
#include "msg_syslog.h"

 /*
  * Stay a little below the 2048-byte limit of older syslog() implementations.
  */
#define MSG_SYSLOG_RECLEN	2000

/* msg_syslog_print - log info to syslog daemon */

static void msg_syslog_print(int level, const char *text)
{
    static const int log_level[] = {
	LOG_INFO, LOG_WARNING, LOG_ERR, LOG_CRIT, LOG_CRIT,
    };
    static const char *severity_name[] = {
	"info", "warning", "error", "fatal", "panic",
    };

    if (level < 0 || level >= (int) (sizeof(log_level) / sizeof(log_level[0])))
	msg_panic("msg_syslog_print: invalid severity level: %d", level);

    if (level == MSG_INFO) {
	syslog(log_level[level], "%.*s", MSG_SYSLOG_RECLEN, text);
    } else {
	syslog(log_level[level], "%s: %.*s",
	       severity_name[level], MSG_SYSLOG_RECLEN, text);
    }
}

/* msg_syslog_init - initialize */

void    msg_syslog_init(const char *name, int logopt, int facility)
{
    static int first_call = 1;

    openlog(name, LOG_NDELAY | logopt, facility);
    if (first_call) {
	first_call = 0;
	msg_output(msg_syslog_print);
    }
}

#ifdef TEST

 /*
  * Proof-of-concept program to test the syslogging diagnostics interface
  * 
  * Usage: msg_syslog_test text...
  */

main(int argc, char **argv)
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
