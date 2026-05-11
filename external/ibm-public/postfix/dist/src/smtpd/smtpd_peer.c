/*	$NetBSD: smtpd_peer.c,v 1.5.4.1 2026/05/11 17:13:58 martin Exp $	*/

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
/* AUXILIARY METHODS
/*	void	smtpd_peer_from_default(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	The smtpd_peer_init() routine attempts to produce a printable
/*	version of the peer name and address of the specified socket.
/*	Where information is unavailable, the name and/or address
/*	are set to "unknown".
/*
/*	Alternatively, the peer address and port may be obtained
/*	from a proxy server or from attributes that postscreen(8)
/*	passes to smtpd(8) over local IPC.
/*
/*	This module uses the local name service via getaddrinfo()
/*	and getnameinfo(). It does not query the DNS directly.
/*
/*	smtpd_peer_init() updates the following fields:
/* .IP flags.SMTPD_FLAG_HANGUP
/*	This flag is raised when the program should hang up
/*	without reading client input.
/* .IP name
/*	The verified client hostname. This name is represented by
/*	the string "unknown" when 1) the address->name lookup failed,
/*	2) the name->address mapping fails, or 3) the name->address
/*	mapping does not produce the client IP address.
/* .IP reverse_name
/*	The unverified client hostname as found with address->name
/*	lookup; it is not verified for consistency with the client
/*	IP address result from name->address lookup.
/* .IP addr
/*	Printable representation of the client address.
/* .IP addr_family
/*	AF_INET or AF_INET6 in case of an open TCP connection.
/*	AF_UNSPEC in all other cases, including an open non-socket
/*	connection, or a closed connection.
/* .IP namaddr
/*	String of the form: "name[addr]:port".
/* .IP rfc_addr
/*	String of the form "ipv4addr" or "ipv6:ipv6addr" for use
/*	in Received: message headers.
/* .IP dest_addr
/*	Server address, used by the Dovecot authentication server,
/*	available as Milter {daemon_addr} macro, and as server_address
/*	policy delegation attribute.
/* .IP dest_port
/*	Server port, available as Milter {daemon_port} macro, and
/*	as server_port policy delegation attribute.
/* .IP sockaddr_len
/* .IP dest_sockaddr_len
/*	These are initialized to zero, to indicate that the corresponding
/*	sockaddr_storage members are not set.
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
/* .RS
/* .IP 2
/*	The address->name lookup succeeded.
/* .IP 4
/*	The address->name lookup failed with a recoverable error.
/* .IP 5
/*	The address->name lookup failed with an unrecoverable error.
/* .RE
/* .PP
/*	smtpd_peer_reset() releases memory allocated by smtpd_peer_init().
/*
/*	smtpd_peer_from_default() looks up connection information
/*	when an up-stream proxy indicates that a connection is not
/*	proxied.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <htable.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>
#include <split_at.h>
#include <inet_prefix_top.h>

/* Global library. */

#include <mail_proto.h>
#include <valid_mailhost_addr.h>
#include <mail_params.h>
#include <haproxy_srvr.h>

/* Application-specific. */

#include "smtpd.h"

 /*
  * XXX If we make local port information available via logging, then we must
  * also support these attributes with the XFORWARD command.
  * 
  * XXX If support were to be added for Milter applications in down-stream MTAs,
  * then consistency demands that we propagate a lot of Sendmail macro
  * information via the XFORWARD command. Otherwise we could end up with a
  * very confusing situation.
  */

/* smtpd_peer_sockaddr_to_hostaddr - client address/port to printable form */

static int smtpd_peer_sockaddr_to_hostaddr(SMTPD_STATE *state)
{
    const char *myname = "smtpd_peer_sockaddr_to_hostaddr";
    struct sockaddr *sa = (struct sockaddr *) &(state->sockaddr);
    SOCKADDR_SIZE *sa_length = &state->sockaddr_len;

    /*
     * XXX If we're given an IPv6 (or IPv4) connection from, e.g., inetd,
     * while Postfix IPv6 (or IPv4) support is turned off, don't (skip to the
     * final else clause, pretend the origin is localhost[127.0.0.1], and
     * become an open relay).
     */
    if (sa->sa_family == AF_INET
#ifdef AF_INET6
	|| sa->sa_family == AF_INET6
#endif
	) {
	MAI_HOSTADDR_STR client_addr;
	MAI_SERVPORT_STR client_port;
	MAI_HOSTADDR_STR server_addr;
	MAI_SERVPORT_STR server_port;
	int     aierr;

	/*
	 * Sanity check: we can't use sockets that we're not configured for.
	 */
	if (strchr((char *) inet_proto_info()->sa_family_list, sa->sa_family) == 0)
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
	if ((aierr = sane_sockaddr_to_hostaddr(sa, sa_length, &client_addr,
					       &client_port, 0)) != 0)
	    msg_fatal("%s: cannot convert client sockaddr type %s length %ld "
		      "to string: %s", myname,
#ifdef AF_INET6
		      sa->sa_family == AF_INET6 ? "AF_INET6" :
#endif
		      sa->sa_family == AF_INET ? "AF_INET" : "other",
		      (long) *sa_length, MAI_STRERROR(aierr));
	state->port = mystrdup(client_port.buf);

	/*
	 * XXX Require that the infrastructure strips off the IPv6 datalink
	 * suffix to avoid false alarms with strict address syntax checks.
	 */
#ifdef HAS_IPV6
	if (strchr(client_addr.buf, '%') != 0)
	    msg_panic("%s: address %s has datalink suffix",
		      myname, client_addr.buf);

	/*
	 * Following RFC 2821 section 4.1.3, an IPv6 address literal gets a
	 * prefix of 'IPv6:'. We do this consistently for all IPv6 addresses
	 * that appear in headers or envelopes. The fact that
	 * valid_mailhost_addr() enforces the form helps of course. We use
	 * the form without IPV6: prefix when doing access control, or when
	 * accessing the connection cache.
	 */
	if (sa->sa_family == AF_INET6) {
	    state->addr = mystrdup(client_addr.buf);
	    state->rfc_addr =
		concatenate(IPV6_COL, client_addr.buf, (char *) 0);
	    state->addr_family = sa->sa_family;
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
	 * Convert the server address/port to printable form.
	 */
	if ((aierr = sane_sockaddr_to_hostaddr((struct sockaddr *)
					       &state->dest_sockaddr,
					       &state->dest_sockaddr_len,
					       &server_addr,
					       &server_port, 0)) != 0)
	    msg_fatal("%s: cannot convert server sockaddr type %s length %ld "
		      "to string: %s", myname,
#ifdef AF_INET6
		   state->dest_sockaddr.ss_family == AF_INET6 ? "AF_INET6" :
#endif
		      state->dest_sockaddr.ss_family == AF_INET ? "AF_INET" :
		      "other", (long) state->dest_sockaddr_len,
		      MAI_STRERROR(aierr));
	state->dest_addr = mystrdup(server_addr.buf);
	state->dest_port = mystrdup(server_port.buf);

	return (0);
    }

    /*
     * It's not Internet.
     */
    else {
	return (-1);
    }
}

/* smtpd_peer_sockaddr_to_hostname - client hostname lookup */

static void smtpd_peer_sockaddr_to_hostname(SMTPD_STATE *state)
{
    struct sockaddr *sa = (struct sockaddr *) &(state->sockaddr);
    SOCKADDR_SIZE sa_length = state->sockaddr_len;
    MAI_HOSTNAME_STR client_name;
    int     aierr;

    /*
     * Look up and sanity check the client hostname.
     * 
     * It is unsafe to allow numeric hostnames, especially because there exists
     * pressure to turn off the name->addr double check. In that case an
     * attacker could trivally bypass access restrictions.
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
	 * Reject the hostname if it does not list the peer address. Without
	 * further validation or qualification, such information must not be
	 * allowed to enter the audit trail, as people would draw false
	 * conclusions.
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
		if (strchr((char *) inet_proto_info()->sa_family_list, res->ai_family) == 0) {
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

/* smtpd_peer_hostaddr_to_sockaddr - convert numeric string to binary */

static void smtpd_peer_hostaddr_to_sockaddr(SMTPD_STATE *state)
{
    const char *myname = "smtpd_peer_hostaddr_to_sockaddr";
    struct addrinfo *res;
    int     aierr;

    /*
     * The client binary address value is provided by non-error peer lookup
     * methods. It is used to compute the anvil aggregation prefix for client
     * IP addresses.
     */
    if ((aierr = hostaddr_to_sockaddr(state->addr, state->port,
				      SOCK_STREAM, &res)) != 0)
	msg_fatal("%s: cannot convert client address '%s' port '%s' to binary: %s",
		  myname, state->addr, state->port, MAI_STRERROR(aierr));
    if (res->ai_addrlen > sizeof(state->sockaddr))
	msg_panic("%s: address length > struct sockaddr_storage", myname);
    memcpy((void *) &(state->sockaddr), res->ai_addr, res->ai_addrlen);
    state->sockaddr_len = res->ai_addrlen;
    freeaddrinfo(res);

    /*
     * The server binary address is provided by non-error peer lookup
     * methods. It is currently unused, but it is the result of a hermetic
     * conversion, therefore low-risk.
     */
    if ((aierr = hostaddr_to_sockaddr(state->dest_addr, state->dest_port,
				      SOCK_STREAM, &res)) != 0)
	msg_fatal("%s: cannot convert server address '%s' port '%s' to binary: %s",
	   myname, state->dest_addr, state->dest_port, MAI_STRERROR(aierr));
    if (res->ai_addrlen > sizeof(state->dest_sockaddr))
	msg_panic("%s: address length > struct sockaddr_storage", myname);
    memcpy((void *) &(state->dest_sockaddr), res->ai_addr, res->ai_addrlen);
    state->dest_sockaddr_len = res->ai_addrlen;
    freeaddrinfo(res);
}

/* smtpd_peer_assume_local_client - non-Internet endpoint */

static void smtpd_peer_assume_local_client(SMTPD_STATE *state)
{

    /*
     * If it's not Internet, assume the client is local, and avoid using the
     * naming service because that can hang when the machine is disconnected.
     */
    state->name = mystrdup("localhost");
    state->reverse_name = mystrdup("localhost");
#ifdef AF_INET6
    if (inet_proto_info()->sa_family_list[0] == PF_INET6) {
	state->addr = mystrdup("::1");		/* XXX bogus. */
	state->rfc_addr = mystrdup(IPV6_COL "::1");	/* XXX bogus. */
    } else
#endif
    {
	state->addr = mystrdup("127.0.0.1");	/* XXX bogus. */
	state->rfc_addr = mystrdup("127.0.0.1");/* XXX bogus. */
    }
    state->addr_family = AF_UNSPEC;
    state->name_status = SMTPD_PEER_CODE_OK;
    state->reverse_name_status = SMTPD_PEER_CODE_OK;
    state->port = mystrdup("0");		/* XXX bogus. */

    state->dest_addr = mystrdup(state->addr);	/* XXX bogus. */
    state->dest_port = mystrdup(state->port);	/* XXX bogus. */

    state->sockaddr_len = 0;
    state->dest_sockaddr_len = 0;
}

/* smtpd_peer_assume_unknown_client - peer went away, or peer info unavailable */

static void smtpd_peer_assume_unknown_client(SMTPD_STATE *state)
{
    state->name = mystrdup(CLIENT_NAME_UNKNOWN);
    state->reverse_name = mystrdup(CLIENT_NAME_UNKNOWN);
    state->addr = mystrdup(CLIENT_ADDR_UNKNOWN);
    state->rfc_addr = mystrdup(CLIENT_ADDR_UNKNOWN);
    state->addr_family = AF_UNSPEC;
    state->name_status = SMTPD_PEER_CODE_PERM;
    state->reverse_name_status = SMTPD_PEER_CODE_PERM;
    state->port = mystrdup(CLIENT_PORT_UNKNOWN);

    state->dest_addr = mystrdup(SERVER_ADDR_UNKNOWN);
    state->dest_port = mystrdup(SERVER_PORT_UNKNOWN);

    state->sockaddr_len = 0;
    state->dest_sockaddr_len = 0;
}

/* smtpd_peer_from_pass_attr - initialize from attribute hash */

static int smtpd_peer_from_pass_attr(SMTPD_STATE *state)
{
    HTABLE *attr = (HTABLE *) vstream_context(state->client);
    const char *cp;

#define BAD_PASS_ATTR(...) do { \
	msg_warn(__VA_ARGS__); \
	return (-1); \
    } while (0)

    /*
     * Extract the client endpoint information from the attribute hash.
     */
    if ((cp = htable_find(attr, MAIL_ATTR_ACT_CLIENT_ADDR)) == 0)
	BAD_PASS_ATTR("missing client address from proxy");
    if (strrchr(cp, ':') != 0) {
	if (valid_ipv6_hostaddr(cp, DO_GRIPE) == 0)
	    BAD_PASS_ATTR("bad IPv6 client address syntax from proxy: %s", cp);
	state->addr = mystrdup(cp);
	state->rfc_addr = concatenate(IPV6_COL, cp, (char *) 0);
	state->addr_family = AF_INET6;
    } else {
	if (valid_ipv4_hostaddr(cp, DO_GRIPE) == 0)
	    BAD_PASS_ATTR("bad IPv4 client address syntax from proxy: %s", cp);
	state->addr = mystrdup(cp);
	state->rfc_addr = mystrdup(cp);
	state->addr_family = AF_INET;
    }
    if ((cp = htable_find(attr, MAIL_ATTR_ACT_CLIENT_PORT)) == 0)
	BAD_PASS_ATTR("missing client port from proxy");
    if (valid_hostport(cp, DO_GRIPE) == 0)
	BAD_PASS_ATTR("bad TCP client port number syntax from proxy: %s", cp);
    state->port = mystrdup(cp);

    /*
     * The Dovecot authentication server needs the server IP address.
     */
    if ((cp = htable_find(attr, MAIL_ATTR_ACT_SERVER_ADDR)) == 0)
	BAD_PASS_ATTR("missing server address from proxy");
    if (valid_hostaddr(cp, DO_GRIPE) == 0)
	BAD_PASS_ATTR("bad IPv6 server address syntax from proxy: %s", cp);
    state->dest_addr = mystrdup(cp);

    if ((cp = htable_find(attr, MAIL_ATTR_ACT_SERVER_PORT)) == 0)
	BAD_PASS_ATTR("missing server port from proxy");
    if (valid_hostport(cp, DO_GRIPE) == 0)
	BAD_PASS_ATTR("bad TCP server port number syntax from proxy: %s", cp);
    state->dest_port = mystrdup(cp);

    /*
     * Convert the client address from string to binary form.
     */
    smtpd_peer_hostaddr_to_sockaddr(state);
    return (0);
}

/* smtpd_peer_from_default - try to initialize peer information from socket */

void    smtpd_peer_from_default(SMTPD_STATE *state)
{

    /*
     * The "no client" routine provides surrogate information so that the
     * application can produce sensible logging when a client disconnects
     * before the server wakes up. The "assume local" routine provides
     * surrogate state for open, presumably, local, IPC channels.
     */
    state->sockaddr_len = sizeof(state->sockaddr);
    state->dest_sockaddr_len = sizeof(state->dest_sockaddr);
    if (getpeername(vstream_fileno(state->client),
		    (struct sockaddr *) &state->sockaddr,
		    &state->sockaddr_len) <0
	|| getsockname(vstream_fileno(state->client),
		       (struct sockaddr *) &state->dest_sockaddr,
		       &state->dest_sockaddr_len) < 0) {
	if (errno == ENOTSOCK)
	    smtpd_peer_assume_local_client(state);
	else
	    smtpd_peer_assume_unknown_client(state);
    } else {
	if (smtpd_peer_sockaddr_to_hostaddr(state) < 0)
	    smtpd_peer_assume_local_client(state);
    }
}

/* smtpd_peer_fall_back_and_hangup - recover after incomplete peer info */

static void smtpd_peer_fall_back_and_hangup(SMTPD_STATE *state)
{

    /*
     * Clear incomplete endpoint info. Populate the SMTPD_STATE with default
     * endpoint info, so that the caller won't trip over a null pointer. Hang
     * up before accepting input: we don't know what we're talking to and
     * what rights they might have.
     */
    smtpd_peer_reset(state);
    smtpd_peer_assume_unknown_client(state);
    state->flags |= SMTPD_FLAG_HANGUP;
}

/* smtpd_peer_from_proxy - get endpoint info from proxy agent */

static int smtpd_peer_from_proxy(SMTPD_STATE *state)
{
    typedef struct {
	const char *name;
	int     (*endpt_lookup) (SMTPD_STATE *);
    } SMTPD_ENDPT_LOOKUP_INFO;
    static const SMTPD_ENDPT_LOOKUP_INFO smtpd_endpt_lookup_info[] = {
	HAPROXY_PROTO_NAME, smtpd_peer_from_haproxy,

	/*
	 * See smtpd_haproxy.c for the a summary of the information that a
	 * proxy endpoint lookup function is expected to provide.
	 */
	0,
    };
    const SMTPD_ENDPT_LOOKUP_INFO *pp;

    /*
     * When the proxy information is unavailable, we can't maintain an audit
     * trail or enforce access control, therefore we forcibly hang up.
     */
    for (pp = smtpd_endpt_lookup_info; /* see below */ ; pp++) {
	if (pp->name == 0)
	    msg_fatal("unsupported %s value: %s",
		      VAR_SMTPD_UPROXY_PROTO, var_smtpd_uproxy_proto);
	if (strcmp(var_smtpd_uproxy_proto, pp->name) == 0)
	    return (pp->endpt_lookup(state));
    }
}

/* smtpd_peer_init - initialize peer information */

void    smtpd_peer_init(SMTPD_STATE *state)
{

    /*
     * Prepare for partial initialization after error.
     */
    memset((void *) &(state->sockaddr), 0, sizeof(state->sockaddr));
    state->sockaddr_len = 0;
    state->name = 0;
    state->reverse_name = 0;
    state->addr = 0;
    state->namaddr = 0;
    state->rfc_addr = 0;
    state->port = 0;
    state->anvil_range = 0;
    state->dest_addr = 0;
    state->dest_port = 0;
    state->dest_sockaddr_len = 0;

    /*
     * Determine the remote SMTP client address and port.
     * 
     * If we can't process the connection hand-off info from postscreen or
     * proxy, fall back to some default endpoint info for logging and force a
     * hangup. We can't determine what rights the peer should have.
     * 
     * XXX In stand-alone mode, don't assume that the peer will be a local
     * process. That could introduce a gaping hole when the SMTP daemon is
     * hooked up to the network via inetd or some other super-server.
     */
    if (vstream_context(state->client) != 0) {
	if (smtpd_peer_from_pass_attr(state) < 0)
	    smtpd_peer_fall_back_and_hangup(state);
	if (*var_smtpd_uproxy_proto != 0)
	    msg_warn("ignoring non-empty %s setting behind postscreen",
		     VAR_SMTPD_UPROXY_PROTO);
    } else if (SMTPD_STAND_ALONE(state) || *var_smtpd_uproxy_proto == 0) {
	smtpd_peer_from_default(state);
    } else {
	if (smtpd_peer_from_proxy(state) < 0)
	    smtpd_peer_fall_back_and_hangup(state);
    }

    /*
     * Determine the remote SMTP client hostname. Note: some of the handlers
     * above provide surrogate endpoint information in case of error. In that
     * case, leave the surrogate information alone.
     */
    if (state->name == 0)
	smtpd_peer_sockaddr_to_hostname(state);

    /*
     * Do the name[addr]:port formatting for pretty reports.
     */
    state->namaddr = SMTPD_BUILD_NAMADDRPORT(state->name, state->addr,
					     state->port);

    /*
     * Generate the 'address' or 'net/mask' index for anvil event
     * aggregation.
     */

    if (state->addr_family != AF_UNSPEC) {
	state->anvil_range = inet_prefix_top(state->addr_family,
					SOCK_ADDR_ADDRP(&(state->sockaddr)),
					     state->addr_family == AF_INET ?
					     var_smtpd_cipv4_prefix :
					     var_smtpd_cipv6_prefix);
    }
}

/* smtpd_peer_reset - destroy peer information */

void    smtpd_peer_reset(SMTPD_STATE *state)
{
#define MYFREE_AND_ZERO(e) do { myfree(e); (e) = 0; } while (0);

    if (state->name)
	MYFREE_AND_ZERO(state->name);
    if (state->reverse_name)
	MYFREE_AND_ZERO(state->reverse_name);
    if (state->addr)
	MYFREE_AND_ZERO(state->addr);
    if (state->namaddr)
	MYFREE_AND_ZERO(state->namaddr);
    if (state->rfc_addr)
	MYFREE_AND_ZERO(state->rfc_addr);
    if (state->port)
	MYFREE_AND_ZERO(state->port);
    if (state->dest_addr)
	MYFREE_AND_ZERO(state->dest_addr);
    if (state->dest_port)
	MYFREE_AND_ZERO(state->dest_port);
    if (state->anvil_range)
	MYFREE_AND_ZERO(state->anvil_range);
}
