/*	$NetBSD: smtp_connect.c,v 1.1.1.4.2.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	smtp_connect 3
/* SUMMARY
/*	connect to SMTP/LMTP server and deliver
/* SYNOPSIS
/*	#include "smtp.h"
/*
/*	int	smtp_connect(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	This module implements SMTP/LMTP connection management and controls
/*	mail delivery.
/*
/*	smtp_connect() attempts to establish an SMTP/LMTP session with a host
/*	that represents the destination domain, or with an optional fallback
/*	relay when {the destination cannot be found, or when all the
/*	destination servers are unavailable}. It skips over IP addresses
/*	that fail to complete the SMTP/LMTP handshake and tries to find
/*	an alternate server when an SMTP/LMTP session fails to deliver.
/*
/*	This layer also controls what connections are retrieved from
/*	the connection cache, and what connections are saved to the cache.
/*
/*	The destination is either a host (or domain) name or a numeric
/*	address. Symbolic or numeric service port information may be
/*	appended, separated by a colon (":"). In the case of LMTP,
/*	destinations may be specified as "unix:pathname", "inet:host"
/*	or "inet:host:port".
/*
/*	With SMTP, the Internet domain name service is queried for mail
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
/*
/*	Connection caching in cooperation with:
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#ifndef IPPORT_SMTP
#define IPPORT_SMTP 25
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
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_params.h>
#include <own_inet_addr.h>
#include <deliver_pass.h>
#include <mail_error.h>
#include <dsn_buf.h>
#include <mail_addr.h>

/* DNS library. */

#include <dns.h>

/* Application-specific. */

#include <smtp.h>
#include <smtp_addr.h>
#include <smtp_reuse.h>

 /*
  * Forward declaration.
  */
static SMTP_SESSION *smtp_connect_sock(int, struct sockaddr *, int,
				               SMTP_ITERATOR *, DSN_BUF *,
				               int);

/* smtp_connect_unix - connect to UNIX-domain address */

static SMTP_SESSION *smtp_connect_unix(SMTP_ITERATOR *iter, DSN_BUF *why,
				               int sess_flags)
{
    const char *myname = "smtp_connect_unix";
    struct sockaddr_un sock_un;
    const char *addr = STR(iter->addr);
    int     len = strlen(addr);
    int     sock;

    dsb_reset(why);				/* Paranoia */

    /*
     * Sanity checks.
     */
    if (len >= (int) sizeof(sock_un.sun_path)) {
	msg_warn("unix-domain name too long: %s", addr);
	dsb_simple(why, "4.3.5", "Server configuration error");
	return (0);
    }

    /*
     * Initialize.
     */
    memset((char *) &sock_un, 0, sizeof(sock_un));
    sock_un.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
    sock_un.sun_len = len + 1;
#endif
    memcpy(sock_un.sun_path, addr, len + 1);

    /*
     * Create a client socket.
     */
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    /*
     * Connect to the server.
     */
    if (msg_verbose)
	msg_info("%s: trying: %s...", myname, addr);

    return (smtp_connect_sock(sock, (struct sockaddr *) &sock_un,
			      sizeof(sock_un), iter, why, sess_flags));
}

/* smtp_connect_addr - connect to explicit address */

static SMTP_SESSION *smtp_connect_addr(SMTP_ITERATOR *iter, DSN_BUF *why,
				               int sess_flags)
{
    const char *myname = "smtp_connect_addr";
    struct sockaddr_storage ss;		/* remote */
    struct sockaddr *sa = (struct sockaddr *) &ss;
    SOCKADDR_SIZE salen = sizeof(ss);
    MAI_HOSTADDR_STR hostaddr;
    DNS_RR *addr = iter->rr;
    unsigned port = iter->port;
    int     sock;
    char   *bind_addr;
    char   *bind_var;

    dsb_reset(why);				/* Paranoia */

    /*
     * Sanity checks.
     */
    if (dns_rr_to_sa(addr, port, sa, &salen) != 0) {
	msg_warn("%s: skip address type %s: %m",
		 myname, dns_strtype(addr->type));
	dsb_simple(why, "4.4.0", "network address conversion failed: %m");
	return (0);
    }

    /*
     * Initialize.
     */
    if ((sock = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    if (inet_windowsize > 0)
	set_inet_windowsize(sock, inet_windowsize);

    /*
     * Allow the sysadmin to specify the source address, for example, as "-o
     * smtp_bind_address=x.x.x.x" in the master.cf file.
     */
#ifdef HAS_IPV6
    if (sa->sa_family == AF_INET6) {
	bind_addr = var_smtp_bind_addr6;
	bind_var = SMTP_X(BIND_ADDR6);
    } else
#endif
    if (sa->sa_family == AF_INET) {
	bind_addr = var_smtp_bind_addr;
	bind_var = SMTP_X(BIND_ADDR);
    } else
	bind_var = bind_addr = "";
    if (*bind_addr) {
	int     aierr;
	struct addrinfo *res0;

	if ((aierr = hostaddr_to_sockaddr(bind_addr, (char *) 0, 0, &res0)) != 0)
	    msg_fatal("%s: bad %s parameter: %s: %s",
		      myname, bind_var, bind_addr, MAI_STRERROR(aierr));
	if (bind(sock, res0->ai_addr, res0->ai_addrlen) < 0)
	    msg_warn("%s: bind %s: %m", myname, bind_addr);
	else if (msg_verbose)
	    msg_info("%s: bind %s", myname, bind_addr);
	freeaddrinfo(res0);
    }

    /*
     * When running as a virtual host, bind to the virtual interface so that
     * the mail appears to come from the "right" machine address.
     * 
     * XXX The IPv6 patch expands the null host (as client endpoint) and uses
     * the result as the loopback address list.
     */
    else {
	int     count = 0;
	struct sockaddr *own_addr = 0;
	INET_ADDR_LIST *addr_list = own_inet_addr_list();
	struct sockaddr_storage *s;

	for (s = addr_list->addrs; s < addr_list->addrs + addr_list->used; s++) {
	    if (SOCK_ADDR_FAMILY(s) == sa->sa_family) {
		if (count++ > 0)
		    break;
		own_addr = SOCK_ADDR_PTR(s);
	    }
	}
	if (count == 1 && !sock_addr_in_loopback(own_addr)) {
	    if (bind(sock, own_addr, SOCK_ADDR_LEN(own_addr)) < 0) {
		SOCKADDR_TO_HOSTADDR(own_addr, SOCK_ADDR_LEN(own_addr),
				     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
		msg_warn("%s: bind %s: %m", myname, hostaddr.buf);
	    } else if (msg_verbose) {
		SOCKADDR_TO_HOSTADDR(own_addr, SOCK_ADDR_LEN(own_addr),
				     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
		msg_info("%s: bind %s", myname, hostaddr.buf);
	    }
	}
    }

    /*
     * Connect to the server.
     */
    if (msg_verbose)
	msg_info("%s: trying: %s[%s] port %d...",
		 myname, STR(iter->host), STR(iter->addr), ntohs(port));

    return (smtp_connect_sock(sock, sa, salen, iter, why, sess_flags));
}

/* smtp_connect_sock - connect a socket over some transport */

static SMTP_SESSION *smtp_connect_sock(int sock, struct sockaddr *sa,
				               int salen,
				               SMTP_ITERATOR *iter,
				               DSN_BUF *why,
				               int sess_flags)
{
    int     conn_stat;
    int     saved_errno;
    VSTREAM *stream;
    time_t  start_time;
    const char *name = STR(iter->host);
    const char *addr = STR(iter->addr);
    unsigned port = iter->port;

    start_time = time((time_t *) 0);
    if (var_smtp_conn_tmout > 0) {
	non_blocking(sock, NON_BLOCKING);
	conn_stat = timed_connect(sock, sa, salen, var_smtp_conn_tmout);
	saved_errno = errno;
	non_blocking(sock, BLOCKING);
	errno = saved_errno;
    } else {
	conn_stat = sane_connect(sock, sa, salen);
    }
    if (conn_stat < 0) {
	if (port)
	    dsb_simple(why, "4.4.1", "connect to %s[%s]:%d: %m",
		       name, addr, ntohs(port));
	else
	    dsb_simple(why, "4.4.1", "connect to %s[%s]: %m", name, addr);
	close(sock);
	return (0);
    }
    stream = vstream_fdopen(sock, O_RDWR);

    /*
     * Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE.
     */
    if (sa->sa_family == AF_INET
#ifdef AF_INET6
	|| sa->sa_family == AF_INET6
#endif
	)
	vstream_tweak_tcp(stream);

    /*
     * Bundle up what we have into a nice SMTP_SESSION object.
     */
    return (smtp_session_alloc(stream, iter, start_time, sess_flags));
}

/* smtp_parse_destination - parse host/port destination */

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
    if ((err = host_port(buf, hostp, (char *) 0, &service, def_service)) != 0)
	msg_fatal("%s in server description: %s", err, destination);

    /*
     * Convert service to port number, network byte order.
     */
    if (alldig(service)) {
	if ((port = atoi(service)) >= 65536 || port == 0)
	    msg_fatal("bad network port in destination: %s", destination);
	*portp = htons(port);
    } else {
	if ((sp = getservbyname(service, protocol)) == 0)
	    msg_fatal("unknown service: %s/%s", service, protocol);
	*portp = sp->s_port;
    }
    return (buf);
}

/* smtp_cleanup_session - clean up after using a session */

static void smtp_cleanup_session(SMTP_STATE *state)
{
    DELIVER_REQUEST *request = state->request;
    SMTP_SESSION *session = state->session;
    int     bad_session;

    /*
     * Inform the postmaster of trouble.
     * 
     * XXX Don't send notifications about errors while sending notifications.
     */
#define POSSIBLE_NOTIFICATION(sender) \
	(*sender == 0 || strcmp(sender, mail_addr_double_bounce()) == 0)

    if (session->history != 0
	&& (session->error_mask & name_mask(VAR_NOTIFY_CLASSES,
					    mail_error_masks,
					    var_notify_classes)) != 0
	&& POSSIBLE_NOTIFICATION(request->sender) == 0)
	smtp_chat_notify(session);

    /*
     * When session caching is enabled, cache the first good session for this
     * delivery request under the next-hop destination, and cache all good
     * sessions under their server network address (destroying the session in
     * the process).
     * 
     * Caching under the next-hop destination name (rather than the fall-back
     * destination) allows us to skip over non-responding primary or backup
     * hosts. In fact, this is the only benefit of caching logical to
     * physical bindings; caching a session under its own hostname provides
     * no performance benefit, given the way smtp_connect() works.
     */
    bad_session = THIS_SESSION_IS_BAD;		/* smtp_quit() may fail */
    if (THIS_SESSION_IS_EXPIRED)
	smtp_quit(state);			/* also disables caching */
    if (THIS_SESSION_IS_CACHED
    /* Redundant tests for safety... */
	&& vstream_ferror(session->stream) == 0
	&& vstream_feof(session->stream) == 0) {
	smtp_save_session(state, SMTP_KEY_MASK_SCACHE_DEST_LABEL,
			  SMTP_KEY_MASK_SCACHE_ENDP_LABEL);
    } else {
	smtp_session_free(session);
    }
    state->session = 0;

    /*
     * If this session was good, reset the logical next-hop state, so that we
     * won't cache connections to alternate servers under the logical
     * next-hop destination. Otherwise we could end up skipping over the
     * available and more preferred servers.
     */
    if (HAVE_NEXTHOP_STATE(state) && !bad_session)
	FREE_NEXTHOP_STATE(state);

    /*
     * Clean up the lists with todo and dropped recipients.
     */
    smtp_rcpt_cleanup(state);

    /*
     * Reset profiling info.
     * 
     * XXX When one delivery request results in multiple sessions, the set-up
     * and transmission latencies of the earlier sessions will count as
     * connection set-up time for the later sessions.
     * 
     * XXX On the other hand, when we first try to connect to one or more dead
     * hosts before we reach a good host, then all that time must be counted
     * as connection set-up time for the session with the good host.
     * 
     * XXX So this set-up attribution problem exists only when we actually
     * engage in a session, spend a lot of time delivering a message, find
     * that it fails, and then connect to an alternate host.
     */
    memset((char *) &request->msg_stats.conn_setup_done, 0,
	   sizeof(request->msg_stats.conn_setup_done));
    memset((char *) &request->msg_stats.deliver_done, 0,
	   sizeof(request->msg_stats.deliver_done));
    request->msg_stats.reuse_count = 0;
}

static void smtp_cache_policy(SMTP_STATE *state, const char *dest)
{
    DELIVER_REQUEST *request = state->request;

    state->misc_flags &= ~SMTP_MISC_FLAG_CONN_CACHE_MASK;

    if (smtp_cache_dest && string_list_match(smtp_cache_dest, dest)) {
	state->misc_flags |= SMTP_MISC_FLAG_CONN_CACHE_MASK;
    } else if (var_smtp_cache_demand) {
	if (request->flags & DEL_REQ_FLAG_CONN_LOAD)
	    state->misc_flags |= SMTP_MISC_FLAG_CONN_LOAD;
	if (request->flags & DEL_REQ_FLAG_CONN_STORE)
	    state->misc_flags |= SMTP_MISC_FLAG_CONN_STORE;
    }
}

/* smtp_connect_local - connect to local server */

static void smtp_connect_local(SMTP_STATE *state, const char *path)
{
    const char *myname = "smtp_connect_local";
    SMTP_ITERATOR *iter = state->iterator;
    SMTP_SESSION *session;
    DSN_BUF *why = state->why;

    /*
     * It's too painful to weave this code into the SMTP connection
     * management routine.
     * 
     * Connection cache management is based on the UNIX-domain pathname, without
     * the "unix:" prefix.
     */
    smtp_cache_policy(state, path);

    /*
     * Here we ensure that the iter->addr member refers to a copy of the
     * UNIX-domain pathname, so that smtp_save_session() will cache the
     * connection using the pathname as the physical endpoint name.
     * 
     * We set dest=path for backwards compatibility.
     */
#define NO_PORT	0

    SMTP_ITER_INIT(iter, path, var_myhostname, path, NO_PORT, state);

    /*
     * Opportunistic TLS for unix domain sockets does not make much sense,
     * since the channel is private, mere encryption without authentication
     * is just wasted cycles and opportunity for breakage. Since we are not
     * willing to retry after TLS handshake failures here, we downgrade "may"
     * no "none". Nothing is lost, and much waste is avoided.
     * 
     * We don't know who is authenticating whom, so if a client cert is
     * available, "encrypt" may be a sensible policy. Otherwise, we also
     * downgrade "encrypt" to "none", this time just to avoid waste.
     * 
     * We use smtp_reuse_nexthop() instead of smtp_reuse_addr(), so that we can
     * reuse a SASL-authenticated connection (however unlikely this scenario
     * may be). The smtp_reuse_addr() interface currently supports only reuse
     * of SASL-unauthenticated connections.
     */
#ifdef USE_TLS
    if (!smtp_tls_policy_cache_query(why, state->tls, iter)) {
	msg_warn("TLS policy lookup error for %s/%s: %s",
		 STR(iter->host), STR(iter->addr), STR(why->reason));
	return;
    }
#endif
    if ((state->misc_flags & SMTP_MISC_FLAG_CONN_LOAD) == 0
	|| (session = smtp_reuse_nexthop(state,
				     SMTP_KEY_MASK_SCACHE_DEST_LABEL)) == 0)
	session = smtp_connect_unix(iter, why, state->misc_flags);
    if ((state->session = session) != 0) {
	session->state = state;
#ifdef USE_TLS
	session->tls = state->tls;		/* TEMPORARY */
	session->tls_nexthop = var_myhostname;	/* for TLS_LEV_SECURE */
	if (session->tls->level == TLS_LEV_MAY) {
	    msg_warn("%s: opportunistic TLS encryption is not appropriate "
		     "for unix-domain destinations.", myname);
	    session->tls->level = TLS_LEV_NONE;
	}
#endif
	/* All delivery errors bounce or defer. */
	state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;

	/*
	 * When a TLS handshake fails, the stream is marked "dead" to avoid
	 * further I/O over a broken channel.
	 */
	if ((session->features & SMTP_FEATURE_FROM_CACHE) == 0
	    && smtp_helo(state) != 0) {
	    if (!THIS_SESSION_IS_DEAD
		&& vstream_ferror(session->stream) == 0
		&& vstream_feof(session->stream) == 0)
		smtp_quit(state);
	} else {
	    smtp_xfer(state);
	}

	/*
	 * With opportunistic TLS disabled we don't expect to be asked to
	 * retry connections without TLS, and so we expect the final server
	 * flag to stay on.
	 */
	if ((state->misc_flags & SMTP_MISC_FLAG_FINAL_SERVER) == 0)
	    msg_panic("%s: unix-domain destination not final!", myname);
	smtp_cleanup_session(state);
    }
}

/* smtp_scrub_address_list - delete all cached addresses from list */

static void smtp_scrub_addr_list(HTABLE *cached_addr, DNS_RR **addr_list)
{
    MAI_HOSTADDR_STR hostaddr;
    DNS_RR *addr;
    DNS_RR *next;

    /*
     * XXX Extend the DNS_RR structure with fields for the printable address
     * and/or binary sockaddr representations, so that we can avoid repeated
     * binary->string transformations for the same address.
     */
    for (addr = *addr_list; addr; addr = next) {
	next = addr->next;
	if (dns_rr_to_pa(addr, &hostaddr) == 0) {
	    msg_warn("cannot convert type %s record to printable address",
		     dns_strtype(addr->type));
	    continue;
	}
	if (htable_locate(cached_addr, hostaddr.buf))
	    *addr_list = dns_rr_remove(*addr_list, addr);
    }
}

/* smtp_update_addr_list - common address list update */

static void smtp_update_addr_list(DNS_RR **addr_list, const char *server_addr,
				          int session_count)
{
    DNS_RR *addr;
    DNS_RR *next;
    int     aierr;
    struct addrinfo *res0;

    if (*addr_list == 0)
	return;

    /*
     * Truncate the address list if we are not going to use it anyway.
     */
    if (session_count == var_smtp_mxsess_limit
	|| session_count == var_smtp_mxaddr_limit) {
	dns_rr_free(*addr_list);
	*addr_list = 0;
	return;
    }

    /*
     * Convert server address to internal form, and look it up in the address
     * list.
     * 
     * XXX smtp_reuse_session() breaks if we remove two or more adjacent list
     * elements but do not truncate the list to zero length.
     * 
     * XXX Extend the SMTP_SESSION structure with sockaddr information so that
     * we can avoid repeated string->binary transformations for the same
     * address.
     */
    if ((aierr = hostaddr_to_sockaddr(server_addr, (char *) 0, 0, &res0)) != 0) {
	msg_warn("hostaddr_to_sockaddr %s: %s",
		 server_addr, MAI_STRERROR(aierr));
    } else {
	for (addr = *addr_list; addr; addr = next) {
	    next = addr->next;
	    if (DNS_RR_EQ_SA(addr, (struct sockaddr *) res0->ai_addr)) {
		*addr_list = dns_rr_remove(*addr_list, addr);
		break;
	    }
	}
	freeaddrinfo(res0);
    }
}

/* smtp_reuse_session - try to use existing connection, return session count */

static int smtp_reuse_session(SMTP_STATE *state, DNS_RR **addr_list,
			              int domain_best_pref)
{
    int     session_count = 0;
    DNS_RR *addr;
    DNS_RR *next;
    MAI_HOSTADDR_STR hostaddr;
    SMTP_SESSION *session;
    SMTP_ITERATOR *iter = state->iterator;
    DSN_BUF *why = state->why;

    /*
     * First, search the cache by request nexthop. We truncate the server
     * address list when all the sessions for this destination are used up,
     * to reduce the number of variables that need to be checked later.
     * 
     * Note: lookup by logical destination restores the "best MX" bit.
     * 
     * smtp_reuse_nexthop() clobbers the iterators's "dest" attribute. We save
     * and restore it here, so that subsequent connections will use the
     * proper nexthop information.
     * 
     * We request a dummy "TLS disabled" policy for connection-cache lookup by
     * request nexthop only. If we find a saved connection, then we know that
     * plaintext was permitted, because we never save a connection after
     * turning on TLS.
     */
#ifdef USE_TLS
    smtp_tls_policy_dummy(state->tls);
#endif
    SMTP_ITER_SAVE_DEST(state->iterator);
    if (*addr_list && SMTP_RCPT_LEFT(state) > 0
	&& HAVE_NEXTHOP_STATE(state)
	&& (session = smtp_reuse_nexthop(state, SMTP_KEY_MASK_SCACHE_DEST_LABEL)) != 0) {
	session_count = 1;
	smtp_update_addr_list(addr_list, STR(iter->addr), session_count);
	if ((state->misc_flags & SMTP_MISC_FLAG_FINAL_NEXTHOP)
	    && *addr_list == 0)
	    state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;
#ifdef USE_TLS
	session->tls = state->tls;		/* TEMPORARY */
#endif
	smtp_xfer(state);
	smtp_cleanup_session(state);
    }
    SMTP_ITER_RESTORE_DEST(state->iterator);

    /*
     * Second, search the cache by primary MX address. Again, we use address
     * list truncation so that we have to check fewer variables later.
     * 
     * XXX This loop is safe because smtp_update_addr_list() either truncates
     * the list to zero length, or removes at most one list element.
     * 
     * Currently, we use smtp_reuse_addr() only for SASL-unauthenticated
     * connections. Furthermore, we rely on smtp_reuse_addr() to look up an
     * existing SASL-unauthenticated connection only when a new connection
     * would be guaranteed not to require SASL authentication.
     * 
     * In addition, we rely on smtp_reuse_addr() to look up an existing
     * plaintext connection only when a new connection would be guaranteed
     * not to use TLS.
     * 
     * For more precise control over reuse, the iterator should look up SASL and
     * TLS policy as it evaluates mail exchangers in order, instead of
     * relying on duplicate lookup request code in smtp_reuse(3) and
     * smtp_session(3).
     */
    for (addr = *addr_list; SMTP_RCPT_LEFT(state) > 0 && addr; addr = next) {
	if (addr->pref != domain_best_pref)
	    break;
	next = addr->next;
	if (dns_rr_to_pa(addr, &hostaddr) == 0) {
	    msg_warn("cannot convert type %s record to printable address",
		     dns_strtype(addr->type));
	    /* XXX Assume there is no code at the end of this loop. */
	    continue;
	}
	vstring_strcpy(iter->addr, hostaddr.buf);
	vstring_strcpy(iter->host, SMTP_HNAME(addr));
	iter->rr = addr;
#ifdef USE_TLS
	if (!smtp_tls_policy_cache_query(why, state->tls, iter)) {
	    msg_warn("TLS policy lookup error for %s/%s: %s",
		     STR(iter->dest), STR(iter->host), STR(why->reason));
	    continue;
	    /* XXX Assume there is no code at the end of this loop. */
	}
#endif
	if ((session = smtp_reuse_addr(state,
				   SMTP_KEY_MASK_SCACHE_ENDP_LABEL)) != 0) {
	    session->features |= SMTP_FEATURE_BEST_MX;
	    session_count += 1;
	    smtp_update_addr_list(addr_list, STR(iter->addr), session_count);
	    if (*addr_list == 0)
		next = 0;
	    if ((state->misc_flags & SMTP_MISC_FLAG_FINAL_NEXTHOP)
		&& next == 0)
		state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;
#ifdef USE_TLS
	    session->tls = state->tls;		/* TEMPORARY */
#endif
	    smtp_xfer(state);
	    smtp_cleanup_session(state);
	}
    }
    return (session_count);
}

/* smtp_connect_inet - establish network connection */

static void smtp_connect_inet(SMTP_STATE *state, const char *nexthop,
			              char *def_service)
{
    DELIVER_REQUEST *request = state->request;
    SMTP_ITERATOR *iter = state->iterator;
    ARGV   *sites;
    char   *dest;
    char  **cpp;
    int     non_fallback_sites;
    int     retry_plain = 0;
    DSN_BUF *why = state->why;

    /*
     * For sanity, require that at least one of INET or INET6 is enabled.
     * Otherwise, we can't look up interface information, and we can't
     * convert names or addresses.
     */
    if (inet_proto_info()->ai_family_list[0] == 0) {
	dsb_simple(why, "4.4.4", "all network protocols are disabled");
	return;
    }

    /*
     * First try to deliver to the indicated destination, then try to deliver
     * to the optional fall-back relays.
     * 
     * Future proofing: do a null destination sanity check in case we allow the
     * primary destination to be a list (it could be just separators).
     */
    sites = argv_alloc(1);
    argv_add(sites, nexthop, (char *) 0);
    if (sites->argc == 0)
	msg_panic("null destination: \"%s\"", nexthop);
    non_fallback_sites = sites->argc;
    /* When we are lmtp(8) var_fallback_relay is null */
    if (smtp_mode)
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
#define IS_FALLBACK_RELAY(cpp, sites, non_fallback_sites) \
	    (*(cpp) && (cpp) >= (sites)->argv + (non_fallback_sites))

    for (cpp = sites->argv, (state->misc_flags |= SMTP_MISC_FLAG_FIRST_NEXTHOP);
	 SMTP_RCPT_LEFT(state) > 0 && (dest = *cpp) != 0;
	 cpp++, (state->misc_flags &= ~SMTP_MISC_FLAG_FIRST_NEXTHOP)) {
	char   *dest_buf;
	char   *domain;
	unsigned port;
	DNS_RR *addr_list;
	DNS_RR *addr;
	DNS_RR *next;
	int     addr_count;
	int     sess_count;
	SMTP_SESSION *session;
	int     lookup_mx;
	unsigned domain_best_pref;
	MAI_HOSTADDR_STR hostaddr;

	if (cpp[1] == 0)
	    state->misc_flags |= SMTP_MISC_FLAG_FINAL_NEXTHOP;

	/*
	 * Parse the destination. Default is to use the SMTP port. Look up
	 * the address instead of the mail exchanger when a quoted host is
	 * specified, or when DNS lookups are disabled.
	 */
	dest_buf = smtp_parse_destination(dest, def_service, &domain, &port);
	if (var_helpful_warnings && ntohs(port) == 465) {
	    msg_info("CLIENT wrappermode (port smtps/465) is unimplemented");
	    msg_info("instead, send to (port submission/587) with STARTTLS");
	}
#define NO_HOST	""				/* safety */
#define NO_ADDR	""				/* safety */

	SMTP_ITER_INIT(iter, dest, NO_HOST, NO_ADDR, port, state);

	/*
	 * Resolve an SMTP server. Skip mail exchanger lookups when a quoted
	 * host is specified, or when DNS lookups are disabled.
	 */
	if (msg_verbose)
	    msg_info("connecting to %s port %d", domain, ntohs(port));
	if (smtp_mode) {
	    if (ntohs(port) == IPPORT_SMTP)
		state->misc_flags |= SMTP_MISC_FLAG_LOOP_DETECT;
	    else
		state->misc_flags &= ~SMTP_MISC_FLAG_LOOP_DETECT;
	    lookup_mx = (smtp_dns_support != SMTP_DNS_DISABLED && *dest != '[');
	} else
	    lookup_mx = 0;
	if (!lookup_mx) {
	    addr_list = smtp_host_addr(domain, state->misc_flags, why);
	    /* XXX We could be an MX host for this destination... */
	} else {
	    int     i_am_mx = 0;

	    addr_list = smtp_domain_addr(domain, &iter->mx, state->misc_flags,
					 why, &i_am_mx);
	    /* If we're MX host, don't connect to non-MX backups. */
	    if (i_am_mx)
		state->misc_flags |= SMTP_MISC_FLAG_FINAL_NEXTHOP;
	}

	/*
	 * Don't try fall-back hosts if mail loops to myself. That would just
	 * make the problem worse.
	 */
	if (addr_list == 0 && SMTP_HAS_LOOP_DSN(why))
	    state->misc_flags |= SMTP_MISC_FLAG_FINAL_NEXTHOP;

	/*
	 * No early loop exit or we have a memory leak with dest_buf.
	 */
	if (addr_list)
	    domain_best_pref = addr_list->pref;

	/*
	 * When session caching is enabled, store the first good session for
	 * this delivery request under the next-hop destination name. All
	 * good sessions will be stored under their specific server IP
	 * address.
	 * 
	 * XXX smtp_session_cache_destinations specifies domain names without
	 * :port, because : is already used for maptype:mapname. Because of
	 * this limitation we use the bare domain without the optional [] or
	 * non-default TCP port.
	 * 
	 * Opportunistic (a.k.a. on-demand) session caching on request by the
	 * queue manager. This is turned temporarily when a destination has a
	 * high volume of mail in the active queue.
	 */
	if (addr_list && (state->misc_flags & SMTP_MISC_FLAG_FIRST_NEXTHOP)) {
	    smtp_cache_policy(state, domain);
	    if (state->misc_flags & SMTP_MISC_FLAG_CONN_STORE)
		SET_NEXTHOP_STATE(state, dest);
	}

	/*
	 * Delete visited cached hosts from the address list.
	 * 
	 * Optionally search the connection cache by domain name or by primary
	 * MX address before we try to create new connections.
	 * 
	 * Enforce the MX session and MX address counts per next-hop or
	 * fall-back destination. smtp_reuse_session() will truncate the
	 * address list when either limit is reached.
	 */
	if (addr_list && (state->misc_flags & SMTP_MISC_FLAG_CONN_LOAD)) {
	    if (state->cache_used->used > 0)
		smtp_scrub_addr_list(state->cache_used, &addr_list);
	    sess_count = addr_count =
		smtp_reuse_session(state, &addr_list, domain_best_pref);
	} else
	    sess_count = addr_count = 0;

	/*
	 * Connect to an SMTP server: create primary MX connections, and
	 * reuse or create backup MX connections.
	 * 
	 * At the start of an SMTP session, all recipients are unmarked. In the
	 * course of an SMTP session, recipients are marked as KEEP (deliver
	 * to alternate mail server) or DROP (remove from recipient list). At
	 * the end of an SMTP session, weed out the recipient list. Unmark
	 * any left-over recipients and try to deliver them to a backup mail
	 * server.
	 * 
	 * Cache the first good session under the next-hop destination name.
	 * Cache all good sessions under their physical endpoint.
	 * 
	 * Don't query the session cache for primary MX hosts. We already did
	 * that in smtp_reuse_session(), and if any were found in the cache,
	 * they were already deleted from the address list.
	 * 
	 * Currently, we use smtp_reuse_addr() only for SASL-unauthenticated
	 * connections. Furthermore, we rely on smtp_reuse_addr() to look up
	 * an existing SASL-unauthenticated connection only when a new
	 * connection would be guaranteed not to require SASL authentication.
	 * 
	 * In addition, we rely on smtp_reuse_addr() to look up an existing
	 * plaintext connection only when a new connection would be
	 * guaranteed not to use TLS.
	 */
	for (addr = addr_list; SMTP_RCPT_LEFT(state) > 0 && addr; addr = next) {
	    next = addr->next;
	    if (++addr_count == var_smtp_mxaddr_limit)
		next = 0;
	    if (dns_rr_to_pa(addr, &hostaddr) == 0) {
		msg_warn("cannot convert type %s record to printable address",
			 dns_strtype(addr->type));
		/* XXX Assume there is no code at the end of this loop. */
		continue;
	    }
	    vstring_strcpy(iter->addr, hostaddr.buf);
	    vstring_strcpy(iter->host, SMTP_HNAME(addr));
	    iter->rr = addr;
#ifdef USE_TLS
	    if (!smtp_tls_policy_cache_query(why, state->tls, iter)) {
		msg_warn("TLS policy lookup for %s/%s: %s",
			 STR(iter->dest), STR(iter->host), STR(why->reason));
		continue;
		/* XXX Assume there is no code at the end of this loop. */
	    }
	    /* Disable TLS when retrying after a handshake failure */
	    if (retry_plain) {
		state->tls->level = TLS_LEV_NONE;
		retry_plain = 0;
	    }
#endif
	    if ((state->misc_flags & SMTP_MISC_FLAG_CONN_LOAD) == 0
		|| addr->pref == domain_best_pref
		|| !(session = smtp_reuse_addr(state,
					  SMTP_KEY_MASK_SCACHE_ENDP_LABEL)))
		session = smtp_connect_addr(iter, why, state->misc_flags);
	    if ((state->session = session) != 0) {
		session->state = state;
#ifdef USE_TLS
		session->tls = state->tls;	/* TEMPORARY */
		session->tls_nexthop = domain;	/* for TLS_LEV_SECURE */
#endif
		if (addr->pref == domain_best_pref)
		    session->features |= SMTP_FEATURE_BEST_MX;
		/* Don't count handshake errors towards the session limit. */
		if ((state->misc_flags & SMTP_MISC_FLAG_FINAL_NEXTHOP)
		    && next == 0)
		    state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;
		if ((session->features & SMTP_FEATURE_FROM_CACHE) == 0
		    && smtp_helo(state) != 0) {
#ifdef USE_TLS

		    /*
		     * When an opportunistic TLS handshake fails, try the
		     * same address again, with TLS disabled. See also the
		     * RETRY_AS_PLAINTEXT macro.
		     */
		    if ((retry_plain = session->tls_retry_plain) != 0) {
			--addr_count;
			next = addr;
		    }
#endif

		    /*
		     * When a TLS handshake fails, the stream is marked
		     * "dead" to avoid further I/O over a broken channel.
		     */
		    if (!THIS_SESSION_IS_DEAD
			&& vstream_ferror(session->stream) == 0
			&& vstream_feof(session->stream) == 0)
			smtp_quit(state);
		} else {
		    /* Do count delivery errors towards the session limit. */
		    if (++sess_count == var_smtp_mxsess_limit)
			next = 0;
		    if ((state->misc_flags & SMTP_MISC_FLAG_FINAL_NEXTHOP)
			&& next == 0)
			state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;
		    smtp_xfer(state);
		}
		smtp_cleanup_session(state);
	    } else {
		/* The reason already includes the IP address and TCP port. */
		msg_info("%s", STR(why->reason));
	    }
	    /* XXX Code above assumes there is no code at this loop ending. */
	}
	dns_rr_free(addr_list);
	if (iter->mx) {
	    dns_rr_free(iter->mx);
	    iter->mx = 0;			/* Just in case */
	}
	myfree(dest_buf);
	if (state->misc_flags & SMTP_MISC_FLAG_FINAL_NEXTHOP)
	    break;
    }

    /*
     * We still need to deliver, bounce or defer some left-over recipients:
     * either mail loops or some backup mail server was unavailable.
     */
    if (SMTP_RCPT_LEFT(state) > 0) {

	/*
	 * In case of a "no error" indication we make up an excuse: we did
	 * find the host address, but we did not attempt to connect to it.
	 * This can happen when the fall-back relay was already tried via a
	 * cached connection, so that the address list scrubber left behind
	 * an empty list.
	 */
	if (!SMTP_HAS_DSN(why)) {
	    dsb_simple(why, "4.3.0",
		       "server unavailable or unable to receive mail");
	}

	/*
	 * Pay attention to what could be configuration problems, and pretend
	 * that these are recoverable rather than bouncing the mail.
	 */
	else if (!SMTP_HAS_SOFT_DSN(why) && smtp_mode) {

	    /*
	     * The fall-back destination did not resolve as expected, or it
	     * is refusing to talk to us, or mail for it loops back to us.
	     */
	    if (IS_FALLBACK_RELAY(cpp, sites, non_fallback_sites)) {
		msg_warn("%s configuration problem", VAR_SMTP_FALLBACK);
		vstring_strcpy(why->status, "4.3.5");
		/* XXX Keep the diagnostic code and MTA. */
	    }

	    /*
	     * The next-hop relayhost did not resolve as expected, or it is
	     * refusing to talk to us, or mail for it loops back to us.
	     */
	    else if (strcmp(sites->argv[0], var_relayhost) == 0) {
		msg_warn("%s configuration problem", VAR_RELAYHOST);
		vstring_strcpy(why->status, "4.3.5");
		/* XXX Keep the diagnostic code and MTA. */
	    }

	    /*
	     * Mail for the next-hop destination loops back to myself. Pass
	     * the mail to the best_mx_transport or bounce it.
	     */
	    else if (SMTP_HAS_LOOP_DSN(why) && *var_bestmx_transp) {
		dsb_reset(why);			/* XXX */
		state->status = deliver_pass_all(MAIL_CLASS_PRIVATE,
						 var_bestmx_transp,
						 request);
		SMTP_RCPT_LEFT(state) = 0;	/* XXX */
	    }
	}
    }

    /*
     * Cleanup.
     */
    if (HAVE_NEXTHOP_STATE(state))
	FREE_NEXTHOP_STATE(state);
    argv_free(sites);
}

/* smtp_connect - establish SMTP connection */

int     smtp_connect(SMTP_STATE *state)
{
    DELIVER_REQUEST *request = state->request;
    char   *destination = request->nexthop;

    /*
     * All deliveries proceed along the same lines, whether they are over TCP
     * or UNIX-domain sockets, and whether they use SMTP or LMTP: get a
     * connection from the cache or create a new connection; deliver mail;
     * update the connection cache or disconnect.
     * 
     * The major differences appear at a higher level: the expansion from
     * destination to address list, and whether to stop before we reach the
     * end of that list.
     */
#define DEF_LMTP_SERVICE	var_lmtp_tcp_port
#define DEF_SMTP_SERVICE	"smtp"

    /*
     * With LMTP we have direct-to-host delivery only. The destination may
     * have multiple IP addresses.
     */
    if (!smtp_mode) {
	if (strncmp(destination, "unix:", 5) == 0) {
	    smtp_connect_local(state, destination + 5);
	} else {
	    if (strncmp(destination, "inet:", 5) == 0)
		destination += 5;
	    smtp_connect_inet(state, destination, DEF_LMTP_SERVICE);
	}
    }

    /*
     * With SMTP we can have indirection via MX host lookup, as well as an
     * optional fall-back relayhost that we must avoid when we are MX host.
     * 
     * XXX We don't add support for "unix:" or "inet:" prefixes in SMTP
     * destinations, because that would break compatibility with existing
     * Postfix configurations that have a host with such a name.
     */
    else {
	smtp_connect_inet(state, destination, DEF_SMTP_SERVICE);
    }

    /*
     * We still need to bounce or defer some left-over recipients: either
     * (SMTP) mail loops or some server was unavailable.
     * 
     * We could avoid this (and the "final server" complexity) by keeping one
     * DSN structure per recipient in memory, by updating those in-memory
     * structures with each delivery attempt, and by always flushing all
     * deferred recipients at the end. We'd probably still want to bounce
     * recipients immediately, so we'd end up with another chunk of code for
     * defer logging only.
     */
    if (SMTP_RCPT_LEFT(state) > 0) {
	state->misc_flags |= SMTP_MISC_FLAG_FINAL_SERVER;	/* XXX */
	smtp_sess_fail(state);

	/*
	 * Sanity check. Don't silently lose recipients.
	 */
	smtp_rcpt_cleanup(state);
	if (SMTP_RCPT_LEFT(state) > 0)
	    msg_panic("smtp_connect: left-over recipients");
    }
    return (state->status);
}
