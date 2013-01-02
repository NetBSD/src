/*	$NetBSD: mail_trigger.c,v 1.1.1.2 2013/01/02 18:58:58 tron Exp $	*/

/*++
/* NAME
/*	mail_trigger 3
/* SUMMARY
/*	trigger a mail service
/* SYNOPSIS
/*	#include <mail_proto.h>
/*
/*	int	mail_trigger(class, service, request, length)
/*	const char *class;
/*	const char *service;
/*	const char *request;
/*	ssize_t	length;
/* DESCRIPTION
/*	mail_trigger() wakes up the specified mail subsystem, by
/*	sending it the specified request.
/*
/*	Arguments:
/* .IP class
/*	Name of a class of local transport channel endpoints,
/*	either \fIpublic\fR (accessible by any local user) or
/*	\fIprivate\fR (administrative access only).
/* .IP service
/*	The name of a local transport endpoint within the named class.
/* .IP request
/*	A string. The list of valid requests is service specific.
/* .IP length
/*	The length of the request string.
/* DIAGNOSTICS
/*	The result is -1 in case of problems, 0 otherwise.
/*	Warnings are logged.
/* BUGS
/*	Works with FIFO or UNIX-domain services only.
/*
/*	Should use master.cf to find out what transport to use.
/* SEE ALSO
/*	fifo_trigger(3) trigger a FIFO-based service
/*	unix_trigger(3) trigger a UNIX_domain service
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <iostuff.h>
#include <trigger.h>
#include <warn_stat.h>

/* Global library. */

#include "mail_params.h"
#include "mail_proto.h"

/* mail_trigger - trigger a service */

int     mail_trigger(const char *class, const char *service,
		             const char *req_buf, ssize_t req_len)
{
    struct stat st;
    char   *path;
    int     status;

    /*
     * XXX Some systems cannot tell the difference between a named pipe
     * (fifo) or a UNIX-domain socket. So we may have to try both.
     */
    path = mail_pathname(class, service);
    if ((status = stat(path, &st)) < 0) {
	 msg_warn("unable to look up %s: %m", path);
    } else if (S_ISFIFO(st.st_mode)) {
	status = fifo_trigger(path, req_buf, req_len, var_trigger_timeout);
	if (status < 0 && S_ISSOCK(st.st_mode))
	    status = LOCAL_TRIGGER(path, req_buf, req_len, var_trigger_timeout);
    } else if (S_ISSOCK(st.st_mode)) {
	status = LOCAL_TRIGGER(path, req_buf, req_len, var_trigger_timeout);
    } else {
	msg_warn("%s is not a socket or a fifo", path);
	status = -1;
    }
    myfree(path);
    return (status);
}
