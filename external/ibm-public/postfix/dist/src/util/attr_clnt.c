/*	$NetBSD: attr_clnt.c,v 1.2 2017/02/14 01:16:48 christos Exp $	*/

/*++
/* NAME
/*	attr_clnt 3
/* SUMMARY
/*	attribute query-reply client
/* SYNOPSIS
/*	#include <attr_clnt.h>
/*
/*	typedef int (*ATTR_CLNT_PRINT_FN) (VSTREAM *, int, va_list);
/*	typedef int (*ATTR_CLNT_SCAN_FN) (VSTREAM *, int, va_list);
/*
/*	ATTR_CLNT *attr_clnt_create(server, timeout, max_idle, max_ttl)
/*	const char *server;
/*	int	timeout;
/*	int	max_idle;
/*	int	max_ttl;
/*
/*	int	attr_clnt_request(client,
/*			send_flags, send_type, send_name, ..., ATTR_TYPE_END,
/*			recv_flags, recv_type, recv_name, ..., ATTR_TYPE_END)
/*	ATTR_CLNT *client;
/*	int	send_flags;
/*	int	send_type;
/*	const char *send_name;
/*	int	recv_flags;
/*	int	recv_type;
/*	const char *recv_name;
/*
/*	void	attr_clnt_free(client)
/*	ATTR_CLNT *client;
/*
/*	void	attr_clnt_control(client, name, value, ... ATTR_CLNT_CTL_END)
/*	ATTR_CLNT *client;
/*	int	name;
/* DESCRIPTION
/*	This module implements a client for a simple attribute-based
/*	protocol. The default protocol is described in attr_scan_plain(3).
/*
/*	attr_clnt_create() creates a client handle. See auto_clnt(3) for
/*	a description of the arguments.
/*
/*	attr_clnt_request() sends the specified request attributes and
/*	receives a reply. The reply argument specifies a name-value table.
/*	The other arguments are as described in attr_print_plain(3). The
/*	result is the number of attributes received or -1 in case of trouble.
/*
/*	attr_clnt_free() destroys a client handle and closes its connection.
/*
/*	attr_clnt_control() allows the user to fine tune the behavior of
/*	the specified client. The arguments are a list of (name, value)
/*	terminated with ATTR_CLNT_CTL_END.
/*	The following lists the names and the types of the corresponding
/*	value arguments.
/* .IP "ATTR_CLNT_CTL_PROTO(ATTR_CLNT_PRINT_FN, ATTR_CLNT_SCAN_FN)"
/*	Specifies alternatives for the attr_plain_print() and
/*	attr_plain_scan() functions.
/* .IP "ATTR_CLNT_CTL_REQ_LIMIT(int)"
/*	The maximal number of requests per connection (default: 0,
/*	i.e. no limit).  To enable the limit, specify a value greater
/*	than zero.
/* .IP "ATTR_CLNT_CTL_TRY_LIMIT(int)"
/*	The maximal number of attempts to send a request before
/*	giving up (default: 2). To disable the limit, specify a
/*	value equal to zero.
/* .IP "ATTR_CLNT_CTL_TRY_DELAY(int)"
/*	The time in seconds between attempts to send a request
/*	(default: 1).  Specify a value greater than zero.
/* DIAGNOSTICS
/*	Warnings: communication failure.
/* SEE ALSO
/*	auto_clnt(3), client endpoint management
/*	attr_scan_plain(3), attribute protocol
/*	attr_print_plain(3), attribute protocol
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
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <htable.h>
#include <attr.h>
#include <iostuff.h>
#include <compat_va_copy.h>
#include <auto_clnt.h>
#include <attr_clnt.h>

/* Application-specific. */

struct ATTR_CLNT {
    AUTO_CLNT *auto_clnt;
    /* Remaining properties are set with attr_clnt_control(). */
    ATTR_CLNT_PRINT_FN print;
    ATTR_CLNT_SCAN_FN scan;
    int     req_limit;
    int     req_count;
    int     try_limit;
    int     try_delay;
};

#define ATTR_CLNT_DEF_REQ_LIMIT	(0)	/* default per-session request limit */
#define ATTR_CLNT_DEF_TRY_LIMIT	(2)	/* default request (re)try limit */
#define ATTR_CLNT_DEF_TRY_DELAY	(1)	/* default request (re)try delay */

/* attr_clnt_free - destroy attribute client */

void    attr_clnt_free(ATTR_CLNT *client)
{
    auto_clnt_free(client->auto_clnt);
    myfree((void *) client);
}

/* attr_clnt_create - create attribute client */

ATTR_CLNT *attr_clnt_create(const char *service, int timeout,
			            int max_idle, int max_ttl)
{
    ATTR_CLNT *client;

    client = (ATTR_CLNT *) mymalloc(sizeof(*client));
    client->auto_clnt = auto_clnt_create(service, timeout, max_idle, max_ttl);
    client->scan = attr_vscan_plain;
    client->print = attr_vprint_plain;
    client->req_limit = ATTR_CLNT_DEF_REQ_LIMIT;
    client->req_count = 0;
    client->try_limit = ATTR_CLNT_DEF_TRY_LIMIT;
    client->try_delay = ATTR_CLNT_DEF_TRY_DELAY;
    return (client);
}

/* attr_clnt_request - send query, receive reply */

int     attr_clnt_request(ATTR_CLNT *client, int send_flags,...)
{
    const char *myname = "attr_clnt_request";
    VSTREAM *stream;
    int     count = 0;
    va_list saved_ap;
    va_list ap;
    int     type;
    int     recv_flags;
    int     err;
    int     ret;

    /*
     * XXX If the stream is readable before we send anything, then assume the
     * remote end disconnected.
     * 
     * XXX For some reason we can't simply call the scan routine after the print
     * routine, that messes up the argument list.
     */
#define SKIP_ARG(ap, type) { \
	(void) va_arg(ap, char *); \
	(void) va_arg(ap, type); \
    }
#define SKIP_ARG2(ap, t1, t2) { \
	SKIP_ARG(ap, t1); \
	(void) va_arg(ap, t2); \
    }

    /* Finalize argument lists before returning. */
    va_start(saved_ap, send_flags);
    for (;;) {
	errno = 0;
	if ((stream = auto_clnt_access(client->auto_clnt)) != 0
	    && readable(vstream_fileno(stream)) == 0) {
	    errno = 0;
	    VA_COPY(ap, saved_ap);
	    err = (client->print(stream, send_flags, ap) != 0
		   || vstream_fflush(stream) != 0);
	    va_end(ap);
	    if (err == 0) {
		VA_COPY(ap, saved_ap);
		while ((type = va_arg(ap, int)) != ATTR_TYPE_END) {
		    switch (type) {
		    case ATTR_TYPE_STR:
			SKIP_ARG(ap, char *);
			break;
		    case ATTR_TYPE_DATA:
			SKIP_ARG2(ap, ssize_t, char *);
			break;
		    case ATTR_TYPE_INT:
			SKIP_ARG(ap, int);
			break;
		    case ATTR_TYPE_LONG:
			SKIP_ARG(ap, long);
			break;
		    case ATTR_TYPE_HASH:
			(void) va_arg(ap, HTABLE *);
			break;
		    default:
			msg_panic("%s: unexpected attribute type %d",
				  myname, type);
		    }
		}
		recv_flags = va_arg(ap, int);
		ret = client->scan(stream, recv_flags, ap);
		va_end(ap);
		/* Finalize argument lists before returning. */
		if (ret > 0) {
		    if (client->req_limit > 0
			&& (client->req_count += 1) >= client->req_limit) {
			auto_clnt_recover(client->auto_clnt);
			client->req_count = 0;
		    }
		    break;
		}
	    }
	}
	if ((++count >= client->try_limit && client->try_limit > 0)
	    || msg_verbose
	    || (errno && errno != EPIPE && errno != ENOENT && errno != ECONNRESET))
	    msg_warn("problem talking to server %s: %m",
		     auto_clnt_name(client->auto_clnt));
	/* Finalize argument lists before returning. */
	if (count >= client->try_limit && client->try_limit > 0) {
	    ret = -1;
	    break;
	}
	sleep(client->try_delay);
	auto_clnt_recover(client->auto_clnt);
	client->req_count = 0;
    }
    /* Finalize argument lists before returning. */
    va_end(saved_ap);
    return (ret);
}

/* attr_clnt_control - fine control */

void    attr_clnt_control(ATTR_CLNT *client, int name,...)
{
    const char *myname = "attr_clnt_control";
    va_list ap;

    for (va_start(ap, name); name != ATTR_CLNT_CTL_END; name = va_arg(ap, int)) {
	switch (name) {
	case ATTR_CLNT_CTL_PROTO:
	    client->print = va_arg(ap, ATTR_CLNT_PRINT_FN);
	    client->scan = va_arg(ap, ATTR_CLNT_SCAN_FN);
	    break;
	case ATTR_CLNT_CTL_REQ_LIMIT:
	    client->req_limit = va_arg(ap, int);
	    if (client->req_limit < 0)
		msg_panic("%s: bad request limit: %d",
			  myname, client->req_limit);
	    if (msg_verbose)
		msg_info("%s: new request limit %d",
			 myname, client->req_limit);
	    break;
	case ATTR_CLNT_CTL_TRY_LIMIT:
	    client->try_limit = va_arg(ap, int);
	    if (client->try_limit < 0)
		msg_panic("%s: bad retry limit: %d", myname, client->try_limit);
	    if (msg_verbose)
		msg_info("%s: new retry limit %d", myname, client->try_limit);
	    break;
	case ATTR_CLNT_CTL_TRY_DELAY:
	    client->try_delay = va_arg(ap, int);
	    if (client->try_delay <= 0)
		msg_panic("%s: bad retry delay: %d", myname, client->try_delay);
	    if (msg_verbose)
		msg_info("%s: new retry delay %d", myname, client->try_delay);
	    break;
	default:
	    msg_panic("%s: bad name %d", myname, name);
	}
    }
    va_end(ap);
}
