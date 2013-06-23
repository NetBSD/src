#include <sys/cdefs.h>
 __RCSID("$NetBSD: dhcp.c,v 1.1.1.21.2.3 2013/06/23 06:26:31 tls Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>

#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
#endif

#include <arpa/inet.h>
#include <net/route.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define __FAVOR_BSD /* Nasty glibc hack so we can use BSD semantics for UDP */
#include <netinet/udp.h>
#undef __FAVOR_BSD

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "arp.h"
#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "dhcp-common.h"
#include "duid.h"
#include "eloop.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "script.h"

#define DAD		"Duplicate address detected"
#define DHCP_MIN_LEASE	20

static uint8_t *packet;

/* Our aggregate option buffer.
 * We ONLY use this when options are split, which for most purposes is
 * practically never. See RFC3396 for details. */
static uint8_t *opt_buffer;

#define IPV4A		ADDRIPV4 | ARRAY
#define IPV4R		ADDRIPV4 | REQUEST

/* We should define a maximum for the NAK exponential backoff */
#define NAKOFF_MAX              60

/* Wait N nanoseconds between sending a RELEASE and dropping the address.
 * This gives the kernel enough time to actually send it. */
#define RELEASE_DELAY_S		0
#define RELEASE_DELAY_NS	10000000

struct dhcp_op {
	uint8_t value;
	const char *name;
};

static const struct dhcp_op dhcp_ops[] = {
	{ DHCP_DISCOVER, "DISCOVER" },
	{ DHCP_OFFER,    "OFFER" },
	{ DHCP_REQUEST,  "REQUEST" },
	{ DHCP_DECLINE,  "DECLINE" },
	{ DHCP_ACK,      "ACK" },
	{ DHCP_NAK,      "NAK" },
	{ DHCP_RELEASE,  "RELEASE" },
	{ DHCP_INFORM,   "INFORM" },
	{ 0, NULL }
};

const struct dhcp_opt dhcp_opts[] = {
	{ 1,	ADDRIPV4 | REQUEST,	"subnet_mask" },
		/* RFC 3442 states that the CSR has to come before all other
		 * routes. For completeness, we also specify static routes,
		 * then routers. */
	{ 121,  RFC3442,	"classless_static_routes" },
	{ 249,  RFC3442,	"ms_classless_static_routes" },
	{ 33,	IPV4A | REQUEST,	"static_routes" },
	{ 3,	IPV4A | REQUEST,	"routers" },
	{ 2,	UINT32,		"time_offset" },
	{ 4,	IPV4A,		"time_servers" },
	{ 5,	IPV4A,		"ien116_name_servers" },
	{ 6,	IPV4A,		"domain_name_servers" },
	{ 7,	IPV4A,		"log_servers" },
	{ 8,	IPV4A,		"cookie_servers" },
	{ 9,	IPV4A,		"lpr_servers" },
	{ 10,	IPV4A,		"impress_servers" },
	{ 11,	IPV4A,		"resource_location_servers" },
	{ 12,	STRING,		"host_name" },
	{ 13,	UINT16,		"boot_size" },
	{ 14,	STRING,		"merit_dump" },
	{ 15,	STRING,		"domain_name" },
	{ 16,	ADDRIPV4,	"swap_server" },
	{ 17,	STRING,		"root_path" },
	{ 18,	STRING,		"extensions_path" },
	{ 19,	UINT8,		"ip_forwarding" },
	{ 20,	UINT8,		"non_local_source_routing" },
	{ 21,	IPV4A,	"policy_filter" },
	{ 22,	SINT16,		"max_dgram_reassembly" },
	{ 23,	UINT16,		"default_ip_ttl" },
	{ 24,	UINT32,		"path_mtu_aging_timeout" },
	{ 25,	UINT16 | ARRAY,	"path_mtu_plateau_table" },
	{ 26,	UINT16,		"interface_mtu" },
	{ 27,	UINT8,		"all_subnets_local" },
	{ 28,	ADDRIPV4 | REQUEST,	"broadcast_address" },
	{ 29,	UINT8,		"perform_mask_discovery" },
	{ 30,	UINT8,		"mask_supplier" },
	{ 31,	UINT8,		"router_discovery" },
	{ 32,	ADDRIPV4,	"router_solicitation_address" },
	{ 34,	UINT8,		"trailer_encapsulation" },
	{ 35,	UINT32,		"arp_cache_timeout" },
	{ 36,	UINT16,		"ieee802_3_encapsulation" },
	{ 37,	UINT8,		"default_tcp_ttl" },
	{ 38,	UINT32,		"tcp_keepalive_interval" },
	{ 39,	UINT8,		"tcp_keepalive_garbage" },
	{ 40,	STRING,		"nis_domain" },
	{ 41,	IPV4A,		"nis_servers" },
	{ 42,	IPV4A,		"ntp_servers" },
	{ 43,	STRING,		"vendor_encapsulated_options" },
	{ 44,	IPV4A,		"netbios_name_servers" },
	{ 45,	ADDRIPV4,	"netbios_dd_server" },
	{ 46,	UINT8,		"netbios_node_type" },
	{ 47,	STRING,		"netbios_scope" },
	{ 48,	IPV4A,		"font_servers" },
	{ 49,	IPV4A,		"x_display_manager" },
	{ 50,	ADDRIPV4,	"dhcp_requested_address" },
	{ 51,	UINT32 | REQUEST,	"dhcp_lease_time" },
	{ 52,	UINT8,		"dhcp_option_overload" },
	{ 53,	UINT8,		"dhcp_message_type" },
	{ 54,	ADDRIPV4,	"dhcp_server_identifier" },
	{ 55,	UINT8 | ARRAY,	"dhcp_parameter_request_list" },
	{ 56,	STRING,		"dhcp_message" },
	{ 57,	UINT16,		"dhcp_max_message_size" },
	{ 58,	UINT32 | REQUEST,	"dhcp_renewal_time" },
	{ 59,	UINT32 | REQUEST,	"dhcp_rebinding_time" },
	{ 64,	STRING,		"nisplus_domain" },
	{ 65,	IPV4A,		"nisplus_servers" },
	{ 66,	STRING,		"tftp_server_name" },
	{ 67,	STRING,		"bootfile_name" },
	{ 68,	IPV4A,		"mobile_ip_home_agent" },
	{ 69,	IPV4A,		"smtp_server" },
	{ 70,	IPV4A,		"pop_server" },
	{ 71,	IPV4A,		"nntp_server" },
	{ 72,	IPV4A,		"www_server" },
	{ 73,	IPV4A,		"finger_server" },
	{ 74,	IPV4A,		"irc_server" },
	{ 75,	IPV4A,		"streettalk_server" },
	{ 76,	IPV4A,		"streettalk_directory_assistance_server" },
	{ 77,	STRING,		"user_class" },
	{ 80,	FLAG | NOREQ,	"rapid_commit" },
	{ 81,	STRING | RFC3397,	"fqdn_name" },
	{ 85,	IPV4A,		"nds_servers" },
	{ 86,	STRING,		"nds_tree_name" },
	{ 87,	STRING,		"nds_context" },
	{ 88,	STRING | RFC3397,	"bcms_controller_names" },
	{ 89,	IPV4A,		"bcms_controller_address" },
	{ 91,	UINT32,		"client_last_transaction_time" },
	{ 92,	IPV4A,		"associated_ip" },
	{ 98,	STRING,		"uap_servers" },
	{ 100,	STRING,		"posix_timezone" },
	{ 101,	STRING,		"tzdb_timezone" },
	{ 112,	IPV4A,		"netinfo_server_address" },
	{ 113,	STRING,		"netinfo_server_tag" },
	{ 114,	STRING,		"default_url" },
	{ 118,	ADDRIPV4,	"subnet_selection" },
	{ 119,	STRING | RFC3397,	"domain_search" },
	{ 120,	STRING | RFC3361,	"sip_server" },
	{ 212,  RFC5969,	"sixrd" },
	{ 0, 0, NULL }
};

static const char *dhcp_params[] = {
	"ip_address",
	"subnet_cidr",
	"network_number",
	"filename",
	"server_name",
	NULL
};

struct udp_dhcp_packet
{
	struct ip ip;
	struct udphdr udp;
	struct dhcp_message dhcp;
};
static const size_t udp_dhcp_len = sizeof(struct udp_dhcp_packet);

static int dhcp_open(struct interface *);

void
dhcp_printoptions(void)
{
	const struct dhcp_opt *opt;
	const char **p;

	for (p = dhcp_params; *p; p++)
		printf("    %s\n", *p);

	for (opt = dhcp_opts; opt->option; opt++)
		if (opt->var)
			printf("%03d %s\n", opt->option, opt->var);
}

static int
validate_length(uint8_t option, int dl, int *type)
{
	const struct dhcp_opt *opt;
	ssize_t sz;

	if (dl == 0)
		return -1;

	for (opt = dhcp_opts; opt->option; opt++) {
		if (opt->option != option)
			continue;

		if (type)
			*type = opt->type;

		if (opt->type == 0 ||
		    opt->type & (STRING | RFC3442 | RFC5969))
			return dl;

		if (opt->type & ADDRIPV4 && opt->type & ARRAY) {
			if (dl < (int)sizeof(uint32_t))
				return -1;
			return dl - (dl % sizeof(uint32_t));
		}

		sz = 0;
		if (opt->type & (UINT32 | ADDRIPV4))
			sz = sizeof(uint32_t);
		if (opt->type & UINT16)
			sz = sizeof(uint16_t);
		if (opt->type & UINT8)
			sz = sizeof(uint8_t);
		/* If we don't know the size, assume it's valid */
		if (sz == 0)
			return dl;
		return (dl < sz ? -1 : sz);
	}

	/* unknown option, so let it pass */
	return dl;
}

#ifdef DEBUG_MEMORY
static void
free_option_buffer(void)
{

	free(packet);
	free(opt_buffer);
}
#endif

#define get_option_raw(dhcp, opt) get_option(dhcp, opt, NULL, NULL)
static const uint8_t *
get_option(const struct dhcp_message *dhcp, uint8_t opt, int *len, int *type)
{
	const uint8_t *p = dhcp->options;
	const uint8_t *e = p + sizeof(dhcp->options);
	uint8_t l, ol = 0;
	uint8_t o = 0;
	uint8_t overl = 0;
	uint8_t *bp = NULL;
	const uint8_t *op = NULL;
	ssize_t bl = 0;

	while (p < e) {
		o = *p++;
		if (o == opt) {
			if (op) {
				if (!opt_buffer) {
					opt_buffer = malloc(sizeof(*dhcp));
					if (opt_buffer == NULL)
						return NULL;
#ifdef DEBUG_MEMORY
					atexit(free_option_buffer);
#endif
				}
				if (!bp)
					bp = opt_buffer;
				memcpy(bp, op, ol);
				bp += ol;
			}
			ol = *p;
			op = p + 1;
			bl += ol;
		}
		switch (o) {
		case DHO_PAD:
			continue;
		case DHO_END:
			if (overl & 1) {
				/* bit 1 set means parse boot file */
				overl &= ~1;
				p = dhcp->bootfile;
				e = p + sizeof(dhcp->bootfile);
			} else if (overl & 2) {
				/* bit 2 set means parse server name */
				overl &= ~2;
				p = dhcp->servername;
				e = p + sizeof(dhcp->servername);
			} else
				goto exit;
			break;
		case DHO_OPTIONSOVERLOADED:
			/* Ensure we only get this option once */
			if (!overl)
				overl = p[1];
			break;
		}
		l = *p++;
		p += l;
	}

exit:

	bl = validate_length(opt, bl, type);
	if (bl == -1) {
		errno = EINVAL;
		return NULL;
	}
	if (len)
		*len = bl;
	if (bp) {
		memcpy(bp, op, ol);
		return (const uint8_t *)opt_buffer;
	}
	if (op)
		return op;
	errno = ENOENT;
	return NULL;
}

int
get_option_addr(struct in_addr *a, const struct dhcp_message *dhcp,
    uint8_t option)
{
	const uint8_t *p = get_option_raw(dhcp, option);

	if (!p)
		return -1;
	memcpy(&a->s_addr, p, sizeof(a->s_addr));
	return 0;
}

int
get_option_uint32(uint32_t *i, const struct dhcp_message *dhcp, uint8_t option)
{
	const uint8_t *p = get_option_raw(dhcp, option);
	uint32_t d;

	if (!p)
		return -1;
	memcpy(&d, p, sizeof(d));
	*i = ntohl(d);
	return 0;
}

int
get_option_uint16(uint16_t *i, const struct dhcp_message *dhcp, uint8_t option)
{
	const uint8_t *p = get_option_raw(dhcp, option);
	uint16_t d;

	if (!p)
		return -1;
	memcpy(&d, p, sizeof(d));
	*i = ntohs(d);
	return 0;
}

int
get_option_uint8(uint8_t *i, const struct dhcp_message *dhcp, uint8_t option)
{
	const uint8_t *p = get_option_raw(dhcp, option);

	if (!p)
		return -1;
	if (i)
		*i = *(p);
	return 0;
}

ssize_t
decode_rfc3442(char *out, ssize_t len, int pl, const uint8_t *p)
{
	const uint8_t *e;
	ssize_t b, bytes = 0, ocets;
	uint8_t cidr;
	struct in_addr addr;
	char *o = out;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (pl < 5) {
		errno = EINVAL;
		return -1;
	}

	e = p + pl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			errno = EINVAL;
			return -1;
		}
		ocets = (cidr + 7) / 8;
		if (!out) {
			p += 4 + ocets;
			bytes += ((4 * 4) * 2) + 4;
			continue;
		}
		if ((((4 * 4) * 2) + 4) > len) {
			errno = ENOBUFS;
			return -1;
		}
		if (o != out) {
			*o++ = ' ';
			len--;
		}
		/* If we have ocets then we have a destination and netmask */
		if (ocets > 0) {
			addr.s_addr = 0;
			memcpy(&addr.s_addr, p, ocets);
			b = snprintf(o, len, "%s/%d", inet_ntoa(addr), cidr);
			p += ocets;
		} else
			b = snprintf(o, len, "0.0.0.0/0");
		o += b;
		len -= b;

		/* Finally, snag the router */
		memcpy(&addr.s_addr, p, 4);
		p += 4;
		b = snprintf(o, len, " %s", inet_ntoa(addr));
		o += b;
		len -= b;
	}

	if (out)
		return o - out;
	return bytes;
}

static struct rt_head *
decode_rfc3442_rt(int dl, const uint8_t *data)
{
	const uint8_t *p = data;
	const uint8_t *e;
	uint8_t cidr;
	size_t ocets;
	struct rt_head *routes;
	struct rt *rt = NULL;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (dl < 5)
		return NULL;

	routes = malloc(sizeof(*routes));
	TAILQ_INIT(routes);
	e = p + dl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			ipv4_freeroutes(routes);
			errno = EINVAL;
			return NULL;
		}

		rt = calloc(1, sizeof(*rt));
		if (rt == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			ipv4_freeroutes(routes);
			return NULL;
		}
		TAILQ_INSERT_TAIL(routes, rt, next);

		ocets = (cidr + 7) / 8;
		/* If we have ocets then we have a destination and netmask */
		if (ocets > 0) {
			memcpy(&rt->dest.s_addr, p, ocets);
			p += ocets;
			rt->net.s_addr = htonl(~0U << (32 - cidr));
		}

		/* Finally, snag the router */
		memcpy(&rt->gate.s_addr, p, 4);
		p += 4;
	}
	return routes;
}

char *
decode_rfc3361(int dl, const uint8_t *data)
{
	uint8_t enc;
	unsigned int l;
	char *sip = NULL;
	struct in_addr addr;
	char *p;

	if (dl < 2) {
		errno = EINVAL;
		return 0;
	}

	enc = *data++;
	dl--;
	switch (enc) {
	case 0:
		if ((l = decode_rfc3397(NULL, 0, dl, data)) > 0) {
			sip = malloc(l);
			if (sip == NULL)
				return 0;
			decode_rfc3397(sip, l, dl, data);
		}
		break;
	case 1:
		if (dl == 0 || dl % 4 != 0) {
			errno = EINVAL;
			break;
		}
		addr.s_addr = INADDR_BROADCAST;
		l = ((dl / sizeof(addr.s_addr)) * ((4 * 4) + 1)) + 1;
		sip = p = malloc(l);
		if (sip == NULL)
			return 0;
		while (dl != 0) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			data += sizeof(addr.s_addr);
			p += snprintf(p, l - (p - sip), "%s ", inet_ntoa(addr));
			dl -= sizeof(addr.s_addr);
		}
		*--p = '\0';
		break;
	default:
		errno = EINVAL;
		return 0;
	}

	return sip;
}

/* Decode an RFC5969 6rd order option into a space
 * separated string. Returns length of string (including
 * terminating zero) or zero on error. */
ssize_t
decode_rfc5969(char *out, ssize_t len, int pl, const uint8_t *p)
{
	uint8_t ipv4masklen, ipv6prefixlen;
	uint8_t ipv6prefix[16];
	uint8_t br[4];
	int i;
	ssize_t b, bytes = 0;

	if (pl < 22) {
		errno = EINVAL;
		return 0;
	}

	ipv4masklen = *p++;
	pl--;
	ipv6prefixlen = *p++;
	pl--;

	for (i = 0; i < 16; i++) {
		ipv6prefix[i] = *p++;
		pl--;
	}
	if (out) {
		b= snprintf(out, len,
		    "%d %d "
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x:"
		    "%02x%02x:%02x%02x",
		    ipv4masklen, ipv6prefixlen,
		    ipv6prefix[0], ipv6prefix[1], ipv6prefix[2], ipv6prefix[3],
		    ipv6prefix[4], ipv6prefix[5], ipv6prefix[6], ipv6prefix[7],
		    ipv6prefix[8], ipv6prefix[9], ipv6prefix[10],ipv6prefix[11],
		    ipv6prefix[12],ipv6prefix[13],ipv6prefix[14], ipv6prefix[15]
		);

		len -= b;
		out += b;
		bytes += b;
	} else {
		bytes += 16 * 2 + 8 + 2 + 1 + 2;
	}

	while (pl >= 4) {
		br[0] = *p++;
		br[1] = *p++;
		br[2] = *p++;
		br[3] = *p++;
		pl -= 4;

		if (out) {
			b= snprintf(out, len, " %d.%d.%d.%d",
			    br[0], br[1], br[2], br[3]);
			len -= b;
			out += b;
			bytes += b;
		} else {
			bytes += (4 * 4);
		}
	}

	return bytes;
}

char *
get_option_string(const struct dhcp_message *dhcp, uint8_t option)
{
	int type = 0;
	int len;
	const uint8_t *p;
	char *s;

	p = get_option(dhcp, option, &len, &type);
	if (!p || *p == '\0')
		return NULL;

	if (type & RFC3397) {
		type = decode_rfc3397(NULL, 0, len, p);
		if (!type) {
			errno = EINVAL;
			return NULL;
		}
		s = malloc(sizeof(char) * type);
		if (s)
			decode_rfc3397(s, type, len, p);
		return s;
	}

	if (type & RFC3361)
		return decode_rfc3361(len, p);

	s = malloc(sizeof(char) * (len + 1));
	if (s) {
		memcpy(s, p, len);
		s[len] = '\0';
	}
	return s;
}

/* This calculates the netmask that we should use for static routes.
 * This IS different from the calculation used to calculate the netmask
 * for an interface address. */
static uint32_t
route_netmask(uint32_t ip_in)
{
	/* used to be unsigned long - check if error */
	uint32_t p = ntohl(ip_in);
	uint32_t t;

	if (IN_CLASSA(p))
		t = ~IN_CLASSA_NET;
	else {
		if (IN_CLASSB(p))
			t = ~IN_CLASSB_NET;
		else {
			if (IN_CLASSC(p))
				t = ~IN_CLASSC_NET;
			else
				t = 0;
		}
	}

	while (t & p)
		t >>= 1;

	return (htonl(~t));
}

/* We need to obey routing options.
 * If we have a CSR then we only use that.
 * Otherwise we add static routes and then routers. */
struct rt_head *
get_option_routes(struct interface *ifp, const struct dhcp_message *dhcp)
{
	struct if_options *ifo = ifp->options;
	const uint8_t *p;
	const uint8_t *e;
	struct rt_head *routes = NULL;
	struct rt *route = NULL;
	int len;
	const char *csr = "";

	/* If we have CSR's then we MUST use these only */
	if (!has_option_mask(ifo->nomask, DHO_CSR))
		p = get_option(dhcp, DHO_CSR, &len, NULL);
	else
		p = NULL;
	/* Check for crappy MS option */
	if (!p && !has_option_mask(ifo->nomask, DHO_MSCSR)) {
		p = get_option(dhcp, DHO_MSCSR, &len, NULL);
		if (p)
			csr = "MS ";
	}
	if (p) {
		routes = decode_rfc3442_rt(len, p);
		if (routes) {
			if (!(ifo->options & DHCPCD_CSR_WARNED)) {
				syslog(LOG_DEBUG,
				    "%s: using %sClassless Static Routes",
				    ifp->name, csr);
				ifo->options |= DHCPCD_CSR_WARNED;
			}
			return routes;
		}
	}

	/* OK, get our static routes first. */
	routes = malloc(sizeof(*routes));
	if (routes == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	TAILQ_INIT(routes);
	if (!has_option_mask(ifo->nomask, DHO_STATICROUTE))
		p = get_option(dhcp, DHO_STATICROUTE, &len, NULL);
	else
		p = NULL;
	if (p) {
		e = p + len;
		while (p < e) {
			route = calloc(1, sizeof(*route));
			if (route == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(routes);
				return NULL;
			}
			memcpy(&route->dest.s_addr, p, 4);
			p += 4;
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
			route->net.s_addr = route_netmask(route->dest.s_addr);
			TAILQ_INSERT_TAIL(routes, route, next);
		}
	}

	/* Now grab our routers */
	if (!has_option_mask(ifo->nomask, DHO_ROUTER))
		p = get_option(dhcp, DHO_ROUTER, &len, NULL);
	else
		p = NULL;
	if (p) {
		e = p + len;
		while (p < e) {
			route = calloc(1, sizeof(*route));
			if (route == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				ipv4_freeroutes(routes);
				return NULL;
			}
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
			TAILQ_INSERT_TAIL(routes, route, next);
		}
	}

	return routes;
}

#define PUTADDR(_type, _val)						      \
	{								      \
		*p++ = _type;						      \
		*p++ = 4;						      \
		memcpy(p, &_val.s_addr, 4);				      \
		p += 4;							      \
	}

int
dhcp_message_add_addr(struct dhcp_message *dhcp,
    uint8_t type, struct in_addr addr)
{
	uint8_t *p;
	size_t len;

	p = dhcp->options;
	while (*p != DHO_END) {
		p++;
		p += *p + 1;
	}

	len = p - (uint8_t *)dhcp;
	if (len + 6 > sizeof(*dhcp)) {
		errno = ENOMEM;
		return -1;
	}

	PUTADDR(type, addr);
	*p = DHO_END;
	return 0;
}

ssize_t
make_message(struct dhcp_message **message,
    const struct interface *iface,
    uint8_t type)
{
	struct dhcp_message *dhcp;
	uint8_t *m, *lp, *p;
	uint8_t *n_params = NULL;
	uint32_t ul;
	uint16_t sz;
	size_t len;
	const char *hp;
	const struct dhcp_opt *opt;
	const struct if_options *ifo = iface->options;
	const struct dhcp_state *state = D_CSTATE(iface);
	const struct dhcp_lease *lease = &state->lease;
	time_t up = uptime() - state->start_uptime;
	const char *hostname;

	dhcp = calloc(1, sizeof (*dhcp));
	if (dhcp == NULL)
		return -1;
	m = (uint8_t *)dhcp;
	p = dhcp->options;

	if ((type == DHCP_INFORM || type == DHCP_RELEASE ||
		(type == DHCP_REQUEST &&
		    state->net.s_addr == lease->net.s_addr &&
		    (state->new == NULL ||
			state->new->cookie == htonl(MAGIC_COOKIE)))))
	{
		dhcp->ciaddr = state->addr.s_addr;
		/* In-case we haven't actually configured the address yet */
		if (type == DHCP_INFORM && state->addr.s_addr == 0)
			dhcp->ciaddr = lease->addr.s_addr;
	}

	dhcp->op = DHCP_BOOTREQUEST;
	dhcp->hwtype = iface->family;
	switch (iface->family) {
	case ARPHRD_ETHER:
	case ARPHRD_IEEE802:
		dhcp->hwlen = iface->hwlen;
		memcpy(&dhcp->chaddr, &iface->hwaddr, iface->hwlen);
		break;
	}

	if (ifo->options & DHCPCD_BROADCAST &&
	    dhcp->ciaddr == 0 &&
	    type != DHCP_DECLINE &&
	    type != DHCP_RELEASE)
		dhcp->flags = htons(BROADCAST_FLAG);

	if (type != DHCP_DECLINE && type != DHCP_RELEASE) {
		if (up < 0 || up > (time_t)UINT16_MAX)
			dhcp->secs = htons((uint16_t)UINT16_MAX);
		else
			dhcp->secs = htons(up);
	}
	dhcp->xid = htonl(state->xid);
	dhcp->cookie = htonl(MAGIC_COOKIE);

	*p++ = DHO_MESSAGETYPE;
	*p++ = 1;
	*p++ = type;

	if (state->clientid) {
		*p++ = DHO_CLIENTID;
		memcpy(p, state->clientid, state->clientid[0] + 1);
		p += state->clientid[0] + 1;
	}

	if (lease->addr.s_addr && lease->cookie == htonl(MAGIC_COOKIE)) {
		if (type == DHCP_DECLINE ||
		    (type == DHCP_REQUEST &&
			lease->addr.s_addr != state->addr.s_addr))
		{
			PUTADDR(DHO_IPADDRESS, lease->addr);
			if (lease->server.s_addr)
				PUTADDR(DHO_SERVERID, lease->server);
		}

		if (type == DHCP_RELEASE) {
			if (lease->server.s_addr)
				PUTADDR(DHO_SERVERID, lease->server);
		}
	}

	if (type == DHCP_DECLINE) {
		*p++ = DHO_MESSAGE;
		len = strlen(DAD);
		*p++ = len;
		memcpy(p, DAD, len);
		p += len;
	}

	if (type == DHCP_DISCOVER &&
	    !(options & DHCPCD_TEST) &&
	    has_option_mask(ifo->requestmask, DHO_RAPIDCOMMIT))
	{
		/* RFC 4039 Section 3 */
		*p++ = DHO_RAPIDCOMMIT;
		*p++ = 0;
	}

	if (type == DHCP_DISCOVER && ifo->options & DHCPCD_REQUEST)
		PUTADDR(DHO_IPADDRESS, ifo->req_addr);

	if (type == DHCP_DISCOVER ||
	    type == DHCP_INFORM ||
	    type == DHCP_REQUEST)
	{
		*p++ = DHO_MAXMESSAGESIZE;
		*p++ = 2;
		sz = get_mtu(iface->name);
		if (sz < MTU_MIN) {
			if (set_mtu(iface->name, MTU_MIN) == 0)
				sz = MTU_MIN;
		} else if (sz > MTU_MAX) {
			/* Even though our MTU could be greater than
			 * MTU_MAX (1500) dhcpcd does not presently
			 * handle DHCP packets any bigger. */
			sz = MTU_MAX;
		}
		sz = htons(sz);
		memcpy(p, &sz, 2);
		p += 2;

		if (ifo->userclass[0]) {
			*p++ = DHO_USERCLASS;
			memcpy(p, ifo->userclass, ifo->userclass[0] + 1);
			p += ifo->userclass[0] + 1;
		}

		if (ifo->vendorclassid[0]) {
			*p++ = DHO_VENDORCLASSID;
			memcpy(p, ifo->vendorclassid,
			    ifo->vendorclassid[0] + 1);
			p += ifo->vendorclassid[0] + 1;
		}


		if (type != DHCP_INFORM) {
			if (ifo->leasetime != 0) {
				*p++ = DHO_LEASETIME;
				*p++ = 4;
				ul = htonl(ifo->leasetime);
				memcpy(p, &ul, 4);
				p += 4;
			}
		}

		/* Regardless of RFC2132, we should always send a hostname
		 * upto the first dot (the short hostname) as otherwise
		 * confuses some DHCP servers when updating DNS.
		 * The FQDN option should be used if a FQDN is required. */
		if (ifo->hostname[0] == '\0')
			hostname = get_hostname();
		else
			hostname = ifo->hostname;
		if (ifo->options & DHCPCD_HOSTNAME && hostname) {
			*p++ = DHO_HOSTNAME;
			hp = strchr(hostname, '.');
			if (hp)
				len = hp - hostname;
			else
				len = strlen(hostname);
			*p++ = len;
			memcpy(p, hostname, len);
			p += len;
		}
		if (ifo->fqdn != FQDN_DISABLE) {
			/* IETF DHC-FQDN option (81), RFC4702 */
			*p++ = DHO_FQDN;
			lp = p;
			*p++ = 3;
			/*
			 * Flags: 0000NEOS
			 * S: 1 => Client requests Server to update
			 *         a RR in DNS as well as PTR
			 * O: 1 => Server indicates to client that
			 *         DNS has been updated
			 * E: 1 => Name data is DNS format
			 * N: 1 => Client requests Server to not
			 *         update DNS
			 */
			if (hostname)
				*p++ = (ifo->fqdn & 0x09) | 0x04;
			else
				*p++ = (FQDN_NONE & 0x09) | 0x04;
			*p++ = 0; /* from server for PTR RR */
			*p++ = 0; /* from server for A RR if S=1 */
			if (hostname) {
				ul = encode_rfc1035(hostname, p);
				*lp += ul;
				p += ul;
			}
		}

		/* vendor is already encoded correctly, so just add it */
		if (ifo->vendor[0]) {
			*p++ = DHO_VENDOR;
			memcpy(p, ifo->vendor, ifo->vendor[0] + 1);
			p += ifo->vendor[0] + 1;
		}

		*p++ = DHO_PARAMETERREQUESTLIST;
		n_params = p;
		*p++ = 0;
		for (opt = dhcp_opts; opt->option; opt++) {
			if (!(opt->type & REQUEST ||
				has_option_mask(ifo->requestmask, opt->option)))
				continue;
			if (opt->type & NOREQ)
				continue;
			if (type == DHCP_INFORM &&
			    (opt->option == DHO_RENEWALTIME ||
				opt->option == DHO_REBINDTIME))
				continue;
			*p++ = opt->option;
		}
		*n_params = p - n_params - 1;
	}
	*p++ = DHO_END;

#ifdef BOOTP_MESSAGE_LENTH_MIN
	/* Some crappy DHCP servers think they have to obey the BOOTP minimum
	 * message length.
	 * They are wrong, but we should still cater for them. */
	while (p - m < BOOTP_MESSAGE_LENTH_MIN)
		*p++ = DHO_PAD;
#endif

	*message = dhcp;
	return p - m;
}

ssize_t
write_lease(const struct interface *ifp, const struct dhcp_message *dhcp)
{
	int fd;
	ssize_t bytes = sizeof(*dhcp);
	const uint8_t *p = dhcp->options;
	const uint8_t *e = p + sizeof(dhcp->options);
	uint8_t l;
	uint8_t o = 0;
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* We don't write BOOTP leases */
	if (is_bootp(dhcp)) {
		unlink(state->leasefile);
		return 0;
	}

	syslog(LOG_DEBUG, "%s: writing lease `%s'",
	    ifp->name, state->leasefile);

	fd = open(state->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		return -1;

	/* Only write as much as we need */
	while (p < e) {
		o = *p;
		if (o == DHO_END) {
			bytes = p - (const uint8_t *)dhcp;
			break;
		}
		p++;
		if (o != DHO_PAD) {
			l = *p++;
			p += l;
		}
	}
	bytes = write(fd, dhcp, bytes);
	close(fd);
	return bytes;
}

struct dhcp_message *
read_lease(const struct interface *ifp)
{
	int fd;
	struct dhcp_message *dhcp;
	const struct dhcp_state *state = D_CSTATE(ifp);
	ssize_t bytes;

	fd = open(state->leasefile, O_RDONLY);
	if (fd == -1) {
		if (errno != ENOENT)
			syslog(LOG_ERR, "%s: open `%s': %m",
			    ifp->name, state->leasefile);
		return NULL;
	}
	syslog(LOG_DEBUG, "%s: reading lease `%s'",
	    ifp->name, state->leasefile);
	dhcp = calloc(1, sizeof(*dhcp));
	if (dhcp == NULL) {
		close(fd);
		return NULL;
	}
	bytes = read(fd, dhcp, sizeof(*dhcp));
	close(fd);
	if (bytes < 0) {
		free(dhcp);
		dhcp = NULL;
	}
	return dhcp;
}

ssize_t
dhcp_env(char **env, const char *prefix, const struct dhcp_message *dhcp,
    const struct interface *ifp)
{
	const struct if_options *ifo;
	const uint8_t *p;
	int pl;
	struct in_addr addr;
	struct in_addr net;
	struct in_addr brd;
	char *val, *v;
	const struct dhcp_opt *opt;
	ssize_t len, e = 0;
	char **ep;
	char cidr[4];
	uint8_t overl = 0;

	ifo = ifp->options;
	get_option_uint8(&overl, dhcp, DHO_OPTIONSOVERLOADED);

	if (!env) {
		for (opt = dhcp_opts; opt->option; opt++) {
			if (!opt->var)
				continue;
			if (has_option_mask(ifo->nomask, opt->option))
				continue;
			if (get_option_raw(dhcp, opt->option))
				e++;
		}
		if (dhcp->yiaddr || dhcp->ciaddr)
			e += 5;
		if (*dhcp->bootfile && !(overl & 1))
			e++;
		if (*dhcp->servername && !(overl & 2))
			e++;
		return e;
	}

	ep = env;
	if (dhcp->yiaddr || dhcp->ciaddr) {
		/* Set some useful variables that we derive from the DHCP
		 * message but are not necessarily in the options */
		addr.s_addr = dhcp->yiaddr ? dhcp->yiaddr : dhcp->ciaddr;
		setvar(&ep, prefix, "ip_address", inet_ntoa(addr));
		if (get_option_addr(&net, dhcp, DHO_SUBNETMASK) == -1) {
			net.s_addr = ipv4_getnetmask(addr.s_addr);
			setvar(&ep, prefix, "subnet_mask", inet_ntoa(net));
		}
		snprintf(cidr, sizeof(cidr), "%d", inet_ntocidr(net));
		setvar(&ep, prefix, "subnet_cidr", cidr);
		if (get_option_addr(&brd, dhcp, DHO_BROADCAST) == -1) {
			brd.s_addr = addr.s_addr | ~net.s_addr;
			setvar(&ep, prefix, "broadcast_address", inet_ntoa(brd));
		}
		addr.s_addr = dhcp->yiaddr & net.s_addr;
		setvar(&ep, prefix, "network_number", inet_ntoa(addr));
	}

	if (*dhcp->bootfile && !(overl & 1))
		setvar(&ep, prefix, "filename", (const char *)dhcp->bootfile);
	if (*dhcp->servername && !(overl & 2))
		setvar(&ep, prefix, "server_name", (const char *)dhcp->servername);

	for (opt = dhcp_opts; opt->option; opt++) {
		if (!opt->var)
			continue;
		if (has_option_mask(ifo->nomask, opt->option))
			continue;
		val = NULL;
		p = get_option(dhcp, opt->option, &pl, NULL);
		if (!p)
			continue;
		/* We only want the FQDN name */
		if (opt->option == DHO_FQDN) {
			p += 3;
			pl -= 3;
		}
		len = print_option(NULL, 0, opt->type, pl, p, ifp->name);
		if (len < 0)
			return -1;
		e = strlen(prefix) + strlen(opt->var) + len + 4;
		v = val = *ep++ = malloc(e);
		if (v == NULL)
			return -1;
		v += snprintf(val, e, "%s_%s=", prefix, opt->var);
		if (len != 0)
			print_option(v, len, opt->type, pl, p, ifp->name);
	}

	return ep - env;
}

void
get_lease(struct dhcp_lease *lease, const struct dhcp_message *dhcp)
{
	struct timeval now;

	lease->cookie = dhcp->cookie;
	/* BOOTP does not set yiaddr for replies when ciaddr is set. */
	if (dhcp->yiaddr)
		lease->addr.s_addr = dhcp->yiaddr;
	else
		lease->addr.s_addr = dhcp->ciaddr;
	if (get_option_addr(&lease->net, dhcp, DHO_SUBNETMASK) == -1)
		lease->net.s_addr = ipv4_getnetmask(lease->addr.s_addr);
	if (get_option_addr(&lease->brd, dhcp, DHO_BROADCAST) == -1)
		lease->brd.s_addr = lease->addr.s_addr | ~lease->net.s_addr;
	if (get_option_uint32(&lease->leasetime, dhcp, DHO_LEASETIME) == 0) {
		/* Ensure that we can use the lease */
		get_monotonic(&now);
		if (now.tv_sec + (time_t)lease->leasetime < now.tv_sec)
			lease->leasetime = ~0U; /* Infinite lease */
	} else
		lease->leasetime = ~0U; /* Default to infinite lease */
	if (get_option_uint32(&lease->renewaltime, dhcp, DHO_RENEWALTIME) != 0)
		lease->renewaltime = 0;
	if (get_option_uint32(&lease->rebindtime, dhcp, DHO_REBINDTIME) != 0)
		lease->rebindtime = 0;
	if (get_option_addr(&lease->server, dhcp, DHO_SERVERID) != 0)
		lease->server.s_addr = INADDR_ANY;
}

static const char *
get_dhcp_op(uint8_t type)
{
	const struct dhcp_op *d;

	for (d = dhcp_ops; d->name; d++)
		if (d->value == type)
			return d->name;
	return NULL;
}

static void
dhcp_fallback(void *arg)
{
	struct interface *iface;

	iface = (struct interface *)arg;
	select_profile(iface, iface->options->fallback);
	start_interface(iface);
}

uint32_t
dhcp_xid(const struct interface *ifp)
{
	uint32_t xid;

	if (ifp->options->options & DHCPCD_XID_HWADDR &&
	    ifp->hwlen >= sizeof(xid))
		/* The lower bits are probably more unique on the network */
		memcpy(&xid, (ifp->hwaddr + ifp->hwlen) - sizeof(xid),
		    sizeof(xid));
	else
		xid = arc4random();

	return xid;
}

void
dhcp_close(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;

	if (state->arp_fd != -1) {
		eloop_event_delete(state->arp_fd);
		close(state->arp_fd);
		state->arp_fd = -1;
	}
	if (state->raw_fd != -1) {
		eloop_event_delete(state->raw_fd);
		close(state->raw_fd);
		state->raw_fd = -1;
	}
	if (state->udp_fd != -1) {
		/* we don't listen to events on the udp */
		close(state->udp_fd);
		state->udp_fd = -1;
	}

	state->interval = 0;
}

static int
dhcp_openudp(struct interface *iface)
{
	int s;
	struct sockaddr_in sin;
	int n;
	struct dhcp_state *state;
#ifdef SO_BINDTODEVICE
	struct ifreq ifr;
	char *p;
#endif

	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return -1;

	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1)
		goto eexit;
#ifdef SO_BINDTODEVICE
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, iface->name, sizeof(ifr.ifr_name));
	/* We can only bind to the real device */
	p = strchr(ifr.ifr_name, ':');
	if (p)
	    *p = '\0';
	if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr,
		    sizeof(ifr)) == -1)
		goto eexit;
#endif
	/* As we don't use this socket for receiving, set the
	 * receive buffer to 1 */
	n = 1;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1)
		goto eexit;
	state = D_STATE(iface);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(DHCP_CLIENT_PORT);
	sin.sin_addr.s_addr = state->addr.s_addr;
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1)
		goto eexit;

	state->udp_fd = s;
	set_cloexec(s);
	return 0;

eexit:
	close(s);
	return -1;
}

static ssize_t
dhcp_sendpacket(const struct interface *iface, struct in_addr to,
    const uint8_t *data, ssize_t len)
{
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = to.s_addr;
	sin.sin_port = htons(DHCP_SERVER_PORT);
	return sendto(D_CSTATE(iface)->udp_fd, data, len, 0,
	    (struct sockaddr *)&sin, sizeof(sin));
}

static uint16_t
checksum(const void *data, uint16_t len)
{
	const uint8_t *addr = data;
	uint32_t sum = 0;

	while (len > 1) {
		sum += addr[0] * 256 + addr[1];
		addr += 2;
		len -= 2;
	}

	if (len == 1)
		sum += *addr * 256;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	sum = htons(sum);

	return ~sum;
}

static ssize_t
dhcp_makeudppacket(uint8_t **p, const uint8_t *data, size_t length,
	struct in_addr source, struct in_addr dest)
{
	struct udp_dhcp_packet *udpp;
	struct ip *ip;
	struct udphdr *udp;

	udpp = calloc(1, sizeof(*udpp));
	if (udpp == NULL)
		return -1;
	ip = &udpp->ip;
	udp = &udpp->udp;

	/* OK, this is important :)
	 * We copy the data to our packet and then create a small part of the
	 * ip structure and an invalid ip_len (basically udp length).
	 * We then fill the udp structure and put the checksum
	 * of the whole packet into the udp checksum.
	 * Finally we complete the ip structure and ip checksum.
	 * If we don't do the ordering like so then the udp checksum will be
	 * broken, so find another way of doing it! */

	memcpy(&udpp->dhcp, data, length);

	ip->ip_p = IPPROTO_UDP;
	ip->ip_src.s_addr = source.s_addr;
	if (dest.s_addr == 0)
		ip->ip_dst.s_addr = INADDR_BROADCAST;
	else
		ip->ip_dst.s_addr = dest.s_addr;

	udp->uh_sport = htons(DHCP_CLIENT_PORT);
	udp->uh_dport = htons(DHCP_SERVER_PORT);
	udp->uh_ulen = htons(sizeof(*udp) + length);
	ip->ip_len = udp->uh_ulen;
	udp->uh_sum = checksum(udpp, sizeof(*udpp));

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(*ip) >> 2;
	ip->ip_id = arc4random() & UINT16_MAX;
	ip->ip_ttl = IPDEFTTL;
	ip->ip_len = htons(sizeof(*ip) + sizeof(*udp) + length);
	ip->ip_sum = checksum(ip, sizeof(*ip));

	*p = (uint8_t *)udpp;
	return sizeof(*ip) + sizeof(*udp) + length;
}

static void
send_message(struct interface *iface, int type,
    void (*callback)(void *))
{
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_message *dhcp;
	uint8_t *udp;
	ssize_t len, r;
	struct in_addr from, to;
	in_addr_t a = 0;
	struct timeval tv;

	if (!callback)
		syslog(LOG_DEBUG, "%s: sending %s with xid 0x%x",
		    iface->name, get_dhcp_op(type), state->xid);
	else {
		if (state->interval == 0)
			state->interval = 4;
		else {
			state->interval *= 2;
			if (state->interval > 64)
				state->interval = 64;
		}
		tv.tv_sec = state->interval + DHCP_RAND_MIN;
		tv.tv_usec = arc4random() % (DHCP_RAND_MAX_U - DHCP_RAND_MIN_U);
		timernorm(&tv);
		syslog(LOG_DEBUG,
		    "%s: sending %s (xid 0x%x), next in %0.2f seconds",
		    iface->name, get_dhcp_op(type), state->xid,
		    timeval_to_double(&tv));
	}

	/* Ensure sockets are open. */
	if (dhcp_open(iface) == -1) {
		if (!(options & DHCPCD_TEST))
			dhcp_drop(iface, "FAIL");
		return;
	}

	/* If we couldn't open a UDP port for our IP address
	 * then we cannot renew.
	 * This could happen if our IP was pulled out from underneath us.
	 * Also, we should not unicast from a BOOTP lease. */
	if (state->udp_fd == -1 ||
	    (!(ifo->options & DHCPCD_INFORM) && is_bootp(state->new)))
	{
		a = state->addr.s_addr;
		state->addr.s_addr = 0;
	}
	len = make_message(&dhcp, iface, type);
	if (a)
		state->addr.s_addr = a;
	from.s_addr = dhcp->ciaddr;
	if (from.s_addr)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = 0;
	if (to.s_addr && to.s_addr != INADDR_BROADCAST) {
		r = dhcp_sendpacket(iface, to, (uint8_t *)dhcp, len);
		if (r == -1) {
			syslog(LOG_ERR, "%s: dhcp_sendpacket: %m", iface->name);
			dhcp_close(iface);
		}
	} else {
		len = dhcp_makeudppacket(&udp, (uint8_t *)dhcp, len, from, to);
		if (len == -1)
			return;
		r = ipv4_sendrawpacket(iface, ETHERTYPE_IP, udp, len);
		free(udp);
		/* If we failed to send a raw packet this normally means
		 * we don't have the ability to work beneath the IP layer
		 * for this interface.
		 * As such we remove it from consideration without actually
		 * stopping the interface. */
		if (r == -1) {
			syslog(LOG_ERR, "%s: send_raw_packet: %m", iface->name);
			if (!(options & DHCPCD_TEST))
				dhcp_drop(iface, "FAIL");
			dhcp_close(iface);
			eloop_timeout_delete(NULL, iface);
			callback = NULL;
		}
	}
	free(dhcp);

	/* Even if we fail to send a packet we should continue as we are
	 * as our failure timeouts will change out codepath when needed. */
	if (callback)
		eloop_timeout_add_tv(&tv, callback, iface);
}

static void
send_inform(void *arg)
{

	send_message((struct interface *)arg, DHCP_INFORM, send_inform);
}

static void
send_discover(void *arg)
{

	send_message((struct interface *)arg, DHCP_DISCOVER, send_discover);
}

static void
send_request(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_request);
}

static void
send_renew(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_renew);
}

static void
send_rebind(void *arg)
{

	send_message((struct interface *)arg, DHCP_REQUEST, send_rebind);
}

void
dhcp_discover(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	int timeout = ifo->timeout;

	/* If we're rebooting and we're not daemonised then we need
	 * to shorten the normal timeout to ensure we try correctly
	 * for a fallback or IPv4LL address. */
	if (state->state == DHS_REBOOT && !(options & DHCPCD_DAEMONISED)) {
		timeout -= ifo->reboot;
		if (timeout <= 0)
			timeout = 2;
	}

	state->state = DHS_DISCOVER;
	state->xid = dhcp_xid(iface);
	eloop_timeout_delete(NULL, iface);
	if (ifo->fallback)
		eloop_timeout_add_sec(timeout, dhcp_fallback, iface);
	else if (ifo->options & DHCPCD_IPV4LL &&
	    !IN_LINKLOCAL(htonl(state->addr.s_addr)))
	{
		if (IN_LINKLOCAL(htonl(state->fail.s_addr)))
			eloop_timeout_add_sec(RATE_LIMIT_INTERVAL,
			    ipv4ll_start, iface);
		else
			eloop_timeout_add_sec(timeout, ipv4ll_start, iface);
	}
	if (ifo->options & DHCPCD_REQUEST)
		syslog(LOG_INFO, "%s: soliciting a DHCP lease (requesting %s)",
		    iface->name, inet_ntoa(ifo->req_addr));
	else
		syslog(LOG_INFO, "%s: soliciting a DHCP lease", iface->name);
	send_discover(iface);
}

static void
dhcp_request(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	state->state = DHS_REQUEST;
	send_request(ifp);
}

static void
dhcp_expire(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	state->interval = 0;
	if (state->addr.s_addr == 0) {
		/* We failed to reboot, so enter discovery. */
		state->lease.addr.s_addr = 0;
		dhcp_discover(ifp);
		return;
	}

	syslog(LOG_ERR, "%s: DHCP lease expired", ifp->name);
	eloop_timeout_delete(NULL, ifp);
	dhcp_drop(ifp, "EXPIRE");
	unlink(state->leasefile);
	if (ifp->carrier != LINK_DOWN)
		start_interface(ifp);
}

void
dhcp_decline(struct interface *ifp)
{

	send_message(ifp, DHCP_DECLINE, NULL);
}

static void
dhcp_renew(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease = &state->lease;

	syslog(LOG_DEBUG, "%s: renewing lease of %s",
	    ifp->name, inet_ntoa(lease->addr));
	syslog(LOG_DEBUG, "%s: rebind in %"PRIu32" seconds,"
	    " expire in %"PRIu32" seconds",
	    ifp->name, lease->rebindtime - lease->renewaltime,
	    lease->leasetime - lease->renewaltime);
	state->state = DHS_RENEW;
	state->xid = dhcp_xid(ifp);
	send_renew(ifp);
}

static void
dhcp_rebind(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct dhcp_lease *lease = &state->lease;

	syslog(LOG_WARNING, "%s: failed to renew DHCP, rebinding",
	    ifp->name);
	syslog(LOG_DEBUG, "%s: expire in %"PRIu32" seconds",
	    ifp->name, lease->leasetime - lease->rebindtime);
	state->state = DHS_REBIND;
	eloop_timeout_delete(send_renew, ifp);
	state->lease.server.s_addr = 0;
	ifp->options->options &= ~ DHCPCD_CSR_WARNED;
	send_rebind(ifp);
}


void
dhcp_bind(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_lease *lease = &state->lease;
	struct timeval tv;

	/* We're binding an address now - ensure that sockets are closed */
	dhcp_close(iface);
	state->reason = NULL;
	if (clock_monotonic)
		get_monotonic(&lease->boundtime);
	state->xid = 0;
	free(state->old);
	state->old = state->new;
	state->new = state->offer;
	state->offer = NULL;
	get_lease(lease, state->new);
	if (ifo->options & DHCPCD_STATIC) {
		syslog(LOG_INFO, "%s: using static address %s",
		    iface->name, inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		lease->net.s_addr = ifo->req_mask.s_addr;
		state->reason = "STATIC";
	} else if (state->new->cookie != htonl(MAGIC_COOKIE)) {
		syslog(LOG_INFO, "%s: using IPv4LL address %s",
		    iface->name, inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "IPV4LL";
	} else if (ifo->options & DHCPCD_INFORM) {
		if (ifo->req_addr.s_addr != 0)
			lease->addr.s_addr = ifo->req_addr.s_addr;
		else
			lease->addr.s_addr = state->addr.s_addr;
		syslog(LOG_INFO, "%s: received approval for %s", iface->name,
		    inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "INFORM";
	} else {
		if (gettimeofday(&tv, NULL) == 0)
			lease->leasedfrom = tv.tv_sec;
		else if (lease->frominfo)
			state->reason = "TIMEOUT";
		if (lease->leasetime == ~0U) {
			lease->renewaltime =
			    lease->rebindtime =
			    lease->leasetime;
			syslog(LOG_INFO, "%s: leased %s for infinity",
			    iface->name, inet_ntoa(lease->addr));
		} else {
			if (lease->leasetime < DHCP_MIN_LEASE) {
				syslog(LOG_WARNING,
				    "%s: minimum lease is %d seconds",
				    iface->name, DHCP_MIN_LEASE);
				lease->leasetime = DHCP_MIN_LEASE;
			}
			if (lease->rebindtime == 0)
				lease->rebindtime = lease->leasetime * T2;
			else if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime = lease->leasetime * T2;
				syslog(LOG_WARNING,
				    "%s: rebind time greater than lease "
				    "time, forcing to %"PRIu32" seconds",
				    iface->name, lease->rebindtime);
			}
			if (lease->renewaltime == 0)
				lease->renewaltime = lease->leasetime * T1;
			else if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime = lease->leasetime * T1;
				syslog(LOG_WARNING,
				    "%s: renewal time greater than rebind "
				    "time, forcing to %"PRIu32" seconds",
				    iface->name, lease->renewaltime);
			}
			syslog(lease->addr.s_addr == state->addr.s_addr ?
			    LOG_DEBUG : LOG_INFO,
			    "%s: leased %s for %"PRIu32" seconds", iface->name,
			    inet_ntoa(lease->addr), lease->leasetime);
		}
	}
	if (options & DHCPCD_TEST) {
		state->reason = "TEST";
		script_runreason(iface, state->reason);
		exit(EXIT_SUCCESS);
	}
	if (state->reason == NULL) {
		if (state->old) {
			if (state->old->yiaddr == state->new->yiaddr &&
			    lease->server.s_addr)
				state->reason = "RENEW";
			else
				state->reason = "REBIND";
		} else if (state->state == DHS_REBOOT)
			state->reason = "REBOOT";
		else
			state->reason = "BOUND";
	}
	if (lease->leasetime == ~0U)
		lease->renewaltime = lease->rebindtime = lease->leasetime;
	else {
		eloop_timeout_add_sec(lease->renewaltime, dhcp_renew, iface);
		eloop_timeout_add_sec(lease->rebindtime, dhcp_rebind, iface);
		eloop_timeout_add_sec(lease->leasetime, dhcp_expire, iface);
		syslog(LOG_DEBUG,
		    "%s: renew in %"PRIu32" seconds, rebind in %"PRIu32
		    " seconds",
		    iface->name, lease->renewaltime, lease->rebindtime);
	}
	ipv4_applyaddr(iface);
	daemonise();
	state->state = DHS_BOUND;
	if (ifo->options & DHCPCD_ARP) {
		state->claims = 0;
		arp_announce(iface);
	}
}

static void
dhcp_timeout(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);

	dhcp_bind(ifp);
	state->interval = 0;
	dhcp_discover(ifp);
}

struct dhcp_message *
dhcp_message_new(struct in_addr *addr, struct in_addr *mask)
{
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = calloc(1, sizeof(*dhcp));
	if (dhcp == NULL)
		return NULL;
	dhcp->yiaddr = addr->s_addr;
	p = dhcp->options;
	if (mask && mask->s_addr != INADDR_ANY) {
		*p++ = DHO_SUBNETMASK;
		*p++ = sizeof(mask->s_addr);
		memcpy(p, &mask->s_addr, sizeof(mask->s_addr));
		p+= sizeof(mask->s_addr);
	}
	*p++ = DHO_END;
	return dhcp;
}

static int
handle_3rdparty(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state;
	struct in_addr addr, net, dst;

	ifo = ifp->options;
	if (ifo->req_addr.s_addr != INADDR_ANY)
		return 0;

	if (ipv4_getaddress(ifp->name, &addr, &net, &dst) == 1)
		ipv4_handleifa(RTM_NEWADDR, ifp->name, &addr, &net, &dst);
	else {
		syslog(LOG_INFO,
		    "%s: waiting for 3rd party to configure IP address",
		    ifp->name);
		state = D_STATE(ifp);
		state->reason = "3RDPARTY";
		script_runreason(ifp, state->reason);
	}
	return 1;
}

static void
dhcp_static(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state;

	if (handle_3rdparty(ifp))
		return;
	ifo = ifp->options;
	state = D_STATE(ifp);
	state->offer = dhcp_message_new(&ifo->req_addr, &ifo->req_mask);
	eloop_timeout_delete(NULL, ifp);
	dhcp_bind(ifp);
}

void
dhcp_inform(struct interface *ifp)
{
	struct dhcp_state *state;

	if (handle_3rdparty(ifp))
		return;

	state = D_STATE(ifp);
	if (options & DHCPCD_TEST) {
		state->addr.s_addr = ifp->options->req_addr.s_addr;
		state->net.s_addr = ifp->options->req_mask.s_addr;
	} else {
		ifp->options->options |= DHCPCD_STATIC;
		dhcp_static(ifp);
	}

	state->state = DHS_INFORM;
	state->xid = dhcp_xid(ifp);
	send_inform(ifp);
}

void
dhcp_reboot_newopts(struct interface *ifp, int oldopts)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;
	ifo = ifp->options;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		state->addr.s_addr != ifo->req_addr.s_addr) ||
	    (oldopts & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		!(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))))
	{
		dhcp_drop(ifp, "EXPIRE");
	}
}

static void
dhcp_reboot(struct interface *ifp)
{
	struct if_options *ifo;
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;
	ifo = ifp->options;
	state->interval = 0;

	if (ifo->options & DHCPCD_LINK && ifp->carrier == LINK_DOWN) {
		syslog(LOG_INFO, "%s: waiting for carrier", ifp->name);
		return;
	}
	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}
	if (ifo->reboot == 0 || state->offer == NULL) {
		dhcp_discover(ifp);
		return;
	}
	if (ifo->options & DHCPCD_INFORM) {
		syslog(LOG_INFO, "%s: informing address of %s",
		    ifp->name, inet_ntoa(state->lease.addr));
	} else if (state->offer->cookie == 0) {
		if (ifo->options & DHCPCD_IPV4LL) {
			state->claims = 0;
			arp_announce(ifp);
		} else
			dhcp_discover(ifp);
		return;
	} else {
		syslog(LOG_INFO, "%s: rebinding lease of %s",
		    ifp->name, inet_ntoa(state->lease.addr));
	}
	state->state = DHS_REBOOT;
	state->xid = dhcp_xid(ifp);
	state->lease.server.s_addr = 0;
	eloop_timeout_delete(NULL, ifp);
	if (ifo->fallback)
		eloop_timeout_add_sec(ifo->reboot, dhcp_fallback, ifp);
	else if (ifo->options & DHCPCD_LASTLEASE && state->lease.frominfo)
		eloop_timeout_add_sec(ifo->reboot, dhcp_timeout, ifp);
	else if (!(ifo->options & DHCPCD_INFORM &&
	    options & (DHCPCD_MASTER | DHCPCD_DAEMONISED)))
		eloop_timeout_add_sec(ifo->reboot, dhcp_expire, ifp);
	/* Don't bother ARP checking as the server could NAK us first. */
	if (ifo->options & DHCPCD_INFORM)
		dhcp_inform(ifp);
	else
		dhcp_request(ifp);
}

void
dhcp_drop(struct interface *ifp, const char *reason)
{
	struct dhcp_state *state;
#ifdef RELEASE_SLOW
	struct timespec ts;
#endif

	state = D_STATE(ifp);
	if (state == NULL)
		return;
	eloop_timeouts_delete(ifp, dhcp_expire, NULL);
	if (ifp->options->options & DHCPCD_RELEASE) {
		unlink(state->leasefile);
		if (ifp->carrier != LINK_DOWN &&
		    state->new != NULL &&
		    state->new->cookie == htonl(MAGIC_COOKIE))
		{
			syslog(LOG_INFO, "%s: releasing lease of %s",
			    ifp->name, inet_ntoa(state->lease.addr));
			state->xid = dhcp_xid(ifp);
			send_message(ifp, DHCP_RELEASE, NULL);
#ifdef RELEASE_SLOW
			/* Give the packet a chance to go */
			ts.tv_sec = RELEASE_DELAY_S;
			ts.tv_nsec = RELEASE_DELAY_NS;
			nanosleep(&ts, NULL);
#endif
		}
	}
	free(state->old);
	state->old = state->new;
	state->new = NULL;
	state->reason = reason;
	ipv4_applyaddr(ifp);
	free(state->old);
	state->old = NULL;
	state->lease.addr.s_addr = 0;
	ifp->options->options &= ~ DHCPCD_CSR_WARNED;
}

static void
log_dhcp(int lvl, const char *msg,
    const struct interface *iface, const struct dhcp_message *dhcp,
    const struct in_addr *from)
{
	const char *tfrom;
	char *a;
	struct in_addr addr;
	int r;

	if (strcmp(msg, "NAK:") == 0)
		a = get_option_string(dhcp, DHO_MESSAGE);
	else if (dhcp->yiaddr != 0) {
		addr.s_addr = dhcp->yiaddr;
		a = strdup(inet_ntoa(addr));
		if (a == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
	} else
		a = NULL;

	tfrom = "from";
	r = get_option_addr(&addr, dhcp, DHO_SERVERID);
	if (dhcp->servername[0] && r == 0)
		syslog(lvl, "%s: %s %s %s %s `%s'", iface->name, msg, a,
		    tfrom, inet_ntoa(addr), dhcp->servername);
	else {
		if (r != 0) {
			tfrom = "via";
			addr = *from;
		}
		if (a == NULL)
			syslog(lvl, "%s: %s %s %s",
			    iface->name, msg, tfrom, inet_ntoa(addr));
		else
			syslog(lvl, "%s: %s %s %s %s",
			    iface->name, msg, a, tfrom, inet_ntoa(addr));
	}
	free(a);
}

static int
blacklisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	for (i = 0; i < ifo->blacklist_len; i += 2)
		if (ifo->blacklist[i] == (addr & ifo->blacklist[i + 1]))
			return 1;
	return 0;
}

static int
whitelisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	if (ifo->whitelist_len == 0)
		return -1;
	for (i = 0; i < ifo->whitelist_len; i += 2)
		if (ifo->whitelist[i] == (addr & ifo->whitelist[i + 1]))
			return 1;
	return 0;
}

static void
dhcp_handle(struct interface *iface, struct dhcp_message **dhcpp,
    const struct in_addr *from)
{
	struct dhcp_state *state = D_STATE(iface);
	struct if_options *ifo = iface->options;
	struct dhcp_message *dhcp = *dhcpp;
	struct dhcp_lease *lease = &state->lease;
	uint8_t type, tmp;
	struct in_addr addr;
	size_t i;

	/* reset the message counter */
	state->interval = 0;

	/* We may have found a BOOTP server */
	if (get_option_uint8(&type, dhcp, DHO_MESSAGETYPE) == -1)
		type = 0;

	if (type == DHCP_NAK) {
		/* For NAK, only check if we require the ServerID */
		if (has_option_mask(ifo->requiremask, DHO_SERVERID) &&
		    get_option_addr(&addr, dhcp, DHO_SERVERID) == -1)
		{
			log_dhcp(LOG_WARNING, "reject NAK", iface, dhcp, from);
			return;
		}
		/* We should restart on a NAK */
		log_dhcp(LOG_WARNING, "NAK:", iface, dhcp, from);
		if (!(options & DHCPCD_TEST)) {
			dhcp_drop(iface, "NAK");
			unlink(state->leasefile);
		}
		dhcp_close(iface);
		/* If we constantly get NAKS then we should slowly back off */
		eloop_timeout_add_sec(state->nakoff, start_interface, iface);
		if (state->nakoff == 0)
			state->nakoff = 1;
		else {
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
		}
		return;
	}

	/* Ensure that all required options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(ifo->requiremask, i) &&
		    get_option_uint8(&tmp, dhcp, i) != 0)
		{
			/* If we are bootp, then ignore the need for serverid.
			 * To ignore bootp, require dhcp_message_type. */
			if (type == 0 && i == DHO_SERVERID)
				continue;
			log_dhcp(LOG_WARNING, "reject DHCP", iface, dhcp, from);
			return;
		}
	}

	/* Ensure that the address offered is valid */
	if ((type == 0 || type == DHCP_OFFER || type == DHCP_ACK) &&
	    (dhcp->ciaddr == INADDR_ANY || dhcp->ciaddr == INADDR_BROADCAST) &&
	    (dhcp->yiaddr == INADDR_ANY || dhcp->yiaddr == INADDR_BROADCAST))
	{
		log_dhcp(LOG_WARNING, "reject invalid address",
		    iface, dhcp, from);
		return;
	}

	/* No NAK, so reset the backoff */
	state->nakoff = 0;

	if ((type == 0 || type == DHCP_OFFER) &&
	    state->state == DHS_DISCOVER)
	{
		lease->frominfo = 0;
		lease->addr.s_addr = dhcp->yiaddr;
		lease->cookie = dhcp->cookie;
		if (type == 0 ||
		    get_option_addr(&lease->server, dhcp, DHO_SERVERID) != 0)
			lease->server.s_addr = INADDR_ANY;
		log_dhcp(LOG_INFO, "offered", iface, dhcp, from);
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
		if (options & DHCPCD_TEST) {
			free(state->old);
			state->old = state->new;
			state->new = state->offer;
			state->offer = NULL;
			state->reason = "TEST";
			script_runreason(iface, state->reason);
			exit(EXIT_SUCCESS);
		}
		eloop_timeout_delete(send_discover, iface);
		/* We don't request BOOTP addresses */
		if (type) {
			/* We used to ARP check here, but that seems to be in
			 * violation of RFC2131 where it only describes
			 * DECLINE after REQUEST.
			 * It also seems that some MS DHCP servers actually
			 * ignore DECLINE if no REQUEST, ie we decline a
			 * DISCOVER. */
			dhcp_request(iface);
			return;
		}
	}

	if (type) {
		if (type == DHCP_OFFER) {
			log_dhcp(LOG_WARNING, "ignoring offer of",
			    iface, dhcp, from);
			return;
		}

		/* We should only be dealing with acks */
		if (type != DHCP_ACK) {
			log_dhcp(LOG_ERR, "not ACK or OFFER",
			    iface, dhcp, from);
			return;
		}

		if (!(ifo->options & DHCPCD_INFORM))
			log_dhcp(LOG_DEBUG, "acknowledged", iface, dhcp, from);
	}

	/* BOOTP could have already assigned this above, so check we still
	 * have a pointer. */
	if (*dhcpp) {
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
	}

	lease->frominfo = 0;
	eloop_timeout_delete(NULL, iface);

	/* We now have an offer, so close the DHCP sockets.
	 * This allows us to safely ARP when broken DHCP servers send an ACK
	 * follows by an invalid NAK. */
	dhcp_close(iface);

	if (ifo->options & DHCPCD_ARP &&
	    state->addr.s_addr != state->offer->yiaddr)
	{
		/* If the interface already has the address configured
		 * then we can't ARP for duplicate detection. */
		addr.s_addr = state->offer->yiaddr;
		if (ipv4_hasaddress(iface->name, &addr, NULL) != 1) {
			state->claims = 0;
			state->probes = 0;
			state->conflicts = 0;
			state->state = DHS_PROBE;
			arp_probe(iface);
			return;
		}
	}

	dhcp_bind(iface);
}

static ssize_t
get_udp_data(const uint8_t **data, const uint8_t *udp)
{
	struct udp_dhcp_packet p;

	memcpy(&p, udp, sizeof(p));
	*data = udp + offsetof(struct udp_dhcp_packet, dhcp);
	return ntohs(p.ip.ip_len) - sizeof(p.ip) - sizeof(p.udp);
}

static int
valid_udp_packet(const uint8_t *data, size_t data_len, struct in_addr *from,
    int noudpcsum)
{
	struct udp_dhcp_packet p;
	uint16_t bytes, udpsum;

	if (data_len < sizeof(p.ip)) {
		if (from)
			from->s_addr = INADDR_ANY;
		errno = EINVAL;
		return -1;
	}
	memcpy(&p, data, MIN(data_len, sizeof(p)));
	if (from)
		from->s_addr = p.ip.ip_src.s_addr;
	if (data_len > sizeof(p)) {
		errno = EINVAL;
		return -1;
	}
	if (checksum(&p.ip, sizeof(p.ip)) != 0) {
		errno = EINVAL;
		return -1;
	}

	bytes = ntohs(p.ip.ip_len);
	if (data_len < bytes) {
		errno = EINVAL;
		return -1;
	}

	if (noudpcsum == 0) {
		udpsum = p.udp.uh_sum;
		p.udp.uh_sum = 0;
		p.ip.ip_hl = 0;
		p.ip.ip_v = 0;
		p.ip.ip_tos = 0;
		p.ip.ip_len = p.udp.uh_ulen;
		p.ip.ip_id = 0;
		p.ip.ip_off = 0;
		p.ip.ip_ttl = 0;
		p.ip.ip_sum = 0;
		if (udpsum && checksum(&p, bytes) != udpsum) {
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

static void
dhcp_handlepacket(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_message *dhcp = NULL;
	const uint8_t *pp;
	ssize_t bytes;
	struct in_addr from;
	int i, partialcsum = 0;
	const struct dhcp_state *state = D_CSTATE(iface);

	/* We loop through until our buffer is empty.
	 * The benefit is that if we get >1 DHCP packet in our buffer and
	 * the first one fails for any reason, we can use the next. */
	if (packet == NULL) {
		packet = malloc(udp_dhcp_len);
		if (packet == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return;
		}
	}

	for(;;) {
		bytes = ipv4_getrawpacket(iface, ETHERTYPE_IP,
		    packet, udp_dhcp_len, &partialcsum);
		if (bytes == 0 || bytes == -1)
			break;
		if (valid_udp_packet(packet, bytes, &from, partialcsum) == -1) {
			syslog(LOG_ERR, "%s: invalid UDP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		i = whitelisted_ip(iface->options, from.s_addr);
		if (i == 0) {
			syslog(LOG_WARNING,
			    "%s: non whitelisted DHCP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		} else if (i != 1 &&
		    blacklisted_ip(iface->options, from.s_addr) == 1)
		{
			syslog(LOG_WARNING,
			    "%s: blacklisted DHCP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		if (iface->flags & IFF_POINTOPOINT &&
		    state->dst.s_addr != from.s_addr)
		{
			syslog(LOG_WARNING,
			    "%s: server %s is not destination",
			    iface->name, inet_ntoa(from));
		}
		bytes = get_udp_data(&pp, packet);
		if ((size_t)bytes > sizeof(*dhcp)) {
			syslog(LOG_ERR,
			    "%s: packet greater than DHCP size from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		if (dhcp == NULL) {
		        dhcp = calloc(1, sizeof(*dhcp));
			if (dhcp == NULL) {
				syslog(LOG_ERR, "%s: calloc: %m", __func__);
				break;
			}
		}
		memcpy(dhcp, pp, bytes);
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			syslog(LOG_DEBUG, "%s: bogus cookie from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		/* Ensure it's the right transaction */
		if (state->xid != ntohl(dhcp->xid)) {
			syslog(LOG_DEBUG,
			    "%s: wrong xid 0x%x (expecting 0x%x) from %s",
			    iface->name, ntohl(dhcp->xid), state->xid,
			    inet_ntoa(from));
			continue;
		}
		/* Ensure packet is for us */
		if (iface->hwlen <= sizeof(dhcp->chaddr) &&
		    memcmp(dhcp->chaddr, iface->hwaddr, iface->hwlen))
		{
			syslog(LOG_DEBUG, "%s: xid 0x%x is not for hwaddr %s",
			    iface->name, ntohl(dhcp->xid),
			    hwaddr_ntoa(dhcp->chaddr, sizeof(dhcp->chaddr)));
			continue;
		}
		dhcp_handle(iface, &dhcp, &from);
		if (state->raw_fd == -1)
			break;
	}
	free(packet);
	packet = NULL;
	free(dhcp);
}

static int
dhcp_open(struct interface *ifp)
{
	int r = 0;
	struct dhcp_state *state;

	state = D_STATE(ifp);
	if (state->raw_fd == -1) {
		if ((r = ipv4_opensocket(ifp, ETHERTYPE_IP)) == -1) {
			syslog(LOG_ERR, "%s: %s: %m", __func__, ifp->name);
			return -1;
		}
		eloop_event_add(state->raw_fd, dhcp_handlepacket, ifp);
	}
	if (state->udp_fd == -1 &&
	    state->addr.s_addr != 0 &&
	    state->new != NULL &&
	    (state->new->cookie == htonl(MAGIC_COOKIE) ||
	    ifp->options->options & DHCPCD_INFORM))
	{
		if (dhcp_openudp(ifp) == -1 && errno != EADDRINUSE) {
			syslog(LOG_ERR, "%s: dhcp_openudp: %m", ifp->name);
			return -1;
		}
	}
	return 0;
}

int
dhcp_dump(const char *ifname)
{
	struct interface *ifp;
	struct dhcp_state *state;

	ifaces = malloc(sizeof(*ifaces));
	if (ifaces == NULL)
		goto eexit;
	TAILQ_INIT(ifaces);
	ifp = calloc(1, sizeof(*ifp));
	if (ifp == NULL)
		goto eexit;
	TAILQ_INSERT_HEAD(ifaces, ifp, next);
	ifp->if_data[IF_DATA_DHCP] = state = calloc(1, sizeof(*state));
	if (state == NULL)
		goto eexit;
	ifp->options = calloc(1, sizeof(*ifp->options));
	if (ifp->options == NULL)
		goto eexit;
	strlcpy(ifp->name, ifname, sizeof(ifp->name));
	snprintf(state->leasefile, sizeof(state->leasefile),
	    LEASEFILE, ifp->name);
	strlcpy(ifp->options->script, if_options->script,
	    sizeof(ifp->options->script));
	state->new = read_lease(ifp);
	if (state->new == NULL && errno == ENOENT) {
		strlcpy(state->leasefile, ifname, sizeof(state->leasefile));
		state->new = read_lease(ifp);
	}
	if (state->new == NULL) {
		if (errno == ENOENT)
			syslog(LOG_ERR, "%s: no lease to dump", ifname);
		return -1;
	}
	state->reason = "DUMP";
	return script_runreason(ifp, state->reason);

eexit:
	syslog(LOG_ERR, "%s: %m", __func__);
	return -1;
}

void
dhcp_free(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state) {
		free(state->old);
		free(state->new);
		free(state->offer);
		free(state->buffer);
		free(state->clientid);
		free(state);
		ifp->if_data[IF_DATA_DHCP] = NULL;
	}
}

static int
dhcp_init(struct interface *ifp)
{
	struct dhcp_state *state;
	const struct if_options *ifo;
	unsigned char *duid;
	size_t len, ifl;

	state = D_STATE(ifp);
	if (state == NULL) {
		ifp->if_data[IF_DATA_DHCP] = calloc(1, sizeof(*state));
		state = D_STATE(ifp);
		if (state == NULL)
			return -1;
		/* 0 is a valid fd, so init to -1 */
		state->raw_fd = state->udp_fd = state->arp_fd = -1;
	}

	state->state = DHS_INIT;
	state->reason = "PREINIT";
	state->nakoff = 0;
	snprintf(state->leasefile, sizeof(state->leasefile),
	    LEASEFILE, ifp->name);

	ifo = ifp->options;
	/* We need to drop the leasefile so that start_interface
	 * doesn't load it. */
	if (ifo->options & DHCPCD_REQUEST)
		unlink(state->leasefile);

	free(state->clientid);
	state->clientid = NULL;

	if (*ifo->clientid) {
		state->clientid = malloc(ifo->clientid[0] + 1);
		if (state->clientid == NULL)
			goto eexit;
		memcpy(state->clientid, ifo->clientid, ifo->clientid[0] + 1);
	} else if (ifo->options & DHCPCD_CLIENTID) {
		len = 0;
		if (ifo->options & DHCPCD_DUID) {
			duid = malloc(DUID_LEN);
			if (duid == NULL)
				goto eexit;
			if ((len = get_duid(duid, ifp)) == 0)
				syslog(LOG_ERR, "get_duid: %m");
		} else
			duid = NULL;
		if (len > 0) {
			state->clientid = malloc(len + 6);
			if (state->clientid == NULL)
				goto eexit;
			state->clientid[0] = len + 5;
			state->clientid[1] = 255; /* RFC 4361 */
			ifl = strlen(ifp->name);
			if (ifl < 5) {
				memcpy(state->clientid + 2, ifp->name, ifl);
				if (ifl < 4)
					memset(state->clientid + 2 + ifl,
					    0, 4 - ifl);
			} else {
				ifl = htonl(ifp->index);
				memcpy(state->clientid + 2, &ifl, 4);
			}
			memcpy(state->clientid + 6, duid, len);
		} else if (len == 0) {
			len = ifp->hwlen + 1;
			state->clientid = malloc(len + 1);
			if (state->clientid == NULL)
				goto eexit;
			state->clientid[0] = len;
			state->clientid[1] = ifp->family;
			memcpy(state->clientid + 2, ifp->hwaddr,
			    ifp->hwlen);
		}
		free(duid);
	}
	if (ifo->options & DHCPCD_CLIENTID)
		syslog(LOG_DEBUG, "%s: using ClientID %s", ifp->name,
		    hwaddr_ntoa(state->clientid + 1, state->clientid[0]));
	else if (ifp->hwlen)
		syslog(LOG_DEBUG, "%s: using hwaddr %s", ifp->name,
		    hwaddr_ntoa(ifp->hwaddr, ifp->hwlen));
	return 0;

eexit:
	syslog(LOG_ERR, "%s: Error making ClientID: %m", __func__);
	return -1;
}

void
dhcp_start(struct interface *ifp)
{
	struct if_options *ifo = ifp->options;
	struct dhcp_state *state;
	struct stat st;
	struct timeval now;
	uint32_t l;
	int nolease;

	if (!(ifo->options & DHCPCD_IPV4))
		return;

	if (dhcp_init(ifp) == -1) {
		syslog(LOG_ERR, "%s: dhcp_init: %m", ifp->name);
		return;
	}

	/* Close any pre-existing sockets as we're starting over */
	dhcp_close(ifp);

	state = D_STATE(ifp);
	state->start_uptime = uptime();
	free(state->offer);
	state->offer = NULL;

	if (state->arping_index < ifo->arping_len) {
		arp_start(ifp);
		return;
	}

	if (ifo->options & DHCPCD_STATIC) {
		dhcp_static(ifp);
		return;
	}

	if (dhcp_open(ifp) == -1)
		return;

	if (ifo->options & DHCPCD_INFORM) {
		dhcp_inform(ifp);
		return;
	}
	if (ifp->hwlen == 0 && ifo->clientid[0] == '\0') {
		syslog(LOG_WARNING, "%s: needs a clientid to configure",
		    ifp->name);
		dhcp_drop(ifp, "FAIL");
		dhcp_close(ifp);
		eloop_timeout_delete(NULL, ifp);
		return;
	}
	/* We don't want to read the old lease if we NAK an old test */
	nolease = state->offer && options & DHCPCD_TEST;
	if (!nolease)
		state->offer = read_lease(ifp);
	if (state->offer) {
		get_lease(&state->lease, state->offer);
		state->lease.frominfo = 1;
		if (state->offer->cookie == 0) {
			if (state->offer->yiaddr == state->addr.s_addr) {
				free(state->offer);
				state->offer = NULL;
			}
		} else if (state->lease.leasetime != ~0U &&
		    stat(state->leasefile, &st) == 0)
		{
			/* Offset lease times and check expiry */
			gettimeofday(&now, NULL);
			if ((time_t)state->lease.leasetime <
			    now.tv_sec - st.st_mtime)
			{
				syslog(LOG_DEBUG,
				    "%s: discarding expired lease",
				    ifp->name);
				free(state->offer);
				state->offer = NULL;
				state->lease.addr.s_addr = 0;
			} else {
				l = now.tv_sec - st.st_mtime;
				state->lease.leasetime -= l;
				state->lease.renewaltime -= l;
				state->lease.rebindtime -= l;
			}
		}
	}
	if (state->offer == NULL)
		dhcp_discover(ifp);
	else if (state->offer->cookie == 0 &&
	    ifp->options->options & DHCPCD_IPV4LL)
		ipv4ll_start(ifp);
	else
		dhcp_reboot(ifp);
}
