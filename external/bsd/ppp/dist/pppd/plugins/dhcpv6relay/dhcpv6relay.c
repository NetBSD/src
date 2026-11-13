/*
 * dhcpv6relay.c - DHCPv6 relay plugin.
 *
 * Copyright (c) 2025-2026 Ultimate Linux Solutions (Pty) Ltd represented by
 * Jaco Kroon <jaco@uls.co.za>. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * When in doubt: https://datatracker.ietf.org/doc/html/rfc9915
 */
#include "dhcpv6relay.h"

#include <pppd/pppd.h>
#include <pppd/options.h>

#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/icmp6.h>
#include <stdio.h>

char pppd_version[] = PPPD_VERSION;

static int dhcpv6relay_setserver(const char** argv);

static bool dhcpv6relay_trusted = false;
static unsigned dhcpv6relay_metric = 0;
static unsigned dhcpv6relay_ra_interval = 0;

static struct option options[] = {
    { "dhcpv6-server", o_special, (void*) &dhcpv6relay_setserver,
	"DHCPv6 server to proxy DHCPv6 requests to",
	OPT_PRIV },
    { "dhcpv6-trusted", o_bool, &dhcpv6relay_trusted,
	"DHCPv6 trusted interface (allow incoming relay messages)",
	OPT_PRIO | OPT_PRIV | 1 },
    { "dhcpv6-untrusted", o_bool, &dhcpv6relay_trusted,
	"DHCPv6 untrusted interface (discard incoming relay messages)",
	OPT_PRIOSUB },
    { "dhcpv6-metric", o_int, &dhcpv6relay_metric,
      "Metric to use DHCPv6 supplied routes",
      OPT_PRIV|OPT_LLIMIT, NULL, 0, 0 },
    { "dhcpv6-ra-intvl", o_int, &dhcpv6relay_ra_interval,
      "How frequently to send unsolicited Router Advertisement frames (default off)",
      OPT_PRIV|OPT_LLIMIT, NULL, 0, 0 },
    { NULL }
};

static char* dhcpv6relay_server = NULL;
static int dhcpv6relay_sock_ll = -1;
static int dhcpv6relay_sock_mc = -1;
static int dhcpv6relay_upstream = -1;
static int dhcpv6relay_sock_rsra = -1;
static struct sockaddr_storage dhcpv6relay_sa;
static struct dhcpv6relay_route_entry *dhcpv6relay_delegations = NULL;

static
const char* dhcpv6_type2string(int msg_type)
{
    switch (msg_type) {
    case DHCPv6_MSGTYPE_SOLICIT:
	return "solicit";
    case DHCPv6_MSGTYPE_ADVERTISE:
	return "advertise";
    case DHCPv6_MSGTYPE_REQUEST:
	return "request";
    case DHCPv6_MSGTYPE_CONFIRM:
	return "confirm";
    case DHCPv6_MSGTYPE_RENEW:
	return "renew";
    case DHCPv6_MSGTYPE_REBIND:
	return "rebind";
    case DHCPv6_MSGTYPE_REPLY:
	return "reply";
    case DHCPv6_MSGTYPE_RELEASE:
	return "release";
    case DHCPv6_MSGTYPE_DECLINE:
	return "decline";
    case DHCPv6_MSGTYPE_RECONFIGURE:
	return "reconfigure";
    case DHCPv6_MSGTYPE_INFORMATION_REQUEST:
	return "information_request";
    case DHCPv6_MSGTYPE_RELAY_FORW:
	return "relay-forw";
    case DHCPv6_MSGTYPE_RELAY_REPL:
	return "relay-repl";
    default:
	return NULL;
    }
}

static
int dhcpv6relay_setserver(const char** argv)
{
    int r;
    struct addrinfo *ai = NULL, *i, hint = {
	.ai_flags = 0,
	.ai_family = 0, /* we *prefer* IPv6, but will accept IPv4 */
	.ai_socktype = SOCK_DGRAM, /* UDP */
	.ai_protocol = 0,
	.ai_addrlen = 0,
	.ai_addr = NULL,
	.ai_canonname = NULL,
	.ai_next = NULL,
    };
    char bfr_ip[INET6_ADDRSTRLEN];
    char bfr_port[6];

    free(dhcpv6relay_server);
    dhcpv6relay_server = NULL;

    if (!*argv || !**argv)
	return 1;

    r = getaddrinfo(*argv, "dhcpv6-server", &hint, &ai);
    if (r != 0) {
	error("DHCPv6 relay: Unable to set server address to %s: %s",
		*argv, gai_strerror(r));
	return 0;
    }

    dhcpv6relay_sa.ss_family = 0;
    for (i = ai; i && dhcpv6relay_sa.ss_family != AF_INET6; i = i->ai_next) {
	if (!dhcpv6relay_sa.ss_family || i->ai_family == AF_INET6) {
	    memcpy(&dhcpv6relay_sa, i->ai_addr, i->ai_addrlen);
	    if (dhcpv6relay_sa.ss_family == AF_INET6)
		break;
	}
    }

    freeaddrinfo(ai);
    if (dhcpv6relay_sa.ss_family) {
	getnameinfo((struct sockaddr*)&dhcpv6relay_sa, sizeof(dhcpv6relay_sa),
		bfr_ip, sizeof(bfr_ip),
		bfr_port, sizeof(bfr_port),
		NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM);
	notice("DHCPv6 relay: Using server [%s]:%s", bfr_ip, bfr_port);

	dhcpv6relay_server = strdup(*argv);
    } else {
	error("DHCPv6 relay: Failed to resolve %s to an IP address.",
		*argv);
    }

    return 1;
}

static
void routes_remove_all()
{
    char in6addr[INET6_ADDRSTRLEN];
    struct dhcpv6relay_route_entry* c = dhcpv6relay_delegations;

    while (c) {
	struct dhcpv6relay_route_entry* n = c->next;

	if (!sifdelroute(AF_INET6, &c->prefix, c->len, dhcpv6relay_metric))
	    error("DHCPv6 relay: failed to remove route for %s/%d",
		    inet_ntop(AF_INET6, &c->prefix, in6addr, sizeof(in6addr)), c->len);

	free(c);
	c = n;
    }

    dhcpv6relay_delegations = NULL;
}

static
void dhcpv6relay_down(__attribute__((unused)) void* unused, __attribute__((unused)) int unusedint)
{
    routes_remove_all();
    if (dhcpv6relay_sock_ll >= 0) {
	remove_fd(dhcpv6relay_sock_ll);
	close(dhcpv6relay_sock_ll);
	dhcpv6relay_sock_ll = -1;
    }
    if (dhcpv6relay_sock_mc >= 0) {
	remove_fd(dhcpv6relay_sock_mc);
	close(dhcpv6relay_sock_mc);
	dhcpv6relay_sock_mc = -1;
    }
    if (dhcpv6relay_upstream >= 0) {
	remove_fd(dhcpv6relay_upstream);
	close(dhcpv6relay_upstream);
	dhcpv6relay_upstream = -1;
    }
    if (dhcpv6relay_sock_rsra >= 0) {
	remove_fd(dhcpv6relay_sock_rsra);
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    }
}

static
struct dhcpv6relay_route_entry** dhcpv6relay_find_route_entry(const struct in6_addr* addr, uint8_t prefixlen)
{
    struct dhcpv6relay_route_entry** s = &dhcpv6relay_delegations;
    while (*s) {
	if (memcmp(&(*s)->prefix, addr, sizeof(*addr)) == 0 && (*s)->len == prefixlen)
	    return s;
	s = &(*s)->next;
    }
    return NULL;
}

static void dhcpv6relay_route_timeout(void* _r);

static
void dhcpv6relay_real_release_route(struct dhcpv6relay_route_entry** _r)
{
    char in6addr[INET6_ADDRSTRLEN];
    struct dhcpv6relay_route_entry* r = *_r;

    if (!sifdelroute(AF_INET6, &r->prefix, r->len, dhcpv6relay_metric))
	error("DHCPv6 relay: failed to remove route for %s/%d",
		inet_ntop(AF_INET6, &r->prefix, in6addr, sizeof(in6addr)), r->len);
    else
	notice("DHCPv6 relay: removed route %s/%d",
		inet_ntop(AF_INET6, &r->prefix, in6addr, sizeof(in6addr)), r->len);

    ppp_untimeout(dhcpv6relay_route_timeout, r);
    *_r = r->next;
    free(r);
}

static
void dhcpv6relay_route_timeout(void* _r)
{
    struct dhcpv6relay_route_entry** s = &dhcpv6relay_delegations;
    while (*s && *s != _r)
	s = &(*s)->next;

    if (!*s) {
	error("DHCPv6 relay: timeout on already released route delegation.");
    } else {
	dhcpv6relay_real_release_route(s);
    }
}

static
void dhcpv6relay_release_route(const struct in6_addr* addr, uint8_t prefixlen, __attribute__((unused)) uint32_t lifetime)
{
    char in6addr[INET6_ADDRSTRLEN];

    struct dhcpv6relay_route_entry** r = dhcpv6relay_find_route_entry(addr, prefixlen);

    if (!r) {
	error("DHCPv6 relay: Release of route %s/%d that was never delegated.",
		inet_ntop(AF_INET6, addr, in6addr, sizeof(in6addr)), prefixlen);
    } else {
	dhcpv6relay_real_release_route(r);
    }
}

static
void dhcpv6relay_add_route(const struct in6_addr* addr, uint8_t prefixlen, uint32_t lifetime)
{
    char in6addr[INET6_ADDRSTRLEN];
    struct dhcpv6relay_route_entry** _r = dhcpv6relay_find_route_entry(addr, prefixlen);
    struct dhcpv6relay_route_entry* r = _r ? *_r : NULL;
    if (r) {
	/* route is already installed, just update valid lifetime. */
	r->valid_until = time(NULL) + lifetime;
	ppp_untimeout(dhcpv6relay_route_timeout, r);
	ppp_timeout(dhcpv6relay_route_timeout, r, lifetime, 0);
	return;
    }

    if (!sifaddroute(AF_INET6, addr, prefixlen, dhcpv6relay_metric))
	return error("DHCPv6 relay: failed to install route for %s/%d",
		inet_ntop(AF_INET6, addr, in6addr, sizeof(in6addr)), prefixlen);

    notice("DHCPv6 relay: installed route %s/%d",
	    inet_ntop(AF_INET6, addr, in6addr, sizeof(in6addr)), prefixlen);

    r = malloc(sizeof(*r));
    r->next = dhcpv6relay_delegations;
    r->prefix = *addr;
    r->len = prefixlen;
    r->valid_until = time(NULL) + lifetime;
    ppp_timeout(dhcpv6relay_route_timeout, r, lifetime, 0);

    dhcpv6relay_delegations = r;
}

static
bool dhcpv6relay_valid_prefix(const struct in6_addr* addr, uint8_t prefixlen)
{
    int octet = prefixlen / 8;
    uint8_t mask = 0xff >> (prefixlen % 8);
    if (prefixlen > 128)
	return false;

    /* Can be done more efficiently but would require more complexity */
    while (octet < 16) {
	if (addr->s6_addr[octet] & mask)
	    return false;
	++octet;
	mask = 0xff;
    }

    return true;
}

#define from_where_str(x)  ((x) ? "server" : "client")
static
void dhcpv6relay_process_ia_pd(bool from_server, const unsigned char *bfr, uint16_t len, dhcpv6relay_route_func routefunc)
{
    char in6addr[INET6_ADDRSTRLEN];

    if (len < 12)
	return; /* IAID, T1, T2, 4 octets each, we don't care */
    bfr += 12;
    len -= 12;
    while (len > 4) {
	uint16_t opttype = ntohs(*(uint16_t*)bfr);
	uint16_t optlen = ntohs(*(uint16_t*)(bfr+2));
	bfr += 4;
	len -= 4;

	if (optlen > len) {
	    error("DHCPv6 relay: IA_PD sub-option overflows IA_PD option length. Corrupt packet?");
	    break;
	}

	switch (opttype) {
	case DHCPv6_OPTION_IAPREFIX:
	    /* 4 octets preferred, 4 octets valid lifetime, 1 octet length, 16 octets prefix, we discard IAprefix-options regardless. */
	    if (optlen < 25) {
		error("DHCPv6 relay: IA_PD option from %s needs at least 25 "
			"bytes, %u available, cannot process IA_PD.", from_where_str(from_server), optlen);
		break;
	    }
	    if (!dhcpv6relay_valid_prefix((const struct in6_addr*)(bfr + 9), bfr[8])) {
		error("DHCPv6 relay: IA_PD option from %s contains an illegal prefix %s/%u.", from_where_str(from_server),
			inet_ntop(AF_INET6, bfr + 9, in6addr, sizeof(in6addr)), bfr[8]);
		break;
	    }
	    routefunc((const struct in6_addr*)(bfr + 9), bfr[8], ntohl(*(const uint32_t*)(bfr+4)));
	    break;
	default:
	    break;
	}

	bfr += optlen;
	len -= optlen;
    }
}

static
void dhcpv6relay_process_ia_na(bool from_server, const unsigned char *bfr, uint16_t len, dhcpv6relay_route_func routefunc)
{
    if (len < 12)
	return; /* IAID, T1, T2, 4 octets each, we don't care */
    bfr += 12;
    len -= 12;
    while (len > 4) {
	uint16_t opttype = ntohs(*(uint16_t*)bfr);
	uint16_t optlen = ntohs(*(uint16_t*)(bfr+2));
	bfr += 4;
	len -= 4;

	if (optlen > len) {
	    error("DHCPv6 relay: IA_NA sub-option from %s overflows IA_NA option length. Corrupt packet?", from_where_str(from_server));
	    break;
	}

	switch (opttype) {
	case DHCPv6_OPTION_IAADDR:
	    if (optlen < 24) {
		error("DHCPv6 relay: IA_NA option from %s needs at least 24 "
			"bytes, only %u available, cannot process IA_NA.", from_where_str(from_server), optlen);
		break;
	    }
	    /* 16 octets address, 4 octets preferred lifetime, 4 octets valid lifetime */
	    routefunc((const struct in6_addr*)bfr, 128, ntohl(*(const uint32_t*)(bfr+20)));
	    break;
	default:
	    break;
	}

	bfr += optlen;
	len -= optlen;
    }
}

static
void dhcpv6relay_process_packet_for_routes(bool from_server, const unsigned char *bfr, uint16_t len)
{
    if (len < 1)
	return;
    uint8_t pkttype = *bfr;
    dhcpv6relay_route_func func;

    switch (pkttype) {
    case DHCPv6_MSGTYPE_RELAY_FORW:
    case DHCPv6_MSGTYPE_RELAY_REPL:
	if (pkttype == DHCPv6_MSGTYPE_RELAY_FORW) {
	    if (from_server) {
		error("DHCPv6 relay: Relay-Fwd message received from the server!  This is a message that only the client should send.");
		return;
	    }
	} else {
	    if (!from_server) {
		error("DHCPv6 relay: Relay-Reply message received from the client!  This is a message that only the server should send.");
		return;
	    }
	}
	/* these have 34 byte headers, so we can jump over that, then look for the relay message option
	 * and recurse on that as we really don't care about anything else. */
	if (len < 34)
	    return;
	bfr += 34;
	len -= 34;
	while (len > 4) {
	    uint16_t opttype = ntohs(*(uint16_t*)bfr);
	    uint16_t optlen = ntohs(*(uint16_t*)(bfr+2));
	    bfr += 4;
	    len -= 4;
	    if (optlen > len)
		return;
	    if (opttype == DHCPv6_OPTION_RELAY_MSG) {
		dhcpv6relay_process_packet_for_routes(from_server, bfr, optlen);
		return; /* there may be only one */
	    }
	    bfr += optlen;
	    len -= optlen;
	}
	break;
    case DHCPv6_MSGTYPE_REPLY:
    case DHCPv6_MSGTYPE_RELEASE:
	if (pkttype == DHCPv6_MSGTYPE_RELEASE) {
	    if (from_server) {
		error("DHCPv6 relay: Release message received from the server!  This is a message that only the client should send.");
		return;
	    }
	    func = dhcpv6relay_release_route;
	} else {
	    if (!from_server) {
		error("DHCPv6 relay: Reply message received from the client!  This is a message that only the server should send.");
		return;
	    }
	    func = dhcpv6relay_add_route;
	}

	/* everything else has a 4 byte header, the packet type (1 octet) and a transaction id (3 octets)
	 * which we don't care about, so just skip ahead to the options that we do care about */
	if (len < 4)
	    return;
	bfr += 4;
	len -= 4;
	while (len > 4) {
	    uint16_t opttype = ntohs(*(uint16_t*)bfr);
	    uint16_t optlen = ntohs(*(uint16_t*)(bfr+2));
	    bfr += 4;
	    len -= 4;
	    if (optlen > len) /* packet overrun */
		return;

	    switch (opttype) {
	    case DHCPv6_OPTION_IA_PD:
		dhcpv6relay_process_ia_pd(from_server, bfr, optlen, func);
		break;
	    case DHCPv6_OPTION_IA_NA:
		dhcpv6relay_process_ia_na(from_server, bfr, optlen, func);
		break;
	    default:
		break;
	    }
	    bfr += optlen;

	    len -= optlen;
	}
	break;
    default:
	/* nothing to do, we don't care about these. */
	break;
    }
}

static
void dhcpv6relay_server_event(int fd, __attribute__((unused)) void* unused)
{
    unsigned char buffer[1024];
    unsigned char *options = buffer + 34; /* skip fixed header */
    unsigned char *fwd_packet = NULL;
    uint16_t fwd_len = 0;
    char in6addr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 sa;
    socklen_t slen = sizeof(sa);
    bool valid_source = true;
    int hlim = 0;
    ssize_t r = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT | MSG_TRUNC,
	    (struct sockaddr*)&sa, &slen);
    if (r < 0) {
	error("DHCPv6 relay: Failed to read from %s socket: %s",
		fd == dhcpv6relay_sock_ll ? "LL" : "MC",
		strerror(errno));
	return;
    }
    if (r > sizeof(buffer)) {
	error("DHCPv6 relay: buffer overrun receiving packet of length %db received with a buffer of size %ub from the DHCP server",
		r, sizeof(buffer));
	return;
    }

    /* notice("Received %d bytes from fd=%d (%s), with source [%s]:%d, packet type: %s", r, fd,
	    "UPSTREAM",
	    inet_ntop(sa.sin6_family, sa.sin6_family == AF_INET ?
		(void*)&((struct sockaddr_in*)&sa)->sin_addr : (void*)&sa.sin6_addr,
		in6addr, sizeof(in6addr)),
	    ntohs(sa.sin6_port), r ? dhcpv6_type2string(buffer[0]) : "empty"); */

    if (sa.sin6_family != dhcpv6relay_sa.ss_family) {
	valid_source = false;
    } else if (sa.sin6_family == AF_INET6) {
	valid_source = sa.sin6_port == ((struct sockaddr_in6*)&dhcpv6relay_sa)->sin6_port

	    && memcmp(&sa.sin6_addr, &((struct sockaddr_in6*)&dhcpv6relay_sa)->sin6_addr,
		    sizeof(sa.sin6_addr)) == 0;
    } else if (sa.sin6_family == AF_INET) {
	valid_source = ((struct sockaddr_in*)&sa)->sin_port ==
	    ((struct sockaddr_in*)&dhcpv6relay_sa)->sin_port

	    && ((struct sockaddr_in*)&sa)->sin_addr.s_addr ==
	    ((struct sockaddr_in*)&dhcpv6relay_sa)->sin_addr.s_addr;
    } else {
	error("DHCv6 relay: Received non-IP packet on upstream socket.");
	return;
    }

    if (!valid_source) {
	error("DHCPv6 relay: Received packet from unexpected source [%s]:%d on upstream socket.",
		inet_ntop(sa.sin6_family, sa.sin6_family == AF_INET ?
		    (void*)&((struct sockaddr_in*)&sa)->sin_addr : (void*)&sa.sin6_addr,
		    in6addr, sizeof(in6addr)),
		sa.sin6_family == AF_INET ? ((struct sockaddr_in*)&sa)->sin_port : sa.sin6_port);
	return;
    }

    /* relay-repl is at least 34 bytes, without required options,
     * so blindly discard anything smaller */
    if (r < 34) {
	error("DHCPv6 relay: Received packet on upstream socket is too small to be a valid relay-repl.");
	return;
    }

    if (buffer[0] != DHCPv6_MSGTYPE_RELAY_REPL) {
	error("DHCPv6 relay: packet received from upstream server is a %s, expected relay-repl.",
		dhcpv6_type2string(buffer[0]));
	return;
    }

    /* don't partircularly care about the hop-count or linkaddr, we do need
     * peeraddr, but can recover that later */

    r -= 34;
    while (r > 4) {
	/* each option header is a 32-bits, 16-bits type and 16-bit length */
	uint16_t type = ntohs(*(uint16_t*)options);
	uint16_t len = ntohs(*(uint16_t*)(options + 2));
	options += 4;
	r -= 4;
	if (len > r) {
	    error("DHCPv6 relay: Error parsing packet from server, option %u specified "
		    "len=%u but only %d bytes available.", type, len, r);
	    return;
	}
	switch (type) {
	case DHCPv6_OPTION_RELAY_MSG:
	    if (fwd_packet)
		warn("DHCPv6 relay: relay-repl message with multiple relay-msg options.");
	    fwd_packet = options;
	    fwd_len = len;
	    break;
	/*case DHCPv6_OPTION_RELAY_PORT:
	    notice("Got relay-port: %u", ntohs(*(uint16_t*)options));
	    break; */
	default:
	    /* notice("DHCPv6 relay: Skipping processing of option %u of length %u.",
		    type, len); */
	    break;
	}
	options += len;
	r -= len;
    }

    if (!fwd_packet) {
	error("DHCPv6 relay: relay-repl message from server did not contain a relay-msg option.");
	return;
    }

    if (!fwd_len) {
	error("DHCPv6 relay: relay-repl message from the server had a zero length relay-msg option.");
	return;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    if (fwd_packet[0] == DHCPv6_MSGTYPE_RELAY_REPL) {
	/* this should only ever happen towards "trusted" ports, which is not the default. */
	/* TODO: Honour option 135 towards downstream, would need to see an example, spec
	 * is unclear and observed behaviour from KEA doesn't make sense. */
	sa.sin6_port = getservbyname("dhcpv6-server", "udp")->s_port;
	hlim = 64;
    } else {
	sa.sin6_port = getservbyname("dhcpv6-client", "udp")->s_port;
    }
    memcpy(&sa.sin6_addr, buffer + 18 /* peer-link address */, sizeof(sa.sin6_addr));
    sa.sin6_scope_id = if_nametoindex(ppp_ifname());

    setsockopt(dhcpv6relay_sock_ll, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hlim, sizeof(hlim));
    r = sendto(dhcpv6relay_sock_ll, fwd_packet, fwd_len, 0,
	    (struct sockaddr*)&sa, sizeof(sa));
    if (r < 0) {
	error("DHCPv6 relay: Error transmitting server response to client: %s",
		strerror(errno));
    }

    dhcpv6relay_process_packet_for_routes(true, fwd_packet, fwd_len);
}

static
int dhcpv6relay_init_upstream()
{
    /* use family from sa so that we can do DHCPv6 / IPv4. */
    dhcpv6relay_upstream = socket(dhcpv6relay_sa.ss_family, SOCK_DGRAM, 0);
    if (dhcpv6relay_upstream < 0) {
	error("DHCPv6 relay: Failed to bind upstream socket: %s",
		strerror(errno));
	return 0;
    }
    if (connect(dhcpv6relay_upstream, (struct sockaddr*)&dhcpv6relay_sa, sizeof(dhcpv6relay_sa)) < 0) {
	error("DHCPv6 relay: Failed to connect upstream socket: %s",
		strerror(errno));
	close(dhcpv6relay_upstream);
	dhcpv6relay_upstream = -1;
	return 0;
    }
    add_fd_callback(dhcpv6relay_upstream, dhcpv6relay_server_event, NULL);

    return 1;
}

static
void dhcpv6relay_client_event(int fd, __attribute__((unused)) void* unused)
{
    unsigned char buffer[1024];
    unsigned char fwd_head[256];
    const char* remote_id;
    const char* subscriber_id;
    struct iovec v[] = {
	{
	    .iov_base = fwd_head,
	    .iov_len = 0,
	},
	{
	    .iov_base = buffer,
	    .iov_len = 0,
	},
    };
    struct msghdr wv = {
	.msg_name = &dhcpv6relay_sa,
	.msg_namelen = sizeof(dhcpv6relay_sa),
	.msg_iov = v,
	.msg_iovlen = sizeof(v) / sizeof(*v),
	.msg_control = NULL,
	.msg_controllen = 0,
	.msg_flags = 0,
    };
    /* char in6addr[INET6_ADDRSTRLEN] */;
    struct sockaddr_in6 sa;
    uint16_t sport;
    socklen_t slen = sizeof(sa);
    ssize_t r = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT | MSG_TRUNC,
	    (struct sockaddr*)&sa, &slen);
    if (r < 0) {
	if (errno != EAGAIN)
	    error("DHCPv6 relay: Failed to read from %s socket: %s",
		    fd == dhcpv6relay_sock_ll ? "LL" : "MC",
		    strerror(errno));
	return;
    }
    if (r > sizeof(buffer)) {
	error("DHCPv6 relay: buffer overrun receiving packet of length %db received with a buffer of size %ub from the DHCP client",
		r, sizeof(buffer), fd == dhcpv6relay_sock_ll ? "LL" : "MC");
	return;
    }
    if (r < 4) {
	error("DHCPv6 relay: buffer underrun, we only got %d bytes from client, need at least 4 to be valid.", r);
	return;
    }
    v[1].iov_len = r;

    /* notice("Received %d bytes from fd=%d (%s), with source [%s]:%d, packet type: %s", r, fd,
	    fd == dhcpv6relay_sock_ll ? "LL" : "MC",
	    inet_ntop(sa.sin6_family, &sa.sin6_addr, in6addr, sizeof(in6addr)),
	    ntohs(sa.sin6_port), r ? dhcpv6_type2string(buffer[0]) : "empty"); */

    /* disallow Reply and Relay-Reply messages */
    if (buffer[0] == DHCPv6_MSGTYPE_REPLY || buffer[0] == DHCPv6_MSGTYPE_RELAY_REPL) {
	warn("Discarding DHCPv6 %s message received on PPP interface.",
		dhcpv6_type2string(buffer[0]));
	return;
    }

    /* if the interface is not trusted, also discard Relay-Fwd messages */
    if (!dhcpv6relay_trusted && buffer[0] == DHCPv6_MSGTYPE_RELAY_FORW) {
	warn("Discarding DHCPv6 %s message received on untrusted PPP interface.",
		dhcpv6_type2string(buffer[0]));
	return;
    }

    if (dhcpv6relay_upstream < 0 && !dhcpv6relay_init_upstream())
	return;

    dhcpv6relay_process_packet_for_routes(false, buffer, r);

    /* populate the forward header */
    fwd_head[0] = DHCPv6_MSGTYPE_RELAY_FORW; /* msg-type */
    fwd_head[1] = buffer[0] == DHCPv6_MSGTYPE_RELAY_FORW ? buffer[1] + 1 : 0; /* hop count */
    memset(&fwd_head[2], 0, 16); /* link-address, unspecified */
    memcpy(&fwd_head[18], &sa.sin6_addr, 16); /* peer-address */
    v[0].iov_len = 34;

    slen = sizeof(sa);
    if (getsockname(dhcpv6relay_upstream, (struct sockaddr*)&sa, &slen) < 0) {
	error("DHCPv6 relay: Unable to determine local sending port: %s",
		strerror(errno));
	return;
    }

#define push_checkbytes(x) do { if ((x) + v[0].iov_len > sizeof(fwd_head)) { error("DHCPv6 relay: Buffer overlow avoidance pushing %d bytes, need %d.", (x), (x) + v[0].iov_len - sizeof(fwd_head)); return; }} while(0)
#define push_uint16(val) do { push_checkbytes(2); uint16_t t = (val); fwd_head[v[0].iov_len++] = t >> 8; fwd_head[v[0].iov_len++] = t & 0xFF; } while(0);
#define push_bytes(ptr, bytes) do { push_checkbytes(bytes); memcpy(&fwd_head[v[0].iov_len], (ptr), (bytes)); v[0].iov_len += (bytes); } while(0)

    /* On Linux at least sin6_port and sin_port would refer the same
     * data but I can't guarantee that for solaris (and others) */
    switch (sa.sin6_family) {
    case AF_INET:
	sport = ((struct sockaddr_in*)&sa)->sin_port;
	break;
    case AF_INET6:
	sport = sa.sin6_port;
	break;
    default:
	error("DHCPv6 relay: Upstream socket is bound to something other than IP ... can't relay.");
	return;
    }

    push_uint16(DHCPv6_OPTION_RELAY_PORT);
    push_uint16(2);
    push_uint16(ntohs(sport));

    remote_id = ppp_get_remote_number();
    if (remote_id) {
	r = strlen(remote_id);
	push_uint16(DHCPv6_OPTION_REMOTE_ID);
	push_uint16(r);
	push_bytes(remote_id, r);
    }

    subscriber_id = ppp_peer_authname(NULL, 0);
    if (subscriber_id) {
	r = strlen(subscriber_id);
	push_uint16(DHCPv6_OPTION_SUBSCRIBER_ID);
	push_uint16(r);
	push_bytes(subscriber_id, r);
    }

    /* This *must* be the last option since it refers the the content from v[1] */
    push_uint16(DHCPv6_OPTION_RELAY_MSG);
    push_uint16(v[1].iov_len);

#undef push_checkbytes
#undef push_uint16
#undef push_bytes

    if (sendmsg(dhcpv6relay_upstream, &wv, 0) < 0) {
	error("DHCPv6 relay: Failed to transmit proxies request: %s",
		strerror(errno));
    }
}

static
void dhcpv6relay_send_router_advertisement(const struct sockaddr* da)
{
    static char ra[] = {
	134, 0, /* type and code */
	0, 0, /* checksum, kernel fills */
	0, /* hop limit, unspecified */
	0xC0, /* Managed + Other, rest unset */
	0xFF, 0xFF, /* Router lifetime, ~18.2h */
	0, 0, 0, 0, /* Reachable Time, unspecified */
	0, 0, 0, 0, /* Retrans Timer, unspecified */
    };
    struct sockaddr_in6 tda;

    if (!da) {
	memset(&tda, 0, sizeof(tda));
	tda.sin6_family = AF_INET6;
	/* just multicast it and move on ... for timed cases this is the only
	 * option, for solicited responses it might be needed to send to the peer's
	 * LL, but the spec is unclear and at least Mikrotik does multicast in
	 * response to solicitation */
	tda.sin6_scope_id = if_nametoindex(ppp_ifname());
	if (inet_pton(AF_INET6, "ff02::1", &tda.sin6_addr) < 0) {
	    error("DHCPv6 relay: Unable to prepare multicast address for sending router advertisement: %s",
		    strerror(errno));
	    return;
	}

	da = (const struct sockaddr*)&tda;
    }

    if (sendto(dhcpv6relay_sock_rsra, ra, sizeof(ra), 0, da, sizeof(tda)) < 0) {
	error("DHCPv6 relay: Failed to send router advertisement: %s", strerror(errno));
    }
}

static
void dhcpv6relay_send_router_advertisement_timed(__attribute__((unused)) void* unused)
{
    dhcpv6relay_send_router_advertisement(NULL);
    ppp_timeout(dhcpv6relay_send_router_advertisement_timed, NULL, dhcpv6relay_ra_interval, 0);
}

static
void dhcpv6relay_router_solicitation(int fd, __attribute__((unused)) void* unused)
{
    unsigned char bfr[1]; /* kernel will truncate packets in case of overflow,
			     and we don't care about the content.  Will
			     typically be 8 bytes, with an additional 8
			     optional bytes. */
    struct sockaddr_in6 sa;
    socklen_t slen = sizeof(sa);
    int r = recvfrom(fd, bfr, sizeof(bfr), MSG_DONTWAIT | MSG_TRUNC,
	    (struct sockaddr*)&sa, &slen);
    if (r < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
	error("Failure to receive router solicitation: %s", strerror(errno));
    } else {
	dhcpv6relay_send_router_advertisement(r < 0 ? NULL : (struct sockaddr*)&sa);
    }
}

static
int dhcpv6relay_populate_ll(struct sockaddr_in6* res)
{
    /* can we rather shortcut to get the address directly from ipv6cp? */
    struct ifaddrs *ifap, *ifa;
    int r = getifaddrs(&ifap);
    const struct sockaddr_in6* sa6;

    if (r < 0) {
	error("DHCPv6 relay: Unable to determine LL address");
	return 0;
    }

    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
	if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET6)
	    continue;

	sa6 = (struct sockaddr_in6*)ifa->ifa_addr;
	if (!sa6->sin6_scope_id)
	    continue; /* LL has sin6_scope_id set to interface id, != 0 */

	if (strcmp(ifa->ifa_name, ppp_ifname()) != 0)
	    continue; /* wrong interface */

	/* use it */
	*res = *sa6;
	freeifaddrs(ifap);
	return 1;
    }

    error("DHCPv6 relay: No matching LL addresses available for use.");
    freeifaddrs(ifap);
    return 0;
}

static
void dhcpv6relay_up(__attribute__((unused)) void* unused, __attribute__((unused)) int unusedint)
{
    struct sockaddr_in6 sa;
    struct ipv6_mreq mreq;
    struct servent *se;
    struct icmp6_filter filter;
    struct ifreq ifr;
    int v;

    /* no relay configured, so we can't work, simply don't listen
     * for DHCP solicitations */
    if (!dhcpv6relay_server)
	return;

    if (!dhcpv6relay_populate_ll(&sa))
	return;

    se = getservbyname("dhcpv6-server", "udp");
    if (!se) {
	error("DHCPv6 relay: Unable to determine UDP port number for dhcpv6-server: %s",
		strerror(errno));
	return;
    }

    sa.sin6_port = se->s_port;

    dhcpv6relay_sock_ll = socket(AF_INET6, SOCK_DGRAM, 0);
    if (dhcpv6relay_sock_ll < 0) {
	error("DHCPv6 relay: Unable to create LL socket: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }
    fcntl(dhcpv6relay_sock_ll, F_SETFD, FD_CLOEXEC);

    if (bind(dhcpv6relay_sock_ll, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
	error("DHCPv6 relay: Unable to bind LL socket: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }

    memset(&mreq, 0, sizeof(mreq));
    if (inet_pton(AF_INET6, "ff02::1:2", &mreq.ipv6mr_multiaddr) < 0) {
	error("DHCPv6 relay: Error preparing multicast binding: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }

    mreq.ipv6mr_interface = sa.sin6_scope_id;
    if (setsockopt(dhcpv6relay_sock_ll, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
	error("DHCPv6 relay: Error joining multicast group: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }

    dhcpv6relay_sock_mc = socket(AF_INET6, SOCK_DGRAM, 0);
    if (dhcpv6relay_sock_mc < 0) {
	error("DHCPv6 relay: Unable to create MC socket: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }
    fcntl(dhcpv6relay_sock_mc, F_SETFD, FD_CLOEXEC);

    sa.sin6_addr = mreq.ipv6mr_multiaddr;
    if (bind(dhcpv6relay_sock_mc, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
	error("DHCPv6 relay: Unable to bind MC socket: %s", strerror(errno));
	return dhcpv6relay_down(NULL, 0);
    }

    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);

    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ppp_ifname());

    dhcpv6relay_sock_rsra = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (dhcpv6relay_sock_rsra < 0) {
	warn("DHCPv6 relay: Unable to create raw socket for receiving router solicitations, this is non-fatal.");
    } else if (setsockopt(dhcpv6relay_sock_rsra, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
	warn("DHCPv6 relay: Unable to bind router solicitation to interface: %s", strerror(errno));
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    } else if (inet_pton(AF_INET6, "ff02::2", &mreq.ipv6mr_multiaddr) < 0) {
	warn("DHCPv6 relay: Unable to prepare multicast binding for router solicitations: %s", strerror(errno));
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    } else if (setsockopt(dhcpv6relay_sock_rsra, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) < 0) {
	warn("DHCPv6 relay: Unable to join multicast binding for receiving routing solicitations: %s", strerror(errno));
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    } else if (setsockopt(dhcpv6relay_sock_rsra, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter)) < 0) {
	warn("DHCPv6 relay: Failed to set ICMPv6 filter to only permit router solicitations: %s",
		strerror(errno));
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    } else if (bind(dhcpv6relay_sock_rsra, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
	warn("DHCPv6 relay: Unable to bind socket to link-local for sending router advertisement: %s",
		strerror(errno));
	close(dhcpv6relay_sock_rsra);
	dhcpv6relay_sock_rsra = -1;
    } else {
	v = 1;
	if (setsockopt(dhcpv6relay_sock_rsra, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &v, sizeof(v)) < 0) {
	    warn("DHCPv6 relay: Unable to set hop limit for receiving router solicitations: %s", strerror(errno));
	}
	v = 255;
	if (setsockopt(dhcpv6relay_sock_rsra, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &v, sizeof(v)) < 0) {
	    warn("DHCPv6 relay: Unable to set multicast hop limit for sending router advertisements: %s", strerror(errno));
	}
	if (setsockopt(dhcpv6relay_sock_rsra, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &v, sizeof(v)) < 0) {
	    warn("DHCPv6 relay: Unable to set unicast hop limit for sending router advertisements: %s", strerror(errno));
	}
	add_fd_callback(dhcpv6relay_sock_rsra, dhcpv6relay_router_solicitation, NULL);

	if (dhcpv6relay_ra_interval) {
	    ppp_timeout(dhcpv6relay_send_router_advertisement_timed, NULL, 1, 0);
	}
    }

    add_fd_callback(dhcpv6relay_sock_ll, dhcpv6relay_client_event, NULL);
    add_fd_callback(dhcpv6relay_sock_mc, dhcpv6relay_client_event, NULL);

    notice("DHCPv6 relay: ready.");
}

void
plugin_init(void)
{
    ppp_add_options(options);
    ppp_add_notify(NF_IPV6_UP, dhcpv6relay_up, NULL);
    ppp_add_notify(NF_IPV6_DOWN, dhcpv6relay_down, NULL);
}
