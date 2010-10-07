/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2010 Roy Marples <roy@marples.name>
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"

#define REQUEST	(1 << 0)
#define UINT8	(1 << 1)
#define UINT16	(1 << 2)
#define SINT16	(1 << 3)
#define UINT32	(1 << 4)
#define SINT32	(1 << 5)
#define IPV4	(1 << 6)
#define STRING	(1 << 7)
#define PAIR	(1 << 8)
#define ARRAY	(1 << 9)
#define RFC3361	(1 << 10)
#define RFC3397	(1 << 11)
#define RFC3442 (1 << 12)

#define IPV4R	IPV4 | REQUEST

#define DAD	"Duplicate address detected"

/* Our aggregate option buffer.
 * We ONLY use this when options are split, which for most purposes is
 * practically never. See RFC3396 for details. */
static uint8_t *opt_buffer;

struct dhcp_opt {
	uint8_t option;
	int type;
	const char *var;
};

static const struct dhcp_opt const dhcp_opts[] = {
	{ 1,	IPV4 | REQUEST,	"subnet_mask" },
		/* RFC 3442 states that the CSR has to come before all other
		 * routes. For completeness, we also specify static routes,
	 	 * then routers. */
	{ 121,  RFC3442,	"classless_static_routes" },
	{ 249,  RFC3442,	"ms_classless_static_routes" },
	{ 33,	IPV4 | ARRAY | REQUEST,	"static_routes" },
	{ 3,	IPV4 | ARRAY | REQUEST,	"routers" },
	{ 2,	UINT32,		"time_offset" },
	{ 4,	IPV4 | ARRAY,	"time_servers" },
	{ 5,	IPV4 | ARRAY,	"ien116_name_servers" },
	{ 6,	IPV4 | ARRAY,	"domain_name_servers" },
	{ 7,	IPV4 | ARRAY,	"log_servers" },
	{ 8,	IPV4 | ARRAY,	"cookie_servers" },
	{ 9, 	IPV4 | ARRAY,	"lpr_servers" },
	{ 10,	IPV4 | ARRAY,	"impress_servers" },
	{ 11,	IPV4 | ARRAY,	"resource_location_servers" },
	{ 12,	STRING,		"host_name" },
	{ 13,	UINT16,		"boot_size" },
	{ 14,	STRING,		"merit_dump" },
	{ 15,	STRING,		"domain_name" },
	{ 16,	IPV4,		"swap_server" },
	{ 17,	STRING,		"root_path" },
	{ 18,	STRING,		"extensions_path" },
	{ 19,	UINT8,		"ip_forwarding" },
	{ 20,	UINT8,		"non_local_source_routing" },
	{ 21,	IPV4 | ARRAY,	"policy_filter" },
	{ 22,	SINT16,		"max_dgram_reassembly" },
	{ 23,	UINT16,		"default_ip_ttl" },
	{ 24,	UINT32,		"path_mtu_aging_timeout" },
	{ 25,	UINT16 | ARRAY,	"path_mtu_plateau_table" },
	{ 26,	UINT16,		"interface_mtu" },
	{ 27,	UINT8,		"all_subnets_local" },
	{ 28,	IPV4 | REQUEST,	"broadcast_address" },
	{ 29,	UINT8,		"perform_mask_discovery" },
	{ 30,	UINT8,		"mask_supplier" },
	{ 31,	UINT8,		"router_discovery" },
	{ 32,	IPV4,		"router_solicitation_address" },
	{ 34,	UINT8,		"trailer_encapsulation" },
	{ 35, 	UINT32,		"arp_cache_timeout" },
	{ 36,	UINT16,		"ieee802_3_encapsulation" },
	{ 37,	UINT8,		"default_tcp_ttl" },
	{ 38,	UINT32,		"tcp_keepalive_interval" },
	{ 39,	UINT8,		"tcp_keepalive_garbage" },
	{ 40,	STRING,		"nis_domain" },
	{ 41,	IPV4 | ARRAY,	"nis_servers" },
	{ 42,	IPV4 | ARRAY,	"ntp_servers" },
	{ 43,	STRING,		"vendor_encapsulated_options" },
	{ 44,	IPV4 | ARRAY,	"netbios_name_servers" },
	{ 45,	IPV4,		"netbios_dd_server" },
	{ 46,	UINT8,		"netbios_node_type" },
	{ 47,	STRING,		"netbios_scope" },
	{ 48,	IPV4 | ARRAY,	"font_servers" },
	{ 49,	IPV4 | ARRAY,	"x_display_manager" },
	{ 50, 	IPV4,		"dhcp_requested_address" },
	{ 51,	UINT32 | REQUEST,	"dhcp_lease_time" },
	{ 52,	UINT8,		"dhcp_option_overload" },
	{ 53,	UINT8,		"dhcp_message_type" },
	{ 54,	IPV4,		"dhcp_server_identifier" },
	{ 55,	UINT8 | ARRAY,	"dhcp_parameter_request_list" },
	{ 56,	STRING,		"dhcp_message" },
	{ 57,	UINT16,		"dhcp_max_message_size" },
	{ 58,	UINT32 | REQUEST,	"dhcp_renewal_time" },
	{ 59,	UINT32 | REQUEST,	"dhcp_rebinding_time" },
	{ 64,	STRING,		"nisplus_domain" },
	{ 65,	IPV4 | ARRAY,	"nisplus_servers" },
	{ 66,	STRING,		"tftp_server_name" },
	{ 67,	STRING,		"bootfile_name" },
	{ 68,	IPV4 | ARRAY,	"mobile_ip_home_agent" },
	{ 69,	IPV4 | ARRAY,	"smtp_server" },
	{ 70,	IPV4 | ARRAY,	"pop_server" },
	{ 71,	IPV4 | ARRAY,	"nntp_server" },
	{ 72,	IPV4 | ARRAY,	"www_server" },
	{ 73,	IPV4 | ARRAY,	"finger_server" },
	{ 74,	IPV4 | ARRAY,	"irc_server" },
	{ 75,	IPV4 | ARRAY,	"streettalk_server" },
	{ 76,	IPV4 | ARRAY,	"streettalk_directory_assistance_server" },
	{ 77,	STRING,		"user_class" },
	{ 81,	STRING | RFC3397,	"fqdn_name" },
	{ 85,	IPV4 | ARRAY,	"nds_servers" },
	{ 86,	STRING,		"nds_tree_name" },
	{ 87,	STRING,		"nds_context" },
	{ 88,	STRING | RFC3397,	"bcms_controller_names" },
	{ 89,	IPV4 | ARRAY,	"bcms_controller_address" },
	{ 91,	UINT32,		"client_last_transaction_time" },
	{ 92,	IPV4 | ARRAY,	"associated_ip" },
	{ 98,	STRING,		"uap_servers" },
	{ 112,	IPV4 | ARRAY,	"netinfo_server_address" },
	{ 113,	STRING,		"netinfo_server_tag" },
	{ 114,	STRING,		"default_url" },
	{ 118,	IPV4,		"subnet_selection" },
	{ 119,	STRING | RFC3397,	"domain_search" },
	{ 0, 0, NULL }
};

static const char *if_params[] = {
	"interface",
	"reason",
	"pid",
	"ifmetric",
	"ifwireless",
	"ifflags",
	"profile",
	"interface_order",
	NULL
};

static const char *dhcp_params[] = {
	"ip_address",
	"subnet_cidr",
	"network_number",
	"ssid",
	"filename",
	"server_name",
	NULL
};

void
print_options(void)
{
	const struct dhcp_opt *opt;
	const char **p;

	for (p = if_params; *p; p++)
		printf(" -  %s\n", *p);

	for (p = dhcp_params; *p; p++)
		printf("    %s\n", *p);

	for (opt = dhcp_opts; opt->option; opt++)
		if (opt->var)
			printf("%03d %s\n", opt->option, opt->var);
}

int make_option_mask(uint8_t *mask, const char *opts, int add)
{
	char *token, *o, *p, *t;
	const struct dhcp_opt *opt;
	int match, n;

	o = p = xstrdup(opts);
	while ((token = strsep(&p, ", "))) {
		if (*token == '\0')
			continue;
		for (opt = dhcp_opts; opt->option; opt++) {
			if (!opt->var)
				continue;
			match = 0;
			if (strcmp(opt->var, token) == 0)
				match = 1;
			else {
				errno = 0;
				n = strtol(token, &t, 0);
				if (errno == 0 && !*t)
					if (opt->option == n)
						match = 1;
			}
			if (match) {
				if (add == 2 && !(opt->type & IPV4)) {
					free(o);
					errno = EINVAL;
					return -1;
				}
				if (add == 1 || add == 2)
					add_option_mask(mask,
					    opt->option);
				else
					del_option_mask(mask,
					    opt->option);
				break;
			}
		}
		if (!opt->option) {
			free(o);
			errno = ENOENT;
			return -1;
		}
	}
	free(o);
	return 0;
}

static int
valid_length(uint8_t option, int dl, int *type)
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
		    opt->type & STRING ||
		    opt->type & RFC3442)
			return 0;

		sz = 0;
		if (opt->type & UINT32 || opt->type & IPV4)
			sz = sizeof(uint32_t);
		if (opt->type & UINT16)
			sz = sizeof(uint16_t);
		if (opt->type & UINT8)
			sz = sizeof(uint8_t);
		if (opt->type & IPV4 || opt->type & ARRAY)
			return dl % sz;
		return (dl == sz ? 0 : -1);
	}

	/* unknown option, so let it pass */
	return 0;
}

#ifdef DEBUG_MEMORY
static void
free_option_buffer(void)
{
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
	int bl = 0;

	while (p < e) {
		o = *p++;
		if (o == opt) {
			if (op) {
				if (!opt_buffer) {
					opt_buffer = xmalloc(sizeof(*dhcp));
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
	if (valid_length(opt, bl, type) == -1) {
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

/* Decode an RFC3397 DNS search order option into a space
 * separated string. Returns length of string (including
 * terminating zero) or zero on error. out may be NULL
 * to just determine output length. */
static ssize_t
decode_rfc3397(char *out, ssize_t len, int pl, const uint8_t *p)
{
	const uint8_t *r, *q = p;
	int count = 0, l, hops;
	uint8_t ltype;

	while (q - p < pl) {
		r = NULL;
		hops = 0;
		/* We check we are inside our length again incase
		 * the data is NOT terminated correctly. */
		while ((l = *q++) && q - p < pl) {
			ltype = l & 0xc0;
			if (ltype == 0x80 || ltype == 0x40)
				return 0;
			else if (ltype == 0xc0) { /* pointer */
				l = (l & 0x3f) << 8;
				l |= *q++;
				/* save source of first jump. */
				if (!r)
					r = q;
				hops++;
				if (hops > 255)
					return 0;
				q = p + l;
				if (q - p >= pl)
					return 0;
			} else {
				/* straightforward name segment, add with '.' */
				count += l + 1;
				if (out) {
					if ((ssize_t)l + 1 > len) {
						errno = ENOBUFS;
						return -1;
					}
					memcpy(out, q, l);
					out += l;
					*out++ = '.';
					len -= l;
					len--;
				}
				q += l;
			}
		}
		/* change last dot to space */
		if (out)
			*(out - 1) = ' ';
		if (r)
			q = r;
	}

	/* change last space to zero terminator */
	if (out)
		*(out - 1) = 0;

	return count;  
}

static ssize_t
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

static struct rt *
decode_rfc3442_rt(int dl, const uint8_t *data)
{
	const uint8_t *p = data;
	const uint8_t *e;
	uint8_t cidr;
	size_t ocets;
	struct rt *routes = NULL;
	struct rt *rt = NULL;

	/* Minimum is 5 -first is CIDR and a router length of 4 */
	if (dl < 5)
		return NULL;

	e = p + dl;
	while (p < e) {
		cidr = *p++;
		if (cidr > 32) {
			free_routes(routes);
			errno = EINVAL;
			return NULL;
		}

		if (rt) {
			rt->next = xzalloc(sizeof(*rt));
			rt = rt->next;
		} else {
			routes = rt = xzalloc(sizeof(*routes));
		}
		rt->next = NULL;

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

static char *
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
			sip = xmalloc(l);
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
		sip = p = xmalloc(l);
		while (l != 0) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			data += sizeof(addr.s_addr);
			p += snprintf(p, l - (p - sip), "%s ", inet_ntoa(addr));
			l -= sizeof(addr.s_addr);
		}
		*--p = '\0';
		break;
	default:
		errno = EINVAL;
		return 0;
	}

	return sip;
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
		s = xmalloc(sizeof(char) * type);
		decode_rfc3397(s, type, len, p);
		return s;
	}

	if (type & RFC3361)
		return decode_rfc3361(len, p);

	s = xmalloc(sizeof(char) * (len + 1));
	memcpy(s, p, len);
	s[len] = '\0';
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
struct rt *
get_option_routes(const struct dhcp_message *dhcp,
    const char *ifname, int *opts)
{
	const uint8_t *p;
	const uint8_t *e;
	struct rt *routes = NULL;
	struct rt *route = NULL;
	int len;

	/* If we have CSR's then we MUST use these only */
	p = get_option(dhcp, DHO_CSR, &len, NULL);
	/* Check for crappy MS option */
	if (!p)
		p = get_option(dhcp, DHO_MSCSR, &len, NULL);
	if (p) {
		routes = decode_rfc3442_rt(len, p);
		if (routes && !(*opts & DHCPCD_CSR_WARNED)) {
			syslog(LOG_DEBUG,
			    "%s: using Classless Static Routes (RFC3442)",
			    ifname);
			*opts |= DHCPCD_CSR_WARNED;
			return routes;
		}
	}

	/* OK, get our static routes first. */
	p = get_option(dhcp, DHO_STATICROUTE, &len, NULL);
	if (p) {
		e = p + len;
		while (p < e) {
			if (route) {
				route->next = xmalloc(sizeof(*route));
				route = route->next;
			} else
				routes = route = xmalloc(sizeof(*routes));
			route->next = NULL;
			memcpy(&route->dest.s_addr, p, 4);
			p += 4;
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
			route->net.s_addr = route_netmask(route->dest.s_addr);
		}
	}

	/* Now grab our routers */
	p = get_option(dhcp, DHO_ROUTER, &len, NULL);
	if (p) {
		e = p + len;
		while (p < e) {
			if (route) {
				route->next = xzalloc(sizeof(*route));
				route = route->next;
			} else
				routes = route = xzalloc(sizeof(*route));
			memcpy(&route->gate.s_addr, p, 4);
			p += 4;
		}
	}

	return routes;
}

static size_t
encode_rfc1035(const char *src, uint8_t *dst)
{
	uint8_t *p = dst;
	uint8_t *lp = p++;

	if (*src == '\0')
		return 0;
	for (; *src; src++) {
		if (*src == '\0')
			break;
		if (*src == '.') {
			/* Skip the trailing . */
			if (src[1] == '\0')
				break;
			*lp = p - lp - 1;
			if (*lp == '\0')
				return p - dst;
			lp = p++;
		} else
			*p++ = (uint8_t)*src;
	}
	*lp = p - lp - 1;
	*p++ = '\0';
	return p - dst;
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
	time_t up = uptime() - iface->start_uptime;
	uint32_t ul;
	uint16_t sz;
	size_t len;
	const char *hp;
	const struct dhcp_opt *opt;
	const struct if_options *ifo = iface->state->options;
	const struct dhcp_lease *lease = &iface->state->lease;

	dhcp = xzalloc(sizeof (*dhcp));
	m = (uint8_t *)dhcp;
	p = dhcp->options;

	if ((type == DHCP_INFORM || type == DHCP_RELEASE ||
		(type == DHCP_REQUEST &&
		    iface->net.s_addr == lease->net.s_addr &&
		    (iface->state->new == NULL ||
			iface->state->new->cookie == htonl(MAGIC_COOKIE)))))
	{
		dhcp->ciaddr = iface->addr.s_addr;
		/* In-case we haven't actually configured the address yet */
		if (type == DHCP_INFORM && iface->addr.s_addr == 0)
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
	dhcp->xid = iface->state->xid;
	dhcp->cookie = htonl(MAGIC_COOKIE);

	*p++ = DHO_MESSAGETYPE; 
	*p++ = 1;
	*p++ = type;

	if (iface->clientid) {
		*p++ = DHO_CLIENTID;
		memcpy(p, iface->clientid, iface->clientid[0] + 1);
		p += iface->clientid[0] + 1;
	}

	if (lease->addr.s_addr && lease->cookie == htonl(MAGIC_COOKIE)) {
		if (type == DHCP_DECLINE ||
		    type == DHCP_DISCOVER ||
		    (type == DHCP_REQUEST &&
			lease->addr.s_addr != iface->addr.s_addr))
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
		if (ifo->options & DHCPCD_HOSTNAME && ifo->hostname[0]) {
			*p++ = DHO_HOSTNAME;
			hp = strchr(ifo->hostname, '.');
			if (hp)
				len = hp - ifo->hostname;
			else
				len = strlen(ifo->hostname);
			*p++ = len;
			memcpy(p, ifo->hostname, len);
			p += len;
		}
		if (ifo->fqdn != FQDN_DISABLE && ifo->hostname[0]) {
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
			*p++ = (ifo->fqdn & 0x09) | 0x04;
			*p++ = 0; /* from server for PTR RR */
			*p++ = 0; /* from server for A RR if S=1 */
			ul = encode_rfc1035(ifo->hostname, p);
			*lp += ul;
			p += ul;
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
write_lease(const struct interface *iface, const struct dhcp_message *dhcp)
{
	int fd;
	ssize_t bytes = sizeof(*dhcp);
	const uint8_t *p = dhcp->options;
	const uint8_t *e = p + sizeof(dhcp->options);
	uint8_t l;
	uint8_t o = 0;

	/* We don't write BOOTP leases */
	if (is_bootp(dhcp)) {
		unlink(iface->leasefile);
		return 0;
	}

	syslog(LOG_DEBUG, "%s: writing lease `%s'",
	    iface->name, iface->leasefile);

	fd = open(iface->leasefile, O_WRONLY | O_CREAT | O_TRUNC, 0444);
	if (fd == -1) {
		syslog(LOG_ERR, "%s: open: %m", iface->name);
		return -1;
	}

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
read_lease(const struct interface *iface)
{
	int fd;
	struct dhcp_message *dhcp;
	ssize_t bytes;

	fd = open(iface->leasefile, O_RDONLY);
	if (fd == -1) {
		if (errno != ENOENT)
			syslog(LOG_ERR, "%s: open `%s': %m",
			    iface->name, iface->leasefile);
		return NULL;
	}
	syslog(LOG_DEBUG, "%s: reading lease `%s'",
	    iface->name, iface->leasefile);
	dhcp = xmalloc(sizeof(*dhcp));
	memset(dhcp, 0, sizeof(*dhcp));
	bytes = read(fd, dhcp, sizeof(*dhcp));
	close(fd);
	if (bytes < 0) {
		free(dhcp);
		dhcp = NULL;
	}
	return dhcp;
}

static ssize_t
print_string(char *s, ssize_t len, int dl, const uint8_t *data)
{
	uint8_t c;
	const uint8_t *e, *p;
	ssize_t bytes = 0;
	ssize_t r;

	e = data + dl;
	while (data < e) {
		c = *data++;
		if (c == '\0') {
			/* If rest is all NULL, skip it. */
			for (p = data; p < e; p++)
				if (*p != '\0')
					break;
			if (p == e)
				break;
		}
		if (!isascii(c) || !isprint(c)) {
			if (s) {
				if (len < 5) {
					errno = ENOBUFS;
					return -1;
				}
				r = snprintf(s, len, "\\%03o", c);
				len -= r;
				bytes += r;
				s += r;
			} else
				bytes += 4;
			continue;
		}
		switch (c) {
		case '"':  /* FALLTHROUGH */
		case '\'': /* FALLTHROUGH */
		case '$':  /* FALLTHROUGH */
		case '`':  /* FALLTHROUGH */
 		case '\\':
			if (s) {
				if (len < 3) {
					errno = ENOBUFS;
					return -1;
				}
				*s++ = '\\';
				len--;
			}
			bytes++;
			break;
		}
		if (s) {
			*s++ = c;
			len--;
		}
		bytes++;
	}

	/* NULL */
	if (s)
		*s = '\0';
	bytes++;
	return bytes;
}

static ssize_t
print_option(char *s, ssize_t len, int type, int dl, const uint8_t *data)
{
	const uint8_t *e, *t;
	uint16_t u16;
	int16_t s16;
	uint32_t u32;
	int32_t s32;
	struct in_addr addr;
	ssize_t bytes = 0;
	ssize_t l;
	char *tmp;

	if (type & RFC3397) {
		l = decode_rfc3397(NULL, 0, dl, data);
		if (l < 1)
			return l;
		tmp = xmalloc(l);
		decode_rfc3397(tmp, l, dl, data);
		l = print_string(s, len, l - 1, (uint8_t *)tmp);
		free(tmp);
		return l;
	}

	if (type & RFC3442)
		return decode_rfc3442(s, len, dl, data);

	if (type & STRING) {
		/* Some DHCP servers return NULL strings */
		if (*data == '\0')
			return 0;
		return print_string(s, len, dl, data);
	}

	if (!s) {
		if (type & UINT8)
			l = 3;
		else if (type & UINT16) {
			l = 5;
			dl /= 2;
		} else if (type & SINT16) {
			l = 6;
			dl /= 2;
		} else if (type & UINT32) {
			l = 10;
			dl /= 4;
		} else if (type & SINT32) {
			l = 11;
			dl /= 4;
		} else if (type & IPV4) {
			l = 16;
			dl /= 4;
		} else {
			errno = EINVAL;
			return -1;
		}
		return (l + 1) * dl;
	}

	t = data;
	e = data + dl;
	while (data < e) {
		if (data != t) {
			*s++ = ' ';
			bytes++;
			len--;
		}
		if (type & UINT8) {
			l = snprintf(s, len, "%d", *data);
			data++;
		} else if (type & UINT16) {
			memcpy(&u16, data, sizeof(u16));
			u16 = ntohs(u16);
			l = snprintf(s, len, "%d", u16);
			data += sizeof(u16);
		} else if (type & SINT16) {
			memcpy(&s16, data, sizeof(s16));
			s16 = ntohs(s16);
			l = snprintf(s, len, "%d", s16);
			data += sizeof(s16);
		} else if (type & UINT32) {
			memcpy(&u32, data, sizeof(u32));
			u32 = ntohl(u32);
			l = snprintf(s, len, "%d", u32);
			data += sizeof(u32);
		} else if (type & SINT32) {
			memcpy(&s32, data, sizeof(s32));
			s32 = ntohl(s32);
			l = snprintf(s, len, "%d", s32);
			data += sizeof(s32);
		} else if (type & IPV4) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			l = snprintf(s, len, "%s", inet_ntoa(addr));
			data += sizeof(addr.s_addr);
		} else
			l = 0;
		len -= l;
		bytes += l;
		s += l;
	}

	return bytes;
}

static void
setvar(char ***e, const char *prefix, const char *var, const char *value)
{
	size_t len = strlen(prefix) + strlen(var) + strlen(value) + 4;

	**e = xmalloc(len);
	snprintf(**e, len, "%s_%s=%s", prefix, var, value);
	(*e)++;
}

ssize_t
configure_env(char **env, const char *prefix, const struct dhcp_message *dhcp,
    const struct if_options *ifo)
{
	unsigned int i;
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
			net.s_addr = get_netmask(addr.s_addr);
			setvar(&ep, prefix, "subnet_mask", inet_ntoa(net));
		}
		i = inet_ntocidr(net);
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
		len = print_option(NULL, 0, opt->type, pl, p);
		if (len < 0)
			return -1;
		e = strlen(prefix) + strlen(opt->var) + len + 4;
		v = val = *ep++ = xmalloc(e);
		v += snprintf(val, e, "%s_%s=", prefix, opt->var);
		if (len != 0)
			print_option(v, len, opt->type, pl, p);
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
		lease->net.s_addr = get_netmask(lease->addr.s_addr);
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
