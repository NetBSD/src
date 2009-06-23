/*	$NetBSD: verify_clnt.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	verify_clnt 3
/* SUMMARY
/*	address verification client interface
/* SYNOPSIS
/*	#include <verify_clnt.h>
/*
/*	int	verify_clnt_query(addr, status, why)
/*	const char *addr;
/*	int	*status;
/*	VSTRING	*why;
/*
/*	int	verify_clnt_update(addr, status, why)
/*	const char *addr;
/*	int	status;
/*	const char *why;
/* DESCRIPTION
/*	verify_clnt_query() requests information about the given address.
/*	The result value is one of the valid status values (see
/*	status description below).
/*	In all cases the \fBwhy\fR argument provides additional
/*	information.
/*
/*	verify_clnt_update() requests that the status of the specified
/*	address be updated. The result status is DEL_REQ_RCPT_STAT_OK upon
/*	success, DEL_REQ_RCPT_STAT_DEFER upon failure.
/*
/*	Arguments
/* .IP addr
/*	The email address in question.
/* .IP status
/*	One of the following status codes:
/* .RS
/* .IP DEL_REQ_RCPT_STAT_OK
/*	The mail system did not detect any problems.
/* .IP DEL_REQ_RCPT_STAT_DEFER
/*	The status of the address is indeterminate.
/* .IP DEL_REQ_RCPT_STAT_BOUNCE
/*	The address is permanently undeliverable.
/* .RE
/* .IP why
/*	textual description of the status.
/* DIAGNOSTICS
/*	These functions return VRFY_STAT_OK in case of success,
/*	VRFY_STAT_BAD in case of a malformed request, and
/*	VRFY_STAT_FAIL when the operation failed.
/* SEE ALSO
/*	verify(8) Postfix address verification server
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
#include <vstream.h>
#include <vstring.h>
#include <attr.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <clnt_stream.h>
#include <verify_clnt.h>

CLNT_STREAM *vrfy_clnt;

/* verify_clnt_init - initialize */

static void verify_clnt_init(void)
{
    if (vrfy_clnt != 0)
	msg_panic("verify_clnt_init: multiple initialization");
    vrfy_clnt = clnt_stream_create(MAIL_CLASS_PRIVATE, var_verify_service,
				   var_ipc_idle_limit, var_ipc_ttl_limit);
}

/* verify_clnt_query - request address verification status */

int     verify_clnt_query(const char *addr, int *addr_status, VSTRING *why)
{
    VSTREAM *stream;
    int     request_status;
    int     count = 0;

    /*
     * Do client-server plumbing.
     */
    if (vrfy_clnt == 0)
	verify_clnt_init();

    /*
     * Request status for this address.
     */
    for (;;) {
	stream = clnt_stream_access(vrfy_clnt);
	errno = 0;
	count += 1;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, VRFY_REQ_QUERY,
		       ATTR_TYPE_STR, MAIL_ATTR_ADDR, addr,
		       ATTR_TYPE_END) != 0
	    || vstream_fflush(stream)
	    || attr_scan(stream, ATTR_FLAG_MISSING,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &request_status,
			 ATTR_TYPE_INT, MAIL_ATTR_ADDR_STATUS, addr_status,
			 ATTR_TYPE_STR, MAIL_ATTR_WHY, why,
			 ATTR_TYPE_END) != 3) {
	    if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno != ENOENT))
		msg_warn("problem talking to service %s: %m",
			 var_verify_service);
	} else {
	    break;
	}
	sleep(1);
	clnt_stream_recover(vrfy_clnt);
    }
    return (request_status);
}

/* verify_clnt_update - request address status update */

int     verify_clnt_update(const char *addr, int addr_status, const char *why)
{
    VSTREAM *stream;
    int     request_status;

    /*
     * Do client-server plumbing.
     */
    if (vrfy_clnt == 0)
	verify_clnt_init();

    /*
     * Send status for this address. Supply a default status if the address
     * verification service is unavailable.
     */
    for (;;) {
	stream = clnt_stream_access(vrfy_clnt);
	errno = 0;
	if (attr_print(stream, ATTR_FLAG_NONE,
		       ATTR_TYPE_STR, MAIL_ATTR_REQ, VRFY_REQ_UPDATE,
		       ATTR_TYPE_STR, MAIL_ATTR_ADDR, addr,
		       ATTR_TYPE_INT, MAIL_ATTR_ADDR_STATUS, addr_status,
		       ATTR_TYPE_STR, MAIL_ATTR_WHY, why,
		       ATTR_TYPE_END) != 0
	    || attr_scan(stream, ATTR_FLAG_MISSING,
			 ATTR_TYPE_INT, MAIL_ATTR_STATUS, &request_status,
			 ATTR_TYPE_END) != 1) {
	    if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		msg_warn("problem talking to service %s: %m",
			 var_verify_service);
	} else {
	    break;
	}
	sleep(1);
	clnt_stream_recover(vrfy_clnt);
    }
    return (request_status);
}

 /*
  * Proof-of-concept test client program.
  */
#ifdef TEST

#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstring_vstream.h>
#include <mail_conf.h>

#define STR(x) vstring_str(x)

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-v]", myname);
}

static void query(char *query, VSTRING *buf)
{
    int     status;

    switch (verify_clnt_query(query, &status, buf)) {
    case VRFY_STAT_OK:
	vstream_printf("%-10s %d\n", "status", status);
	vstream_printf("%-10s %s\n", "text", STR(buf));
	vstream_fflush(VSTREAM_OUT);
	break;
    case VRFY_STAT_BAD:
	msg_warn("bad request format");
	break;
    case VRFY_STAT_FAIL:
	msg_warn("request failed");
	break;
    }
}

static void update(char *query)
{
    char   *addr;
    char   *status_text;
    char   *cp = query;

    if ((addr = mystrtok(&cp, " \t\r\n")) == 0
	|| (status_text = mystrtok(&cp, " \t\r\n")) == 0) {
	msg_warn("bad request format");
	return;
    }
    while (*cp && ISSPACE(*cp))
	cp++;
    if (*cp == 0) {
	msg_warn("bad request format");
	return;
    }
    switch (verify_clnt_update(query, atoi(status_text), cp)) {
    case VRFY_STAT_OK:
	vstream_printf("OK\n");
	vstream_fflush(VSTREAM_OUT);
	break;
    case VRFY_STAT_BAD:
	msg_warn("bad request format");
	break;
    case VRFY_STAT_FAIL:
	msg_warn("request failed");
	break;
    }
}

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);
    char   *cp;
    int     ch;
    char   *command;

    signal(SIGPIPE, SIG_IGN);

    msg_vstream_init(argv[0], VSTREAM_ERR);

    mail_conf_read();
    msg_info("using config files in %s", var_config_dir);
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc - optind > 1)
	usage(argv[0]);

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	cp = STR(buffer);
	if ((command = mystrtok(&cp, " \t\r\n")) == 0)
	    continue;
	if (strcmp(command, "query") == 0)
	    query(cp, buffer);
	else if (strcmp(command, "update") == 0)
	    update(cp);
	else
	    msg_warn("unrecognized command: %s", command);
    }
    vstring_free(buffer);
    return (0);
}

#endif
