/*++
/* NAME
/*	smtp_connect 3
/* SUMMARY
/*	connect to SMTP server and deliver
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	int	smtp_connect(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	This module implements SMTP connection management and controls
/*	mail delivery.
/*
/*	smtp_connect() attempts to establish an SMTP session with a host
/*	that represents the destination domain, or with an optional fallback
/*	relay when the destination cannot be found, or when all the
/*	destination servers are unavailable. It skips over IP addresses
/*	that fail to complete the SMTP handshake and tries to find
/*	an alternate server when an SMTP session fails to deliver.
/*
/*	The destination is either a host (or domain) name or a numeric
/*	address. Symbolic or numeric service port information may be
/*	appended, separated by a colon (":").
/*
/*	By default, the Internet domain name service is queried for mail
/*	exchanger hosts. Quote the domain name with `[' and `]' to
/*	suppress mail exchanger lookups.
/*
/*	Numerical address information should always be quoted with `[]'.
/* DIAGNOSTICS
/*	The delivery status is the result value.
/* SEE ALSO
/*	smtp_proto(3) SMTP client protocol
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <split_at.h>
#include <mymalloc.h>
#include <inet_addr_list.h>
#include <iostuff.h>
#include <timed_connect.h>
#include <stringops.h>
#include <host_port.h>
#include <sane_connect.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>
#include <debug_peer.h>
#include <deliver_pass.h>
#include <mail_error.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_addr.h"

/* smtp_connect_addr - connect to explicit address */

static SMTP_SESSION *smtp_connect_addr(DNS_RR *addr, unsigned port,
				               VSTRING *why)
{
    char   *myname = "smtp_connect_addr";
    struct sockaddr_in sin;
    int     sock;
    INET_ADDR_LIST *addr_list;
    int     conn_stat;
    int     saved_errno;
    VSTREAM *stream;
    int     ch;
    unsigned long inaddr;

    smtp_errno = SMTP_ERR_NONE;			/* Paranoia */

    /*
     * Sanity checks.
     */
    if (addr->data_len > sizeof(sin.sin_addr)) {
	msg_warn("%s: skip address with length %d", myname, addr->data_len);
	smtp_errno = SMTP_ERR_RETRY;
	return (0);
    }

    /*
     * Initialize.
     */
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    if ((sock = socket(sin.sin_family, SOCK_STREAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    /*
     * Allow the sysadmin to specify the source address, for example, as "-o
     * smtp_bind_address=x.x.x.x" in the master.cf file.
     */
    if (*var_smtp_bind_addr) {
	sin.sin_addr.s_addr = inet_addr(var_smtp_bind_addr);
	if (sin.sin_addr.s_addr == INADDR_NONE)
	    msg_fatal("%s: bad %s parameter: %s",
		      myname, VAR_SMTP_BIND_ADDR, var_smtp_bind_addr);
	if (bind(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
	    msg_warn("%s: bind %s: %m", myname, inet_ntoa(sin.sin_addr));
	if (msg_verbose)
	    msg_info("%s: bind %s", myname, inet_ntoa(sin.sin_addr));
    }

    /*
     * When running as a virtual host, bind to the virtual interface so that
     * the mail appears to come from the "right" machine address.
     */
    else if ((addr_list = own_inet_addr_list())->used == 1) {
	memcpy((char *) &sin.sin_addr, addr_list->addrs, sizeof(sin.sin_addr));
	inaddr = ntohl(sin.sin_addr.s_addr);
	if (!IN_CLASSA(inaddr)
	    || !(((inaddr & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET)) {
	    if (bind(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
		msg_warn("%s: bind %s: %m", myname, inet_ntoa(sin.sin_addr));
	    if (msg_verbose)
		msg_info("%s: bind %s", myname, inet_ntoa(sin.sin_addr));
	}
    }

    /*
     * Connect to the SMTP server.
     */
    sin.sin_port = port;
    memcpy((char *) &sin.sin_addr, addr->data, sizeof(sin.sin_addr));

    if (msg_verbose)
	msg_info("%s: trying: %s[%s] port %d...",
		 myname, addr->name, inet_ntoa(sin.sin_addr), ntohs(port));
    if (var_smtp_conn_tmout > 0) {
	non_blocking(sock, NON_BLOCKING);
	conn_stat = timed_connect(sock, (struct sockaddr *) & sin,
				  sizeof(sin), var_smtp_conn_tmout);
	saved_errno = errno;
	non_blocking(sock, BLOCKING);
	errno = saved_errno;
    } else {
	conn_stat = sane_connect(sock, (struct sockaddr *) & sin, sizeof(sin));
    }
    if (conn_stat < 0) {
	vstring_sprintf(why, "connect to %s[%s]: %m",
			addr->name, inet_ntoa(sin.sin_addr));
	smtp_errno = SMTP_ERR_RETRY;
	close(sock);
	return (0);
    }

    /*
     * Skip this host if it takes no action within some time limit.
     */
    if (read_wait(sock, var_smtp_helo_tmout) < 0) {
	vstring_sprintf(why, "connect to %s[%s]: read timeout",
			addr->name, inet_ntoa(sin.sin_addr));
	smtp_errno = SMTP_ERR_RETRY;
	close(sock);
	return (0);
    }

    /*
     * Skip this host if it disconnects without talking to us.
     */
    stream = vstream_fdopen(sock, O_RDWR);
    if ((ch = VSTREAM_GETC(stream)) == VSTREAM_EOF) {
	vstring_sprintf(why, "connect to %s[%s]: server dropped connection without sending the initial SMTP greeting",
			addr->name, inet_ntoa(sin.sin_addr));
	smtp_errno = SMTP_ERR_RETRY;
	vstream_fclose(stream);
	return (0);
    }
    vstream_ungetc(stream, ch);
    return (smtp_session_alloc(stream, addr->name, inet_ntoa(sin.sin_addr)));
}

/* smtp_parse_destination - parse destination */

static char *smtp_parse_destination(char *destination, char *def_service,
				            char **hostp, unsigned *portp)
{
    char   *buf = mystrdup(destination);
    char   *service;
    struct servent *sp;
    char   *protocol = "tcp";		/* XXX configurable? */
    unsigned port;
    const char *err;

    if (msg_verbose)
	msg_info("smtp_parse_destination: %s %s", destination, def_service);

    /*
     * Parse the host/port information. We're working with a copy of the
     * destination argument so the parsing can be destructive.
     */
    if ((err = host_port(buf, hostp, &service, def_service)) != 0)
	msg_fatal("%s in SMTP server description: %s", err, destination);

    /*
     * Convert service to port number, network byte order.
     */
    if (alldig(service) && (port = atoi(service)) != 0) {
	*portp = htons(port);
    } else {
	if ((sp = getservbyname(service, protocol)) == 0)
	    msg_fatal("unknown service: %s/%s", service, protocol);
	*portp = sp->s_port;
    }
    return (buf);
}

/* smtp_connect - establish SMTP connection */

int     smtp_connect(SMTP_STATE *state)
{
    DELIVER_REQUEST *request = state->request;
    VSTRING *why = vstring_alloc(10);
    char   *dest_buf;
    char   *host;
    unsigned port;
    char   *def_service = "smtp";	/* XXX configurable? */
    ARGV   *sites;
    char   *dest;
    char  **cpp;
    DNS_RR *addr_list;
    DNS_RR *addr;
    DNS_RR *next;
    int     addr_count;
    int     sess_count;
    int     misc_flags = SMTP_MISC_FLAG_DEFAULT;

    /*
     * First try to deliver to the indicated destination, then try to deliver
     * to the optional fall-back relays.
     * 
     * Future proofing: do a null destination sanity check in case we allow the
     * primary destination to be a list (it could be just separators).
     */
    sites = argv_alloc(1);
    argv_add(sites, request->nexthop, (char *) 0);
    if (sites->argc == 0)
	msg_panic("null destination: \"%s\"", request->nexthop);
    argv_split_append(sites, var_fallback_relay, ", \t\r\n");

    /*
     * Don't give up after a hard host lookup error until we have tried the
     * fallback relay servers.
     * 
     * Don't bounce mail after a host lookup problem with a relayhost or with a
     * fallback relay.
     * 
     * Don't give up after a qualifying soft error until we have tried all
     * qualifying backup mail servers.
     * 
     * All this means that error handling and error reporting depends on whether
     * the error qualifies for trying to deliver to a backup mail server, or
     * whether we're looking up a relayhost or fallback relay. The challenge
     * then is to build this into the pre-existing SMTP client without
     * getting lost in the complexity.
     */
    for (cpp = sites->argv; SMTP_RCPT_LEFT(state) > 0 && (dest = *cpp) != 0; cpp++) {
	state->final_server = (cpp[1] == 0);

	/*
	 * Parse the destination. Default is to use the SMTP port. Look up
	 * the address instead of the mail exchanger when a quoted host is
	 * specified, or when DNS lookups are disabled.
	 */
	dest_buf = smtp_parse_destination(dest, def_service, &host, &port);

	/*
	 * Resolve an SMTP server. Skip mail exchanger lookups when a quoted
	 * host is specified, or when DNS lookups are disabled.
	 */
	if (msg_verbose)
	    msg_info("connecting to %s port %d", host, ntohs(port));
	if (ntohs(port) != 25)
	    misc_flags &= ~SMTP_MISC_FLAG_LOOP_DETECT;
	else
	    misc_flags |= SMTP_MISC_FLAG_LOOP_DETECT;
	if (var_disable_dns || *dest == '[') {
	    addr_list = smtp_host_addr(host, misc_flags, why);
	} else {
	    addr_list = smtp_domain_addr(host, misc_flags, why);
	}
	myfree(dest_buf);

	/*
	 * Don't try any backup host if mail loops to myself. That would just
	 * make the problem worse.
	 */
	if (addr_list == 0 && smtp_errno == SMTP_ERR_LOOP)
	    break;

	/*
	 * Connect to an SMTP server.
	 * 
	 * At the start of an SMTP session, all recipients are unmarked. In the
	 * course of an SMTP session, recipients are marked as KEEP (deliver
	 * to alternate mail server) or DROP (remove from recipient list). At
	 * the end of an SMTP session, weed out the recipient list. Unmark
	 * any left-over recipients and try to deliver them to a backup mail
	 * server.
	 */
	sess_count = addr_count = 0;
	for (addr = addr_list; SMTP_RCPT_LEFT(state) > 0 && addr; addr = next) {
	    next = addr->next;
	    if (++addr_count == var_smtp_mxaddr_limit)
		next = 0;
	    if ((state->session = smtp_connect_addr(addr, port, why)) != 0) {
		if (++sess_count == var_smtp_mxsess_limit)
		    next = 0;
		state->final_server = (cpp[1] == 0 && next == 0);
		state->session->best = (addr->pref == addr_list->pref);
		debug_peer_check(state->session->host, state->session->addr);
		if (smtp_helo(state, misc_flags) == 0)
		    smtp_xfer(state);
		if (state->history != 0
		    && (state->error_mask & name_mask(VAR_NOTIFY_CLASSES,
				     mail_error_masks, var_notify_classes)))
		    smtp_chat_notify(state);
		/* XXX smtp_xfer() may abort in the middle of DATA. */
		smtp_session_free(state->session);
		state->session = 0;
		debug_peer_restore();
		smtp_rcpt_cleanup(state);
	    } else {
		msg_info("%s (port %d)", vstring_str(why), ntohs(port));
	    }
	}
	dns_rr_free(addr_list);
    }

    /*
     * We still need to deliver, bounce or defer some left-over recipients:
     * either mail loops or some backup mail server was unavailable.
     * 
     * Pay attention to what could be configuration problems, and pretend that
     * these are recoverable rather than bouncing the mail.
     */
    if (SMTP_RCPT_LEFT(state) > 0) {
	switch (smtp_errno) {

	default:
	    msg_panic("smtp_connect: bad error indication %d", smtp_errno);

	case SMTP_ERR_LOOP:
	case SMTP_ERR_FAIL:

	    /*
	     * The fall-back destination did not resolve as expected, or it
	     * is refusing to talk to us, or mail for it loops back to us.
	     */
	    if (sites->argc > 1 && cpp > sites->argv) {
		msg_warn("%s configuration problem", VAR_FALLBACK_RELAY);
		smtp_errno = SMTP_ERR_RETRY;
	    }

	    /*
	     * The next-hop relayhost did not resolve as expected, or it is
	     * refusing to talk to us, or mail for it loops back to us.
	     */
	    else if (strcmp(sites->argv[0], var_relayhost) == 0) {
		msg_warn("%s configuration problem", VAR_RELAYHOST);
		smtp_errno = SMTP_ERR_RETRY;
	    }

	    /*
	     * Mail for the next-hop destination loops back to myself. Pass
	     * the mail to the best_mx_transport or bounce it.
	     */
	    else if (smtp_errno == SMTP_ERR_LOOP && *var_bestmx_transp) {
		state->status = deliver_pass_all(MAIL_CLASS_PRIVATE,
						 var_bestmx_transp,
						 request);
		SMTP_RCPT_LEFT(state) = 0;	/* XXX */
		break;
	    }
	    /* FALLTHROUGH */

	case SMTP_ERR_RETRY:

	    /*
	     * We still need to bounce or defer some left-over recipients:
	     * either mail loops or some backup mail server was unavailable.
	     */
	    state->final_server = 1;		/* XXX */
	    smtp_site_fail(state, smtp_errno == SMTP_ERR_RETRY ? 450 : 550,
			   "%s", vstring_str(why));

	    /*
	     * Sanity check. Don't silently lose recipients.
	     */
	    smtp_rcpt_cleanup(state);
	    if (SMTP_RCPT_LEFT(state) > 0)
		msg_panic("smtp_connect: left-over recipients");
	}
    }

    /*
     * Cleanup.
     */
    argv_free(sites);
    vstring_free(why);
    return (state->status);
}
