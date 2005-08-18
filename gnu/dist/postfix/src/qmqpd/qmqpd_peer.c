/*	$NetBSD: qmqpd_peer.c,v 1.1.1.4 2005/08/18 21:08:44 rpaulo Exp $	*/

/*++
/* NAME
/*	qmqpd_peer 3
/* SUMMARY
/*	look up peer name/address information
/* SYNOPSIS
/*	#include "qmqpd.h"
/*
/*	void	qmqpd_peer_init(state)
/*	QMQPD_STATE *state;
/*
/*	void	qmqpd_peer_reset(state)
/*	QMQPD_STATE *state;
/* DESCRIPTION
/*	The qmqpd_peer_init() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*
/*	qmqpd_peer_init() updates the following fields:
/* .IP name
/*	The client hostname. An unknown name is represented by the
/*	string "unknown".
/* .IP addr
/*	Printable representation of the client address.
/* .IP namaddr
/*	String of the form: "name[addr]".
/* .PP
/*	qmqpd_peer_reset() releases memory allocated by qmqpd_peer_init().
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
#include <stdio.h>			/* strerror() */
#include <errno.h>
#include <netdb.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_proto.h>
#include <valid_mailhost_addr.h>

/* Application-specific. */

#include "qmqpd.h"

/* qmqpd_peer_init - initialize peer information */

void    qmqpd_peer_init(QMQPD_STATE *state)
{
    char   *myname = "qmqpd_peer_init";
    struct sockaddr_storage ss;
    struct sockaddr *sa;
    SOCKADDR_SIZE sa_len;
    INET_PROTO_INFO *proto_info = inet_proto_info();

    sa = (struct sockaddr *) & ss;
    sa_len = sizeof(ss);

    /*
     * Look up the peer address information.
     */
    if (getpeername(vstream_fileno(state->client), sa, &sa_len) >= 0) {
	errno = 0;
    }

    /*
     * If peer went away, give up.
     */
    if (errno == ECONNRESET || errno == ECONNABORTED) {
	state->name = mystrdup(CLIENT_NAME_UNKNOWN);
	state->addr = mystrdup(CLIENT_ADDR_UNKNOWN);
	state->rfc_addr = mystrdup(CLIENT_ADDR_UNKNOWN);
    }

    /*
     * Convert the client address to printable address and hostname.
     */
    else if (errno == 0
	     && strchr((char *) proto_info->sa_family_list, sa->sa_family)) {
	MAI_HOSTNAME_STR client_name;
	MAI_HOSTADDR_STR client_addr;
	int     aierr;
	char   *colonp;

	/*
	 * Convert the client address to printable form.
	 */
	if ((aierr = sockaddr_to_hostaddr(sa, sa_len, &client_addr,
					  (MAI_SERVPORT_STR *) 0, 0)) != 0)
	    msg_fatal("%s: cannot convert client address to string: %s",
		      myname, MAI_STRERROR(aierr));

	/*
	 * We convert IPv4-in-IPv6 address to 'true' IPv4 address early on,
	 * but only if IPv4 support is enabled (why would anyone want to turn
	 * it off)? With IPv4 support enabled we have no need for the IPv6
	 * form in logging, hostname verification and access checks.
	 */
#ifdef HAS_IPV6
	if (sa->sa_family == AF_INET6) {
	    if (strchr((char *) proto_info->sa_family_list, AF_INET) != 0
		&& IN6_IS_ADDR_V4MAPPED(&SOCK_ADDR_IN6_ADDR(sa))
		&& (colonp = strrchr(client_addr.buf, ':')) != 0) {
		struct addrinfo *res0;

		if (msg_verbose > 1)
		    msg_info("%s: rewriting V4-mapped address \"%s\" to \"%s\"",
			     myname, client_addr.buf, colonp + 1);

		state->addr = mystrdup(colonp + 1);
		state->rfc_addr = mystrdup(colonp + 1);
		aierr = hostaddr_to_sockaddr(state->addr, (char *) 0, 0, &res0);
		if (aierr)
		    msg_fatal("%s: cannot convert %s from string to binary: %s",
			      myname, state->addr, MAI_STRERROR(aierr));
		sa_len = res0->ai_addrlen;
		memcpy((char *) sa, res0->ai_addr, sa_len);
		freeaddrinfo(res0);
	    }

	    /*
	     * Following RFC 2821 section 4.1.3, an IPv6 address literal gets
	     * a prefix of 'IPv6:'. We do this consistently for all IPv6
	     * addresses that that appear in headers or envelopes. The fact
	     * that valid_mailhost_addr() enforces the form helps of course.
	     * We use the form without IPV6: prefix when doing access
	     * control, or when accessing the connection cache.
	     */
	    else {
		state->addr = mystrdup(client_addr.buf);
		state->rfc_addr =
		    concatenate(IPV6_COL, client_addr.buf, (char *) 0);
	    }
	}

	/*
	 * An IPv4 address is in dotted quad decimal form.
	 */
	else
#endif
	{
	    state->addr = mystrdup(client_addr.buf);
	    state->rfc_addr = mystrdup(client_addr.buf);
	}

	/*
	 * Look up and sanity check the client hostname.
	 * 
	 * It is unsafe to allow numeric hostnames, especially because there
	 * exists pressure to turn off the name->addr double check. In that
	 * case an attacker could trivally bypass access restrictions.
	 * 
	 * sockaddr_to_hostname() already rejects malformed or numeric names.
	 */
#define REJECT_PEER_NAME(state) { \
	myfree(state->name); \
	state->name = mystrdup(CLIENT_NAME_UNKNOWN); \
    }

	if ((aierr = sockaddr_to_hostname(sa, sa_len, &client_name,
					  (MAI_SERVNAME_STR *) 0, 0)) != 0) {
	    state->name = mystrdup(CLIENT_NAME_UNKNOWN);
	} else {
	    struct addrinfo *res0;
	    struct addrinfo *res;

	    state->name = mystrdup(client_name.buf);

	    /*
	     * Reject the hostname if it does not list the peer address.
	     */
	    aierr = hostname_to_sockaddr(state->name, (char *) 0, 0, &res0);
	    if (aierr) {
		msg_warn("%s: hostname %s verification failed: %s",
			 state->addr, state->name, MAI_STRERROR(aierr));
		REJECT_PEER_NAME(state);
	    } else {
		for (res = res0; /* void */ ; res = res->ai_next) {
		    if (res == 0) {
			msg_warn("%s: address not listed for hostname %s",
				 state->addr, state->name);
			REJECT_PEER_NAME(state);
			break;
		    }
		    if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
			msg_info("skipping address family %d for host %s",
				 res->ai_family, state->name);
			continue;
		    }
		    if (sock_addr_cmp_addr(res->ai_addr, sa) == 0)
			break;			/* keep peer name */
		}
		freeaddrinfo(res0);
	    }
	}
    }

    /*
     * If it's not Internet, assume the client is local, and avoid using the
     * naming service because that can hang when the machine is disconnected.
     */
    else {
	state->name = mystrdup("localhost");
	state->addr = mystrdup("127.0.0.1");	/* XXX bogus. */
	state->rfc_addr = mystrdup("127.0.0.1");/* XXX bogus. */
    }

    /*
     * Do the name[addr] formatting for pretty reports.
     */
    state->namaddr =
	concatenate(state->name, "[", state->addr, "]", (char *) 0);
}

/* qmqpd_peer_reset - destroy peer information */

void    qmqpd_peer_reset(QMQPD_STATE *state)
{
    myfree(state->name);
    myfree(state->addr);
    myfree(state->namaddr);
    myfree(state->rfc_addr);
}
