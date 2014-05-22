#include <sys/cdefs.h>
 __RCSID("$NetBSD: if-options.c,v 1.1.1.13.4.4 2014/05/22 15:44:40 yamt Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

#define _WITH_GETLINE /* Stop FreeBSD bitching */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "dhcpcd-embedded.h"
#include "if-options.h"
#include "ipv4.h"
#include "platform.h"

/* These options only make sense in the config file, so don't use any
   valid short options for them */
#define O_BASE			MAX('z', 'Z') + 1
#define O_ARPING		O_BASE + 1
#define O_FALLBACK		O_BASE + 2
#define O_DESTINATION		O_BASE + 3
#define O_IPV6RS		O_BASE + 4
#define O_NOIPV6RS		O_BASE + 5
#define O_IPV6RA_FORK		O_BASE + 6
#define O_IPV6RA_OWN		O_BASE + 7
#define O_IPV6RA_OWN_D		O_BASE + 8
#define O_NOALIAS		O_BASE + 9
#define O_IA_NA			O_BASE + 10
#define O_IA_TA			O_BASE + 11
#define O_IA_PD			O_BASE + 12
#define O_HOSTNAME_SHORT	O_BASE + 13
#define O_DEV			O_BASE + 14
#define O_NODEV			O_BASE + 15
#define O_NOIPV4		O_BASE + 16
#define O_NOIPV6		O_BASE + 17
#define O_IAID			O_BASE + 18
#define O_DEFINE		O_BASE + 19
#define O_DEFINE6		O_BASE + 20
#define O_EMBED			O_BASE + 21
#define O_ENCAP			O_BASE + 22
#define O_VENDOPT		O_BASE + 23
#define O_VENDCLASS		O_BASE + 24
#define O_AUTHPROTOCOL		O_BASE + 25
#define O_AUTHTOKEN		O_BASE + 26
#define O_AUTHNOTREQUIRED	O_BASE + 27
#define O_NODHCP		O_BASE + 28
#define O_NODHCP6		O_BASE + 29

const struct option cf_options[] = {
	{"background",      no_argument,       NULL, 'b'},
	{"script",          required_argument, NULL, 'c'},
	{"debug",           no_argument,       NULL, 'd'},
	{"env",             required_argument, NULL, 'e'},
	{"config",          required_argument, NULL, 'f'},
	{"reconfigure",     no_argument,       NULL, 'g'},
	{"hostname",        optional_argument, NULL, 'h'},
	{"vendorclassid",   optional_argument, NULL, 'i'},
	{"release",         no_argument,       NULL, 'k'},
	{"leasetime",       required_argument, NULL, 'l'},
	{"metric",          required_argument, NULL, 'm'},
	{"rebind",          no_argument,       NULL, 'n'},
	{"option",          required_argument, NULL, 'o'},
	{"persistent",      no_argument,       NULL, 'p'},
	{"quiet",           no_argument,       NULL, 'q'},
	{"request",         optional_argument, NULL, 'r'},
	{"inform",          optional_argument, NULL, 's'},
	{"timeout",         required_argument, NULL, 't'},
	{"userclass",       required_argument, NULL, 'u'},
	{"vendor",          required_argument, NULL, 'v'},
	{"waitip",          optional_argument, NULL, 'w'},
	{"exit",            no_argument,       NULL, 'x'},
	{"allowinterfaces", required_argument, NULL, 'z'},
	{"reboot",          required_argument, NULL, 'y'},
	{"noarp",           no_argument,       NULL, 'A'},
	{"nobackground",    no_argument,       NULL, 'B'},
	{"nohook",          required_argument, NULL, 'C'},
	{"duid",            no_argument,       NULL, 'D'},
	{"lastlease",       no_argument,       NULL, 'E'},
	{"fqdn",            optional_argument, NULL, 'F'},
	{"nogateway",       no_argument,       NULL, 'G'},
	{"xidhwaddr",       no_argument,       NULL, 'H'},
	{"clientid",        optional_argument, NULL, 'I'},
	{"broadcast",       no_argument,       NULL, 'J'},
	{"nolink",          no_argument,       NULL, 'K'},
	{"noipv4ll",        no_argument,       NULL, 'L'},
	{"master",          no_argument,       NULL, 'M'},
	{"nooption",        optional_argument, NULL, 'O'},
	{"require",         required_argument, NULL, 'Q'},
	{"static",          required_argument, NULL, 'S'},
	{"test",            no_argument,       NULL, 'T'},
	{"dumplease",       no_argument,       NULL, 'U'},
	{"variables",       no_argument,       NULL, 'V'},
	{"whitelist",       required_argument, NULL, 'W'},
	{"blacklist",       required_argument, NULL, 'X'},
	{"denyinterfaces",  required_argument, NULL, 'Z'},
	{"arping",          required_argument, NULL, O_ARPING},
	{"destination",     required_argument, NULL, O_DESTINATION},
	{"fallback",        required_argument, NULL, O_FALLBACK},
	{"ipv6rs",          no_argument,       NULL, O_IPV6RS},
	{"noipv6rs",        no_argument,       NULL, O_NOIPV6RS},
	{"ipv6ra_fork",     no_argument,       NULL, O_IPV6RA_FORK},
	{"ipv6ra_own",      no_argument,       NULL, O_IPV6RA_OWN},
	{"ipv6ra_own_default", no_argument,    NULL, O_IPV6RA_OWN_D},
	{"ipv4only",        no_argument,       NULL, '4'},
	{"ipv6only",        no_argument,       NULL, '6'},
	{"noipv4",          no_argument,       NULL, O_NOIPV4},
	{"noipv6",          no_argument,       NULL, O_NOIPV6},
	{"noalias",         no_argument,       NULL, O_NOALIAS},
	{"iaid",            required_argument, NULL, O_IAID},
	{"ia_na",           no_argument,       NULL, O_IA_NA},
	{"ia_ta",           no_argument,       NULL, O_IA_TA},
	{"ia_pd",           no_argument,       NULL, O_IA_PD},
	{"hostname_short",  no_argument,       NULL, O_HOSTNAME_SHORT},
	{"dev",             required_argument, NULL, O_DEV},
	{"nodev",           no_argument,       NULL, O_NODEV},
	{"define",          required_argument, NULL, O_DEFINE},
	{"define6",         required_argument, NULL, O_DEFINE6},
	{"embed",           required_argument, NULL, O_EMBED},
	{"encap",           required_argument, NULL, O_ENCAP},
	{"vendopt",         required_argument, NULL, O_VENDOPT},
	{"vendclass",       required_argument, NULL, O_VENDCLASS},
	{"authprotocol",    required_argument, NULL, O_AUTHPROTOCOL},
	{"authtoken",       required_argument, NULL, O_AUTHTOKEN},
	{"noauthrequired",  no_argument,       NULL, O_AUTHNOTREQUIRED},
	{"nodhcp",          no_argument,       NULL, O_NODHCP},
	{"nodhcp6",         no_argument,       NULL, O_NODHCP6},
	{NULL,              0,                 NULL, '\0'}
};

static int
atoint(const char *s)
{
	char *t;
	long n;

	errno = 0;
	n = strtol(s, &t, 0);
	if ((errno != 0 && n == 0) || s == t ||
	    (errno == ERANGE && (n == LONG_MAX || n == LONG_MIN)))
	{
		if (errno == 0)
			errno = EINVAL;
		syslog(LOG_ERR, "`%s' out of range", s);
		return -1;
	}

	return (int)n;
}

static char *
add_environ(struct if_options *ifo, const char *value, int uniq)
{
	char **newlist;
	char **lst = ifo->environ;
	size_t i = 0, l, lv;
	char *match = NULL, *p, *n;

	match = strdup(value);
	if (match == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	p = strchr(match, '=');
	if (p == NULL) {
		syslog(LOG_ERR, "%s: no assignment: %s", __func__, value);
		free(match);
		return NULL;
	}
	*p++ = '\0';
	l = strlen(match);

	while (lst && lst[i]) {
		if (match && strncmp(lst[i], match, l) == 0) {
			if (uniq) {
				n = strdup(value);
				if (n == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					free(match);
					return NULL;
				}
				free(lst[i]);
				lst[i] = n;
			} else {
				/* Append a space and the value to it */
				l = strlen(lst[i]);
				lv = strlen(p);
				n = realloc(lst[i], l + lv + 2);
				if (n == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					free(match);
					return NULL;
				}
				lst[i] = n;
				lst[i][l] = ' ';
				memcpy(lst[i] + l + 1, p, lv);
				lst[i][l + lv + 1] = '\0';
			}
			free(match);
			return lst[i];
		}
		i++;
	}

	free(match);
	n = strdup(value);
	if (n == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	newlist = realloc(lst, sizeof(char *) * (i + 2));
	if (newlist == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		free(n);
		return NULL;
	}
	newlist[i] = n;
	newlist[i + 1] = NULL;
	ifo->environ = newlist;
	return newlist[i];
}

#define parse_string(buf, len, arg) parse_string_hwaddr(buf, len, arg, 0)
static ssize_t
parse_string_hwaddr(char *sbuf, ssize_t slen, const char *str, int clid)
{
	ssize_t l;
	const char *p;
	int i, punt_last = 0;
	char c[4];

	/* If surrounded by quotes then it's a string */
	if (*str == '"') {
		str++;
		l = strlen(str);
		p = str + l - 1;
		if (*p == '"')
			punt_last = 1;
	} else {
		l = hwaddr_aton(NULL, str);
		if (l > 1) {
			if (l > slen) {
				errno = ENOBUFS;
				return -1;
			}
			hwaddr_aton((uint8_t *)sbuf, str);
			return l;
		}
	}

	/* Process escapes */
	l = 0;
	/* If processing a string on the clientid, first byte should be
	 * 0 to indicate a non hardware type */
	if (clid && *str) {
		if (sbuf)
			*sbuf++ = 0;
		l++;
	}
	c[3] = '\0';
	while (*str) {
		if (++l > slen && sbuf) {
			errno = ENOBUFS;
			return -1;
		}
		if (*str == '\\') {
			str++;
			switch(*str) {
			case '\0':
				break;
			case 'b':
				if (sbuf)
					*sbuf++ = '\b';
				str++;
				break;
			case 'n':
				if (sbuf)
					*sbuf++ = '\n';
				str++;
				break;
			case 'r':
				if (sbuf)
					*sbuf++ = '\r';
				str++;
				break;
			case 't':
				if (sbuf)
					*sbuf++ = '\t';
				str++;
				break;
			case 'x':
				/* Grab a hex code */
				c[1] = '\0';
				for (i = 0; i < 2; i++) {
					if (isxdigit((unsigned char)*str) == 0)
						break;
					c[i] = *str++;
				}
				if (c[1] != '\0' && sbuf) {
					c[2] = '\0';
					*sbuf++ = strtol(c, NULL, 16);
				} else
					l--;
				break;
			case '0':
				/* Grab an octal code */
				c[2] = '\0';
				for (i = 0; i < 3; i++) {
					if (*str < '0' || *str > '7')
						break;
					c[i] = *str++;
				}
				if (c[2] != '\0' && sbuf) {
					i = strtol(c, NULL, 8);
					if (i > 255)
						i = 255;
					*sbuf ++= i;
				} else
					l--;
				break;
			default:
				if (sbuf)
					*sbuf++ = *str;
				str++;
				break;
			}
		} else {
			if (sbuf)
				*sbuf++ = *str;
			str++;
		}
	}
	if (punt_last) {
		if (sbuf)
			*--sbuf = '\0';
		l--;
	}
	return l;
}

static int
parse_iaid1(uint8_t *iaid, const char *arg, size_t len, int n)
{
	unsigned long l;
	size_t s;
	uint32_t u32;
	char *np;

	errno = 0;
	l = strtoul(arg, &np, 0);
	if (l <= (unsigned long)UINT32_MAX && errno == 0 && *np == '\0') {
		if (n)
			u32 = htonl(l);
		else
			u32 = l;
		memcpy(iaid, &u32, sizeof(u32));
		return 0;
	}

	if ((s = parse_string((char *)iaid, len, arg)) < 1) {
		syslog(LOG_ERR, "%s: invalid IAID", arg);
		return -1;
	}
	if (s < 4)
		iaid[3] = '\0';
	if (s < 3)
		iaid[2] = '\0';
	if (s < 2)
		iaid[1] = '\0';
	return 0;
}

static int
parse_iaid(uint8_t *iaid, const char *arg, size_t len)
{

	return parse_iaid1(iaid, arg, len, 1);
}

static int
parse_uint32(uint32_t *i, const char *arg)
{

	return parse_iaid1((uint8_t *)i, arg, sizeof(uint32_t), 0);
}

static char **
splitv(int *argc, char **argv, const char *arg)
{
	char **n, **v = argv;
	char *o = strdup(arg), *p, *t, *nt;

	if (o == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return v;
	}
	p = o;
	while ((t = strsep(&p, ", "))) {
		nt = strdup(t);
		if (nt == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			free(o);
			return v;
		}
		n = realloc(v, sizeof(char *) * ((*argc) + 1));
		if (n == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			free(o);
			free(nt);
			return v;
		}
		v = n;
		v[(*argc)++] = nt;
	}
	free(o);
	return v;
}

#ifdef INET
static int
parse_addr(struct in_addr *addr, struct in_addr *net, const char *arg)
{
	char *p;
	int i;

	if (arg == NULL || *arg == '\0') {
		if (addr != NULL)
			addr->s_addr = 0;
		if (net != NULL)
			net->s_addr = 0;
		return 0;
	}
	if ((p = strchr(arg, '/')) != NULL) {
		*p++ = '\0';
		if (net != NULL &&
		    (sscanf(p, "%d", &i) != 1 ||
			inet_cidrtoaddr(i, net) != 0))
		{
			syslog(LOG_ERR, "`%s' is not a valid CIDR", p);
			return -1;
		}
	}

	if (addr != NULL && inet_aton(arg, addr) == 0) {
		syslog(LOG_ERR, "`%s' is not a valid IP address", arg);
		return -1;
	}
	if (p != NULL)
		*--p = '/';
	else if (net != NULL && addr != NULL)
		net->s_addr = ipv4_getnetmask(addr->s_addr);
	return 0;
}
#else
static int
parse_addr(__unused struct in_addr *addr, __unused struct in_addr *net,
    __unused const char *arg)
{

	syslog(LOG_ERR, "No IPv4 support");
	return -1;
}
#endif

static const char *
set_option_space(struct dhcpcd_ctx *ctx,
    const char *arg, const struct dhcp_opt **d, size_t *dl,
    struct if_options *ifo,
    uint8_t *request[], uint8_t *require[], uint8_t *no[])
{

#ifdef INET6
	if (strncmp(arg, "dhcp6_", strlen("dhcp6_")) == 0) {
		*d = ctx->dhcp6_opts;
		*dl = ctx->dhcp6_opts_len;
		*request = ifo->requestmask6;
		*require = ifo->requiremask6;
		*no = ifo->nomask6;
		return arg + strlen("dhcp6_");
	}
#endif

#ifdef INET
	*d = ctx->dhcp_opts;
	*dl = ctx->dhcp_opts_len;
#else
	*d = NULL;
	*dl = 0;
#endif
	*request = ifo->requestmask;
	*require = ifo->requiremask;
	*no = ifo->nomask;
	return arg;
}

void
free_dhcp_opt_embenc(struct dhcp_opt *opt)
{
	size_t i;
	struct dhcp_opt *o;

	free(opt->var);

	for (i = 0, o = opt->embopts; i < opt->embopts_len; i++, o++)
		free_dhcp_opt_embenc(o);
	free(opt->embopts);
	opt->embopts_len = 0;
	opt->embopts = NULL;

	for (i = 0, o = opt->encopts; i < opt->encopts_len; i++, o++)
		free_dhcp_opt_embenc(o);
	free(opt->encopts);
	opt->encopts_len = 0;
	opt->encopts = NULL;
}

static char *
strwhite(const char *s)
{

	if (s == NULL)
		return NULL;
	while (*s != ' ' && *s != '\t') {
		if (*s == '\0')
			return NULL;
		s++;
	}
	return UNCONST(s);
}

static char *
strskipwhite(const char *s)
{

	if (s == NULL)
		return NULL;
	while (*s == ' ' || *s == '\t') {
		if (*s == '\0')
			return NULL;
		s++;
	}
	return UNCONST(s);
}

/* Find the end pointer of a string. */
static char *
strend(const char *s)
{

	s = strskipwhite(s);
	if (s == NULL)
		return NULL;
	if (*s != '"')
		return strchr(s, ' ');
	s++;
	for (; *s != '"' ; s++) {
		if (*s == '\0')
			return NULL;
		if (*s == '\\') {
			if (*(++s) == '\0')
				return NULL;
		}
	}
	return UNCONST(++s);
}

static int
parse_option(struct dhcpcd_ctx *ctx, const char *ifname, struct if_options *ifo,
    int opt, const char *arg, struct dhcp_opt **ldop, struct dhcp_opt **edop)
{
	int i, l, t;
	unsigned int u;
	char *p = NULL, *fp, *np, **nconf;
	ssize_t s;
	struct in_addr addr, addr2;
	in_addr_t *naddr;
	struct rt *rt;
	const struct dhcp_opt *d;
	uint8_t *request, *require, *no;
	struct dhcp_opt **dop, *ndop;
	size_t *dop_len, dl;
	struct vivco *vivco;
	struct token *token;
#ifdef INET6
	size_t sl;
	struct if_ia *ia;
	uint8_t iaid[4];
	struct if_sla *sla, *slap;
#endif

	dop = NULL;
	dop_len = NULL;
#ifdef INET6
	i = 0;
#endif
	switch(opt) {
	case 'f': /* FALLTHROUGH */
	case 'g': /* FALLTHROUGH */
	case 'n': /* FALLTHROUGH */
	case 'x': /* FALLTHROUGH */
	case 'T': /* FALLTHROUGH */
	case 'U': /* FALLTHROUGH */
	case 'V': /* We need to handle non interface options */
		break;
	case 'b':
		ifo->options |= DHCPCD_BACKGROUND;
		break;
	case 'c':
		free(ifo->script);
		ifo->script = strdup(arg);
		if (ifo->script == NULL)
			syslog(LOG_ERR, "%s: %m", __func__);
		break;
	case 'd':
		ifo->options |= DHCPCD_DEBUG;
		break;
	case 'e':
		add_environ(ifo, arg, 1);
		break;
	case 'h':
		if (!arg) {
			ifo->options |= DHCPCD_HOSTNAME;
			break;
		}
		s = parse_string(ifo->hostname, HOSTNAME_MAX_LEN, arg);
		if (s == -1) {
			syslog(LOG_ERR, "hostname: %m");
			return -1;
		}
		if (s != 0 && ifo->hostname[0] == '.') {
			syslog(LOG_ERR, "hostname cannot begin with .");
			return -1;
		}
		ifo->hostname[s] = '\0';
		if (ifo->hostname[0] == '\0')
			ifo->options &= ~DHCPCD_HOSTNAME;
		else
			ifo->options |= DHCPCD_HOSTNAME;
		break;
	case 'i':
		if (arg)
			s = parse_string((char *)ifo->vendorclassid + 1,
			    VENDORCLASSID_MAX_LEN, arg);
		else
			s = 0;
		if (s == -1) {
			syslog(LOG_ERR, "vendorclassid: %m");
			return -1;
		}
		*ifo->vendorclassid = (uint8_t)s;
		break;
	case 'k':
		ifo->options |= DHCPCD_RELEASE;
		break;
	case 'l':
		if (*arg == '-') {
			syslog(LOG_ERR,
			    "leasetime must be a positive value");
			return -1;
		}
		errno = 0;
		ifo->leasetime = (uint32_t)strtoul(arg, NULL, 0);
		if (errno == EINVAL || errno == ERANGE) {
			syslog(LOG_ERR, "`%s' out of range", arg);
			return -1;
		}
		break;
	case 'm':
		ifo->metric = atoint(arg);
		if (ifo->metric < 0) {
			syslog(LOG_ERR, "metric must be a positive value");
			return -1;
		}
		break;
	case 'o':
		arg = set_option_space(ctx, arg, &d, &dl, ifo,
		    &request, &require, &no);
		if (make_option_mask(d, dl, request, arg, 1) != 0) {
			syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case 'p':
		ifo->options |= DHCPCD_PERSISTENT;
		break;
	case 'q':
		ifo->options |= DHCPCD_QUIET;
		break;
	case 'r':
		if (parse_addr(&ifo->req_addr, NULL, arg) != 0)
			return -1;
		ifo->options |= DHCPCD_REQUEST;
		ifo->req_mask.s_addr = 0;
		break;
	case 's':
		if (ifo->options & DHCPCD_IPV6 &&
		    !(ifo->options & DHCPCD_IPV4))
		{
			ifo->options |= DHCPCD_INFORM;
			break;
		}
		if (arg && *arg != '\0') {
			if (parse_addr(&ifo->req_addr, &ifo->req_mask,
				arg) != 0)
				return -1;
		} else {
			ifo->req_addr.s_addr = 0;
			ifo->req_mask.s_addr = 0;
		}
		ifo->options |= DHCPCD_INFORM | DHCPCD_PERSISTENT;
		ifo->options &= ~(DHCPCD_ARP | DHCPCD_STATIC);
		break;
	case 't':
		ifo->timeout = atoint(arg);
		if (ifo->timeout < 0) {
			syslog(LOG_ERR, "timeout must be a positive value");
			return -1;
		}
		break;
	case 'u':
		s = USERCLASS_MAX_LEN - ifo->userclass[0] - 1;
		s = parse_string((char *)ifo->userclass +
		    ifo->userclass[0] + 2,
		    s, arg);
		if (s == -1) {
			syslog(LOG_ERR, "userclass: %m");
			return -1;
		}
		if (s != 0) {
			ifo->userclass[ifo->userclass[0] + 1] = s;
			ifo->userclass[0] += s + 1;
		}
		break;
	case 'v':
		p = strchr(arg, ',');
		if (!p || !p[1]) {
			syslog(LOG_ERR, "invalid vendor format: %s", arg);
			return -1;
		}

		/* If vendor starts with , then it is not encapsulated */
		if (p == arg) {
			arg++;
			s = parse_string((char *)ifo->vendor + 1,
			    VENDOR_MAX_LEN, arg);
			if (s == -1) {
				syslog(LOG_ERR, "vendor: %m");
				return -1;
			}
			ifo->vendor[0] = (uint8_t)s;
			ifo->options |= DHCPCD_VENDORRAW;
			break;
		}

		/* Encapsulated vendor options */
		if (ifo->options & DHCPCD_VENDORRAW) {
			ifo->options &= ~DHCPCD_VENDORRAW;
			ifo->vendor[0] = 0;
		}

		/* No need to strip the comma */
		i = atoint(arg);
		if (i < 1 || i > 254) {
			syslog(LOG_ERR, "vendor option should be between"
			    " 1 and 254 inclusive");
			return -1;
		}

		arg = p + 1;
		s = VENDOR_MAX_LEN - ifo->vendor[0] - 2;
		if (inet_aton(arg, &addr) == 1) {
			if (s < 6) {
				s = -1;
				errno = ENOBUFS;
			} else {
				memcpy(ifo->vendor + ifo->vendor[0] + 3,
				    &addr.s_addr, sizeof(addr.s_addr));
				s = sizeof(addr.s_addr);
			}
		} else {
			s = parse_string((char *)ifo->vendor +
			    ifo->vendor[0] + 3, s, arg);
		}
		if (s == -1) {
			syslog(LOG_ERR, "vendor: %m");
			return -1;
		}
		if (s != 0) {
			ifo->vendor[ifo->vendor[0] + 1] = i;
			ifo->vendor[ifo->vendor[0] + 2] = s;
			ifo->vendor[0] += s + 2;
		}
		break;
	case 'w':
		ifo->options |= DHCPCD_WAITIP;
		if (arg != NULL && arg[0] != '\0') {
			if (arg[0] == '4' || arg[1] == '4')
				ifo->options |= DHCPCD_WAITIP4;
			if (arg[0] == '6' || arg[1] == '6')
				ifo->options |= DHCPCD_WAITIP6;
		}
		break;
	case 'y':
		ifo->reboot = atoint(arg);
		if (ifo->reboot < 0) {
			syslog(LOG_ERR, "reboot must be a positive value");
			return -1;
		}
		break;
	case 'z':
		if (ifname == NULL)
			ctx->ifav = splitv(&ctx->ifac, ctx->ifav, arg);
		break;
	case 'A':
		ifo->options &= ~DHCPCD_ARP;
		/* IPv4LL requires ARP */
		ifo->options &= ~DHCPCD_IPV4LL;
		break;
	case 'B':
		ifo->options &= ~DHCPCD_DAEMONISE;
		break;
	case 'C':
		/* Commas to spaces for shell */
		while ((p = strchr(arg, ',')))
			*p = ' ';
		s = strlen("skip_hooks=") + strlen(arg) + 1;
		p = malloc(sizeof(char) * s);
		if (p == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		snprintf(p, s, "skip_hooks=%s", arg);
		add_environ(ifo, p, 0);
		free(p);
		break;
	case 'D':
		ifo->options |= DHCPCD_CLIENTID | DHCPCD_DUID;
		break;
	case 'E':
		ifo->options |= DHCPCD_LASTLEASE;
		break;
	case 'F':
		if (!arg) {
			ifo->fqdn = FQDN_BOTH;
			break;
		}
		if (strcmp(arg, "none") == 0)
			ifo->fqdn = FQDN_NONE;
		else if (strcmp(arg, "ptr") == 0)
			ifo->fqdn = FQDN_PTR;
		else if (strcmp(arg, "both") == 0)
			ifo->fqdn = FQDN_BOTH;
		else if (strcmp(arg, "disable") == 0)
			ifo->fqdn = FQDN_DISABLE;
		else {
			syslog(LOG_ERR, "invalid value `%s' for FQDN", arg);
			return -1;
		}
		break;
	case 'G':
		ifo->options &= ~DHCPCD_GATEWAY;
		break;
	case 'H':
		ifo->options |= DHCPCD_XID_HWADDR;
		break;
	case 'I':
		/* Strings have a type of 0 */;
		ifo->clientid[1] = 0;
		if (arg)
			s = parse_string_hwaddr((char *)ifo->clientid + 1,
			    CLIENTID_MAX_LEN, arg, 1);
		else
			s = 0;
		if (s == -1) {
			syslog(LOG_ERR, "clientid: %m");
			return -1;
		}
		ifo->options |= DHCPCD_CLIENTID;
		ifo->clientid[0] = (uint8_t)s;
		break;
	case 'J':
		ifo->options |= DHCPCD_BROADCAST;
		break;
	case 'K':
		ifo->options &= ~DHCPCD_LINK;
		break;
	case 'L':
		ifo->options &= ~DHCPCD_IPV4LL;
		break;
	case 'M':
		ifo->options |= DHCPCD_MASTER;
		break;
	case 'O':
		arg = set_option_space(ctx, arg, &d, &dl, ifo,
		    &request, &require, &no);
		if (make_option_mask(d, dl, request, arg, -1) != 0 ||
		    make_option_mask(d, dl, require, arg, -1) != 0 ||
		    make_option_mask(d, dl, no, arg, 1) != 0)
		{
			syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case 'Q':
		arg = set_option_space(ctx, arg, &d, &dl, ifo,
		    &request, &require, &no);
		if (make_option_mask(d, dl, require, arg, 1) != 0 ||
		    make_option_mask(d, dl, request, arg, 1) != 0)
		{
			syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case 'S':
		p = strchr(arg, '=');
		if (p == NULL) {
			syslog(LOG_ERR, "static assignment required");
			return -1;
		}
		p++;
		if (strncmp(arg, "ip_address=", strlen("ip_address=")) == 0) {
			if (parse_addr(&ifo->req_addr,
			    ifo->req_mask.s_addr == 0 ? &ifo->req_mask : NULL,
			    p) != 0)
				return -1;

			ifo->options |= DHCPCD_STATIC;
			ifo->options &= ~DHCPCD_INFORM;
		} else if (strncmp(arg, "subnet_mask=", strlen("subnet_mask=")) == 0) {
			if (parse_addr(&ifo->req_mask, NULL, p) != 0)
				return -1;
		} else if (strncmp(arg, "routes=", strlen("routes=")) == 0 ||
		    strncmp(arg, "static_routes=", strlen("static_routes=")) == 0 ||
		    strncmp(arg, "classless_static_routes=", strlen("classless_static_routes=")) == 0 ||
		    strncmp(arg, "ms_classless_static_routes=", strlen("ms_classless_static_routes=")) == 0)
		{
			fp = np = strwhite(p);
			if (np == NULL) {
				syslog(LOG_ERR, "all routes need a gateway");
				return -1;
			}
			*np++ = '\0';
			np = strskipwhite(np);
			if (ifo->routes == NULL) {
				ifo->routes = malloc(sizeof(*ifo->routes));
				if (ifo->routes == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					return -1;
				}
				TAILQ_INIT(ifo->routes);
			}
			rt = malloc(sizeof(*rt));
			if (rt == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				*fp = ' ';
				return -1;
			}
			if (parse_addr(&rt->dest, &rt->net, p) == -1 ||
			    parse_addr(&rt->gate, NULL, np) == -1)
			{
				free(rt);
				*fp = ' ';
				return -1;
			}
			TAILQ_INSERT_TAIL(ifo->routes, rt, next);
			*fp = ' ';
		} else if (strncmp(arg, "routers=", strlen("routers=")) == 0) {
			if (ifo->routes == NULL) {
				ifo->routes = malloc(sizeof(*ifo->routes));
				if (ifo->routes == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					return -1;
				}
				TAILQ_INIT(ifo->routes);
			}
			rt = malloc(sizeof(*rt));
			if (rt == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			rt->dest.s_addr = INADDR_ANY;
			rt->net.s_addr = INADDR_ANY;
			if (parse_addr(&rt->gate, NULL, p) == -1) {
				free(rt);
				return -1;
			}
			TAILQ_INSERT_TAIL(ifo->routes, rt, next);
		} else {
			s = 0;
			if (ifo->config != NULL) {
				while (ifo->config[s] != NULL) {
					if (strncmp(ifo->config[s], arg,
						p - arg) == 0)
					{
						p = strdup(arg);
						if (p == NULL) {
							syslog(LOG_ERR,
							    "%s: %m", __func__);
							return -1;
						}
						free(ifo->config[s]);
						ifo->config[s] = p;
						return 1;
					}
					s++;
				}
			}
			p = strdup(arg);
			if (p == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			nconf = realloc(ifo->config, sizeof(char *) * (s + 2));
			if (nconf == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ifo->config = nconf;
			ifo->config[s] = p;
			ifo->config[s + 1] = NULL;
		}
		break;
	case 'W':
		if (parse_addr(&addr, &addr2, arg) != 0)
			return -1;
		if (strchr(arg, '/') == NULL)
			addr2.s_addr = INADDR_BROADCAST;
		naddr = realloc(ifo->whitelist,
		    sizeof(in_addr_t) * (ifo->whitelist_len + 2));
		if (naddr == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		ifo->whitelist = naddr;
		ifo->whitelist[ifo->whitelist_len++] = addr.s_addr;
		ifo->whitelist[ifo->whitelist_len++] = addr2.s_addr;
		break;
	case 'X':
		if (parse_addr(&addr, &addr2, arg) != 0)
			return -1;
		if (strchr(arg, '/') == NULL)
			addr2.s_addr = INADDR_BROADCAST;
		naddr = realloc(ifo->blacklist,
		    sizeof(in_addr_t) * (ifo->blacklist_len + 2));
		if (naddr == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		ifo->blacklist = naddr;
		ifo->blacklist[ifo->blacklist_len++] = addr.s_addr;
		ifo->blacklist[ifo->blacklist_len++] = addr2.s_addr;
		break;
	case 'Z':
		if (ifname == NULL)
			ctx->ifdv = splitv(&ctx->ifdc, ctx->ifdv, arg);
		break;
	case '4':
		ifo->options &= ~DHCPCD_IPV6;
		ifo->options |= DHCPCD_IPV4;
		break;
	case '6':
		ifo->options &= ~DHCPCD_IPV4;
		ifo->options |= DHCPCD_IPV6;
		break;
	case O_NOIPV4:
		ifo->options &= ~DHCPCD_IPV4;
		break;
	case O_NOIPV6:
		ifo->options &= ~DHCPCD_IPV6;
		break;
#ifdef INET
	case O_ARPING:
		while (arg && *arg != '\0') {
			fp = strwhite(arg);
			if (fp)
				*fp++ = '\0';
			if (parse_addr(&addr, NULL, arg) != 0)
				return -1;
			naddr = realloc(ifo->arping,
			    sizeof(in_addr_t) * (ifo->arping_len + 1));
			if (naddr == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ifo->arping = naddr;
			ifo->arping[ifo->arping_len++] = addr.s_addr;
			arg = strskipwhite(fp);
		}
		break;
	case O_DESTINATION:
		if (make_option_mask(ctx->dhcp_opts, ctx->dhcp_opts_len,
		    ifo->dstmask, arg, 2) != 0) {
			if (errno == EINVAL)
				syslog(LOG_ERR, "option `%s' does not take"
				    " an IPv4 address", arg);
			else
				syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case O_FALLBACK:
		free(ifo->fallback);
		ifo->fallback = strdup(arg);
		if (ifo->fallback == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		break;
#endif
	case O_IAID:
		if (ifname == NULL) {
			syslog(LOG_ERR,
			    "IAID must belong in an interface block");
			return -1;
		}
		if (parse_iaid(ifo->iaid, arg, sizeof(ifo->iaid)) == -1)
			return -1;
		ifo->options |= DHCPCD_IAID;
		break;
	case O_IPV6RS:
		ifo->options |= DHCPCD_IPV6RS;
		break;
	case O_NOIPV6RS:
		ifo->options &= ~DHCPCD_IPV6RS;
		break;
	case O_IPV6RA_FORK:
		ifo->options &= ~DHCPCD_IPV6RA_REQRDNSS;
		break;
	case O_IPV6RA_OWN:
		ifo->options |= DHCPCD_IPV6RA_OWN;
		break;
	case O_IPV6RA_OWN_D:
		ifo->options |= DHCPCD_IPV6RA_OWN_DEFAULT;
		break;
	case O_NOALIAS:
		ifo->options |= DHCPCD_NOALIAS;
		break;
#ifdef INET6
	case O_IA_NA:
		i = D6_OPTION_IA_NA;
		/* FALLTHROUGH */
	case O_IA_TA:
		if (i == 0)
			i = D6_OPTION_IA_TA;
		/* FALLTHROUGH */
	case O_IA_PD:
		if (i == 0) {
			if (ifname == NULL) {
				syslog(LOG_ERR,
				    "IA PD must belong in an interface block");
				return -1;
			}
			i = D6_OPTION_IA_PD;
		}
		if (arg != NULL && ifname == NULL) {
			syslog(LOG_ERR,
			    "IA with IAID must belong in an interface block");
			return -1;
		}
		ifo->options |= DHCPCD_IA_FORCED;
		if (ifo->ia_type != 0 && ifo->ia_type != i) {
			syslog(LOG_ERR, "cannot specify a different IA type");
			return -1;
		}
		ifo->ia_type = i;
		if (arg == NULL)
			break;
		fp = strwhite(arg);
		if (fp)
			*fp++ = '\0';
		if (parse_iaid(iaid, arg, sizeof(iaid)) == -1)
			return -1;
		ia = NULL;
		for (sl = 0; sl < ifo->ia_len; sl++) {
			if (ifo->ia[sl].iaid[0] == iaid[0] &&
			    ifo->ia[sl].iaid[1] == iaid[1] &&
			    ifo->ia[sl].iaid[2] == iaid[2] &&
			    ifo->ia[sl].iaid[3] == iaid[3])
			{
			        ia = &ifo->ia[sl];
				break;
			}
		}
		if (ia == NULL) {
			ia = realloc(ifo->ia,
			    sizeof(*ifo->ia) * (ifo->ia_len + 1));
			if (ia == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ifo->ia = ia;
			ia = &ifo->ia[ifo->ia_len++];
			ia->iaid[0] = iaid[0];
			ia->iaid[1] = iaid[1];
			ia->iaid[2] = iaid[2];
			ia->iaid[3] = iaid[3];
			ia->sla = NULL;
			ia->sla_len = 0;
		}
		if (ifo->ia_type != D6_OPTION_IA_PD)
			break;
		for (p = fp; p; p = fp) {
			fp = strwhite(p);
			if (fp) {
				*fp++ = '\0';
				fp = strskipwhite(fp);
			}
			sla = realloc(ia->sla,
			    sizeof(*ia->sla) * (ia->sla_len + 1));
			if (sla == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ia->sla = sla;
			sla = &ia->sla[ia->sla_len++];
			np = strchr(p, '/');
			if (np)
				*np++ = '\0';
			if (strcmp(ifname, p) == 0) {
				syslog(LOG_ERR,
				    "%s: cannot assign IA_PD to itself",
				    ifname);
				return -1;
			}
			if (strlcpy(sla->ifname, p,
			    sizeof(sla->ifname)) >= sizeof(sla->ifname))
			{
				syslog(LOG_ERR, "%s: interface name too long",
				    arg);
				return -1;
			}
			p = np;
			if (p) {
				np = strchr(p, '/');
				if (np)
					*np++ = '\0';
				if (*p == '\0')
					sla->sla_set = 0;
				else {
					errno = 0;
					sla->sla = atoint(p);
					sla->sla_set = 1;
					if (errno)
						return -1;
				}
				if (np) {
					sla->prefix_len = atoint(np);
					if (sla->prefix_len < 0 ||
					    sla->prefix_len > 128)
						return -1;
				} else
					sla->prefix_len = 64;
			} else {
				sla->sla_set = 0;
				/* Sanity - check there are no more
				 * unspecified SLA's */
				for (sl = 0; sl < ia->sla_len - 1; sl++) {
					slap = &ia->sla[sl];
					if (slap->sla_set == 0 &&
					    strcmp(slap->ifname, sla->ifname)
					    == 0)
					{
						syslog(LOG_WARNING,
						    "%s: cannot specify the "
						    "same interface twice with "
						    "an automatic SLA",
						    sla->ifname);
						ia->sla_len--;
						break;
					}
				}
			}
		}
#endif
		break;
	case O_HOSTNAME_SHORT:
		ifo->options |= DHCPCD_HOSTNAME | DHCPCD_HOSTNAME_SHORT;
		break;
	case O_DEV:
#ifdef PLUGIN_DEV
		if (ctx->dev_load)
			free(ctx->dev_load);
		ctx->dev_load = strdup(arg);
#endif
		break;
	case O_NODEV:
		ifo->options &= ~DHCPCD_DEV;
		break;
	case O_DEFINE:
		dop = &ifo->dhcp_override;
		dop_len = &ifo->dhcp_override_len;
		/* FALLTHROUGH */
	case O_DEFINE6:
		if (dop == NULL) {
			dop = &ifo->dhcp6_override;
			dop_len = &ifo->dhcp6_override_len;
		}
		/* FALLTHROUGH */
	case O_VENDOPT:
		if (dop == NULL) {
			dop = &ifo->vivso_override;
			dop_len = &ifo->vivso_override_len;
		}
		*edop = *ldop = NULL;
		/* FALLTHROUGH */
	case O_EMBED:
		if (dop == NULL) {
			if (*edop) {
				dop = &(*edop)->embopts;
				dop_len = &(*edop)->embopts_len;
			} else if (ldop) {
				dop = &(*ldop)->embopts;
				dop_len = &(*ldop)->embopts_len;
			} else {
				syslog(LOG_ERR,
				    "embed must be after a define or encap");
				return -1;
			}
		}
		/* FALLTHROUGH */
	case O_ENCAP:
		if (dop == NULL) {
			if (*ldop == NULL) {
				syslog(LOG_ERR, "encap must be after a define");
				return -1;
			}
			dop = &(*ldop)->encopts;
			dop_len = &(*ldop)->encopts_len;
		}

		/* Shared code for define, define6, embed and encap */

		/* code */
		if (opt == O_EMBED) /* Embedded options don't have codes */
			u = 0;
		else {
			fp = strwhite(arg);
			if (fp == NULL) {
				syslog(LOG_ERR, "invalid syntax: %s", arg);
				return -1;
			}
			*fp++ = '\0';
			errno = 0;
			u = strtoul(arg, &np, 0);
			if (u > UINT32_MAX || errno != 0 || *np != '\0') {
				syslog(LOG_ERR, "invalid code: %s", arg);
				return -1;
			}
			arg = strskipwhite(fp);
			if (arg == NULL) {
				syslog(LOG_ERR, "invalid syntax");
				return -1;
			}
		}
		/* type */
		fp = strwhite(arg);
		if (fp)
			*fp++ = '\0';
		np = strchr(arg, ':');
		/* length */
		if (np) {
			*np++ = '\0';
			if ((l = atoint(np)) == -1)
				return -1;
		} else
			l = 0;
		t = 0;
		if (strcasecmp(arg, "request") == 0) {
			t |= REQUEST;
			arg = strskipwhite(fp);
			fp = strwhite(arg);
			if (fp == NULL) {
				syslog(LOG_ERR, "incomplete request type");
				return -1;
			}
			*fp++ = '\0';
		} else if (strcasecmp(arg, "norequest") == 0) {
			t |= NOREQ;
			arg = strskipwhite(fp);
			fp = strwhite(arg);
			if (fp == NULL) {
				syslog(LOG_ERR, "incomplete request type");
				return -1;
			}
			*fp++ = '\0';
		}
		if (strcasecmp(arg, "index") == 0) {
			t |= INDEX;
			arg = strskipwhite(fp);
			fp = strwhite(arg);
			if (fp == NULL) {
				syslog(LOG_ERR, "incomplete index type");
				return -1;
			}
			*fp++ = '\0';
		}
		if (strcasecmp(arg, "array") == 0) {
			t |= ARRAY;
			arg = strskipwhite(fp);
			fp = strwhite(arg);
			if (fp == NULL) {
				syslog(LOG_ERR, "incomplete array type");
				return -1;
			}
			*fp++ = '\0';
		}
		if (strcasecmp(arg, "ipaddress") == 0)
			t |= ADDRIPV4;
		else if (strcasecmp(arg, "ip6address") == 0)
			t |= ADDRIPV6;
		else if (strcasecmp(arg, "string") == 0)
			t |= STRING;
		else if (strcasecmp(arg, "byte") == 0)
			t |= UINT8;
		else if (strcasecmp(arg, "uint16") == 0)
			t |= UINT16;
		else if (strcasecmp(arg, "int16") == 0)
			t |= SINT16;
		else if (strcasecmp(arg, "uint32") == 0)
			t |= UINT32;
		else if (strcasecmp(arg, "int32") == 0)
			t |= SINT32;
		else if (strcasecmp(arg, "flag") == 0)
			t |= FLAG;
		else if (strcasecmp(arg, "domain") == 0)
			t |= STRING | RFC3397;
		else if (strcasecmp(arg, "binhex") == 0)
			t |= BINHEX;
		else if (strcasecmp(arg, "embed") == 0)
			t |= EMBED;
		else if (strcasecmp(arg, "encap") == 0)
			t |= ENCAP;
		else if (strcasecmp(arg, "rfc3361") ==0)
			t |= STRING | RFC3361;
		else if (strcasecmp(arg, "rfc3442") ==0)
			t |= STRING | RFC3442;
		else if (strcasecmp(arg, "rfc5969") == 0)
			t |= STRING | RFC5969;
		else if (strcasecmp(arg, "option") == 0)
			t |= OPTION;
		else {
			syslog(LOG_ERR, "unknown type: %s", arg);
			return -1;
		}
		if (l && !(t & (STRING | BINHEX))) {
			syslog(LOG_WARNING,
			    "ignoring length for type `%s'", arg);
			l = 0;
		}
		if (t & ARRAY && t & (STRING | BINHEX)) {
			syslog(LOG_WARNING, "ignoring array for strings");
			t &= ~ARRAY;
		}
		/* variable */
		if (!fp) {
			if (!(t & OPTION)) {
			        syslog(LOG_ERR,
				    "type %s requires a variable name", arg);
				return -1;
			}
			np = NULL;
		} else {
			arg = strskipwhite(fp);
			fp = strwhite(arg);
			if (fp)
				*fp++ = '\0';
			np = strdup(arg);
			if (np == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
		}
		if (opt != O_EMBED) {
			for (dl = 0, ndop = *dop; dl < *dop_len; dl++, ndop++)
			{
				/* type 0 seems freshly malloced struct
				 * for us to use */
				if (ndop->option == u || ndop->type == 0)
					break;
			}
			if (dl == *dop_len)
				ndop = NULL;
		} else
			ndop = NULL;
		if (ndop == NULL) {
			if ((ndop = realloc(*dop,
			    sizeof(**dop) * ((*dop_len) + 1))) == NULL)
			{
				syslog(LOG_ERR, "%s: %m", __func__);
				free(np);
				return -1;
			}
			*dop = ndop;
			ndop = &(*dop)[(*dop_len)++];
			ndop->embopts = NULL;
			ndop->embopts_len = 0;
			ndop->encopts = NULL;
			ndop->encopts_len = 0;
		} else
			free_dhcp_opt_embenc(ndop);
		ndop->option = u; /* could have been 0 */
		ndop->type = t;
		ndop->len = l;
		ndop->var = np;
		/* Save the define for embed and encap options */
		if (opt == O_DEFINE || opt == O_DEFINE6 || opt == O_VENDOPT)
			*ldop = ndop;
		else if (opt == O_ENCAP)
			*edop = ndop;
		break;
	case O_VENDCLASS:
		fp = strwhite(arg);
		if (fp)
			*fp++ = '\0';
		errno = 0;
		u = strtoul(arg, &np, 0);
		if (u > UINT32_MAX || errno != 0 || *np != '\0') {
			syslog(LOG_ERR, "invalid code: %s", arg);
			return -1;
		}
		if (fp) {
			s = parse_string(NULL, 0, fp);
			if (s == -1) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			if (s + (sizeof(uint16_t) * 2) > UINT16_MAX) {
				syslog(LOG_ERR, "vendor class is too big");
				return -1;
			}
			np = malloc(s);
			if (np == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			parse_string(np, s, fp);
		} else {
			s = 0;
			np = NULL;
		}
		vivco = realloc(ifo->vivco, sizeof(*ifo->vivco) *
		    (ifo->vivco_len + 1));
		if (vivco == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		ifo->vivco = vivco;
		ifo->vivco_en = u;
		vivco = &ifo->vivco[ifo->vivco_len++];
		vivco->len = s;
		vivco->data = (uint8_t *)np;
		break;
	case O_AUTHPROTOCOL:
		fp = strwhite(arg);
		if (fp)
			*fp++ = '\0';
		if (strcasecmp(arg, "token") == 0)
			ifo->auth.protocol = AUTH_PROTO_TOKEN;
		else if (strcasecmp(arg, "delayed") == 0)
			ifo->auth.protocol = AUTH_PROTO_DELAYED;
		else if (strcasecmp(arg, "delayedrealm") == 0)
			ifo->auth.protocol = AUTH_PROTO_DELAYEDREALM;
		else {
			syslog(LOG_ERR, "%s: unsupported protocol", arg);
			return -1;
		}
		arg = strskipwhite(fp);
		fp = strwhite(arg);
		if (arg == NULL) {
			ifo->auth.options |= DHCPCD_AUTH_SEND;
			ifo->auth.algorithm = AUTH_ALG_HMAC_MD5;
			ifo->auth.rdm = AUTH_RDM_MONOTONIC;
			break;
		}
		if (fp)
			*fp++ = '\0';
		if (strcasecmp(arg, "hmacmd5") == 0 ||
		    strcasecmp(arg, "hmac-md5") == 0)
			ifo->auth.algorithm = AUTH_ALG_HMAC_MD5;
		else {
			syslog(LOG_ERR, "%s: unsupported algorithm", arg);
			return 1;
		}
		arg = fp;
		if (arg == NULL) {
			ifo->auth.options |= DHCPCD_AUTH_SEND;
			ifo->auth.rdm = AUTH_RDM_MONOTONIC;
			break;
		}
		if (strcasecmp(arg, "monocounter") == 0) {
			ifo->auth.rdm = AUTH_RDM_MONOTONIC;
			ifo->auth.options |= DHCPCD_AUTH_RDM_COUNTER;
		} else if (strcasecmp(arg, "monotonic") ==0 ||
		    strcasecmp(arg, "monotime") == 0)
			ifo->auth.rdm = AUTH_RDM_MONOTONIC;
		else {
			syslog(LOG_ERR, "%s: unsupported RDM", arg);
			return -1;
		}
		ifo->auth.options |= DHCPCD_AUTH_SEND;
		break;
	case O_AUTHTOKEN:
		fp = strwhite(arg);
		if (fp == NULL) {
			syslog(LOG_ERR, "authtoken requires a realm");
			return -1;
		}
		*fp++ = '\0';
		token = malloc(sizeof(*token));
		if (token == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return -1;
		}
		if (parse_uint32(&token->secretid, arg) == -1) {
			syslog(LOG_ERR, "%s: not a number", arg);
			free(token);
			return -1;
		}
		arg = fp;
		fp = strend(arg);
		if (fp == NULL) {
			syslog(LOG_ERR, "authtoken requies an a key");
			free(token);
			return -1;
		}
		*fp++ = '\0';
		token->realm_len = parse_string(NULL, 0, arg);
		if (token->realm_len) {
			token->realm = malloc(token->realm_len);
			if (token->realm == NULL) {
				free(token);
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			parse_string((char *)token->realm, token->realm_len,
			    arg);
		}
		arg = fp;
		fp = strend(arg);
		if (fp == NULL) {
			syslog(LOG_ERR, "authtoken requies an an expiry date");
			free(token->realm);
			free(token);
			return -1;
		}
		*fp++ = '\0';
		if (*arg == '"') {
			arg++;
			np = strchr(arg, '"');
			if (np)
				*np = '\0';
		}
		if (strcmp(arg, "0") == 0 || strcasecmp(arg, "forever") == 0)
			token->expire =0;
		else {
			struct tm tm;

			memset(&tm, 0, sizeof(tm));
			if (strptime(arg, "%Y-%m-%d %H:%M", &tm) == NULL) {
				syslog(LOG_ERR, "%s: invalid date time", arg);
				free(token->realm);
				free(token);
				return -1;
			}
			if ((token->expire = mktime(&tm)) == (time_t)-1) {
				syslog(LOG_ERR, "%s: mktime: %m", __func__);
				free(token->realm);
				free(token);
				return -1;
			}
		}
		arg = fp;
		token->key_len = parse_string(NULL, 0, arg);
		if (token->key_len == 0) {
			syslog(LOG_ERR, "authtoken needs a key");
			free(token->realm);
			free(token);
			return -1;
		}
		token->key = malloc(token->key_len);
		parse_string((char *)token->key, token->key_len, arg);
		TAILQ_INSERT_TAIL(&ifo->auth.tokens, token, next);
		break;
	case O_AUTHNOTREQUIRED:
		ifo->auth.options &= ~DHCPCD_AUTH_REQUIRE;
		break;
	case O_NODHCP:
		ifo->options &= ~DHCPCD_DHCP;
		break;
	case O_NODHCP6:
		ifo->options &= ~DHCPCD_DHCP6;
		break;
	default:
		return 0;
	}

	return 1;
}

static int
parse_config_line(struct dhcpcd_ctx *ctx, const char *ifname,
    struct if_options *ifo, const char *opt, char *line,
    struct dhcp_opt **ldop, struct dhcp_opt **edop)
{
	unsigned int i;

	for (i = 0; i < sizeof(cf_options) / sizeof(cf_options[0]); i++) {
		if (!cf_options[i].name ||
		    strcmp(cf_options[i].name, opt) != 0)
			continue;

		if (cf_options[i].has_arg == required_argument && !line) {
			fprintf(stderr,
			    PACKAGE ": option requires an argument -- %s\n",
			    opt);
			return -1;
		}

		return parse_option(ctx, ifname, ifo, cf_options[i].val, line,
		    ldop, edop);
	}

	syslog(LOG_ERR, "unknown option: %s", opt);
	return -1;
}

static void
finish_config(struct if_options *ifo)
{

	/* Terminate the encapsulated options */
	if (ifo->vendor[0] && !(ifo->options & DHCPCD_VENDORRAW)) {
		ifo->vendor[0]++;
		ifo->vendor[ifo->vendor[0]] = DHO_END;
		/* We are called twice.
		 * This should be fixed, but in the meantime, this
		 * guard should suffice */
		ifo->options |= DHCPCD_VENDORRAW;
	}
}

/* Handy routine to read very long lines in text files.
 * This means we read the whole line and avoid any nasty buffer overflows.
 * We strip leading space and avoid comment lines, making the code that calls
 * us smaller. */
static char *
get_line(char ** __restrict buf, size_t * __restrict buflen,
    FILE * __restrict fp)
{
	char *p;
	ssize_t bytes;

	do {
		bytes = getline(buf, buflen, fp);
		if (bytes == -1)
			return NULL;
		for (p = *buf; *p == ' ' || *p == '\t'; p++)
			;
	} while (*p == '\0' || *p == '\n' || *p == '#' || *p == ';');
	if ((*buf)[--bytes] == '\n')
		(*buf)[bytes] = '\0';
	return p;
}

struct if_options *
read_config(struct dhcpcd_ctx *ctx,
    const char *ifname, const char *ssid, const char *profile)
{
	struct if_options *ifo;
	FILE *fp;
	char *line, *buf, *option, *p;
	size_t buflen;
	int skip = 0, have_profile = 0;
#ifndef EMBEDDED_CONFIG
	const char * const *e;
	size_t ol;
#endif
#if !defined(INET) || !defined(INET6)
	size_t i;
	struct dhcp_opt *opt;
#endif
	struct dhcp_opt *ldop, *edop;

	/* Seed our default options */
	ifo = calloc(1, sizeof(*ifo));
	if (ifo == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	ifo->options |= DHCPCD_DAEMONISE | DHCPCD_LINK;
#ifdef PLUGIN_DEV
	ifo->options |= DHCPCD_DEV;
#endif
#ifdef INET
	ifo->options |= DHCPCD_IPV4 | DHCPCD_DHCP | DHCPCD_IPV4LL;
	ifo->options |= DHCPCD_GATEWAY | DHCPCD_ARP;
#endif
#ifdef INET6
	ifo->options |= DHCPCD_IPV6 | DHCPCD_IPV6RS | DHCPCD_IPV6RA_REQRDNSS;
	ifo->options |= DHCPCD_DHCP6;
	ifo->dadtransmits = ipv6_dadtransmits(ifname);
#endif
	ifo->timeout = DEFAULT_TIMEOUT;
	ifo->reboot = DEFAULT_REBOOT;
	ifo->metric = -1;
	ifo->auth.options |= DHCPCD_AUTH_REQUIRE;
	TAILQ_INIT(&ifo->auth.tokens);

	ifo->vendorclassid[0] = dhcp_vendor((char *)ifo->vendorclassid + 1,
	    sizeof(ifo->vendorclassid) - 1);

	buf = NULL;
	buflen = 0;

	/* Parse our embedded options file */
	if (ifname == NULL) {
		/* Space for initial estimates */
#if defined(INET) && defined(INITDEFINES)
		ifo->dhcp_override =
		    calloc(INITDEFINES, sizeof(*ifo->dhcp_override));
		if (ifo->dhcp_override == NULL)
			syslog(LOG_ERR, "%s: %m", __func__);
		else
			ifo->dhcp_override_len = INITDEFINES;
#endif

#if defined(INET6) && defined(INITDEFINE6S)
		ifo->dhcp6_override =
		    calloc(INITDEFINE6S, sizeof(*ifo->dhcp6_override));
		if (ifo->dhcp6_override == NULL)
			syslog(LOG_ERR, "%s: %m", __func__);
		else
			ifo->dhcp6_override_len = INITDEFINE6S;
#endif

		/* Now load our embedded config */
#ifdef EMBEDDED_CONFIG
		fp = fopen(EMBEDDED_CONFIG, "r");
		if (fp == NULL)
			syslog(LOG_ERR, "fopen `%s': %m", EMBEDDED_CONFIG);

		while (fp && (line = get_line(&buf, &buflen, fp))) {
#else
		buflen = 80;
		buf = malloc(buflen);
		if (buf == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		ldop = edop = NULL;
		for (e = dhcpcd_embedded_conf; *e; e++) {
			ol = strlen(*e) + 1;
			if (ol > buflen) {
				free(buf);
				buflen = ol;
				buf = malloc(buflen);
				if (buf == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
					return NULL;
				}
			}
			memcpy(buf, *e, ol);
			line = buf;
#endif
			option = strsep(&line, " \t");
			if (line)
				line = strskipwhite(line);
			/* Trim trailing whitespace */
			if (line && *line) {
				p = line + strlen(line) - 1;
				while (p != line &&
				    (*p == ' ' || *p == '\t') &&
				    *(p - 1) != '\\')
					*p-- = '\0';
			}
			parse_config_line(ctx, NULL, ifo, option, line,
			    &ldop, &edop);

		}

#ifdef EMBEDDED_CONFIG
		if (fp)
			fclose(fp);
#endif
#ifdef INET
		ctx->dhcp_opts = ifo->dhcp_override;
		ctx->dhcp_opts_len = ifo->dhcp_override_len;
#else
		for (i = 0, opt = ifo->dhcp_override;
		    i < ifo->dhcp_override_len;
		    i++, opt++)
			free_dhcp_opt_embenc(opt);
		free(ifo->dhcp_override);
#endif
		ifo->dhcp_override = NULL;
		ifo->dhcp_override_len = 0;

#ifdef INET6
		ctx->dhcp6_opts = ifo->dhcp6_override;
		ctx->dhcp6_opts_len = ifo->dhcp6_override_len;
#else
		for (i = 0, opt = ifo->dhcp6_override;
		    i < ifo->dhcp6_override_len;
		    i++, opt++)
			free_dhcp_opt_embenc(opt);
		free(ifo->dhcp6_override);
#endif
		ifo->dhcp6_override = NULL;
		ifo->dhcp6_override_len = 0;

		ctx->vivso = ifo->vivso_override;
		ctx->vivso_len = ifo->vivso_override_len;
		ifo->vivso_override = NULL;
		ifo->vivso_override_len = 0;
	}

	/* Parse our options file */
	fp = fopen(ctx->cffile, "r");
	if (fp == NULL) {
		if (strcmp(ctx->cffile, CONFIG))
			syslog(LOG_ERR, "fopen `%s': %m", ctx->cffile);
		free(buf);
		return ifo;
	}

	ldop = edop = NULL;
	while ((line = get_line(&buf, &buflen, fp))) {
		option = strsep(&line, " \t");
		if (line)
			line = strskipwhite(line);
		/* Trim trailing whitespace */
		if (line && *line) {
			p = line + strlen(line) - 1;
			while (p != line &&
			    (*p == ' ' || *p == '\t') &&
			    *(p - 1) != '\\')
				*p-- = '\0';
		}
		/* Start of an interface block, skip if not ours */
		if (strcmp(option, "interface") == 0) {
			if (ifname && line && strcmp(line, ifname) == 0)
				skip = 0;
			else
				skip = 1;
			continue;
		}
		/* Start of an ssid block, skip if not ours */
		if (strcmp(option, "ssid") == 0) {
			if (ssid && line && strcmp(line, ssid) == 0)
				skip = 0;
			else
				skip = 1;
			continue;
		}
		/* Start of a profile block, skip if not ours */
		if (strcmp(option, "profile") == 0) {
			if (profile && line && strcmp(line, profile) == 0) {
				skip = 0;
				have_profile = 1;
			} else
				skip = 1;
			continue;
		}
		if (skip)
			continue;
		parse_config_line(ctx, ifname, ifo, option, line, &ldop, &edop);
	}
	fclose(fp);
	free(buf);

	if (profile && !have_profile) {
		free_options(ifo);
		errno = ENOENT;
		return NULL;
	}

	finish_config(ifo);
	return ifo;
}

int
add_options(struct dhcpcd_ctx *ctx, const char *ifname,
    struct if_options *ifo, int argc, char **argv)
{
	int oi, opt, r;

	if (argc == 0)
		return 1;

	optind = 0;
	r = 1;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		r = parse_option(ctx, ifname, ifo, opt, optarg, NULL, NULL);
		if (r != 1)
			break;
	}

	finish_config(ifo);
	return r;
}

void
free_options(struct if_options *ifo)
{
	size_t i;
	struct dhcp_opt *opt;
	struct vivco *vo;
	struct token *token;

	if (ifo) {
		if (ifo->environ) {
			i = 0;
			while (ifo->environ[i])
				free(ifo->environ[i++]);
			free(ifo->environ);
		}
		if (ifo->config) {
			i = 0;
			while (ifo->config[i])
				free(ifo->config[i++]);
			free(ifo->config);
		}
		ipv4_freeroutes(ifo->routes);
		free(ifo->script);
		free(ifo->arping);
		free(ifo->blacklist);
		free(ifo->fallback);

		for (opt = ifo->dhcp_override;
		    ifo->dhcp_override_len > 0;
		    opt++, ifo->dhcp_override_len--)
			free_dhcp_opt_embenc(opt);
		free(ifo->dhcp_override);
		for (opt = ifo->dhcp6_override;
		    ifo->dhcp6_override_len > 0;
		    opt++, ifo->dhcp6_override_len--)
			free_dhcp_opt_embenc(opt);
		free(ifo->dhcp6_override);
		for (vo = ifo->vivco;
		    ifo->vivco_len > 0;
		    vo++, ifo->vivco_len--)
			free(vo->data);
		free(ifo->vivco);
		for (opt = ifo->vivso_override;
		    ifo->vivso_override_len > 0;
		    opt++, ifo->vivso_override_len--)
			free_dhcp_opt_embenc(opt);
		free(ifo->vivso_override);

#ifdef INET6
		for (; ifo->ia_len > 0; ifo->ia_len--)
			free(ifo->ia[ifo->ia_len - 1].sla);
#endif
		free(ifo->ia);

		while ((token = TAILQ_FIRST(&ifo->auth.tokens))) {
			TAILQ_REMOVE(&ifo->auth.tokens, token, next);
			if (token->realm_len)
				free(token->realm);
			free(token->key);
			free(token);
		}
		free(ifo);
	}
}
