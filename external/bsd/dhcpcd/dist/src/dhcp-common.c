/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

#include <sys/stat.h>
#include <sys/utsname.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/nameser.h> /* after normal includes for sunos */

#include "config.h"

#include "common.h"
#include "dhcp-common.h"
#include "dhcp.h"
#include "if.h"
#include "ipv6.h"
#include "logerr.h"

/* Support very old arpa/nameser.h as found in OpenBSD */
#ifndef NS_MAXDNAME
#define NS_MAXCDNAME MAXCDNAME
#define NS_MAXDNAME MAXDNAME
#define NS_MAXLABEL MAXLABEL
#endif

const char *
dhcp_get_hostname(char *buf, size_t buf_len, const struct if_options *ifo)
{

	if (ifo->hostname[0] == '\0') {
		if (gethostname(buf, buf_len) != 0)
			return NULL;
		buf[buf_len - 1] = '\0';
	} else
		strlcpy(buf, ifo->hostname, buf_len);

	/* Deny sending of these local hostnames */
	if (buf[0] == '\0' || buf[0] == '.' ||
	    strcmp(buf, "(none)") == 0 ||
	    strcmp(buf, "localhost") == 0 ||
	    strncmp(buf, "localhost.", strlen("localhost.")) == 0)
		return NULL;

	/* Shorten the hostname if required */
	if (ifo->options & DHCPCD_HOSTNAME_SHORT) {
		char *hp;

		hp = strchr(buf, '.');
		if (hp != NULL)
			*hp = '\0';
	}

	return buf;
}

void
dhcp_print_option_encoding(const struct dhcp_opt *opt, int cols)
{

	while (cols < 40) {
		putchar(' ');
		cols++;
	}
	putchar('\t');
	if (opt->type & OT_EMBED)
		printf(" embed");
	if (opt->type & OT_ENCAP)
		printf(" encap");
	if (opt->type & OT_INDEX)
		printf(" index");
	if (opt->type & OT_ARRAY)
		printf(" array");
	if (opt->type & OT_UINT8)
		printf(" uint8");
	else if (opt->type & OT_INT8)
		printf(" int8");
	else if (opt->type & OT_UINT16)
		printf(" uint16");
	else if (opt->type & OT_INT16)
		printf(" int16");
	else if (opt->type & OT_UINT32)
		printf(" uint32");
	else if (opt->type & OT_INT32)
		printf(" int32");
	else if (opt->type & OT_ADDRIPV4)
		printf(" ipaddress");
	else if (opt->type & OT_ADDRIPV6)
		printf(" ip6address");
	else if (opt->type & OT_FLAG)
		printf(" flag");
	else if (opt->type & OT_BITFLAG)
		printf(" bitflags");
	else if (opt->type & OT_RFC1035)
		printf(" domain");
	else if (opt->type & OT_DOMAIN)
		printf(" dname");
	else if (opt->type & OT_ASCII)
		printf(" ascii");
	else if (opt->type & OT_RAW)
		printf(" raw");
	else if (opt->type & OT_BINHEX)
		printf(" binhex");
	else if (opt->type & OT_STRING)
		printf(" string");
	if (opt->type & OT_RFC3361)
		printf(" rfc3361");
	if (opt->type & OT_RFC3442)
		printf(" rfc3442");
	if (opt->type & OT_REQUEST)
		printf(" request");
	if (opt->type & OT_NOREQ)
		printf(" norequest");
	putchar('\n');
}

struct dhcp_opt *
vivso_find(uint32_t iana_en, const void *arg)
{
	const struct interface *ifp;
	size_t i;
	struct dhcp_opt *opt;

	ifp = arg;
	for (i = 0, opt = ifp->options->vivso_override;
	    i < ifp->options->vivso_override_len;
	    i++, opt++)
		if (opt->option == iana_en)
			return opt;
	for (i = 0, opt = ifp->ctx->vivso;
	    i < ifp->ctx->vivso_len;
	    i++, opt++)
		if (opt->option == iana_en)
			return opt;
	return NULL;
}

ssize_t
dhcp_vendor(char *str, size_t len)
{
	struct utsname utn;
	char *p;
	int l;

	if (uname(&utn) == -1)
		return (ssize_t)snprintf(str, len, "%s-%s",
		    PACKAGE, VERSION);
	p = str;
	l = snprintf(p, len,
	    "%s-%s:%s-%s:%s", PACKAGE, VERSION,
	    utn.sysname, utn.release, utn.machine);
	if (l == -1 || (size_t)(l + 1) > len)
		return -1;
	p += l;
	len -= (size_t)l;
	l = if_machinearch(p, len);
	if (l == -1 || (size_t)(l + 1) > len)
		return -1;
	p += l;
	return p - str;
}

int
make_option_mask(const struct dhcp_opt *dopts, size_t dopts_len,
    const struct dhcp_opt *odopts, size_t odopts_len,
    uint8_t *mask, const char *opts, int add)
{
	char *token, *o, *p;
	const struct dhcp_opt *opt;
	int match, e;
	unsigned int n;
	size_t i;

	if (opts == NULL)
		return -1;
	o = p = strdup(opts);
	while ((token = strsep(&p, ", "))) {
		if (*token == '\0')
			continue;
		match = 0;
		for (i = 0, opt = odopts; i < odopts_len; i++, opt++) {
			if (opt->var == NULL || opt->option == 0)
				continue; /* buggy dhcpcd-definitions.conf */
			if (strcmp(opt->var, token) == 0)
				match = 1;
			else {
				n = (unsigned int)strtou(token, NULL, 0,
				    0, UINT_MAX, &e);
				if (e == 0 && opt->option == n)
					match = 1;
			}
			if (match)
				break;
		}
		if (match == 0) {
			for (i = 0, opt = dopts; i < dopts_len; i++, opt++) {
				if (strcmp(opt->var, token) == 0)
				        match = 1;
				else {
					n = (unsigned int)strtou(token, NULL, 0,
					    0, UINT_MAX, &e);
					if (e == 0 && opt->option == n)
						match = 1;
				}
				if (match)
					break;
			}
		}
		if (!match || !opt->option) {
			free(o);
			errno = ENOENT;
			return -1;
		}
		if (add == 2 && !(opt->type & OT_ADDRIPV4)) {
			free(o);
			errno = EINVAL;
			return -1;
		}
		if (add == 1 || add == 2)
			add_option_mask(mask, opt->option);
		else
			del_option_mask(mask, opt->option);
	}
	free(o);
	return 0;
}

size_t
encode_rfc1035(const char *src, uint8_t *dst)
{
	uint8_t *p;
	uint8_t *lp;
	size_t len;
	uint8_t has_dot;

	if (src == NULL || *src == '\0')
		return 0;

	if (dst) {
		p = dst;
		lp = p++;
	}
	/* Silence bogus GCC warnings */
	else
		p = lp = NULL;

	len = 1;
	has_dot = 0;
	for (; *src; src++) {
		if (*src == '\0')
			break;
		if (*src == '.') {
			/* Skip the trailing . */
			if (src[1] == '\0')
				break;
			has_dot = 1;
			if (dst) {
				*lp = (uint8_t)(p - lp - 1);
				if (*lp == '\0')
					return len;
				lp = p++;
			}
		} else if (dst)
			*p++ = (uint8_t)*src;
		len++;
	}

	if (dst) {
		*lp = (uint8_t)(p - lp - 1);
		if (has_dot)
			*p++ = '\0';
	}

	if (has_dot)
		len++;

	return len;
}

/* Decode an RFC1035 DNS search order option into a space
 * separated string. Returns length of string (including
 * terminating zero) or zero on error. out may be NULL
 * to just determine output length. */
ssize_t
decode_rfc1035(char *out, size_t len, const uint8_t *p, size_t pl)
{
	const char *start;
	size_t start_len, l, d_len, o_len;
	const uint8_t *r, *q = p, *e;
	int hops;
	uint8_t ltype;

	o_len = 0;
	start = out;
	start_len = len;
	q = p;
	e = p + pl;
	while (q < e) {
		r = NULL;
		d_len = 0;
		hops = 0;
		/* Check we are inside our length again in-case
		 * the name isn't fully qualified (ie, not terminated) */
		while (q < e && (l = (size_t)*q++)) {
			ltype = l & 0xc0;
			if (ltype == 0x80 || ltype == 0x40) {
				/* Currently reserved for future use as noted
				 * in RFC1035 4.1.4 as the 10 and 01
				 * combinations. */
				errno = ENOTSUP;
				return -1;
			}
			else if (ltype == 0xc0) { /* pointer */
				if (q == e) {
					errno = ERANGE;
					return -1;
				}
				l = (l & 0x3f) << 8;
				l |= *q++;
				/* save source of first jump. */
				if (!r)
					r = q;
				hops++;
				if (hops > 255) {
					errno = ERANGE;
					return -1;
				}
				q = p + l;
				if (q >= e) {
					errno = ERANGE;
					return -1;
				}
			} else {
				/* straightforward name segment, add with '.' */
				if (q + l > e) {
					errno = ERANGE;
					return -1;
				}
				if (l > NS_MAXLABEL) {
					errno = EINVAL;
					return -1;
				}
				d_len += l + 1;
				if (out) {
					if (l + 1 > len) {
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

		/* Don't count the trailing NUL */
		if (d_len > NS_MAXDNAME + 1) {
			errno = E2BIG;
			return -1;
		}
		o_len += d_len;

		/* change last dot to space */
		if (out && out != start)
			*(out - 1) = ' ';
		if (r)
			q = r;
	}

	/* change last space to zero terminator */
	if (out) {
		if (out != start)
			*(out - 1) = '\0';
		else if (start_len > 0)
			*out = '\0';
	}

	/* Remove the trailing NUL */
	if (o_len != 0)
		o_len--;

	return (ssize_t)o_len;
}

/* Check for a valid name as per RFC952 and RFC1123 section 2.1 */
static int
valid_domainname(char *lbl, int type)
{
	char *slbl, *lst;
	unsigned char c;
	int start, len, errset;

	if (lbl == NULL || *lbl == '\0') {
		errno = EINVAL;
		return 0;
	}

	slbl = lbl;
	lst = NULL;
	start = 1;
	len = errset = 0;
	for (;;) {
		c = (unsigned char)*lbl++;
		if (c == '\0')
			return 1;
		if (c == ' ') {
			if (lbl - 1 == slbl) /* No space at start */
				break;
			if (!(type & OT_ARRAY))
				break;
			/* Skip to the next label */
			if (!start) {
				start = 1;
				lst = lbl - 1;
			}
			if (len)
				len = 0;
			continue;
		}
		if (c == '.') {
			if (*lbl == '.')
				break;
			len = 0;
			continue;
		}
		if (((c == '-' || c == '_') &&
		    !start && *lbl != ' ' && *lbl != '\0') ||
		    isalnum(c))
		{
			if (++len > NS_MAXLABEL) {
				errno = ERANGE;
				errset = 1;
				break;
			}
		} else
			break;
		if (start)
			start = 0;
	}

	if (!errset)
		errno = EINVAL;
	if (lst) {
		/* At least one valid domain, return it */
		*lst = '\0';
		return 1;
	}
	return 0;
}

/*
 * Prints a chunk of data to a string.
 * PS_SHELL goes as it is these days, it's upto the target to validate it.
 * PS_SAFE has all non ascii and non printables changes to escaped octal.
 */
static const char hexchrs[] = "0123456789abcdef";
ssize_t
print_string(char *dst, size_t len, int type, const uint8_t *data, size_t dl)
{
	char *odst;
	uint8_t c;
	const uint8_t *e;
	size_t bytes;

	odst = dst;
	bytes = 0;
	e = data + dl;

	while (data < e) {
		c = *data++;
		if (type & OT_BINHEX) {
			if (dst) {
				if (len  == 0 || len == 1) {
					errno = ENOBUFS;
					return -1;
				}
				*dst++ = hexchrs[(c & 0xF0) >> 4];
				*dst++ = hexchrs[(c & 0x0F)];
				len -= 2;
			}
			bytes += 2;
			continue;
		}
		if (type & OT_ASCII && (!isascii(c))) {
			errno = EINVAL;
			break;
		}
		if (!(type & (OT_ASCII | OT_RAW | OT_ESCSTRING | OT_ESCFILE)) &&
		    (!isascii(c) && !isprint(c)))
		{
			errno = EINVAL;
			break;
		}
		if ((type & (OT_ESCSTRING | OT_ESCFILE) &&
		    (c == '\\' || !isascii(c) || !isprint(c))) ||
		    (type & OT_ESCFILE && (c == '/' || c == ' ')))
		{
			errno = EINVAL;
			if (c == '\\') {
				if (dst) {
					if (len  == 0 || len == 1) {
						errno = ENOBUFS;
						return -1;
					}
					*dst++ = '\\'; *dst++ = '\\';
					len -= 2;
				}
				bytes += 2;
				continue;
			}
			if (dst) {
				if (len < 5) {
					errno = ENOBUFS;
					return -1;
				}
				*dst++ = '\\';
		                *dst++ = (char)(((c >> 6) & 03) + '0');
		                *dst++ = (char)(((c >> 3) & 07) + '0');
		                *dst++ = (char)(( c       & 07) + '0');
				len -= 4;
			}
			bytes += 4;
		} else {
			if (dst) {
				if (len == 0) {
					errno = ENOBUFS;
					return -1;
				}
				*dst++ = (char)c;
				len--;
			}
			bytes++;
		}
	}

	/* NULL */
	if (dst) {
		if (len == 0) {
			errno = ENOBUFS;
			return -1;
		}
		*dst = '\0';

		/* Now we've printed it, validate the domain */
		if (type & OT_DOMAIN && !valid_domainname(odst, type)) {
			*odst = '\0';
			return 1;
		}

	}

	return (ssize_t)bytes;
}

#define ADDR6SZ		16
static ssize_t
dhcp_optlen(const struct dhcp_opt *opt, size_t dl)
{
	size_t sz;

	if (opt->type & OT_ADDRIPV6)
		sz = ADDR6SZ;
	else if (opt->type & (OT_INT32 | OT_UINT32 | OT_ADDRIPV4))
		sz = sizeof(uint32_t);
	else if (opt->type & (OT_INT16 | OT_UINT16))
		sz = sizeof(uint16_t);
	else if (opt->type & (OT_INT8 | OT_UINT8 | OT_BITFLAG))
		sz = sizeof(uint8_t);
	else if (opt->type & OT_FLAG)
		return 0;
	else {
		/* All other types are variable length */
		if (opt->len) {
			if ((size_t)opt->len > dl) {
				errno = EOVERFLOW;
				return -1;
			}
			return (ssize_t)opt->len;
		}
		return (ssize_t)dl;
	}
	if (dl < sz) {
		errno = EOVERFLOW;
		return -1;
	}

	/* Trim any extra data.
	 * Maybe we need a settng to reject DHCP options with extra data? */
	if (opt->type & OT_ARRAY)
		return (ssize_t)(dl - (dl % sz));
	return (ssize_t)sz;
}

/* It's possible for DHCPv4 to contain an IPv6 address */
static ssize_t
ipv6_printaddr(char *s, size_t sl, const uint8_t *d, const char *ifname)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	size_t l;

	p = inet_ntop(AF_INET6, d, buf, sizeof(buf));
	if (p == NULL)
		return -1;

	l = strlen(p);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80)
		l += 1 + strlen(ifname);

	if (s == NULL)
		return (ssize_t)l;

	if (sl < l) {
		errno = ENOMEM;
		return -1;
	}

	s += strlcpy(s, p, sl);
	if (d[0] == 0xfe && (d[1] & 0xc0) == 0x80) {
		*s++ = '%';
		s += strlcpy(s, ifname, sl);
	}
	*s = '\0';
	return (ssize_t)l;
}

static ssize_t
print_option(char *s, size_t len, const struct dhcp_opt *opt,
    const uint8_t *data, size_t dl, const char *ifname)
{
	const uint8_t *e, *t;
	uint16_t u16;
	int16_t s16;
	uint32_t u32;
	int32_t s32;
	struct in_addr addr;
	ssize_t bytes = 0, sl;
	size_t l;
#ifdef INET
	char *tmp;
#endif

	if (opt->type & OT_RFC1035) {
		sl = decode_rfc1035(s, len, data, dl);
		if (sl == 0 || sl == -1)
			return sl;
		if (s != NULL) {
			if (valid_domainname(s, opt->type) == -1)
				return -1;
		}
		return sl;
	}

#ifdef INET
	if (opt->type & OT_RFC3361) {
		if ((tmp = decode_rfc3361(data, dl)) == NULL)
			return -1;
		l = strlen(tmp);
		sl = print_string(s, len, opt->type, (uint8_t *)tmp, l);
		free(tmp);
		return sl;
	}

	if (opt->type & OT_RFC3442)
		return decode_rfc3442(s, len, data, dl);
#endif

	if (opt->type & OT_STRING)
		return print_string(s, len, opt->type, data, dl);

	if (opt->type & OT_FLAG) {
		if (s) {
			*s++ = '1';
			*s = '\0';
		}
		return 1;
	}

	if (opt->type & OT_BITFLAG) {
		/* bitflags are a string, MSB first, such as ABCDEFGH
		 * where A is 10000000, B is 01000000, etc. */
		bytes = 0;
		for (l = 0, sl = sizeof(opt->bitflags) - 1;
		    l < sizeof(opt->bitflags);
		    l++, sl--)
		{
			/* Don't print NULL or 0 flags */
			if (opt->bitflags[l] != '\0' &&
			    opt->bitflags[l] != '0' &&
			    *data & (1 << sl))
			{
				if (s)
					*s++ = opt->bitflags[l];
				bytes++;
			}
		}
		if (s)
			*s = '\0';
		return bytes;
	}

	if (!s) {
		if (opt->type & OT_UINT8)
			l = 3;
		else if (opt->type & OT_INT8)
			l = 4;
		else if (opt->type & OT_UINT16) {
			l = 5;
			dl /= 2;
		} else if (opt->type & OT_INT16) {
			l = 6;
			dl /= 2;
		} else if (opt->type & OT_UINT32) {
			l = 10;
			dl /= 4;
		} else if (opt->type & OT_INT32) {
			l = 11;
			dl /= 4;
		} else if (opt->type & OT_ADDRIPV4) {
			l = 16;
			dl /= 4;
		} else if (opt->type & OT_ADDRIPV6) {
			e = data + dl;
			l = 0;
			while (data < e) {
				if (l)
					l++; /* space */
				sl = ipv6_printaddr(NULL, 0, data, ifname);
				if (sl == -1)
					return l == 0 ? -1 : (ssize_t)l;
				l += (size_t)sl;
				data += 16;
			}
			return (ssize_t)l;
		} else {
			errno = EINVAL;
			return -1;
		}
		return (ssize_t)(l * dl);
	}

	t = data;
	e = data + dl;
	while (data < e) {
		if (data != t) {
			*s++ = ' ';
			bytes++;
			len--;
		}
		if (opt->type & OT_UINT8) {
			sl = snprintf(s, len, "%u", *data);
			data++;
		} else if (opt->type & OT_INT8) {
			sl = snprintf(s, len, "%d", *data);
			data++;
		} else if (opt->type & OT_UINT16) {
			memcpy(&u16, data, sizeof(u16));
			u16 = ntohs(u16);
			sl = snprintf(s, len, "%u", u16);
			data += sizeof(u16);
		} else if (opt->type & OT_INT16) {
			memcpy(&u16, data, sizeof(u16));
			s16 = (int16_t)ntohs(u16);
			sl = snprintf(s, len, "%d", s16);
			data += sizeof(u16);
		} else if (opt->type & OT_UINT32) {
			memcpy(&u32, data, sizeof(u32));
			u32 = ntohl(u32);
			sl = snprintf(s, len, "%u", u32);
			data += sizeof(u32);
		} else if (opt->type & OT_INT32) {
			memcpy(&u32, data, sizeof(u32));
			s32 = (int32_t)ntohl(u32);
			sl = snprintf(s, len, "%d", s32);
			data += sizeof(u32);
		} else if (opt->type & OT_ADDRIPV4) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			sl = snprintf(s, len, "%s", inet_ntoa(addr));
			data += sizeof(addr.s_addr);
		} else if (opt->type & OT_ADDRIPV6) {
			sl = ipv6_printaddr(s, len, data, ifname);
			data += 16;
		} else {
			errno = EINVAL;
			return -1;
		}
		if (sl == -1)
			return bytes == 0 ? -1 : bytes;
		len -= (size_t)sl;
		bytes += sl;
		s += sl;
	}

	return bytes;
}

int
dhcp_set_leasefile(char *leasefile, size_t len, int family,
    const struct interface *ifp)
{
	char ssid[1 + (IF_SSIDLEN * 4) + 1]; /* - prefix and NUL terminated. */

	if (ifp->name[0] == '\0') {
		strlcpy(leasefile, ifp->ctx->pidfile, len);
		return 0;
	}

	switch (family) {
	case AF_INET:
	case AF_INET6:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (ifp->wireless) {
		ssid[0] = '-';
		print_string(ssid + 1, sizeof(ssid) - 1,
		    OT_ESCFILE,
		    (const uint8_t *)ifp->ssid, ifp->ssid_len);
	} else
		ssid[0] = '\0';
	return snprintf(leasefile, len,
	    family == AF_INET ? LEASEFILE : LEASEFILE6,
	    ifp->name, ssid);
}

static size_t
dhcp_envoption1(char **env, const char *prefix,
    const struct dhcp_opt *opt, int vname, const uint8_t *od, size_t ol,
    const char *ifname)
{
	ssize_t len;
	size_t e;
	char *v, *val;
	int r;

	/* Ensure a valid length */
	ol = (size_t)dhcp_optlen(opt, ol);
	if ((ssize_t)ol == -1)
		return 0;

	len = print_option(NULL, 0, opt, od, ol, ifname);
	if (len < 0)
		return 0;
	if (vname)
		e = strlen(opt->var) + 1;
	else
		e = 0;
	if (prefix)
		e += strlen(prefix);
	e += (size_t)len + 2;
	if (env == NULL)
		return e;
	v = val = *env = malloc(e);
	if (v == NULL)
		return 0;
	if (vname)
		r = snprintf(val, e, "%s_%s=", prefix, opt->var);
	else
		r = snprintf(val, e, "%s=", prefix);
	if (r != -1 && len != 0) {
		v += r;
		if (print_option(v, (size_t)len + 1, opt, od, ol, ifname) == -1)
			r = -1;
	}
	if (r == -1) {
		free(val);
		return 0;
	}
	return e;
}

size_t
dhcp_envoption(struct dhcpcd_ctx *ctx, char **env, const char *prefix,
    const char *ifname, struct dhcp_opt *opt,
    const uint8_t *(*dgetopt)(struct dhcpcd_ctx *,
    size_t *, unsigned int *, size_t *,
    const uint8_t *, size_t, struct dhcp_opt **),
    const uint8_t *od, size_t ol)
{
	size_t e, i, n, eos, eol;
	ssize_t eo;
	unsigned int eoc;
	const uint8_t *eod;
	int ov;
	struct dhcp_opt *eopt, *oopt;
	char *pfx;

	/* If no embedded or encapsulated options, it's easy */
	if (opt->embopts_len == 0 && opt->encopts_len == 0) {
		if (!(opt->type & OT_RESERVED)) {
			if (dhcp_envoption1(env == NULL ? NULL : &env[0],
			    prefix, opt, 1, od, ol, ifname))
				return 1;
			else
				logerr("%s: %s %d",
				    ifname, __func__, opt->option);
		}
		return 0;
	}

	/* Create a new prefix based on the option */
	if (env) {
		if (opt->type & OT_INDEX) {
			if (opt->index > 999) {
				errno = ENOBUFS;
				logerr(__func__);
				return 0;
			}
		}
		e = strlen(prefix) + strlen(opt->var) + 2 +
		    (opt->type & OT_INDEX ? 3 : 0);
		pfx = malloc(e);
		if (pfx == NULL) {
			logerr(__func__);
			return 0;
		}
		if (opt->type & OT_INDEX)
			snprintf(pfx, e, "%s_%s%d", prefix,
			    opt->var, ++opt->index);
		else
			snprintf(pfx, e, "%s_%s", prefix, opt->var);
	} else
		pfx = NULL;

	/* Embedded options are always processed first as that
	 * is a fixed layout */
	n = 0;
	for (i = 0, eopt = opt->embopts; i < opt->embopts_len; i++, eopt++) {
		eo = dhcp_optlen(eopt, ol);
		if (eo == -1) {
			if (env == NULL)
				logerrx("%s: %s %d.%d/%zu: "
				    "malformed embedded option",
				    ifname, __func__, opt->option,
				    eopt->option, i);
			goto out;
		}
		if (eo == 0) {
			/* An option was expected, but there is no data
			 * data for it.
			 * This may not be an error as some options like
			 * DHCP FQDN in RFC4702 have a string as the last
			 * option which is optional. */
			if (env == NULL &&
			    (ol != 0 || !(eopt->type & OT_OPTIONAL)))
				logerrx("%s: %s %d.%d/%zu: missing embedded option",
				    ifname, __func__, opt->option,
				    eopt->option, i);
			goto out;
		}
		/* Use the option prefix if the embedded option
		 * name is different.
		 * This avoids new_fqdn_fqdn which would be silly. */
		if (!(eopt->type & OT_RESERVED)) {
			ov = strcmp(opt->var, eopt->var);
			if (dhcp_envoption1(env == NULL ? NULL : &env[n],
			    pfx, eopt, ov, od, (size_t)eo, ifname))
				n++;
			else if (env == NULL)
				logerr("%s: %s %d.%d/%zu",
				    ifname, __func__,
				    opt->option, eopt->option, i);
		}
		od += (size_t)eo;
		ol -= (size_t)eo;
	}

	/* Enumerate our encapsulated options */
	if (opt->encopts_len && ol > 0) {
		/* Zero any option indexes
		 * We assume that referenced encapsulated options are NEVER
		 * recursive as the index order could break. */
		for (i = 0, eopt = opt->encopts;
		    i < opt->encopts_len;
		    i++, eopt++)
		{
			eoc = opt->option;
			if (eopt->type & OT_OPTION) {
				dgetopt(ctx, NULL, &eoc, NULL, NULL, 0, &oopt);
				if (oopt)
					oopt->index = 0;
			}
		}

		while ((eod = dgetopt(ctx, &eos, &eoc, &eol, od, ol, &oopt))) {
			for (i = 0, eopt = opt->encopts;
			    i < opt->encopts_len;
			    i++, eopt++)
			{
				if (eopt->option == eoc) {
					if (eopt->type & OT_OPTION) {
						if (oopt == NULL)
							/* Report error? */
							continue;
					}
					n += dhcp_envoption(ctx,
					    env == NULL ? NULL : &env[n], pfx,
					    ifname,
					    eopt->type & OT_OPTION ? oopt:eopt,
					    dgetopt, eod, eol);
					break;
				}
			}
			od += eos + eol;
			ol -= eos + eol;
		}
	}

out:
	if (env)
		free(pfx);

	/* Return number of options found */
	return n;
}

void
dhcp_zero_index(struct dhcp_opt *opt)
{
	size_t i;
	struct dhcp_opt *o;

	opt->index = 0;
	for (i = 0, o = opt->embopts; i < opt->embopts_len; i++, o++)
		dhcp_zero_index(o);
	for (i = 0, o = opt->encopts; i < opt->encopts_len; i++, o++)
		dhcp_zero_index(o);
}

size_t
dhcp_read_lease_fd(int fd, void **lease)
{
	struct stat st;
	size_t sz;
	void *buf;
	ssize_t len;
	
	if (fstat(fd, &st) != 0)
		goto out;
	if (!S_ISREG(st.st_mode)) {
		errno = EINVAL;
		goto out;
	}
	if (st.st_size > UINT32_MAX) {
		errno = E2BIG;
		goto out;
	}

	sz = (size_t)st.st_size;
	if ((buf = malloc(sz)) == NULL)
		goto out;
	if ((len = read(fd, buf, sz)) == -1) {
		free(buf);
		goto out;
	}
	*lease = buf;
	return (size_t)len;

out:
	*lease = NULL;
	return 0;
}
