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
/*	attr_clnt_create() creates a client handle. The server
/*	argument specifies "transport:servername" where transport is
/*	currently limited to "inet" or "unix", and servername has the
/*	form "host:port", "private/servicename" or "public/servicename".
/*	The timeout parameter limits the time for sending or receiving
/*	a reply, max_idle specifies how long an idle connection is
/*	kept open, and the max_ttl parameter bounds the time that a
/*	connection is kept open. 
/*	Specify zero to disable a max_idle or max_ttl limit.
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <vstream.h>
#include <connect.h>
#include <htable.h>
#include <attr.h>
#include <auto_clnt.h>
#include <attr_clnt.h>

/* Application-specific. */

struct ATTR_CLNT {
    AUTO_CLNT *auto_clnt;
    int     (*connect) (const char *, int, int);
    char   *endpoint;
    int     timeout;
    ATTR_CLNT_PRINT_FN print;
    ATTR_CLNT_SCAN_FN scan;
};

/* attr_clnt_connect - connect to server */

static VSTREAM *attr_clnt_connect(void *context)
{
    const char *myname = "attr_clnt_connect";
    ATTR_CLNT *client = (ATTR_CLNT *) context;
    VSTREAM *fp;
    int     fd;

    fd = client->connect(client->endpoint, BLOCKING, client->timeout);
    if (fd < 0) {
	msg_warn("connect to %s: %m", client->endpoint);
	return (0);
    } else {
	if (msg_verbose)
	    msg_info("%s: connected to %s", myname, client->endpoint);
	fp = vstream_fdopen(fd, O_RDWR);
	vstream_control(fp, VSTREAM_CTL_PATH, client->endpoint,
			VSTREAM_CTL_TIMEOUT, client->timeout,
			VSTREAM_CTL_END);
	return (fp);
    }
}

/* attr_clnt_free - destroy attribute client */

void    attr_clnt_free(ATTR_CLNT *client)
{
    myfree(client->endpoint);
    auto_clnt_free(client->auto_clnt);
    myfree((char *) client);
}

/* attr_clnt_create - create attribute client */

ATTR_CLNT *attr_clnt_create(const char *service, int timeout,
			            int max_idle, int max_ttl)
{
    const char *myname = "attr_clnt_create";
    char   *transport = mystrdup(service);
    char   *endpoint;
    ATTR_CLNT *client;

    if ((endpoint = split_at(transport, ':')) == 0
	|| *endpoint == 0 || *transport == 0)
	msg_fatal("need service transport:endpoint instead of \"%s\"", service);
    if (msg_verbose)
	msg_info("%s: transport=%s endpoint=%s", myname, transport, endpoint);

    client = (ATTR_CLNT *) mymalloc(sizeof(*client));
    client->scan = attr_vscan_plain;
    client->print = attr_vprint_plain;
    client->endpoint = mystrdup(endpoint);
    client->timeout = timeout;
    if (strcmp(transport, "inet") == 0) {
	client->connect = inet_connect;
    } else if (strcmp(transport, "local") == 0) {
	client->connect = LOCAL_CONNECT;
    } else if (strcmp(transport, "unix") == 0) {
	client->connect = unix_connect;
    } else {
	msg_fatal("invalid attribute transport name: %s", service);
    }
    client->auto_clnt = auto_clnt_create(max_idle, max_ttl,
					 attr_clnt_connect, (void *) client);
    myfree(transport);
    return (client);
}

/* attr_clnt_request - send query, receive reply */

int     attr_clnt_request(ATTR_CLNT *client, int send_flags,...)
{
    const char *myname = "attr_clnt_request";
    VSTREAM *stream;
    int     count = 0;
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

    for (;;) {
	errno = 0;
	if ((stream = auto_clnt_access(client->auto_clnt)) != 0
	    && readable(vstream_fileno(stream)) == 0) {
	    errno = 0;
	    va_start(ap, send_flags);
	    err = (client->print(stream, send_flags, ap) != 0
		   || vstream_fflush(stream) != 0);
	    va_end(ap);
	    if (err == 0) {
		va_start(ap, send_flags);
		while ((type = va_arg(ap, int)) != ATTR_TYPE_END) {
		    switch (type) {
		    case ATTR_TYPE_STR:
			SKIP_ARG(ap, char *);
			break;
		    case ATTR_TYPE_NUM:
			SKIP_ARG(ap, int);
			break;
		    case ATTR_TYPE_LONG:
			SKIP_ARG(ap, long);
			break;
		    case ATTR_TYPE_HASH:
			SKIP_ARG(ap, HTABLE *);
			break;
		    default:
			msg_panic("%s: unexpected attribute type %d",
				  myname, type);
		    }
		}
		recv_flags = va_arg(ap, int);
		ret = client->scan(stream, recv_flags, ap);
		va_end(ap);
		if (ret > 0)
		    return (ret);
	    }
	}
	if (++count >= 2
	    || msg_verbose
	    || (errno && errno != EPIPE && errno != ENOENT && errno != ECONNRESET))
	    msg_warn("problem talking to server %s: %m", client->endpoint);
	if (count >= 2)
	    return (-1);
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(client->auto_clnt);
    }
}

/* attr_clnt_control - fine control */

void    attr_clnt_control(ATTR_CLNT *client, int name,...)
{
    char   *myname = "attr_clnt_control";
    va_list ap;

    for (va_start(ap, name); name != ATTR_CLNT_CTL_END; name = va_arg(ap, int)) {
	switch (name) {
	case ATTR_CLNT_CTL_PROTO:
	    client->print = va_arg(ap, ATTR_CLNT_PRINT_FN);
	    client->scan = va_arg(ap, ATTR_CLNT_SCAN_FN);
	    break;
	default:
	    msg_panic("%s: bad name %d", myname, name);
	}
    }
}
