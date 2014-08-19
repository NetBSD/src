#include <sys/cdefs.h>
 __RCSID("$NetBSD: dhcp-common.c,v 1.1.1.1.2.3 2014/08/19 23:46:43 tls Exp $");

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

#include <sys/utsname.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp-common.h"
#include "dhcp.h"
#include "if.h"
#include "ipv6.h"

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

size_t
dhcp_vendor(char *str, size_t len)
{
	struct utsname utn;
	char *p;
	int l;

	if (uname(&utn) != 0)
		return (size_t)snprintf(str, len, "%s-%s",
		    PACKAGE, VERSION);
	p = str;
	l = snprintf(p, len,
	    "%s-%s:%s-%s:%s", PACKAGE, VERSION,
	    utn.sysname, utn.release, utn.machine);
	p += l;
	len -= (size_t)l;
	l = if_machinearch(p, len);
	p += l;
	return (size_t)(p - str);
}

int
make_option_mask(const struct dhcp_opt *dopts, size_t dopts_len,
    const struct dhcp_opt *odopts, size_t odopts_len,
    uint8_t *mask, const char *opts, int add)
{
	char *token, *o, *p, *t;
	const struct dhcp_opt *opt;
	int match;
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
			if (strcmp(opt->var, token) == 0)
				match = 1;
			else {
				errno = 0;
				n = (unsigned int)strtol(token, &t, 0);
				if (errno == 0 && !*t)
					if (opt->option == n)
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
					errno = 0;
					n = (unsigned int)strtol(token, &t, 0);
					if (errno == 0 && !*t)
						if (opt->option == n)
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
		if (add == 2 && !(opt->type & ADDRIPV4)) {
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

/* Decode an RFC3397 DNS search order option into a space
 * separated string. Returns length of string (including
 * terminating zero) or zero on error. out may be NULL
 * to just determine output length. */
ssize_t
decode_rfc3397(char *out, size_t len, const uint8_t *p, size_t pl)
{
	const char *start;
	size_t start_len, l, count;
	const uint8_t *r, *q = p, *e;
	int hops;
	uint8_t ltype;

	count = 0;
	start = out;
	start_len = len;
	q = p;
	e = p + pl;
	while (q < e) {
		r = NULL;
		hops = 0;
		/* Check we are inside our length again in-case
		 * the name isn't fully qualified (ie, not terminated) */
		while (q < e && (l = (size_t)*q++)) {
			ltype = l & 0xc0;
			if (ltype == 0x80 || ltype == 0x40)
				return -1;
			else if (ltype == 0xc0) { /* pointer */
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
				count += l + 1;
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

	return (ssize_t)count;
}

ssize_t
print_string(char *s, size_t len, const uint8_t *data, size_t dl)
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
				len -= (size_t)r;
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
		case '\\': /* FALLTHROUGH */
		case '|':  /* FALLTHROUGH */
		case '&':
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
			*s++ = (char)c;
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

#define ADDRSZ		4
#define ADDR6SZ		16
static size_t
dhcp_optlen(const struct dhcp_opt *opt, size_t dl)
{
	size_t sz;

	if (dl == 0)
		return 0;

	if (opt->type == 0 ||
	    opt->type & (STRING | BINHEX | RFC3442 | RFC5969))
	{
		if (opt->len) {
			if ((size_t)opt->len > dl)
				return 0;
			return (size_t)opt->len;
		}
		return dl;
	}

	if ((opt->type & (ADDRIPV4 | ARRAY)) == (ADDRIPV4 | ARRAY)) {
		if (dl < ADDRSZ)
			return 0;
		return dl - (dl % ADDRSZ);
	}

	if ((opt->type & (ADDRIPV6 | ARRAY)) == (ADDRIPV6 | ARRAY)) {
		if (dl < ADDR6SZ)
			return 0;
		return dl - (dl % ADDR6SZ);
	}

	if (opt->type & (UINT32 | ADDRIPV4))
		sz = sizeof(uint32_t);
	else if (opt->type & UINT16)
		sz = sizeof(uint16_t);
	else if (opt->type & UINT8)
		sz = sizeof(uint8_t);
	else if (opt->type & ADDRIPV6)
		sz = ADDR6SZ;
	else
		/* If we don't know the size, assume it's valid */
		return dl;
	return (dl < sz ? 0 : sz);
}

#ifdef INET6
#define PO_IFNAME
#else
#define PO_IFNAME __unused
#endif

ssize_t
print_option(char *s, size_t len, int type, const uint8_t *data, size_t dl,
    PO_IFNAME const char *ifname)
{
	const uint8_t *e, *t;
	uint16_t u16;
	int16_t s16;
	uint32_t u32;
	int32_t s32;
	struct in_addr addr;
	ssize_t bytes = 0, sl;
	size_t l;
	char *tmp;

	if (type & RFC3397) {
		sl = decode_rfc3397(NULL, 0, data, dl);
		if (sl == 0 || sl == -1)
			return sl;
		l = (size_t)sl;
		tmp = malloc(l);
		if (tmp == NULL)
			return -1;
		decode_rfc3397(tmp, l, data, dl);
		sl = print_string(s, len, (uint8_t *)tmp, l - 1);
		free(tmp);
		return sl;
	}

#ifdef INET
	if (type & RFC3361) {
		if ((tmp = decode_rfc3361(data, dl)) == NULL)
			return -1;
		l = strlen(tmp);
		sl = print_string(s, len, (uint8_t *)tmp, l);
		free(tmp);
		return sl;
	}

	if (type & RFC3442)
		return decode_rfc3442(s, len, data, dl);

	if (type & RFC5969)
		return decode_rfc5969(s, len, data, dl);
#endif

	if (type & STRING) {
		/* Some DHCP servers return NULL strings */
		if (*data == '\0')
			return 0;
		return print_string(s, len, data, dl);
	}

	if (type & FLAG) {
		if (s) {
			*s++ = '1';
			*s = '\0';
		}
		return 2;
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
		} else if (type & ADDRIPV4) {
			l = 16;
			dl /= 4;
		}
#ifdef INET6
		else if (type & ADDRIPV6) {
			e = data + dl;
			l = 0;
			while (data < e) {
				if (l)
					l++; /* space */
				sl = ipv6_printaddr(NULL, 0, data, ifname);
				if (sl != -1)
					l += (size_t)sl;
				data += 16;
			}
			return (ssize_t)(l + 1);
		}
#endif
		else if (type & BINHEX) {
			l = 2;
		} else {
			errno = EINVAL;
			return -1;
		}
		return (ssize_t)((l + 1) * dl);
	}

	t = data;
	e = data + dl;
	while (data < e) {
		if (data != t && type != BINHEX) {
			*s++ = ' ';
			bytes++;
			len--;
		}
		if (type & UINT8) {
			sl = snprintf(s, len, "%u", *data);
			data++;
		} else if (type & UINT16) {
			memcpy(&u16, data, sizeof(u16));
			u16 = ntohs(u16);
			sl = snprintf(s, len, "%u", u16);
			data += sizeof(u16);
		} else if (type & SINT16) {
			memcpy(&u16, data, sizeof(u16));
			s16 = (int16_t)ntohs(u16);
			sl = snprintf(s, len, "%d", s16);
			data += sizeof(u16);
		} else if (type & UINT32) {
			memcpy(&u32, data, sizeof(u32));
			u32 = ntohl(u32);
			sl = snprintf(s, len, "%u", u32);
			data += sizeof(u32);
		} else if (type & SINT32) {
			memcpy(&u32, data, sizeof(u32));
			s32 = (int32_t)ntohl(u32);
			sl = snprintf(s, len, "%d", s32);
			data += sizeof(u32);
		} else if (type & ADDRIPV4) {
			memcpy(&addr.s_addr, data, sizeof(addr.s_addr));
			sl = snprintf(s, len, "%s", inet_ntoa(addr));
			data += sizeof(addr.s_addr);
		}
#ifdef INET6
		else if (type & ADDRIPV6) {
			ssize_t r;

			r = ipv6_printaddr(s, len, data, ifname);
			if (r != -1)
				sl = r;
			else
				sl = 0;
			data += 16;
		}
#endif
		else if (type & BINHEX) {
			sl = snprintf(s, len, "%.2x", data[0]);
			data++;
		} else
			sl = 0;
		len -= (size_t)sl;
		bytes += sl;
		s += sl;
	}

	return bytes;
}

static size_t
dhcp_envoption1(char **env, const char *prefix,
    const struct dhcp_opt *opt, int vname, const uint8_t *od, size_t ol,
    const char *ifname)
{
	ssize_t len;
	size_t e;
	char *v, *val;

	if (opt->len && opt->len < ol)
		ol = opt->len;
	len = print_option(NULL, 0, opt->type, od, ol, ifname);
	if (len < 0)
		return 0;
	if (vname)
		e = strlen(opt->var) + 1;
	else
		e = 0;
	if (prefix)
		e += strlen(prefix);
	e += (size_t)len + 4;
	if (env == NULL)
		return e;
	v = val = *env = malloc(e);
	if (v == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return 0;
	}
	if (vname)
		v += snprintf(val, e, "%s_%s=", prefix, opt->var);
	else
		v += snprintf(val, e, "%s=", prefix);
	if (len != 0)
		print_option(v, (size_t)len, opt->type, od, ol, ifname);
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
	unsigned int eoc;
	const uint8_t *eod;
	int ov;
	struct dhcp_opt *eopt, *oopt;
	char *pfx;

	/* If no embedded or encapsulated options, it's easy */
	if (opt->embopts_len == 0 && opt->encopts_len == 0) {
		if (dhcp_envoption1(env == NULL ? NULL : &env[0],
		    prefix, opt, 1, od, ol, ifname))
			return 1;
		return 0;
	}

	/* Create a new prefix based on the option */
	if (env) {
		if (opt->type & INDEX) {
			if (opt->index > 999) {
				errno = ENOBUFS;
				syslog(LOG_ERR, "%s: %m", __func__);
				return 0;
			}
		}
		e = strlen(prefix) + strlen(opt->var) + 2 +
		    (opt->type & INDEX ? 3 : 0);
		pfx = malloc(e);
		if (pfx == NULL) {
			syslog(LOG_ERR, "%s: %m", __func__);
			return 0;
		}
		if (opt->type & INDEX)
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
		e = dhcp_optlen(eopt, ol);
		if (e == 0)
			/* Report error? */
			return 0;
		/* Use the option prefix if the embedded option
		 * name is different.
		 * This avoids new_fqdn_fqdn which would be silly. */
		ov = strcmp(opt->var, eopt->var);
		if (dhcp_envoption1(env == NULL ? NULL : &env[n],
		    pfx, eopt, ov, od, e, ifname))
			n++;
		od += e;
		ol -= e;
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
			if (eopt->type & OPTION) {
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
					if (eopt->type & OPTION) {
						if (oopt == NULL)
							/* Report error? */
							continue;
					}
					n += dhcp_envoption(ctx,
					    env == NULL ? NULL : &env[n], pfx,
					    ifname,
					    eopt->type & OPTION ? oopt : eopt,
					    dgetopt, eod, eol);
					break;
				}
			}
			od += eos + eol;
			ol -= eos + eol;
		}
	}

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
