/*	$NetBSD: haproxy_srvr.c,v 1.1.1.1.8.2 2014/08/19 23:59:42 tls Exp $	*/

/*++
/* NAME
/*	haproxy_srvr 3
/* SUMMARY
/*	server-side haproxy protocol support
/* SYNOPSIS
/*	#include <haproxy_srvr.h>
/*
/*	const char *haproxy_srvr_parse(str,
/*			smtp_client_addr, smtp_client_port,
/*			smtp_server_addr, smtp_server_port)
/*	const char *str;
/*	MAI_HOSTADDR_STR *smtp_client_addr,
/*	MAI_SERVPORT_STR *smtp_client_port,
/*	MAI_HOSTADDR_STR *smtp_server_addr,
/*	MAI_SERVPORT_STR *smtp_server_port;
/* DESCRIPTION
/*	haproxy_srvr_parse() parses a haproxy line. The result is
/*	null in case of success, a pointer to text (with the error
/*	type) in case of error. If both IPv6 and IPv4 support are
/*	enabled, IPV4_IN_IPV6 address syntax (::ffff:1.2.3.4) is
/*	converted to IPV4 syntax.
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

/* Global library. */

#include <haproxy_srvr.h>

/* Application-specific. */

static INET_PROTO_INFO *proto_info;

/* haproxy_srvr_parse_lit - extract and validate string literal */

static int haproxy_srvr_parse_lit(const char *str,...)
{
    va_list ap;
    const char *cp;
    int     result = -1;

    if (msg_verbose)
	msg_info("haproxy_srvr_parse: %s", str);

    if (str != 0) {
	va_start(ap, str);
	while (result < 0 && (cp = va_arg(ap, const char *)) != 0)
	    if (strcmp(str, cp) == 0)
		result = 0;
	va_end(ap);
    }
    return (result);
}

/* haproxy_srvr_parse_proto - parse and validate the protocol type */

static int haproxy_srvr_parse_proto(const char *str, int *addr_family)
{
    if (msg_verbose)
	msg_info("haproxy_srvr_parse: proto=%s", str);

#ifdef AF_INET6
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
				           int addr_family)
{
    if (msg_verbose)
	msg_info("haproxy_srvr_parse: addr=%s proto=%d", str, addr_family);

    if (str == 0 || strlen(str) >= sizeof(MAI_HOSTADDR_STR))
	return (-1);

    switch (addr_family) {
#ifdef AF_INET6
    case AF_INET6:
	if (!valid_ipv6_hostaddr(str, DONT_GRIPE))
	    return (-1);
	if (strncasecmp("::ffff:", str, 7) == 0
	    && strchr((char *) proto_info->sa_family_list, AF_INET) != 0) {
	    memcpy(addr->buf, str + 7, strlen(str) + 1 - 7);
	    return (0);
	} else {
	    memcpy(addr->buf, str, strlen(str) + 1);
	    return (0);
	}
#endif
    case AF_INET:
	if (!valid_ipv4_hostaddr(str, DONT_GRIPE))
	    return (-1);
	memcpy(addr->buf, str, strlen(str) + 1);
	return (0);
    default:
	msg_panic("haproxy_srvr_parse: unexpected address family: %d",
		  addr_family);
    }
}

/* haproxy_srvr_parse_port - extract and validate TCP port */

static int haproxy_srvr_parse_port(const char *str, MAI_SERVPORT_STR *port)
{
    if (msg_verbose)
	msg_info("haproxy_srvr_parse: port=%s", str);
    if (str == 0 || strlen(str) >= sizeof(MAI_SERVPORT_STR)
	|| !valid_hostport(str, DONT_GRIPE)) {
	return (-1);
    } else {
	memcpy(port->buf, str, strlen(str) + 1);
	return (0);
    }
}

/* haproxy_srvr_parse - parse haproxy line */

const char *haproxy_srvr_parse(const char *str,
			               MAI_HOSTADDR_STR *smtp_client_addr,
			               MAI_SERVPORT_STR *smtp_client_port,
			               MAI_HOSTADDR_STR *smtp_server_addr,
			               MAI_SERVPORT_STR *smtp_server_port)
{
    char   *saved_str = mystrdup(str);
    char   *cp = saved_str;
    const char *err;
    int     addr_family;

    if (proto_info == 0)
	proto_info = inet_proto_info();

    /*
     * XXX We don't accept connections with the "UNKNOWN" protocol type,
     * because those would sidestep address-based access control mechanisms.
     */
#define NEXT_TOKEN mystrtok(&cp, " \r\n")
    if (haproxy_srvr_parse_lit(NEXT_TOKEN, "PROXY", (char *) 0) < 0)
	err = "unexpected protocol header";
    else if (haproxy_srvr_parse_proto(NEXT_TOKEN, &addr_family) < 0)
	err = "unsupported protocol type";
    else if (haproxy_srvr_parse_addr(NEXT_TOKEN, smtp_client_addr,
				     addr_family) < 0)
	err = "unexpected client address syntax";
    else if (haproxy_srvr_parse_addr(NEXT_TOKEN, smtp_server_addr,
				     addr_family) < 0)
	err = "unexpected server address syntax";
    else if (haproxy_srvr_parse_port(NEXT_TOKEN, smtp_client_port) < 0)
	err = "unexpected client port syntax";
    else if (haproxy_srvr_parse_port(NEXT_TOKEN, smtp_server_port) < 0)
	err = "unexpected server port syntax";
    else
	err = 0;
    myfree(saved_str);
    return (err);
}
