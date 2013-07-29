#include <sys/cdefs.h>
 __RCSID("$NetBSD: if-options.c,v 1.1.1.20 2013/07/29 20:35:32 roy Exp $");

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
#include <sys/types.h>

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
#include "if-options.h"
#include "ipv4.h"
#include "platform.h"

unsigned long long options = 0;

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
	{"waitip",          no_argument,       NULL, 'w'},
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
	{"noalias",         no_argument,       NULL, O_NOALIAS},
	{"ia_na",           no_argument,       NULL, O_IA_NA},
	{"ia_ta",           no_argument,       NULL, O_IA_TA},
	{"ia_pd",           no_argument,       NULL, O_IA_PD},
	{"hostname_short",  no_argument,       NULL, O_HOSTNAME_SHORT},
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
	if (p)
		*p++ = '\0';
	l = strlen(match);

	while (lst && lst[i]) {
		if (match && strncmp(lst[i], match, l) == 0) {
			if (uniq) {
				n = strdup(value);
				if (n == NULL) {
					syslog(LOG_ERR, "%s: %m", __func__);
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

	n = strdup(value);
	if (n == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	newlist = realloc(lst, sizeof(char *) * (i + 2));
	if (newlist == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	newlist[i] = n;
	newlist[i + 1] = NULL;
	ifo->environ = newlist;
	free(match);
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
		*sbuf++ = 0;
		l++;
	}
	c[3] = '\0';
	while (*str) {
		if (++l > slen) {
			errno = ENOBUFS;
			return -1;
		}
		if (*str == '\\') {
			str++;
			switch(*str) {
			case '\0':
				break;
			case 'b':
				*sbuf++ = '\b';
				str++;
				break;
			case 'n':
				*sbuf++ = '\n';
				str++;
				break;
			case 'r':
				*sbuf++ = '\r';
				str++;
				break;
			case 't':
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
				if (c[1] != '\0') {
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
				if (c[2] != '\0') {
					i = strtol(c, NULL, 8);
					if (i > 255)
						i = 255;
					*sbuf ++= i;
				} else
					l--;
				break;
			default:
				*sbuf++ = *str++;
			}
		} else
			*sbuf++ = *str++;
	}
	if (punt_last) {
		*--sbuf = '\0';
		l--;
	}
	return l;
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
			return NULL;
		}
		(*argc)++;
		n = realloc(v, sizeof(char *) * ((*argc)));
		if (n == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return NULL;
		}
		v = n;
		v[(*argc) - 1] = nt;
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
	else if (net != NULL)
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
set_option_space(const char *arg, const struct dhcp_opt **d,
    struct if_options *ifo,
    uint8_t *request[], uint8_t *require[], uint8_t *no[])
{

#ifdef INET6
	if (strncmp(arg, "dhcp6_", strlen("dhcp6_")) == 0) {
		*d = dhcp6_opts;
		*request = ifo->requestmask6;
		*require = ifo->requiremask6;
		*no = ifo->nomask6;
		return arg + strlen("dhcp6_");
	}
#endif

#ifdef INET
	*d = dhcp_opts;
#else
	*d = NULL;
#endif
	*request = ifo->requestmask;
	*require = ifo->requiremask;
	*no = ifo->nomask;
	return arg;
}

static int
parse_option(struct if_options *ifo, int opt, const char *arg)
{
	int i;
	char *p = NULL, *fp, *np, **nconf;
	ssize_t s;
	struct in_addr addr, addr2;
	in_addr_t *naddr;
	struct rt *rt;
	const struct dhcp_opt *d;
	uint8_t *request, *require, *no;
#ifdef INET6
	long l;
	uint32_t u32;
	size_t sl;
	struct if_iaid *iaid;
	uint8_t _iaid[4];
	struct if_sla *sla, *slap;
#endif

	i = 0;
	switch(opt) {
	case 'f': /* FALLTHROUGH */
	case 'g': /* FALLTHROUGH */
	case 'n': /* FALLTHROUGH */
	case 'x': /* FALLTHROUGH */
	case 'T': /* FALLTHROUGH */
	case 'U': /* We need to handle non interface options */
		break;
	case 'b':
		ifo->options |= DHCPCD_BACKGROUND;
		break;
	case 'c':
		strlcpy(ifo->script, arg, sizeof(ifo->script));
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
		ifo->leasetime = (uint32_t)strtol(arg, NULL, 0);
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
		arg = set_option_space(arg, &d, ifo, &request, &require, &no);
		if (make_option_mask(d, request, arg, 1) != 0) {
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
			syslog(LOG_ERR, "invalid vendor format");
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

		*p = '\0';
		i = atoint(arg);
		arg = p + 1;
		if (i < 1 || i > 254) {
			syslog(LOG_ERR, "vendor option should be between"
			    " 1 and 254 inclusive");
			return -1;
		}
		s = VENDOR_MAX_LEN - ifo->vendor[0] - 2;
		if (inet_aton(arg, &addr) == 1) {
			if (s < 6) {
				s = -1;
				errno = ENOBUFS;
			} else
				memcpy(ifo->vendor + ifo->vendor[0] + 3,
				    &addr.s_addr, sizeof(addr.s_addr));
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
		break;
	case 'y':
		ifo->reboot = atoint(arg);
		if (ifo->reboot < 0) {
			syslog(LOG_ERR, "reboot must be a positive value");
			return -1;
		}
		break;
	case 'z':
		ifav = splitv(&ifac, ifav, arg);
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
	case 'O':
		arg = set_option_space(arg, &d, ifo, &request, &require, &no);
		if (make_option_mask(d, request, arg, -1) != 0 ||
		    make_option_mask(d, require, arg, -1) != 0 ||
		    make_option_mask(d, no, arg, 1) != 0)
		{
			syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case 'Q':
		arg = set_option_space(arg, &d, ifo, &request, &require, &no);
		if (make_option_mask(d, require, arg, 1) != 0 ||
		    make_option_mask(d, request, arg, 1) != 0)
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
			fp = np = strchr(p, ' ');
			if (np == NULL) {
				syslog(LOG_ERR, "all routes need a gateway");
				return -1;
			}
			*np++ = '\0';
			while (*np == ' ')
				np++;
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
		ifdv = splitv(&ifdc, ifdv, arg);
		break;
	case '4':
		ifo->options &= ~DHCPCD_IPV6;
		ifo->options |= DHCPCD_IPV4;
		break;
	case '6':
		ifo->options &= ~DHCPCD_IPV4;
		ifo->options |= DHCPCD_IPV6;
		break;
#ifdef INET
	case O_ARPING:
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
		break;
	case O_DESTINATION:
		if (make_option_mask(dhcp_opts, ifo->dstmask, arg, 2) != 0) {
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
		if (i == 0)
			i = D6_OPTION_IA_PD;
		ifo->options |= DHCPCD_IA_FORCED;
		if (ifo->ia_type != 0 && ifo->ia_type != i) {
			syslog(LOG_ERR, "cannot specify a different IA type");
			return -1;
		}
		ifo->ia_type = i;
		if (arg == NULL)
			break;
		fp = strchr(arg, ' ');
		if (fp)
			*fp++ = '\0';
		errno = 0;
		l = strtol(arg, &np, 0);
		if (l >= 0 && l <= (long)UINT32_MAX &&
		    errno == 0 && *np == '\0')
		{
			u32 = htonl(l);
			memcpy(&_iaid, &u32, sizeof(_iaid));
			goto got_iaid;
		}
		if ((s = parse_string((char *)_iaid, sizeof(_iaid), arg)) < 1) {
			syslog(LOG_ERR, "%s: invalid IAID", arg);
			return -1;
		}
		if (s < 4)
			_iaid[3] = '\0';
		if (s < 3)
			_iaid[2] = '\0';
		if (s < 2)
			_iaid[1] = '\0';
got_iaid:
		iaid = NULL;
		for (sl = 0; sl < ifo->iaid_len; sl++) {
			if (ifo->iaid[sl].iaid[0] == _iaid[0] &&
			    ifo->iaid[sl].iaid[1] == _iaid[1] &&
			    ifo->iaid[sl].iaid[2] == _iaid[2] &&
			    ifo->iaid[sl].iaid[3] == _iaid[3])
			{
			        iaid = &ifo->iaid[sl];
				break;
			}
		}
		if (iaid == NULL) {
			iaid = realloc(ifo->iaid,
			    sizeof(*ifo->iaid) * (ifo->iaid_len + 1));
			if (iaid == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ifo->iaid = iaid;
			iaid = &ifo->iaid[ifo->iaid_len++];
			iaid->iaid[0] = _iaid[0];
			iaid->iaid[1] = _iaid[1];
			iaid->iaid[2] = _iaid[2];
			iaid->iaid[3] = _iaid[3];
			iaid->sla = NULL;
			iaid->sla_len = 0;
		}
		for (p = fp; p; p = fp) {
			fp = strchr(p, ' ');
			if (fp)
				*fp++ = '\0';
			sla = realloc(iaid->sla,
			    sizeof(*iaid->sla) * (iaid->sla_len + 1));
			if (sla == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			iaid->sla = sla;
			sla = &iaid->sla[iaid->sla_len++];
			np = strchr(p, '/');
			if (np)
				*np++ = '\0';
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
				for (sl = 0; sl < iaid->sla_len - 1; sl++) {
					slap = &iaid->sla[sl];
					if (slap->sla_set == 0 &&
					    strcmp(slap->ifname, sla->ifname)
					    == 0)
					{
						syslog(LOG_WARNING,
						    "%s: cannot specify the "
						    "same interface twice with "
						    "an automatic SLA",
						    sla->ifname);
						iaid->sla_len--;
						break;
					}
				}
			}
		}
		break;
	case O_HOSTNAME_SHORT:
		ifo->options |= DHCPCD_HOSTNAME | DHCPCD_HOSTNAME_SHORT;
		break;
#endif
	default:
		return 0;
	}

	return 1;
}

static int
parse_config_line(struct if_options *ifo, const char *opt, char *line)
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

		return parse_option(ifo, cf_options[i].val, line);
	}

	fprintf(stderr, PACKAGE ": unknown option -- %s\n", opt);
	return -1;
}

static void
finish_config(struct if_options *ifo, const char *ifname)
{

	/* Terminate the encapsulated options */
	if (ifo->vendor[0] && !(ifo->options & DHCPCD_VENDORRAW)) {
		ifo->vendor[0]++;
		ifo->vendor[ifo->vendor[0]] = DHO_END;
	}

#ifdef INET6
	if (!(ifo->options & DHCPCD_IPV6))
		ifo->options &= ~DHCPCD_IPV6RS;

	if (ifname && ifo->iaid_len == 0 && ifo->options & DHCPCD_IPV6) {
		ifo->iaid = malloc(sizeof(*ifo->iaid));
		if (ifo->iaid == NULL)
			syslog(LOG_ERR, "%s: %m", __func__);
		else {
			if (ifo->ia_type == 0)
				ifo->ia_type = D6_OPTION_IA_NA;
			ifo->iaid_len = strlen(ifname);
			if (ifo->iaid_len <= sizeof(ifo->iaid->iaid)) {
				strncpy((char *)ifo->iaid->iaid, ifname,
					sizeof(ifo->iaid->iaid));
				memset(ifo->iaid->iaid + ifo->iaid_len, 0,
					sizeof(ifo->iaid->iaid) -ifo->iaid_len);
			} else {
				uint32_t idx = if_nametoindex(ifname);
				memcpy(ifo->iaid->iaid, &idx, sizeof(idx));
			}
			ifo->iaid_len = 1;
			ifo->iaid->sla = NULL;
			ifo->iaid->sla_len = 0;
		}
	}
#endif
}

struct if_options *
read_config(const char *file,
    const char *ifname, const char *ssid, const char *profile)
{
	struct if_options *ifo;
	FILE *f;
	char *line, *option, *p;
	int skip = 0, have_profile = 0;

	/* Seed our default options */
	ifo = calloc(1, sizeof(*ifo));
	if (ifo == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	ifo->options |= DHCPCD_DAEMONISE | DHCPCD_LINK;
#ifdef INET
	ifo->options |= DHCPCD_IPV4 | DHCPCD_IPV4LL;
	ifo->options |= DHCPCD_GATEWAY | DHCPCD_ARP;
#endif
#ifdef INET6
	ifo->options |= DHCPCD_IPV6 | DHCPCD_IPV6RS | DHCPCD_IPV6RA_REQRDNSS;
	ifo->dadtransmits = ipv6_dadtransmits(ifname);
#endif
	ifo->timeout = DEFAULT_TIMEOUT;
	ifo->reboot = DEFAULT_REBOOT;
	ifo->metric = -1;
	strlcpy(ifo->script, SCRIPT, sizeof(ifo->script));

	ifo->vendorclassid[0] = strlen(vendor);
	memcpy(ifo->vendorclassid + 1, vendor, ifo->vendorclassid[0]);

	/* Parse our options file */
	f = fopen(file ? file : CONFIG, "r");
	if (f == NULL) {
		if (file != NULL)
			syslog(LOG_ERR, "fopen `%s': %m", file);
		return ifo;
	}

	while ((line = get_line(f))) {
		option = strsep(&line, " \t");
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
		parse_config_line(ifo, option, line);
	}
	fclose(f);

	if (profile && !have_profile) {
		free_options(ifo);
		errno = ENOENT;
		ifo = NULL;
	}

	finish_config(ifo, ifname);
	return ifo;
}

int
add_options(struct if_options *ifo, int argc, char **argv)
{
	int oi, opt, r;

	if (argc == 0)
		return 1;

	optind = 0;
	r = 1;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		r = parse_option(ifo, opt, optarg);
		if (r != 1)
			break;
	}

	finish_config(ifo, NULL);
	return r;
}

void
free_options(struct if_options *ifo)
{
	size_t i;

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
		free(ifo->arping);
		free(ifo->blacklist);
		free(ifo->fallback);
#ifdef INET6
		for (i = 0; i < ifo->iaid_len; i++)
			free(ifo->iaid[i].sla);
		free(ifo->iaid);
#endif
		free(ifo);
	}
}
