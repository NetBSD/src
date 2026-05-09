/*	$NetBSD: haproxy_srvr.c,v 1.5 2026/05/09 18:49:16 christos Exp $	*/

/*++
/* NAME
/*	haproxy_srvr 3
/* SUMMARY
/*	server-side haproxy protocol support
/* SYNOPSIS
/*	#include <haproxy_srvr.h>
/*
/*	const char *haproxy_srvr_parse_sa(
/*	const char *str,
/*	ssize_t	*str_len,
/*	int	*non_proxy,
/*	MAI_HOSTADDR_STR *smtp_client_addr,
/*	MAI_SERVPORT_STR *smtp_client_port,
/*	MAI_HOSTADDR_STR *smtp_server_addr,
/*	MAI_SERVPORT_STR *smtp_server_port,
/*	struct sockaddr *client_sa,
/*	SOCKADDR_SIZE *client_sa_len,
/*	struct sockaddr *server_sa,
/*	SOCKADDR_SIZE *server_sa_len)
/*
/*	const char *haproxy_srvr_receive_sa(
/*	int	fd,
/*	int	*non_proxy,
/*	MAI_HOSTADDR_STR *smtp_client_addr,
/*	MAI_SERVPORT_STR *smtp_client_port,
/*	MAI_HOSTADDR_STR *smtp_server_addr,
/*	MAI_SERVPORT_STR *smtp_server_port,
/*	struct sockaddr *client_sa,
/*	SOCKADDR_SIZE *client_sa_len,
/*	struct sockaddr *server_sa,
/*	SOCKADDR_SIZE *server_sa_len)
/* ABI COMPATIBILITY
/*	const char *haproxy_srvr_parse(str, str_len, non_proxy,
/*			smtp_client_addr, smtp_client_port,
/*			smtp_server_addr, smtp_server_port)
/*	const char *str;
/*	ssize_t	*str_len;
/*	int	*non_proxy;
/*	MAI_HOSTADDR_STR *smtp_client_addr,
/*	MAI_SERVPORT_STR *smtp_client_port,
/*	MAI_HOSTADDR_STR *smtp_server_addr,
/*	MAI_SERVPORT_STR *smtp_server_port;
/*
/*	const char *haproxy_srvr_receive(fd, non_proxy,
/*			smtp_client_addr, smtp_client_port,
/*			smtp_server_addr, smtp_server_port)
/*	int	fd;
/*	int	*non_proxy;
/*	MAI_HOSTADDR_STR *smtp_client_addr,
/*	MAI_SERVPORT_STR *smtp_client_port,
/*	MAI_HOSTADDR_STR *smtp_server_addr,
/*	MAI_SERVPORT_STR *smtp_server_port;
/* DESCRIPTION
/*	haproxy_srvr_parse_sa() parses a haproxy v1 or v2 protocol
/*	message. The result is null in case of success, a pointer
/*	to text (with the error type) in case of error. If both
/*	IPv6 and IPv4 support are enabled, IPV4_IN_IPV6 address
/*	form (::ffff:1.2.3.4) is converted to IPV4 form. In case
/*	of success, the str_len argument is updated with the number
/*	of bytes parsed, and the non_proxy argument is true or false
/*	if the haproxy message specifies a non-proxied connection.
/*
/*	haproxy_srvr_receive_sa() receives and parses a haproxy protocol
/*	handshake. This must be called before any I/O is done on
/*	the specified file descriptor. The result is 0 in case of
/*	success, -1 in case of error. All errors are logged.
/*
/*	The haproxy v2 protocol support is limited to TCP over IPv4,
/*	TCP over IPv6, and non-proxied connections. In the latter
/*	case, the caller is responsible for any local or remote
/*	address/port lookup.
/*
/*	The client or server sockaddr and length storage are updated
/*	when their pointers are non-null.
/*
/*	haproxy_srvr_parse() and haproxy_srvr_receive() provide ABI
/*	backwards compatibility, passing null pointers for the sockaddr
/*	and length storage arguments.
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <myaddrinfo.h>
#include <valid_hostname.h>
#include <stringops.h>
#include <mymalloc.h>
#include <inet_proto.h>
#include <split_at.h>
#include <sock_addr.h>
#include <normalize_v4mapped_addr.h>

/* Global library. */

#define _HAPROXY_SRVR_INTERNAL_
#include <haproxy_srvr.h>

static const INET_PROTO_INFO *proto_info;

#define STR_OR_NULL(str) ((str) ? (str) : "(null)")

/* haproxy_srvr_parse_lit - extract and validate string literal */

static int haproxy_srvr_parse_lit(const char *str,...)
{
    va_list ap;
    const char *cp;
    int     result = -1;
    int     count;

    if (msg_verbose)
	msg_info("haproxy_srvr_parse: %s", STR_OR_NULL(str));

    if (str != 0) {
	va_start(ap, str);
	for (count = 0; (cp = va_arg(ap, const char *)) != 0; count++) {
	    if (strcmp(str, cp) == 0) {
		result = count;
		break;
	    }
	}
	va_end(ap);
    }
    return (result);
}

/* haproxy_srvr_parse_proto - parse and validate the protocol type */

static int haproxy_srvr_parse_proto(const char *str, int *addr_family)
{
    if (msg_verbose)
	msg_info("haproxy_srvr_parse: proto=%s", STR_OR_NULL(str));

    if (str == 0)
	return (-1);
#ifdef HAS_IPV6
    if (strcasecmp(str, "TCP6") == 0) {
	if (strchr((char *) proto_info->sa_family_list, AF_INET6) != 0) {
	    *addr_family = AF_INET6;
	    return (0);
	}
    } else
#endif
    if (strcasecmp(str, "TCP4") == 0) {
	if (strchr((char *) proto_info->sa_family_list, AF_INET) != 0) {
	    *addr_family = AF_INET;
	    return (0);
	}
    }
    return (-1);
}

/* haproxy_srvr_parse_addr - extract and validate IP address */

static int haproxy_srvr_parse_addr(const char *str, MAI_HOSTADDR_STR *addr,
				           int addr_family,
				           struct sockaddr *sa,
				           SOCKADDR_SIZE *sa_len)
{
    struct addrinfo *res;
    int     err;
    struct sockaddr_storage ss;
    SOCKADDR_SIZE ss_len;

    if (msg_verbose)
	msg_info("haproxy_srvr_parse: addr=%s proto=%d",
		 STR_OR_NULL(str), addr_family);

    if (str == 0 || strlen(str) >= sizeof(MAI_HOSTADDR_STR))
	return (-1);

    switch (addr_family) {
#ifdef HAS_IPV6
    case AF_INET6:
	if (!valid_ipv6_hostaddr(str, DONT_GRIPE))
	    return (-1);
	break;
#endif
    case AF_INET:
	if (!valid_ipv4_hostaddr(str, DONT_GRIPE))
	    return (-1);
	break;
    default:
	msg_panic("haproxy_srvr_parse: unexpected address family: %d",
		  addr_family);
    }

    /*
     * Convert the printable address to canonical form. Don't rely on the
     * proxy. This requires a conversion to binary form and back, even if a
     * caller such as postscreen does not need the binary form.
     */
    if ((err = hostaddr_to_sockaddr(str, (char *) 0, 0, &res)) != 0) {
	msg_warn("haproxy_srvr_parse: hostaddr_to_sockaddr(\"%s\") failed: %s",
		 str, MAI_STRERROR(err));
	return (-1);
    }
    if (sa == 0) {
	sa = (struct sockaddr *) &ss;
	ss_len = sizeof(ss);
	sa_len = &ss_len;
    } else {
	if (sa_len == 0)
	    msg_panic("haproxy_srvr_parse: sockaddr length not specified");
    }
    if (*sa_len < res->ai_addrlen)
	msg_panic("haproxy_srvr_parse: sockaddr size %d too small",
		  (int) *sa_len);
    *sa_len = res->ai_addrlen;
    memcpy((void *) sa, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
#ifdef HAS_IPV6
    if (sa->sa_family == AF_INET6)
	normalize_v4mapped_sockaddr(sa, sa_len);
#endif
    if ((err = sockaddr_to_hostaddr(sa, *sa_len,
				    addr, (MAI_SERVPORT_STR *) 0, 0)) != 0) {
	msg_warn("haproxy_srvr_parse: sockaddr_to_hostaddr() failed: %s",
		 MAI_STRERROR(err));
	return (-1);
    }
    return (0);
}

/* haproxy_srvr_parse_port - extract and validate TCP port */

static int haproxy_srvr_parse_port(const char *str, MAI_SERVPORT_STR *port,
				           struct sockaddr *sa)
{
    if (msg_verbose)
	msg_info("haproxy_srvr_parse: port=%s", STR_OR_NULL(str));
    if (str == 0 || strlen(str) >= sizeof(MAI_SERVPORT_STR)
	|| !valid_hostport(str, DONT_GRIPE)) {
	return (-1);
    } else {
	memcpy(port->buf, str, strlen(str) + 1);
	if (sa != 0) {
	    switch (sa->sa_family) {
#ifdef HAS_IPV6
	    case AF_INET6:
		SOCK_ADDR_IN6_PORT(sa) = htons(atoi(str));
		break;
#endif
	    case AF_INET:
		SOCK_ADDR_IN_PORT(sa) = htons(atoi(str));
		break;
	    default:
		msg_panic("haproxy_srvr_parse: unexpected address family: %d",
			  sa->sa_family);
	    }
	}
	return (0);
    }
}

/* haproxy_srvr_parse_v2_addr_v4 - parse IPv4 info from v2 header */

static int haproxy_srvr_parse_v2_addr_v4(uint32_t sin_addr,
					         unsigned sin_port,
					         MAI_HOSTADDR_STR *addr,
					         MAI_SERVPORT_STR *port,
					         struct sockaddr *sa,
					         SOCKADDR_SIZE *sa_len)
{
    struct sockaddr_in sin;
    SOCKADDR_SIZE sin_len;

    /*
     * Convert the binary address and port to printable form.
     */
    if (sa == 0) {
	sa = (struct sockaddr *) &sin;
	sin_len = sizeof(sin);
	sa_len = &sin_len;
    } else {
	if (sa_len == 0)
	    msg_panic("haproxy_srvr_parse: sockaddr length not specified");
	if (*sa_len < sizeof(sin))
	    msg_panic("haproxy_srvr_parse: sockaddr size %d too small",
		      (int) *sa_len);
	*sa_len = sizeof(sin);
    }
    memset((void *) sa, 0, *sa_len);
    SOCK_ADDR_IN_FAMILY(sa) = AF_INET;
    SOCK_ADDR_IN_ADDR(sa).s_addr = sin_addr;
    SOCK_ADDR_IN_PORT(sa) = sin_port;
    if (sockaddr_to_hostaddr(sa, *sa_len, addr, port, 0) < 0)
	return (-1);
    return (0);
}

#ifdef HAS_IPV6

/* haproxy_srvr_parse_v2_addr_v6 - parse IPv6 info from v2 header */

static int haproxy_srvr_parse_v2_addr_v6(uint8_t *sin6_addr,
					         unsigned sin6_port,
					         MAI_HOSTADDR_STR *addr,
					         MAI_SERVPORT_STR *port,
					         struct sockaddr *sa,
					         SOCKADDR_SIZE *sa_len)
{
    struct sockaddr_in6 sin6;
    SOCKADDR_SIZE sin6_len;

    /*
     * Convert the binary address and port to printable form.
     */
    if (sa == 0) {
	sa = (struct sockaddr *) &sin6;
	sin6_len = sizeof(sin6);
	sa_len = &sin6_len;
    } else {
	if (sa_len == 0)
	    msg_panic("haproxy_srvr_parse: sockaddr length not specified");
	if (*sa_len < sizeof(sin6))
	    msg_panic("haproxy_srvr_parse: sockaddr size %d too small",
		      (int) *sa_len);
	*sa_len = sizeof(sin6);
    }
    memset((void *) sa, 0, *sa_len);
    SOCK_ADDR_IN6_FAMILY(sa) = AF_INET6;
    memcpy(&SOCK_ADDR_IN6_ADDR(sa), sin6_addr, sizeof(SOCK_ADDR_IN6_ADDR(sa)));
    SOCK_ADDR_IN6_PORT(sa) = sin6_port;
    normalize_v4mapped_sockaddr(sa, sa_len);
    if (sockaddr_to_hostaddr(sa, *sa_len, addr, port, 0) < 0)
	return (-1);
    return (0);
}

#endif

/* haproxy_srvr_parse_v2_hdr - parse v2 header address info */

static const char *haproxy_srvr_parse_v2_hdr(const char *str, ssize_t *str_len,
					             int *non_proxy,
				         MAI_HOSTADDR_STR *smtp_client_addr,
				         MAI_SERVPORT_STR *smtp_client_port,
				         MAI_HOSTADDR_STR *smtp_server_addr,
				         MAI_SERVPORT_STR *smtp_server_port,
					         struct sockaddr *client_sa,
				               SOCKADDR_SIZE *client_sa_len,
					         struct sockaddr *server_sa,
				               SOCKADDR_SIZE *server_sa_len)
{
    const char myname[] = "haproxy_srvr_parse_v2_hdr";
    struct proxy_hdr_v2 *hdr_v2;

    if (*str_len < PP2_HEADER_LEN)
	return ("short protocol header");
    hdr_v2 = (struct proxy_hdr_v2 *) str;
    if (memcmp(hdr_v2->sig, PP2_SIGNATURE, PP2_SIGNATURE_LEN) != 0)
	return ("unrecognized protocol header");
    if ((hdr_v2->ver_cmd & PP2_VERSION_MASK) != PP2_VERSION)
	return ("unrecognized protocol version");
    if (*str_len < PP2_HEADER_LEN + ntohs(hdr_v2->len))
	return ("short version 2 protocol header");

    switch (hdr_v2->ver_cmd & PP2_CMD_MASK) {

	/*
	 * Proxied connection, use the proxy-provided connection info.
	 */
    case PP2_CMD_PROXY:
	switch (hdr_v2->fam) {
	case PP2_FAM_INET | PP2_TRANS_STREAM:{	/* TCP4 */
		if (strchr((char *) proto_info->sa_family_list, AF_INET) == 0)
		    return ("Postfix IPv4 support is disabled");
		if (ntohs(hdr_v2->len) < PP2_ADDR_LEN_INET)
		    return ("short address field");
		if (haproxy_srvr_parse_v2_addr_v4(hdr_v2->addr.ip4.src_addr,
						  hdr_v2->addr.ip4.src_port,
					 smtp_client_addr, smtp_client_port,
					      client_sa, client_sa_len) < 0)
		    return ("client network address conversion error");
		if (msg_verbose)
		    msg_info("%s: smtp_client_addr=%s smtp_client_port=%s",
		      myname, smtp_client_addr->buf, smtp_client_port->buf);
		if (haproxy_srvr_parse_v2_addr_v4(hdr_v2->addr.ip4.dst_addr,
						  hdr_v2->addr.ip4.dst_port,
						  smtp_server_addr,
						  smtp_server_port,
						  server_sa,
						  server_sa_len) < 0)
		    return ("server network address conversion error");
		if (msg_verbose)
		    msg_info("%s: smtp_server_addr=%s smtp_server_port=%s",
		      myname, smtp_server_addr->buf, smtp_server_port->buf);
		break;
	    }
	case PP2_FAM_INET6 | PP2_TRANS_STREAM:{/* TCP6 */
#ifdef HAS_IPV6
		if (strchr((char *) proto_info->sa_family_list, AF_INET6) == 0)
		    return ("Postfix IPv6 support is disabled");
		if (ntohs(hdr_v2->len) < PP2_ADDR_LEN_INET6)
		    return ("short address field");
		if (haproxy_srvr_parse_v2_addr_v6(hdr_v2->addr.ip6.src_addr,
						  hdr_v2->addr.ip6.src_port,
						  smtp_client_addr,
						  smtp_client_port,
						  client_sa,
						  client_sa_len) < 0)
		    return ("client network address conversion error");
		if (msg_verbose)
		    msg_info("%s: smtp_client_addr=%s smtp_client_port=%s",
		      myname, smtp_client_addr->buf, smtp_client_port->buf);
		if (haproxy_srvr_parse_v2_addr_v6(hdr_v2->addr.ip6.dst_addr,
						  hdr_v2->addr.ip6.dst_port,
						  smtp_server_addr,
						  smtp_server_port,
						  server_sa,
						  server_sa_len) < 0)
		    return ("server network address conversion error");
		if (msg_verbose)
		    msg_info("%s: smtp_server_addr=%s smtp_server_port=%s",
		      myname, smtp_server_addr->buf, smtp_server_port->buf);
		break;
#else
		return ("Postfix IPv6 support is not compiled in");
#endif
	    }
	default:
	    return ("unsupported network protocol");
	}
	/* For now, skip and ignore TLVs. */
	*str_len = PP2_HEADER_LEN + ntohs(hdr_v2->len);
	return (0);

	/*
	 * Non-proxied connection, use the proxy-to-server connection info.
	 */
    case PP2_CMD_LOCAL:
	/* For now, skip and ignore TLVs. */
	*non_proxy = 1;
	*str_len = PP2_HEADER_LEN + ntohs(hdr_v2->len);
	return (0);
    default:
	return ("bad command in proxy header");
    }
}

/* haproxy_srvr_parse_sa - parse haproxy line */

const char *haproxy_srvr_parse_sa(const char *str, ssize_t *str_len,
				          int *non_proxy,
				          MAI_HOSTADDR_STR *smtp_client_addr,
				          MAI_SERVPORT_STR *smtp_client_port,
				          MAI_HOSTADDR_STR *smtp_server_addr,
				          MAI_SERVPORT_STR *smtp_server_port,
				          struct sockaddr *client_sa,
				          SOCKADDR_SIZE *client_sa_len,
				          struct sockaddr *server_sa,
				          SOCKADDR_SIZE *server_sa_len)
{
    const char *err;

    if (proto_info == 0)
	proto_info = inet_proto_info();

    *non_proxy = 0;

    /*
     * XXX We don't accept connections with the "UNKNOWN" protocol type,
     * because those would sidestep address-based access control mechanisms.
     */

    /*
     * Try version 1 protocol.
     */
    if (strncmp(str, "PROXY ", 6) == 0) {
	char   *saved_str = mystrndup(str, *str_len);
	char   *cp = saved_str;
	char   *beyond_header = split_at(saved_str, '\n');
	int     addr_family;

#define NEXT_TOKEN mystrtok(&cp, " \r")
	if (beyond_header == 0)
	    err = "missing protocol header terminator";
	else if (haproxy_srvr_parse_lit(NEXT_TOKEN, "PROXY", (char *) 0) < 0)
	    err = "bad or missing protocol header";
	else if (haproxy_srvr_parse_proto(NEXT_TOKEN, &addr_family) < 0)
	    err = "bad or missing protocol type";
	else if (haproxy_srvr_parse_addr(NEXT_TOKEN, smtp_client_addr,
					 addr_family, client_sa,
					 client_sa_len) < 0)
	    err = "bad or missing client address";
	else if (haproxy_srvr_parse_addr(NEXT_TOKEN, smtp_server_addr,
					 addr_family, server_sa,
					 server_sa_len) < 0)
	    err = "bad or missing server address";
	else if (haproxy_srvr_parse_port(NEXT_TOKEN, smtp_client_port,
					 client_sa) < 0)
	    err = "bad or missing client port";
	else if (haproxy_srvr_parse_port(NEXT_TOKEN, smtp_server_port,
					 server_sa) < 0)
	    err = "bad or missing server port";
	else {
	    err = 0;
	    *str_len = beyond_header - saved_str;
	}
	myfree(saved_str);

	return (err);
    }

    /*
     * Try version 2 protocol.
     */
    else {
	return (haproxy_srvr_parse_v2_hdr(str, str_len, non_proxy,
					  smtp_client_addr, smtp_client_port,
					  smtp_server_addr, smtp_server_port,
					  client_sa, client_sa_len,
					  server_sa, server_sa_len));
    }
}

/* haproxy_srvr_parse - ABI compatibility */

#undef haproxy_srvr_parse

const char *haproxy_srvr_parse(const char *str, ssize_t *str_len,
			               int *non_proxy,
			               MAI_HOSTADDR_STR *smtp_client_addr,
			               MAI_SERVPORT_STR *smtp_client_port,
			               MAI_HOSTADDR_STR *smtp_server_addr,
			               MAI_SERVPORT_STR *smtp_server_port);

const char *haproxy_srvr_parse(const char *str, ssize_t *str_len,
			               int *non_proxy,
			               MAI_HOSTADDR_STR *smtp_client_addr,
			               MAI_SERVPORT_STR *smtp_client_port,
			               MAI_HOSTADDR_STR *smtp_server_addr,
			               MAI_SERVPORT_STR *smtp_server_port)
{
    return (haproxy_srvr_parse_sa(str, str_len, non_proxy,
				  smtp_client_addr, smtp_client_port,
				  smtp_server_addr, smtp_server_port,
				  (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0,
			       (struct sockaddr *) 0, (SOCKADDR_SIZE *) 0));
}

/* haproxy_srvr_receive_sa - receive and parse haproxy protocol handshake */

int     haproxy_srvr_receive_sa(int fd, int *non_proxy,
				        MAI_HOSTADDR_STR *smtp_client_addr,
				        MAI_SERVPORT_STR *smtp_client_port,
				        MAI_HOSTADDR_STR *smtp_server_addr,
				        MAI_SERVPORT_STR *smtp_server_port,
				        struct sockaddr *client_sa,
				        SOCKADDR_SIZE *client_sa_len,
				        struct sockaddr *server_sa,
				        SOCKADDR_SIZE *server_sa_len)
{
    const char *err;
    VSTRING *escape_buf;
    char    read_buf[HAPROXY_HEADER_MAX_LEN + 1];
    ssize_t read_len;

    /*
     * We must not read(2) past the end of the HaProxy handshake. The v2
     * protocol assumes that the handshake will never be fragmented,
     * therefore we peek, parse the entire input, then read(2) only the
     * number of bytes parsed.
     */
    if ((read_len = recv(fd, read_buf, sizeof(read_buf) - 1, MSG_PEEK)) <= 0) {
	msg_warn("haproxy read: EOF");
	return (-1);
    }

    /*
     * Parse the haproxy handshake, and determine the handshake length.
     */
    read_buf[read_len] = 0;

    if ((err = haproxy_srvr_parse_sa(read_buf, &read_len, non_proxy,
				     smtp_client_addr, smtp_client_port,
				     smtp_server_addr, smtp_server_port,
				     client_sa, client_sa_len,
				     server_sa, server_sa_len)) != 0) {
	escape_buf = vstring_alloc(read_len * 2);
	escape(escape_buf, read_buf, read_len);
	msg_warn("haproxy read: %s: %s", err, vstring_str(escape_buf));
	vstring_free(escape_buf);
	return (-1);
    }

    /*
     * Try to pop the haproxy handshake off the input queue.
     */
    if (recv(fd, read_buf, read_len, 0) != read_len) {
	msg_warn("haproxy read: %m");
	return (-1);
    }
    return (0);
}

/* haproxy_srvr_receive - ABI compatibility */

#undef haproxy_srvr_receive

int     haproxy_srvr_receive(int fd, int *non_proxy,
			             MAI_HOSTADDR_STR *smtp_client_addr,
			             MAI_SERVPORT_STR *smtp_client_port,
			             MAI_HOSTADDR_STR *smtp_server_addr,
			             MAI_SERVPORT_STR *smtp_server_port);

int     haproxy_srvr_receive(int fd, int *non_proxy,
			             MAI_HOSTADDR_STR *smtp_client_addr,
			             MAI_SERVPORT_STR *smtp_client_port,
			             MAI_HOSTADDR_STR *smtp_server_addr,
			             MAI_SERVPORT_STR *smtp_server_port)
{
    return (haproxy_srvr_receive_sa(fd, non_proxy,
				    smtp_client_addr, smtp_client_port,
				    smtp_server_addr, smtp_server_port,
				    (struct sockaddr *) 0,
				    (SOCKADDR_SIZE *) 0,
				    (struct sockaddr *) 0,
				    (SOCKADDR_SIZE *) 0));
}
