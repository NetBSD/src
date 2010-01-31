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

#include <sys/types.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "common.h"
#include "if-options.h"
#include "net.h"

/* These options only make sense in the config file, so don't use any
   valid short options for them */
#define O_BASE		MAX('z', 'Z') + 1
#define O_ARPING	O_BASE + 1
#define O_FALLBACK	O_BASE + 2

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
	{"destination",     required_argument, NULL, 'N'},
	{"nooption",        optional_argument, NULL, 'O'},
	{"require",         required_argument, NULL, 'Q'},
	{"static",          required_argument, NULL, 'S'},
	{"test",            no_argument,       NULL, 'T'},
	{"variables",       no_argument,       NULL, 'V'},
	{"whitelist",       required_argument, NULL, 'W'},
	{"blacklist",       required_argument, NULL, 'X'},
	{"denyinterfaces",  required_argument, NULL, 'Z'},
	{"arping",          required_argument, NULL, O_ARPING},
	{"fallback",        required_argument, NULL, O_FALLBACK},
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
	char *match = NULL, *p;

	match = xstrdup(value);
	p = strchr(match, '=');
	if (p)
		*p++ = '\0';
	l = strlen(match);

	while (lst && lst[i]) {
		if (match && strncmp(lst[i], match, l) == 0) {
			if (uniq) {
				free(lst[i]);
				lst[i] = xstrdup(value);
			} else {
				/* Append a space and the value to it */
				l = strlen(lst[i]);
				lv = strlen(p);
				lst[i] = xrealloc(lst[i], l + lv + 2);
				lst[i][l] = ' ';
				memcpy(lst[i] + l + 1, p, lv);
				lst[i][l + lv + 1] = '\0';
			}
			free(match);
			return lst[i];
		}
		i++;
	}

	newlist = xrealloc(lst, sizeof(char *) * (i + 2));
	newlist[i] = xstrdup(value);
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
	char **v = argv;
	char *o = xstrdup(arg), *p, *t;

	p = o;
	while ((t = strsep(&p, ", "))) {
		(*argc)++;
		v = xrealloc(v, sizeof(char *) * ((*argc)));
		v[(*argc) - 1] = xstrdup(t);
	}
	free(o);
	return v;	
}

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
		net->s_addr = get_netmask(addr->s_addr);
	return 0;
}

static int
parse_option(struct if_options *ifo, int opt, const char *arg)
{
	int i;
	char *p = NULL, *np;
	ssize_t s;
	struct in_addr addr, addr2;
	struct rt *rt;

	switch(opt) {
	case 'f': /* FALLTHROUGH */
	case 'g': /* FALLTHROUGH */
	case 'n': /* FALLTHROUGH */
	case 'x': /* FALLTHROUGH */
	case 'T': /* We need to handle non interface options */
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
		if (arg) {
			s = parse_string(ifo->hostname,
			    HOSTNAME_MAX_LEN, arg);
			if (s == -1) {
				syslog(LOG_ERR, "hostname: %m");
				return -1;
			}
			if (s != 0 && ifo->hostname[0] == '.') {
				syslog(LOG_ERR,
				    "hostname cannot begin with .");
				return -1;
			}
			ifo->hostname[s] = '\0';
		}
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
		if (make_option_mask(ifo->requestmask, arg, 1) != 0) {
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
		ifo->options |= DHCPCD_REQUEST;
		if (parse_addr(&ifo->req_addr, NULL, arg) != 0)
			return -1;
		ifo->req_mask.s_addr = 0;
		break;
	case 's':
		ifo->options |= DHCPCD_INFORM | DHCPCD_PERSISTENT;
		ifo->options &= ~(DHCPCD_ARP | DHCPCD_STATIC);
		if (arg && *arg != '\0') {
			if (parse_addr(&ifo->req_addr, &ifo->req_mask,
				arg) != 0)
				return -1;
		} else {
			ifo->req_addr.s_addr = 0;
			ifo->req_mask.s_addr = 0;
		}
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
		/* We only set this if we haven't got any interfaces */
		if (!ifaces)
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
		p = xmalloc(sizeof(char) * s);
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
	case 'N':
		if (make_option_mask(ifo->dstmask, arg, 2) != 0) {
			if (errno == EINVAL)
				syslog(LOG_ERR, "option `%s' does not take"
				    " an IPv4 address", arg);
			else
				syslog(LOG_ERR, "unknown otpion `%s'", arg);
			return -1;
		}
		break;
	case 'O':
		if (make_option_mask(ifo->requestmask, arg, -1) != 0 ||
		    make_option_mask(ifo->requiremask, arg, -1) != 0 ||
		    make_option_mask(ifo->nomask, arg, 1) != 0)
		{
			syslog(LOG_ERR, "unknown option `%s'", arg);
			return -1;
		}
		break;
	case 'Q':
		if (make_option_mask(ifo->requiremask, arg, 1) != 0 ||
		    make_option_mask(ifo->requestmask, arg, 1) != 0)
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
			if (parse_addr(&ifo->req_addr, &ifo->req_mask, p) != 0)
				return -1;

			ifo->options |= DHCPCD_STATIC;
			ifo->options &= ~DHCPCD_INFORM;
		} else if (strncmp(arg, "routes=", strlen("routes=")) == 0 ||
		    strncmp(arg, "static_routes=", strlen("static_routes=")) == 0 ||
		    strncmp(arg, "classless_static_routes=", strlen("classless_static_routes=")) == 0 ||
		    strncmp(arg, "ms_classless_static_routes=", strlen("ms_classless_static_routes=")) == 0)
		{
			np = strchr(p, ' ');
			if (np == NULL) {
				syslog(LOG_ERR, "all routes need a gateway");
				return -1;
			}
			*np++ = '\0';
			while (*np == ' ')
				np++;
			if (ifo->routes == NULL) {
				rt = ifo->routes = xmalloc(sizeof(*rt));
			} else {
				rt = ifo->routes;
				while (rt->next)
					rt = rt->next;
				rt->next = xmalloc(sizeof(*rt));
				rt = rt->next;
			}
			rt->next = NULL;
			if (parse_addr(&rt->dest, &rt->net, p) == -1 ||
			    parse_addr(&rt->gate, NULL, np) == -1)
				return -1;
		} else if (strncmp(arg, "routers=", strlen("routers=")) == 0) {
			if (ifo->routes == NULL) {
				rt = ifo->routes = xzalloc(sizeof(*rt));
			} else {
				rt = ifo->routes;
				while (rt->next)
					rt = rt->next;
				rt->next = xmalloc(sizeof(*rt));
				rt = rt->next;
			}
			rt->dest.s_addr = INADDR_ANY;
			rt->net.s_addr = INADDR_ANY;
			rt->next = NULL;
			if (parse_addr(&rt->gate, NULL, p) == -1)
				return -1;
		} else {
			s = 0;
			if (ifo->config != NULL) {
				while (ifo->config[s] != NULL) {
					if (strncmp(ifo->config[s], arg,
						p - arg) == 0)
					{
						free(ifo->config[s]);
						ifo->config[s] = xstrdup(arg);
						return 1;
					}
					s++;
				}
			}
			ifo->config = xrealloc(ifo->config,
			    sizeof(char *) * (s + 2));
			ifo->config[s] = xstrdup(arg);
			ifo->config[s + 1] = NULL;
		}
		break;
	case 'W':
		if (parse_addr(&addr, &addr2, arg) != 0)
			return -1;
		if (strchr(arg, '/') == NULL)
			addr2.s_addr = INADDR_BROADCAST;
		ifo->whitelist = xrealloc(ifo->whitelist,
		    sizeof(in_addr_t) * (ifo->whitelist_len + 2));
		ifo->whitelist[ifo->whitelist_len++] = addr.s_addr;
		ifo->whitelist[ifo->whitelist_len++] = addr2.s_addr;
		break;
	case 'X':
		if (parse_addr(&addr, &addr2, arg) != 0)
			return -1;
		if (strchr(arg, '/') == NULL)
			addr2.s_addr = INADDR_BROADCAST;
		ifo->blacklist = xrealloc(ifo->blacklist,
		    sizeof(in_addr_t) * (ifo->blacklist_len + 2));
		ifo->blacklist[ifo->blacklist_len++] = addr.s_addr;
		ifo->blacklist[ifo->blacklist_len++] = addr2.s_addr;
		break;
	case 'Z':
		/* We only set this if we haven't got any interfaces */
		if (!ifaces)
			ifdv = splitv(&ifdc, ifdv, arg);
		break;
	case O_ARPING:
		if (parse_addr(&addr, NULL, arg) != 0)
			return -1;
		ifo->arping = xrealloc(ifo->arping,
		    sizeof(in_addr_t) * (ifo->arping_len + 1));
		ifo->arping[ifo->arping_len++] = addr.s_addr;
		break;
	case O_FALLBACK:
		free(ifo->fallback);
		ifo->fallback = xstrdup(arg);
		break;
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

struct if_options *
read_config(const char *file,
    const char *ifname, const char *ssid, const char *profile)
{
	struct if_options *ifo;
	FILE *f;
	char *line, *option, *p;
	int skip = 0, have_profile = 0;

	/* Seed our default options */
	ifo = xzalloc(sizeof(*ifo));
	ifo->options |= DHCPCD_GATEWAY | DHCPCD_DAEMONISE;
	ifo->options |= DHCPCD_ARP | DHCPCD_IPV4LL | DHCPCD_LINK;
	ifo->timeout = DEFAULT_TIMEOUT;
	ifo->reboot = DEFAULT_REBOOT;
	ifo->metric = -1;
	strlcpy(ifo->script, SCRIPT, sizeof(ifo->script));
	gethostname(ifo->hostname, HOSTNAME_MAX_LEN);
	/* Ensure that the hostname is NULL terminated */
	ifo->hostname[HOSTNAME_MAX_LEN] = '\0';
	if (strcmp(ifo->hostname, "(none)") == 0 ||
	    strcmp(ifo->hostname, "localhost") == 0)
		ifo->hostname[0] = '\0';
	ifo->vendorclassid[0] = snprintf((char *)ifo->vendorclassid + 1,
	    VENDORCLASSID_MAX_LEN,
	    "%s %s", PACKAGE, VERSION);

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
		if (parse_config_line(ifo, option, line) != 1)
			break;
	}
	fclose(f);

	if (profile && !have_profile) {
		free_options(ifo);
		errno = ENOENT;
		ifo = NULL;
	}

	/* Terminate the encapsulated options */
	if (ifo && ifo->vendor[0] && !(ifo->options & DHCPCD_VENDORRAW)) {
		ifo->vendor[0]++;
		ifo->vendor[ifo->vendor[0]] = DHO_END;
	}
	return ifo;
}

int
add_options(struct if_options *ifo, int argc, char **argv)
{
	int oi, opt, r = 1;

	optind = 0;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		r = parse_option(ifo, opt, optarg);
		if (r != 1)
			break;
	}
	/* Terminate the encapsulated options */
	if (r == 1 && ifo->vendor[0] && !(ifo->options & DHCPCD_VENDORRAW)) {
		ifo->vendor[0]++;
		ifo->vendor[ifo->vendor[0]] = DHO_END;
	}
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
		free_routes(ifo->routes);
		free(ifo->arping);
		free(ifo->blacklist);
		free(ifo->fallback);
		free(ifo);
	}
}
