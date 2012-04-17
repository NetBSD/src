/*	$NetBSD: show.c,v 1.14.2.1 2012/04/17 00:09:37 yamt Exp $	*/
/*	$OpenBSD: show.c,v 1.1 2006/05/27 19:16:37 claudio Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/pfvar.h>
#include <net/pfkeyv2.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netmpls/mpls.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"
#include "prog_ops.h"

char	*any_ntoa(const struct sockaddr *);
char	*link_print(struct sockaddr *);

#define PFKEYV2_CHUNK sizeof(u_int64_t)

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	int	b_mask;
	char	b_val;
};
static const struct bits bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_REJECT,	'R' },
	{ RTF_BLACKHOLE, 'B' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_DONE,	'd' }, /* Completed -- for routing messages only */
	{ RTF_MASK,	'm' }, /* Mask Present -- for routing messages only */
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_STATIC,	'S' },
	{ RTF_PROTO1,	'1' },
	{ RTF_PROTO2,	'2' },
	/* { RTF_PROTO3,	'3' }, */
	{ RTF_CLONED,	'c' },
	/* { RTF_JUMBO,	'J' }, */
	{ RTF_ANNOUNCE,	'p' },
	{ 0, 0 }
};

void	 pr_rthdr(int, int);
void	 p_rtentry(struct rt_msghdr *);
void	 pr_family(int);
void	 p_sockaddr(struct sockaddr *, struct sockaddr *, int, int);
char	*routename4(in_addr_t);
char	*routename6(struct sockaddr_in6 *);
static void p_tag(const struct sockaddr *sa);

/*
 * Print routing tables.
 */
void
p_rttables(int paf)
{
	struct rt_msghdr *rtm;
	char *buf = NULL, *next, *lim = NULL;
	size_t needed;
	int mib[6];
	struct sockaddr *sa;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = paf;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;
	if (prog_sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed > 0) {
		if ((buf = malloc(needed)) == 0)
			err(1, NULL);
		if (prog_sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
			err(1, "sysctl of routing table");
		lim = buf + needed;
	}

	printf("Routing tables\n");

	if (buf) {
		for (next = buf; next < lim; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			sa = (struct sockaddr *)(rtm + 1);
			if (paf != AF_UNSPEC && sa->sa_family != paf)
				continue;
			p_rtentry(rtm);
		}
		free(buf);
		buf = NULL;
	}

	if (paf != 0 && paf != PF_KEY)
		return;

#if 0 /* XXX-elad */
	mib[0] = CTL_NET;
	mib[1] = PF_KEY;
	mib[2] = PF_KEY_V2;
	mib[3] = NET_KEY_SPD_DUMP;
	mib[4] = mib[5] = 0;

	if (prog_sysctl(mib, 4, NULL, &needed, NULL, 0) == -1) {
		if (errno == ENOPROTOOPT)
			return;
		err(1, "spd-sysctl-estimate");
	}
	if (needed > 0) {
		if ((buf = malloc(needed)) == 0)
			err(1, NULL);
		if (prog_sysctl(mib, 4, buf, &needed, NULL, 0) == -1)
			err(1,"sysctl of spd");
		lim = buf + needed;
	}

	if (buf) {
		printf("\nEncap:\n");

		for (next = buf; next < lim; next += msg->sadb_msg_len *
		    PFKEYV2_CHUNK) {
			msg = (struct sadb_msg *)next;
			if (msg->sadb_msg_len == 0)
				break;
			p_pfkentry(msg);
		}
		free(buf);
		buf = NULL;
	}
#endif /* 0 */
}

/* 
 * column widths; each followed by one space
 * width of destination/gateway column
 * strlen("fe80::aaaa:bbbb:cccc:dddd@gif0") == 30, strlen("/128") == 4
 */
#define	WID_DST(af)	((af) == AF_INET6 ? (nflag ? 34 : 18) : 18)
#define	WID_GW(af)	((af) == AF_INET6 ? (nflag ? 30 : 18) : 18)

/*
 * Print header for routing table columns.
 */
void
pr_rthdr(int paf, int pAflag)
{
	if (pAflag)
		printf("%-*.*s ", PLEN, PLEN, "Address");
	if (paf != PF_KEY) {
		if (tagflag == 1)
			printf("%-*.*s %-*.*s %-6.6s %6.6s %8.8s %6.6s %7.7s"
			    " %s\n", WID_DST(paf), WID_DST(paf), "Destination",
			    WID_GW(paf), WID_GW(paf), "Gateway",
			    "Flags", "Refs", "Use", "Mtu", "Tag", "Interface");
		else
			printf("%-*.*s %-*.*s %-6.6s %6.6s %8.8s %6.6s %s\n",
			    WID_DST(paf), WID_DST(paf), "Destination",
			    WID_GW(paf), WID_GW(paf), "Gateway",
			    "Flags", "Refs", "Use", "Mtu", "Interface");
	} else
		printf("%-18s %-5s %-18s %-5s %-5s %-22s\n",
		    "Source", "Port", "Destination",
		    "Port", "Proto", "SA(Address/Proto/Type/Direction)");
}

static void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int	i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			sa = (struct sockaddr *)((char *)(sa) +
			    RT_ROUNDUP(sa->sa_len));
		} else
			rti_info[i] = NULL;
	}
}

/*
 * Print a routing table entry.
 */
void
p_rtentry(struct rt_msghdr *rtm)
{
	static int	 old_af = -1;
	struct sockaddr	*sa = (struct sockaddr *)(rtm + 1);
	struct sockaddr	*mask, *rti_info[RTAX_MAX];
	char		 ifbuf[IF_NAMESIZE];


	if (old_af != sa->sa_family) {
		old_af = sa->sa_family;
		pr_family(sa->sa_family);
		pr_rthdr(sa->sa_family, 0);
	}
	get_rtaddrs(rtm->rtm_addrs, sa, rti_info);

	mask = rti_info[RTAX_NETMASK];
	if ((sa = rti_info[RTAX_DST]) == NULL)
		return;

	p_sockaddr(sa, mask, rtm->rtm_flags, WID_DST(sa->sa_family));
	p_sockaddr(rti_info[RTAX_GATEWAY], NULL, RTF_HOST,
	    WID_GW(sa->sa_family));
	p_flags(rtm->rtm_flags, "%-6.6s ");
#if 0 /* XXX-elad */
	printf("%6d %8"PRId64" ", (int)rtm->rtm_rmx.rmx_refcnt,
	    rtm->rtm_rmx.rmx_pksent);
#else
	printf("%6s %8s ", "-", "-");
#endif
	if (rtm->rtm_rmx.rmx_mtu)
		printf("%6"PRId64, rtm->rtm_rmx.rmx_mtu);
	else
		printf("%6s", "-");
	putchar((rtm->rtm_rmx.rmx_locks & RTV_MTU) ? 'L' : ' ');
	if (tagflag == 1)
		p_tag(rti_info[RTAX_TAG]);
	printf(" %.16s", if_indextoname(rtm->rtm_index, ifbuf));
	putchar('\n');
}

/*
 * Print address family header before a section of the routing table.
 */
void
pr_family(int paf)
{
	const char *afname;

	switch (paf) {
	case AF_INET:
		afname = "Internet";
		break;
	case AF_INET6:
		afname = "Internet6";
		break;
	case PF_KEY:
		afname = "Encap";
		break;
	case AF_APPLETALK:
		afname = "AppleTalk";
		break;
	case AF_MPLS:
		afname = "MPLS";
		break;
	default:
		afname = NULL;
		break;
	}
	if (afname)
		printf("\n%s:\n", afname);
	else
		printf("\nProtocol Family %d:\n", paf);
}

void
p_addr(struct sockaddr *sa, struct sockaddr *mask, int flags)
{
	p_sockaddr(sa, mask, flags, WID_DST(sa->sa_family));
}

void
p_gwaddr(struct sockaddr *sa, int gwaf)
{
	p_sockaddr(sa, 0, RTF_HOST, WID_GW(gwaf));
}

void
p_sockaddr(struct sockaddr *sa, struct sockaddr *mask, int flags, int width)
{
	char *cp;

	switch (sa->sa_family) {
	case AF_INET6:
	    {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
		struct in6_addr *in6 = &sa6->sin6_addr;

		/*
		 * XXX: This is a special workaround for KAME kernels.
		 * sin6_scope_id field of SA should be set in the future.
		 */
		if (IN6_IS_ADDR_LINKLOCAL(in6) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(in6)) {
			/* XXX: override is ok? */
			sa6->sin6_scope_id = (u_int32_t)ntohs(*(u_short *)
			    &in6->s6_addr[2]);
			*(u_short *)&in6->s6_addr[2] = 0;
		}
		if (flags & RTF_HOST)
			cp = routename((struct sockaddr *)sa6);
		else
			cp = netname((struct sockaddr *)sa6, mask);
		break;
	    }
	default:
		if ((flags & RTF_HOST) || mask == NULL)
			cp = routename(sa);
		else
			cp = netname(sa, mask);
		break;
	}
	if (width < 0)
		printf("%s", cp);
	else {
		if (nflag)
			printf("%-*s ", width, cp);
		else
			printf("%-*.*s ", width, width, cp);
	}
}

void
p_flags(int f, const char *format)
{
	char name[33], *flags;
	const struct bits *p = bits;

	for (flags = name; p->b_mask && flags < &name[sizeof(name) - 2]; p++)
		if (p->b_mask & f)
			*flags++ = p->b_val;
	*flags = '\0';
	printf(format, name);
}

static void
p_tag(const struct sockaddr *sa)
{
	char *line;

	if (sa == NULL || sa->sa_family != AF_MPLS) {
		printf("%7s", "-");
		return;
	}
	line = mpls_ntoa(sa);
	if (strlen(line) < 7)
		printf("%7s", line);
	else
		printf("%s", line);
}

static char line[MAXHOSTNAMELEN];
static char domain[MAXHOSTNAMELEN];

char *
routename(struct sockaddr *sa)
{
	char *cp = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		if (gethostname(domain, sizeof(domain)) == 0 &&
		    (cp = strchr(domain, '.')))
			(void)strlcpy(domain, cp + 1, sizeof(domain));
		else
			domain[0] = '\0';
		cp = NULL;
	}

	if (sa->sa_len == 0) {
		(void)strlcpy(line, "default", sizeof(line));
		return (line);
	}

	switch (sa->sa_family) {
	case AF_INET:
		return
		    (routename4(((struct sockaddr_in *)sa)->sin_addr.s_addr));

	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
		return (routename6(&sin6));
	    }

	case AF_LINK:
		return (link_print(sa));

	case AF_MPLS:
		return mpls_ntoa(sa);

#if 0 /* XXX-elad */
	case AF_UNSPEC:
		if (sa->sa_len == sizeof(struct sockaddr_rtlabel)) {
			static char name[RTLABEL_LEN];
			struct sockaddr_rtlabel *sr;

			sr = (struct sockaddr_rtlabel *)sa;
			strlcpy(name, sr->sr_label, sizeof(name));
			return (name);
		}
		/* FALLTHROUGH */
#endif
	default:
		(void)snprintf(line, sizeof(line), "(%d) %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

char *
routename4(in_addr_t in)
{
	const char	*cp = NULL;
	struct in_addr	 ina;
	struct hostent	*hp;

	if (in == INADDR_ANY)
		cp = "default";
	if (!cp && !nflag) {
		if ((hp = gethostbyaddr((char *)&in,
		    sizeof(in), AF_INET)) != NULL) {
			char *p;
			if ((p = strchr(hp->h_name, '.')) &&
			    !strcmp(p + 1, domain))
				*p = '\0';
			cp = hp->h_name;
		}
	}
	ina.s_addr = in;
	strlcpy(line, cp ? cp : inet_ntoa(ina), sizeof(line));

	return (line);
}

char *
routename6(struct sockaddr_in6 *sin6)
{
	int	 niflags = 0;

	if (nflag)
		niflags |= NI_NUMERICHOST;
	else
		niflags |= NI_NOFQDN;

	if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
	    line, sizeof(line), NULL, 0, niflags) != 0)
		strncpy(line, "invalid", sizeof(line));

	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname4(in_addr_t in, in_addr_t mask)
{
	const char *cp = NULL;
	struct netent *np = NULL;
	int mbits;

	in = ntohl(in);
	mask = ntohl(mask);
	if (!nflag && in != INADDR_ANY) {
		if ((np = getnetbyaddr(in, AF_INET)) != NULL)
			cp = np->n_name;
	}
	mbits = mask ? 33 - ffs(mask) : 0;
	if (in == INADDR_ANY && !mbits)
			cp = "default";
	if (cp)
		strlcpy(line, cp, sizeof(line));
#define C(x)	((x) & 0xff)
	else if (mbits < 9)
		snprintf(line, sizeof(line), "%u/%d", C(in >> 24), mbits);
	else if (mbits < 17)
		snprintf(line, sizeof(line), "%u.%u/%d",
		    C(in >> 24) , C(in >> 16), mbits);
	else if (mbits < 25)
		snprintf(line, sizeof(line), "%u.%u.%u/%d",
		    C(in >> 24), C(in >> 16), C(in >> 8), mbits);
	else
		snprintf(line, sizeof(line), "%u.%u.%u.%u/%d", C(in >> 24),
		    C(in >> 16), C(in >> 8), C(in), mbits);
#undef C
	return (line);
}

#ifdef INET6
char *
netname6(struct sockaddr_in6 *sa6, struct sockaddr_in6 *mask)
{
	struct sockaddr_in6 sin6;
	u_char *p;
	int masklen, final = 0, illegal = 0;
	int i, lim, flag, error;
	char hbuf[NI_MAXHOST];

	sin6 = *sa6;

	flag = 0;
	masklen = 0;
	if (mask) {
		lim = mask->sin6_len - offsetof(struct sockaddr_in6, sin6_addr);
		if (lim < 0)
			lim = 0;
		else if (lim > (int)sizeof(struct in6_addr))
			lim = sizeof(struct in6_addr);
		for (p = (u_char *)&mask->sin6_addr, i = 0; i < lim; p++) {
			if (final && *p) {
				illegal++;
				sin6.sin6_addr.s6_addr[i++] = 0x00;
				continue;
			}

			switch (*p & 0xff) {
			case 0xff:
				masklen += 8;
				break;
			case 0xfe:
				masklen += 7;
				final++;
				break;
			case 0xfc:
				masklen += 6;
				final++;
				break;
			case 0xf8:
				masklen += 5;
				final++;
				break;
			case 0xf0:
				masklen += 4;
				final++;
				break;
			case 0xe0:
				masklen += 3;
				final++;
				break;
			case 0xc0:
				masklen += 2;
				final++;
				break;
			case 0x80:
				masklen += 1;
				final++;
				break;
			case 0x00:
				final++;
				break;
			default:
				final++;
				illegal++;
				break;
			}

			if (!illegal)
				sin6.sin6_addr.s6_addr[i++] &= *p;
			else
				sin6.sin6_addr.s6_addr[i++] = 0x00;
		}
		while (i < (int)sizeof(struct in6_addr))
			sin6.sin6_addr.s6_addr[i++] = 0x00;
	} else
		masklen = 128;

	if (masklen == 0 && IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr)) {
		snprintf(line, sizeof(line), "default");
		return (line);
	}

	if (illegal)
		warnx("illegal prefixlen");

	if (nflag)
		flag |= NI_NUMERICHOST;
	error = getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
	    hbuf, sizeof(hbuf), NULL, 0, flag);
	if (error)
		snprintf(hbuf, sizeof(hbuf), "invalid");

	snprintf(line, sizeof(line), "%s/%d", hbuf, masklen);
	return (line);
}
#endif

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(struct sockaddr *sa, struct sockaddr *mask)
{
	switch (sa->sa_family) {

	case AF_INET:
		return netname4(((struct sockaddr_in *)sa)->sin_addr.s_addr,
		    ((struct sockaddr_in *)mask)->sin_addr.s_addr);
#ifdef INET6
	case AF_INET6:
		return netname6((struct sockaddr_in6 *)sa,
		    (struct sockaddr_in6 *)mask);
#endif
	case AF_LINK:
		return (link_print(sa));
	default:
		snprintf(line, sizeof(line), "af %d: %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

static const char hexlist[] = "0123456789abcdef";

char *
any_ntoa(const struct sockaddr *sa)
{
	static char obuf[240];
	const char *in = sa->sa_data;
	char *out = obuf;
	int len = sa->sa_len - offsetof(struct sockaddr, sa_data);

	*out++ = 'Q';
	do {
		*out++ = hexlist[(*in >> 4) & 15];
		*out++ = hexlist[(*in++)    & 15];
		*out++ = '.';
	} while (--len > 0 && (out + 3) < &obuf[sizeof(obuf) - 1]);
	out[-1] = '\0';
	return (obuf);
}

char *
link_print(struct sockaddr *sa)
{
	struct sockaddr_dl	*sdl = (struct sockaddr_dl *)sa;
	u_char			*lla = (u_char *)sdl->sdl_data + sdl->sdl_nlen;

	if (sdl->sdl_nlen == 0 && sdl->sdl_alen == 0 &&
	    sdl->sdl_slen == 0) {
		(void)snprintf(line, sizeof(line), "link#%d", sdl->sdl_index);
		return (line);
	}
	switch (sdl->sdl_type) {
	case IFT_ETHER:
	case IFT_CARP:
		return (ether_ntoa((struct ether_addr *)lla));
	default:
		return (link_ntoa(sdl));
	}
}

char *
mpls_ntoa(const struct sockaddr *sa)
{
	static char obuf[16];
	const union mpls_shim *pms;
	union mpls_shim ms;
	int psize = sizeof(struct sockaddr_mpls);

	pms = &((const struct sockaddr_mpls*)sa)->smpls_addr;
	ms.s_addr = ntohl(pms->s_addr);

	snprintf(obuf, sizeof(obuf), "%u", ms.shim.label);

	while(psize < sa->sa_len) {
		pms++;
		ms.s_addr = ntohl(pms->s_addr);
		snprintf(obuf, sizeof(obuf), "%s,%u", obuf,
		    ms.shim.label);
		psize+=sizeof(ms);
	}
	return obuf;
}
