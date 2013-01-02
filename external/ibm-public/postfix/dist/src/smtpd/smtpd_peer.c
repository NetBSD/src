/*	$NetBSD: smtpd_peer.c,v 1.1.1.3 2013/01/02 18:59:09 tron Exp $	*/

/*++
/* NAME
/*	smtpd_peer 3
/* SUMMARY
/*	look up peer name/address information
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	void	smtpd_peer_init(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_peer_reset(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	The smtpd_peer_init() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*
/*	This module uses the local name service via getaddrinfo()
/*	and getnameinfo(). It does not query the DNS directly.
/*
/*	smtpd_peer_init() updates the following fields:
/* .IP name
/*	The verified client hostname. This name is represented by
/*	the string "unknown" when 1) the address->name lookup failed,
/*	2) the name->address mapping fails, or 3) the name->address
/*	mapping does not produce the client IP address.
/* .IP reverse_name
/*	The unverified client hostname as found with address->name
/*	lookup; it is not verified for consistency with the client
/*	IP address result from name->address lookup.
/* .IP forward_name
/*	The unverified client hostname as found with address->name
/*	lookup followed by name->address lookup; it is not verified
/*	for consistency with the result from address->name lookup.
/*	For example, when the address->name lookup produces as
/*	hostname an alias, the name->address lookup will produce
/*	as hostname the expansion of that alias, so that the two
/*	lookups produce different names.
/* .IP addr
/*	Printable representation of the client address.
/* .IP namaddr
/*	String of the form: "name[addr]:port".
/* .IP rfc_addr
/*      String of the form "ipv4addr" or "ipv6:ipv6addr" for use
/*	in Received: message headers.
/* .IP name_status
/*	The name_status result field specifies how the name
/*	information should be interpreted:
/* .RS
/* .IP 2
/*	The address->name lookup and name->address lookup produced
/*	the client IP address.
/* .IP 4
/*	The address->name lookup or name->address lookup failed
/*	with a recoverable error.
/* .IP 5
/*	The address->name lookup or name->address lookup failed
/*	with an unrecoverable error, or the result did not match
/*	the client IP address.
/* .RE
/* .IP reverse_name_status
/*	The reverse_name_status result field specifies how the
/*	reverse_name information should be interpreted:
/* .RS .IP 2
/*	The address->name lookup succeeded.
/* .IP 4
/*	The address->name lookup failed with a recoverable error.
/* .IP 5
/*	The address->name lookup failed with an unrecoverable error.
/* .RE .IP forward_name_status
/*	The forward_name_status result field specifies how the
/*	forward_name information should be interpreted:
/* .RS .IP 2
/*	The address->name and name->address lookup succeeded.
/* .IP 4
/*	The address->name lookup or name->address failed with a
/*	recoverable error.
/* .IP 5
/*	The address->name lookup or name->address failed with an
/*	unrecoverable error.
/* .RE
/* .PP
/*	smtpd_peer_reset() releases memory allocated by smtpd_peer_init().
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
#include <split_at.h>

/* Global library. */

#include <mail_proto.h>
#include <valid_mailhost_addr.h>
#include <mail_params.h>

/* Application-specific. */

#include "smtpd.h"

/* smtpd_peer_init - initialize peer information */

void    smtpd_peer_init(SMTPD_STATE *state)
{
    const char *myname = "smtpd_peer_init";
    SOCKADDR_SIZE sa_length;
    struct sockaddr *sa;
    INET_PROTO_INFO *proto_info = inet_proto_info();

    sa = (struct sockaddr *) & (state->sockaddr);
    sa_length = sizeof(state->sockaddr);

    /*
     * Look up the peer address information.
     * 
     * XXX If we make local endpoint (getsockname) information available to
     * Milter applications as {if_name} and {if_addr}, then we also must be
     * able to provide this via the XCLIENT command for Milter testing.
     * 
     * XXX If we make local or remote port information available to policy
     * servers or Milter applications, then we must also make this testable
     * with the XCLIENT command, otherwise there will be confusion.
     * 
     * XXX If we make local or remote port information available via logging,
     * then we must also support these attributes with the XFORWARD command.
     * 
     * XXX If support were to be added for Milter applications in down-stream
     * MTAs, then consistency demands that we propagate a lot of Sendmail
     * macro information via the XFORWARD command. Otherwise we could end up
     * with a very confusing situation.
     */
    if (getpeername(vstream_fileno(state->client), sa, &sa_length) >= 0) {
	errno = 0;
    }

    /*
     * If peer went away, give up.
     */
    if (errno != 0 && errno != ENOTSOCK) {
	state->name = mystrdup(CLIENT_NAME_UNKNOWN);
	state->reverse_name = mystrdup(CLIENT_NAME_UNKNOWN);
	state->addr = mystrdup(CLIENT_ADDR_UNKNOWN);
	state->rfc_addr = mystrdup(CLIENT_ADDR_UNKNOWN);
	state->addr_family = AF_UNSPEC;
	state->name_status = SMTPD_PEER_CODE_PERM;
	state->reverse_name_status = SMTPD_PEER_CODE_PERM;
	state->port = mystrdup(CLIENT_PORT_UNKNOWN);
    }

    /*
     * Convert the client address to printable address and hostname.
     * 
     * XXX If we're given an IPv6 (or IPv4) connection from, e.g., inetd, while
     * Postfix IPv6 (or IPv4) support is turned off, don't (skip to the final
     * else clause, pretend the origin is localhost[127.0.0.1], and become an
     * open relay).
     */
    else if (errno == 0
	     && (sa->sa_family == AF_INET
#ifdef AF_INET6
		 || sa->sa_family == AF_INET6
#endif
		 )) {
	MAI_HOSTNAME_STR client_name;
	MAI_HOSTADDR_STR client_addr;
	MAI_SERVPORT_STR client_port;
	int     aierr;
	char   *colonp;

	/*
	 * Sanity check: we can't use sockets that we're not configured for.
	 */
	if (strchr((char *) proto_info->sa_family_list, sa->sa_family) == 0)
	    msg_fatal("cannot handle socket type %s with \"%s = %s\"",
#ifdef AF_INET6
		      sa->sa_family == AF_INET6 ? "AF_INET6" :
#endif
		      sa->sa_family == AF_INET ? "AF_INET" :
		      "other", VAR_INET_PROTOCOLS, var_inet_protocols);

	/*
	 * Sorry, but there are some things that we just cannot do while
	 * connected to the network.
	 */
	if (geteuid() != var_owner_uid || getuid() != var_owner_uid) {
	    msg_error("incorrect SMTP server privileges: uid=%lu euid=%lu",
		      (unsigned long) getuid(), (unsigned long) geteuid());
	    msg_fatal("the Postfix SMTP server must run with $%s privileges",
		      VAR_MAIL_OWNER);
	}

	/*
	 * Convert the client address to printable form.
	 */
	if ((aierr = sockaddr_to_hostaddr(sa, sa_length, &client_addr,
					  &client_port, 0)) != 0)
	    msg_fatal("%s: cannot convert client address/port to string: %s",
		      myname, MAI_STRERROR(aierr));
	state->port = mystrdup(client_port.buf);

	/*
	 * XXX Strip off the IPv6 datalink suffix to avoid false alarms with
	 * strict address syntax checks.
	 */
#ifdef HAS_IPV6
	(void) split_at(client_addr.buf, '%');
#endif

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
		state->addr_family = AF_INET;
		aierr = hostaddr_to_sockaddr(state->addr, (char *) 0, 0, &res0);
		if (aierr)
		    msg_fatal("%s: cannot convert %s from string to binary: %s",
			      myname, state->addr, MAI_STRERROR(aierr));
		sa_length = res0->ai_addrlen;
		if (sa_length > sizeof(state->sockaddr))
		    sa_length = sizeof(state->sockaddr);
		memcpy((char *) sa, res0->ai_addr, sa_length);
		freeaddrinfo(res0);		/* 200412 */
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
		state->addr_family = sa->sa_family;
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
	    state->addr_family = sa->sa_family;
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
#define TEMP_AI_ERROR(e) \
	((e) == EAI_AGAIN || (e) == EAI_MEMORY || (e) == EAI_SYSTEM)

#define REJECT_PEER_NAME(state, code) { \
	myfree(state->name); \
	state->name = mystrdup(CLIENT_NAME_UNKNOWN); \
	state->name_status = code; \
    }

	if (var_smtpd_peername_lookup == 0) {
	    state->name = mystrdup(CLIENT_NAME_UNKNOWN);
	    state->reverse_name = mystrdup(CLIENT_NAME_UNKNOWN);
	    state->name_status = SMTPD_PEER_CODE_PERM;
	    state->reverse_name_status = SMTPD_PEER_CODE_PERM;
	} else if ((aierr = sockaddr_to_hostname(sa, sa_length, &client_name,
					 (MAI_SERVNAME_STR *) 0, 0)) != 0) {
	    state->name = mystrdup(CLIENT_NAME_UNKNOWN);
	    state->reverse_name = mystrdup(CLIENT_NAME_UNKNOWN);
	    state->name_status = (TEMP_AI_ERROR(aierr) ?
			       SMTPD_PEER_CODE_TEMP : SMTPD_PEER_CODE_PERM);
	    state->reverse_name_status = (TEMP_AI_ERROR(aierr) ?
			       SMTPD_PEER_CODE_TEMP : SMTPD_PEER_CODE_PERM);
	} else {
	    struct addrinfo *res0;
	    struct addrinfo *res;

	    state->name = mystrdup(client_name.buf);
	    state->reverse_name = mystrdup(client_name.buf);
	    state->name_status = SMTPD_PEER_CODE_OK;
	    state->reverse_name_status = SMTPD_PEER_CODE_OK;

	    /*
	     * Reject the hostname if it does not list the peer address.
	     * Without further validation or qualification, such information
	     * must not be allowed to enter the audit trail, as people would
	     * draw false conclusions.
	     */
	    aierr = hostname_to_sockaddr_pf(state->name, state->addr_family,
					    (char *) 0, 0, &res0);
	    if (aierr) {
		msg_warn("hostname %s does not resolve to address %s: %s",
			 state->name, state->addr, MAI_STRERROR(aierr));
		REJECT_PEER_NAME(state, (TEMP_AI_ERROR(aierr) ?
			    SMTPD_PEER_CODE_TEMP : SMTPD_PEER_CODE_FORGED));
	    } else {
		for (res = res0; /* void */ ; res = res->ai_next) {
		    if (res == 0) {
			msg_warn("hostname %s does not resolve to address %s",
				 state->name, state->addr);
			REJECT_PEER_NAME(state, SMTPD_PEER_CODE_FORGED);
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
	state->reverse_name = mystrdup("localhost");
	if (proto_info->sa_family_list[0] == PF_INET6) {
	    state->addr = mystrdup("::1");	/* XXX bogus. */
	    state->rfc_addr = mystrdup(IPV6_COL "::1");	/* XXX bogus. */
	} else {
	    state->addr = mystrdup("127.0.0.1");/* XXX bogus. */
	    state->rfc_addr = mystrdup("127.0.0.1");	/* XXX bogus. */
	}
	state->addr_family = AF_UNSPEC;
	state->name_status = SMTPD_PEER_CODE_OK;
	state->reverse_name_status = SMTPD_PEER_CODE_OK;
	state->port = mystrdup("0");		/* XXX bogus. */
    }

    /*
     * Do the name[addr]:port formatting for pretty reports.
     */
    state->namaddr = SMTPD_BUILD_NAMADDRPORT(state->name, state->addr,
					     state->port);
}

/* smtpd_peer_reset - destroy peer information */

void    smtpd_peer_reset(SMTPD_STATE *state)
{
    myfree(state->name);
    myfree(state->reverse_name);
    myfree(state->addr);
    myfree(state->namaddr);
    myfree(state->rfc_addr);
    myfree(state->port);
}
