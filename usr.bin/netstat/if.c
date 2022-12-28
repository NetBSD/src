/*	$NetBSD: if.c,v 1.109 2022/12/28 18:34:33 mrg Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)if.c	8.2 (Berkeley) 2/21/94";
#else
__RCSID("$NetBSD: if.c,v 1.109 2022/12/28 18:34:33 mrg Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <kvm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <err.h>

#include "netstat.h"
#include "rtutil.h"
#include "prog_ops.h"

#define	MAXIF	100

#define HUMBUF_SIZE 7

struct	iftot {
	char ift_name[IFNAMSIZ];	/* interface name */
	uint64_t ift_ip;		/* input packets */
	uint64_t ift_ib;		/* input bytes */
	uint64_t ift_ie;		/* input errors */
	uint64_t ift_iq;		/* input drops */
	uint64_t ift_op;		/* output packets */
	uint64_t ift_ob;		/* output bytes */
	uint64_t ift_oe;		/* output errors */
	uint64_t ift_oq;		/* output drops */
	uint64_t ift_co;		/* collisions */
};

struct if_data_ext {
	uint64_t ifi_oqdrops;
};

static void set_lines(void);
static void print_addr(const int, struct sockaddr *, struct sockaddr **,
    struct if_data *, struct ifnet *, struct if_data_ext *);
static void sidewaysintpr(u_int, u_long);

static void iftot_banner(struct iftot *);
static void iftot_print_sum(struct iftot *, struct iftot *);
static void iftot_print(struct iftot *, struct iftot *);

static void catchalarm(int);
static void get_rtaddrs(int, struct sockaddr *, struct sockaddr **);
static void fetchifs(void);

static int if_data_ext_get(const char *, struct if_data_ext *);
static void intpr_sysctl(void);
static void intpr_kvm(u_long, void (*)(const char *));

struct iftot iftot[MAXIF], ip_cur, ip_old, sum_cur, sum_old;
static sig_atomic_t signalled;		/* set when alarm goes off */

static unsigned redraw_lines = 21;

static void
set_lines(void)
{
	static bool first = true;
	struct ttysize ts;

	if (!first)
		return;
	first = false;
	if (ioctl(STDOUT_FILENO, TIOCGSIZE, &ts) != -1 && ts.ts_lines)
		redraw_lines = ts.ts_lines - 3;
}


/*
 * Print a description of the network interfaces.
 * NOTE: ifnetaddr is the location of the kernel global "ifnet",
 * which is a TAILQ_HEAD.
 */
void
intpr(int interval, u_long ifnetaddr, void (*pfunc)(const char *))
{

	if (interval) {
		sidewaysintpr((unsigned)interval, ifnetaddr);
		return;
	}

	if (use_sysctl)
		intpr_sysctl();
	else
		intpr_kvm(ifnetaddr, pfunc);
}

static void
intpr_header(void)
{

	if (!sflag && !pflag) {
		if (bflag) {
			printf("%-5.5s %-5.5s %-13.13s %-17.17s "
			    "%10.10s %10.10s",
			    "Name", "Mtu", "Network", "Address",
			    "Ibytes", "Obytes");
		} else {
			printf("%-5.5s %-5.5s %-13.13s %-17.17s "
			    "%8.8s %5.5s",
			    "Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs");
			if (dflag)
				printf(" %6.6s", "Idrops");
			printf(" %8.8s %5.5s %5.5s",
			    "Opkts", "Oerrs", "Colls");
		}
		if (dflag)
			printf(" %6.6s", "Odrops");
		if (tflag)
			printf(" %4.4s", "Time");
		putchar('\n');
	}
}

int
if_data_ext_get(const char *ifname, struct if_data_ext *dext)
{
	char namebuf[1024];
	size_t len;
	uint64_t drops;

	/* For sysctl */
	snprintf(namebuf, sizeof(namebuf),
	    "net.interfaces.%s.sndq.drops", ifname);
	len = sizeof(drops);
	if (sysctlbyname(namebuf, &drops, &len, NULL, 0) == -1) {
		warn("%s", namebuf);
		dext->ifi_oqdrops = 0;
		return -1;
	} else
		dext->ifi_oqdrops = drops;

	return 0;
}

static void
intpr_sysctl(void)
{
	struct if_msghdr *ifm;
	int mib[6] = { CTL_NET, AF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };
	static char *buf = NULL;
	static size_t olen;
	char *next, *lim, *cp;
	struct rt_msghdr *rtm;
	struct ifa_msghdr *ifam;
	struct if_data *ifd = NULL;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	struct sockaddr_dl *sdl;
	uint64_t total = 0;
	size_t len;
	int did = 1, rtax = 0, n;
	char name[IFNAMSIZ + 1];	/* + 1 for `*' */
	char origname[IFNAMSIZ];	/* without `*' */
	int ifindex = 0;

	if (prog_sysctl(mib, 6, NULL, &len, NULL, 0) == -1)
		err(1, "sysctl");
	if (len > olen) {
		free(buf);
		if ((buf = malloc(len)) == NULL)
			err(1, NULL);
		olen = len;
	}
	if (prog_sysctl(mib, 6, buf, &len, NULL, 0) == -1)
		err(1, "sysctl");

	intpr_header();

	lim = buf + len;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		struct if_data_ext dext;

		rtm = (struct rt_msghdr *)next;
		if (rtm->rtm_version != RTM_VERSION)
			continue;
		switch (rtm->rtm_type) {
		case RTM_IFINFO:
			total = 0;
			ifm = (struct if_msghdr *)next;
			ifd = &ifm->ifm_data;

			sa = (struct sockaddr *)(ifm + 1);
			get_rtaddrs(ifm->ifm_addrs, sa, rti_info);

			sdl = (struct sockaddr_dl *)rti_info[RTAX_IFP];
			if (sdl == NULL || sdl->sdl_family != AF_LINK)
				continue;

			bzero(name, sizeof(name));
			if (sdl->sdl_nlen >= IFNAMSIZ)
				memcpy(name, sdl->sdl_data, IFNAMSIZ - 1);
			else if (sdl->sdl_nlen > 0)
				memcpy(name, sdl->sdl_data, sdl->sdl_nlen);

			if (interface != NULL && strcmp(name, interface) != 0)
				continue;

			ifindex = sdl->sdl_index;

			/* Keep the original name */
			strcpy(origname, name);

			/* Mark inactive interfaces with a '*' */
			cp = strchr(name, '\0');
			if ((ifm->ifm_flags & IFF_UP) == 0) {
				*cp++ = '*';
				*cp = '\0';
			}

			if (qflag) {
				total = ifd->ifi_ibytes + ifd->ifi_obytes +
				    ifd->ifi_ipackets + ifd->ifi_ierrors +
				    ifd->ifi_opackets + ifd->ifi_oerrors +
				    ifd->ifi_collisions;
				if (dflag)
					total += ifd->ifi_iqdrops;
				if (total == 0)
					continue;
			}
			/* Skip the first one */
			if (did) {
				did = 0;
				continue;
			}
			rtax = RTAX_IFP;
			break;
		case RTM_NEWADDR:
			if (qflag && total == 0)
				continue;
			if (interface != NULL && strcmp(name, interface) != 0)
				continue;
			ifam = (struct ifa_msghdr *)next;
			if ((ifam->ifam_addrs & (RTA_NETMASK | RTA_IFA |
			    RTA_BRD)) == 0)
				break;

			sa = (struct sockaddr *)(ifam + 1);

			get_rtaddrs(ifam->ifam_addrs, sa, rti_info);
			rtax = RTAX_IFA;
			did = 1;
			break;
		default:
			continue;
		}
		if (vflag)
			n = strlen(name) < 5 ? 5 : strlen(name);
		else
			n = 5;

		printf("%-*.*s %-5" PRIu64 " ", n, n, name, ifd->ifi_mtu);
		if (dflag)
			if_data_ext_get(origname, &dext);

		print_addr(ifindex, rti_info[rtax], rti_info, ifd,
		    NULL, dflag ? &dext : NULL);
	}
}

union ifaddr_u {
	struct ifaddr ifa;
	struct in_ifaddr in;
#ifdef INET6
	struct in6_ifaddr in6;
#endif /* INET6 */
};

static void
ifname_to_ifdata(int s, const char *ifname, struct if_data * const ifd)
{
	struct ifdatareq ifdr;

	memset(ifd, 0, sizeof(*ifd));
	strlcpy(ifdr.ifdr_name, ifname, sizeof(ifdr.ifdr_name));
	if (ioctl(s, SIOCGIFDATA, &ifdr) != 0)
		return;
	memcpy(ifd, &ifdr.ifdr_data, sizeof(ifdr.ifdr_data));
}

static void
intpr_kvm(u_long ifnetaddr, void (*pfunc)(const char *))
{
	struct ifnet ifnet;
	struct if_data ifd;
	union ifaddr_u ifaddr;
	u_long ifaddraddr;
	struct ifnet_head ifhead;	/* TAILQ_HEAD */
	char name[IFNAMSIZ + 1];	/* + 1 for `*' */
	int s;

	if (ifnetaddr == 0) {
		printf("ifnet: symbol not defined\n");
		return;
	}

	/*
	 * Find the pointer to the first ifnet structure.  Replace
	 * the pointer to the TAILQ_HEAD with the actual pointer
	 * to the first list element.
	 */
	if (kread(ifnetaddr, (char *)&ifhead, sizeof ifhead))
		return;
	ifnetaddr = (u_long)ifhead.tqh_first;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return;

	intpr_header();

	ifaddraddr = 0;
	while (ifnetaddr || ifaddraddr) {
		char *cp;
		int n;

		if (ifaddraddr == 0) {
			if (kread(ifnetaddr, (char *)&ifnet, sizeof ifnet))
				return;
			memmove(name, ifnet.if_xname, IFNAMSIZ);
			name[IFNAMSIZ - 1] = '\0';	/* sanity */
			ifnetaddr = (u_long)ifnet.if_list.tqe_next;
			if (interface != NULL && strcmp(name, interface) != 0)
				continue;
			cp = strchr(name, '\0');

			if (pfunc) {
				(*pfunc)(name);
				continue;
			}

			if ((ifnet.if_flags & IFF_UP) == 0)
				*cp++ = '*';
			*cp = '\0';
			ifaddraddr = (u_long)ifnet.if_addrlist.tqh_first;
		}
		if (vflag)
			n = strlen(name) < 5 ? 5 : strlen(name);
		else
			n = 5;
		printf("%-*.*s %-5llu ", n, n, name,
		    (unsigned long long)ifnet.if_mtu);
		if (ifaddraddr == 0) {
			printf("%-13.13s ", "none");
			printf("%-17.17s ", "none");
		} else {
			struct sockaddr *sa;

			if (kread(ifaddraddr, (char *)&ifaddr, sizeof ifaddr))
			{
				ifaddraddr = 0;
				continue;
			}
#define CP(x) ((char *)(x))
			cp = (CP(ifaddr.ifa.ifa_addr) - CP(ifaddraddr)) +
			    CP(&ifaddr);
			sa = (struct sockaddr *)cp;
			ifname_to_ifdata(s, name, &ifd);
			print_addr(ifnet.if_index, sa, (void *)&ifaddr,
			    &ifd, &ifnet, NULL);
		}
		ifaddraddr = (u_long)ifaddr.ifa.ifa_list.tqe_next;
	}
	close(s);
}

static void
mc_print(const int ifindex, const size_t ias, const char *oid, int *mcast_oids,
    void (*pr)(const void *))
{
	uint8_t *mcast_addrs, *p;
	const size_t incr = 2 * ias + sizeof(uint32_t);
	size_t len;

	if (mcast_oids[0] == 0) {
		size_t oidlen = 4;
		if (sysctlnametomib(oid, mcast_oids, &oidlen) == -1) {
			warnx("'%s' not found", oid);
			return;
		}
		if (oidlen != 3) {
			warnx("Wrong OID path for '%s'", oid);
			return;
		}
	}

	if (mcast_oids[3] == ifindex)
		return;
	mcast_oids[3] = ifindex;

	mcast_addrs = asysctl(mcast_oids, 4, &len);
	if (mcast_addrs == NULL && len != 0) {
		warn("failed to read '%s'", oid);
		return;
	}
	if (len) {
		p = mcast_addrs;
		while (len >= incr) {
			(*pr)((p + ias));
			p += incr;
			len -= incr;
		}
	}
	free(mcast_addrs);
}

#ifdef INET6
static void
ia6_print(const struct in6_addr *ia)
{
	struct sockaddr_in6 as6;
	char hbuf[NI_MAXHOST];		/* for getnameinfo() */
	int n;

	memset(&as6, 0, sizeof(as6));
	as6.sin6_len = sizeof(struct sockaddr_in6);
	as6.sin6_family = AF_INET6;
	as6.sin6_addr = *ia;
	inet6_getscopeid(&as6, INET6_IS_ADDR_MC_LINKLOCAL);
	if (getnameinfo((struct sockaddr *)&as6, as6.sin6_len, hbuf,
	    sizeof(hbuf), NULL, 0, NI_NUMERICHOST) != 0) {
		strlcpy(hbuf, "??", sizeof(hbuf));
	}
	if (vflag)
		n = strlen(hbuf) < 17 ? 17 : strlen(hbuf);
	else
		n = 17;
	printf("\n%25s %-*.*s ", "", n, n, hbuf);
}

static void
mc6_print(const int ifindex)
{
	static int mcast_oids[4];

	mc_print(ifindex, sizeof(struct in6_addr), "net.inet6.multicast",
	    mcast_oids, (void (*)(const void *))ia6_print);
}
#endif

static void
ia4_print(const struct in_addr *ia)
{
	printf("\n%25s %-17.17s ", "", routename4(ia->s_addr, nflag));
}

static void
mc4_print(const int ifindex)
{
	static int mcast_oids[4];

	mc_print(ifindex, sizeof(struct in_addr), "net.inet.multicast",
	    mcast_oids, (void (*)(const void *))ia4_print);
}

static void
print_addr(const int ifindex, struct sockaddr *sa, struct sockaddr **rtinfo,
    struct if_data *ifd, struct ifnet *ifnet, struct if_data_ext *dext)
{
	char hexsep = '.';		/* for hexprint */
	static const char hexfmt[] = "%02x%c";	/* for hexprint */
	char hbuf[NI_MAXHOST];		/* for getnameinfo() */
#ifdef INET6
	const int niflag = NI_NUMERICHOST;
	struct sockaddr_in6 *sin6, *netmask6;
#endif
	struct sockaddr_in netmask;
	struct sockaddr_in *sin;
	char *cp;
	int n, m;

	switch (sa->sa_family) {
	case AF_UNSPEC:
		printf("%-13.13s ", "none");
		printf("%-17.17s ", "none");
		break;
	case AF_INET:
		sin = (struct sockaddr_in *)sa;
		if (use_sysctl) {
			netmask =
			    *((struct sockaddr_in *)rtinfo[RTAX_NETMASK]);
		} else {
			struct in_ifaddr *ifaddr_in = (void *)rtinfo;
			netmask.sin_addr.s_addr = ifaddr_in->ia_subnetmask;
		}
		cp = netname4(sin, &netmask, nflag);
		if (vflag)
			n = strlen(cp) < 13 ? 13 : strlen(cp);
		else
			n = 13;
		printf("%-*.*s ", n, n, cp);
		cp = routename4(sin->sin_addr.s_addr, nflag);
		if (vflag)
			n = strlen(cp) < 17 ? 17 : strlen(cp);
		else
			n = 17;
		printf("%-*.*s ", n, n, cp);

		if (!aflag)
			break;
		if (ifnet) {
			u_long multiaddr;
			struct in_multi inm;
			union ifaddr_u *ifaddr = (union ifaddr_u *)rtinfo;

			multiaddr = (u_long)ifaddr->in.ia_multiaddrs.lh_first;
			while (multiaddr != 0) {
				kread(multiaddr, (char *)&inm, sizeof inm);
				ia4_print(&inm.inm_addr);
				multiaddr = (u_long)inm.inm_list.le_next;
			}
		} else
			mc4_print(ifindex);
		break;
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)sa;
		inet6_getscopeid(sin6, INET6_IS_ADDR_LINKLOCAL);
#ifdef __KAME__
		if (!vflag)
			sin6->sin6_scope_id = 0;
#endif

		if (use_sysctl)
			netmask6 = (struct sockaddr_in6 *)rtinfo[RTAX_NETMASK];
		else {
			struct in6_ifaddr *ifaddr_in6 = (void *)rtinfo;
			netmask6 = &ifaddr_in6->ia_prefixmask;
		}

		cp = netname6(sin6, netmask6, nflag);
		if (vflag)
			n = strlen(cp) < 13 ? 13 : strlen(cp);
		else
			n = 13;
		printf("%-*.*s ", n, n, cp);
		if (getnameinfo((struct sockaddr *)sin6,
				sin6->sin6_len,
				hbuf, sizeof(hbuf), NULL, 0,
				niflag) != 0) {
			strlcpy(hbuf, "?", sizeof(hbuf));
		}
		cp = hbuf;
		if (vflag)
			n = strlen(cp) < 17 ? 17 : strlen(cp);
		else
			n = 17;
		printf("%-*.*s ", n, n, cp);

		if (!aflag)
			break;
		if (ifnet) {
			u_long multiaddr;
			struct in6_multi inm;
			union ifaddr_u *ifaddr = (union ifaddr_u *)rtinfo;

			multiaddr = (u_long)ifaddr->in6._ia6_multiaddrs.lh_first;
			while (multiaddr != 0) {
				kread(multiaddr, (char *)&inm, sizeof inm);
				ia6_print(&inm.in6m_addr);
				multiaddr = (u_long)inm.in6m_entry.le_next;
			}
		} else
			mc6_print(ifindex);
		break;
#endif /*INET6*/
#ifndef SMALL
	case AF_APPLETALK:
		printf("atalk:%-7.7s ", atalk_print(sa, 0x10));
		printf("%-17.17s ", atalk_print(sa, 0x0b));
		break;
#endif
	case AF_LINK:
		printf("%-13.13s ", "<Link>");
		if (getnameinfo(sa, sa->sa_len,
		    hbuf, sizeof(hbuf), NULL, 0,
		    NI_NUMERICHOST) != 0) {
			strlcpy(hbuf, "?", sizeof(hbuf));
		}
		cp = hbuf;
		if (vflag)
			n = strlen(cp) < 17 ? 17 : strlen(cp);
		else
			n = 17;
		printf("%-*.*s ", n, n, cp);
		break;

	default:
		m = printf("(%d)", sa->sa_family);
		for (cp = sa->sa_len + (char *)sa;
			--cp > sa->sa_data && (*cp == 0);) {}
		n = cp - sa->sa_data + 1;
		cp = sa->sa_data;

		while (--n >= 0)
			m += printf(hexfmt, *cp++ & 0xff,
				    n > 0 ? hexsep : ' ');
		m = 32 - m;
		while (m-- > 0)
			putchar(' ');
		break;
	}

	if (bflag) {
		char humbuf[HUMBUF_SIZE];

		if (hflag && humanize_number(humbuf, sizeof(humbuf),
		    ifd->ifi_ibytes, "", HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
			printf("%10s ", humbuf);
		else
			printf("%10llu ", (unsigned long long)ifd->ifi_ibytes);

		if (hflag && humanize_number(humbuf, sizeof(humbuf),
		    ifd->ifi_obytes, "", HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
			printf("%10s", humbuf);
		else
			printf("%10llu", (unsigned long long)ifd->ifi_obytes);
	} else {
		printf("%8llu %5llu",
			(unsigned long long)ifd->ifi_ipackets,
			(unsigned long long)ifd->ifi_ierrors);
		if (dflag)
			printf(" %6" PRIu64, ifd->ifi_iqdrops);
		printf(" %8llu %5llu %5llu",
			(unsigned long long)ifd->ifi_opackets,
			(unsigned long long)ifd->ifi_oerrors,
			(unsigned long long)ifd->ifi_collisions);
	}
	if (dflag)
		printf(" %6" PRIu64, ifnet ?
		    ifnet->if_snd.ifq_drops : dext->ifi_oqdrops);
	if (tflag)
		printf(" %4d", ifnet ? ifnet->if_timer : 0);
	putchar('\n');
}

static void
iftot_banner(struct iftot *ift)
{
	if (bflag)
		printf("%7.7s in %8.8s %6.6s out %5.5s",
		    ift->ift_name, " ",
		    ift->ift_name, " ");
	else
		printf("%5.5s in %5.5s%5.5s out %5.5s %5.5s",
		    ift->ift_name, " ",
		    ift->ift_name, " ", " ");
	if (dflag)
		printf(" %5.5s", " ");

	if (bflag)
		printf("  %7.7s in %8.8s %6.6s out %5.5s",
		    "total", " ", "total", " ");
	else
		printf("  %5.5s in %5.5s%5.5s out %5.5s %5.5s",
		    "total", " ", "total", " ", " ");
	if (dflag)
		printf(" %5.5s", " ");
	putchar('\n');
	if (bflag)
		printf("%10.10s %8.8s %10.10s %5.5s",
		    "bytes", " ", "bytes", " ");
	else
		printf("%8.8s %5.5s %8.8s %5.5s %5.5s",
		    "packets", "errs", "packets", "errs", "colls");
	if (dflag)
		printf(" %5.5s", "drops");

	if (bflag)
		printf("  %10.10s %8.8s %10.10s %5.5s",
		    "bytes", " ", "bytes", " ");
	else
		printf("  %8.8s %5.5s %8.8s %5.5s %5.5s",
		    "packets", "errs", "packets", "errs", "colls");
	if (dflag)
		printf(" %5.5s", "drops");
	putchar('\n');
	fflush(stdout);
}

static void
iftot_print(struct iftot *cur, struct iftot *old)
{
	if (bflag)
		printf("%10" PRIu64 " %8.8s %10" PRIu64 " %5.5s",
		    cur->ift_ib - old->ift_ib, " ",
		    cur->ift_ob - old->ift_ob, " ");
	else
		printf("%8" PRIu64 " %5" PRIu64 " %8" PRIu64 " %5" PRIu64 " %5" PRIu64,
		    cur->ift_ip - old->ift_ip,
		    cur->ift_ie - old->ift_ie,
		    cur->ift_op - old->ift_op,
		    cur->ift_oe - old->ift_oe,
		    cur->ift_co - old->ift_co);
	if (dflag)
		printf(" %5" PRIu64, cur->ift_oq - old->ift_oq);
}

static void
iftot_print_sum(struct iftot *cur, struct iftot *old)
{
	if (bflag)
		printf("  %10" PRIu64 " %8.8s %10" PRIu64 " %5.5s",
		    cur->ift_ib - old->ift_ib, " ",
		    cur->ift_ob - old->ift_ob, " ");
	else
		printf("  %8" PRIu64 " %5" PRIu64 " %8" PRIu64 " %5" PRIu64 " %5" PRIu64,
		    cur->ift_ip - old->ift_ip,
		    cur->ift_ie - old->ift_ie,
		    cur->ift_op - old->ift_op,
		    cur->ift_oe - old->ift_oe,
		    cur->ift_co - old->ift_co);

	if (dflag)
		printf(" %5" PRIu64, cur->ift_oq - old->ift_oq);
}

__dead static void
sidewaysintpr_sysctl(unsigned interval)
{
	struct itimerval it;
	sigset_t emptyset;
	sigset_t noalrm;
	unsigned line;

	set_lines();

	fetchifs();
	if (ip_cur.ift_name[0] == '\0') {
		fprintf(stderr, "%s: %s: unknown interface\n",
		    getprogname(), interface);
		exit(1);
	}

	sigemptyset(&emptyset);
	sigemptyset(&noalrm);
	sigaddset(&noalrm, SIGALRM);
	sigprocmask(SIG_SETMASK, &noalrm, NULL);

	signalled = 0;
	(void)signal(SIGALRM, catchalarm);

	it.it_interval.tv_sec = it.it_value.tv_sec = interval;
	it.it_interval.tv_usec = it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

banner:
	iftot_banner(&ip_cur);

	line = 0;
	bzero(&ip_old, sizeof(ip_old));
	bzero(&sum_old, sizeof(sum_old));
loop:
	bzero(&sum_cur, sizeof(sum_cur));

	fetchifs();

	iftot_print(&ip_cur, &ip_old);

	ip_old = ip_cur;

	iftot_print_sum(&sum_cur, &sum_old);

	sum_old = sum_cur;

	putchar('\n');
	fflush(stdout);
	line++;
	if (signalled == 0)
		sigsuspend(&emptyset);

	signalled = 0;
	if (line == redraw_lines)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

static void
sidewaysintpr_kvm(unsigned interval, u_long off)
{
	struct itimerval it;
	sigset_t emptyset;
	sigset_t noalrm;
	struct ifnet ifnet;
	struct if_data ifd;
	u_long firstifnet;
	struct iftot *ip, *total;
	unsigned line;
	struct iftot *lastif, *sum, *interesting;
	struct ifnet_head ifhead;	/* TAILQ_HEAD */
	int s;

	set_lines();

	/*
	 * Find the pointer to the first ifnet structure.  Replace
	 * the pointer to the TAILQ_HEAD with the actual pointer
	 * to the first list element.
	 */
	if (kread(off, (char *)&ifhead, sizeof ifhead))
		return;
	firstifnet = (u_long)ifhead.tqh_first;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return;

	lastif = iftot;
	sum = iftot + MAXIF - 1;
	total = sum - 1;
	interesting = (interface == NULL) ? iftot : NULL;
	for (off = firstifnet, ip = iftot; off;) {
		if (kread(off, (char *)&ifnet, sizeof ifnet))
			break;
		memset(ip->ift_name, 0, sizeof(ip->ift_name));
		snprintf(ip->ift_name, IFNAMSIZ, "%s", ifnet.if_xname);
		if (interface && strcmp(ifnet.if_xname, interface) == 0)
			interesting = ip;
		ip++;
		if (ip >= iftot + MAXIF - 2)
			break;
		off = (u_long)ifnet.if_list.tqe_next;
	}
	if (interesting == NULL) {
		fprintf(stderr, "%s: %s: unknown interface\n",
		    getprogname(), interface);
		exit(1);
	}
	lastif = ip;

	sigemptyset(&emptyset);
	sigemptyset(&noalrm);
	sigaddset(&noalrm, SIGALRM);
	sigprocmask(SIG_SETMASK, &noalrm, NULL);

	signalled = 0;
	(void)signal(SIGALRM, catchalarm);

	it.it_interval.tv_sec = it.it_value.tv_sec = interval;
	it.it_interval.tv_usec = it.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

banner:
	if (bflag)
		printf("%7.7s in %8.8s %6.6s out %5.5s",
		    interesting->ift_name, " ",
		    interesting->ift_name, " ");
	else
		printf("%5.5s in %5.5s%5.5s out %5.5s %5.5s",
		    interesting->ift_name, " ",
		    interesting->ift_name, " ", " ");
	if (dflag)
		printf(" %5.5s", " ");
	if (lastif - iftot > 0) {
		if (bflag)
			printf("  %7.7s in %8.8s %6.6s out %5.5s",
			    "total", " ", "total", " ");
		else
			printf("  %5.5s in %5.5s%5.5s out %5.5s %5.5s",
			    "total", " ", "total", " ", " ");
		if (dflag)
			printf(" %5.5s", " ");
	}
	for (ip = iftot; ip < iftot + MAXIF; ip++) {
		ip->ift_ip = 0;
		ip->ift_ib = 0;
		ip->ift_ie = 0;
		ip->ift_iq = 0;
		ip->ift_op = 0;
		ip->ift_ob = 0;
		ip->ift_oe = 0;
		ip->ift_oq = 0;
		ip->ift_co = 0;
	}
	putchar('\n');
	if (bflag)
		printf("%10.10s %8.8s %10.10s %5.5s",
		    "bytes", " ", "bytes", " ");
	else
		printf("%8.8s %5.5s %8.8s %5.5s %5.5s",
		    "packets", "errs", "packets", "errs", "colls");
	if (dflag)
		printf(" %5.5s", "drops");
	if (lastif - iftot > 0) {
		if (bflag)
			printf("  %10.10s %8.8s %10.10s %5.5s",
			    "bytes", " ", "bytes", " ");
		else
			printf("  %8.8s %5.5s %8.8s %5.5s %5.5s",
			    "packets", "errs", "packets", "errs", "colls");
		if (dflag)
			printf(" %5.5s", "drops");
	}
	putchar('\n');
	fflush(stdout);
	line = 0;
loop:
	sum->ift_ip = 0;
	sum->ift_ib = 0;
	sum->ift_ie = 0;
	sum->ift_iq = 0;
	sum->ift_op = 0;
	sum->ift_ob = 0;
	sum->ift_oe = 0;
	sum->ift_oq = 0;
	sum->ift_co = 0;
	for (off = firstifnet, ip = iftot; off && ip < lastif; ip++) {
		if (kread(off, (char *)&ifnet, sizeof ifnet)) {
			off = 0;
			continue;
		}
		ifname_to_ifdata(s, ip->ift_name, &ifd);
		if (ip == interesting) {
			if (bflag) {
				char humbuf[HUMBUF_SIZE];

				if (hflag && humanize_number(humbuf,
				    sizeof(humbuf),
				    ifd.ifi_ibytes - ip->ift_ib, "",
				    HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
					printf("%10s %8.8s ", humbuf, " ");
				else
					printf("%10llu %8.8s ",
					    (unsigned long long)
					    (ifd.ifi_ibytes-ip->ift_ib), " ");

				if (hflag && humanize_number(humbuf,
				    sizeof(humbuf),
				    ifd.ifi_obytes - ip->ift_ob, "",
				    HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
					printf("%10s %5.5s", humbuf, " ");
				else
					printf("%10llu %5.5s",
					    (unsigned long long)
					    (ifd.ifi_obytes-ip->ift_ob), " ");
			} else {
				printf("%8llu %5llu %8llu %5llu %5llu",
				    (unsigned long long)
					(ifd.ifi_ipackets - ip->ift_ip),
				    (unsigned long long)
					(ifd.ifi_ierrors - ip->ift_ie),
				    (unsigned long long)
					(ifd.ifi_opackets - ip->ift_op),
				    (unsigned long long)
					(ifd.ifi_oerrors - ip->ift_oe),
				    (unsigned long long)
					(ifd.ifi_collisions - ip->ift_co));
			}
			if (dflag)
				printf(" %5" PRIu64,
					ifnet.if_snd.ifq_drops - ip->ift_oq);
		}
		ip->ift_ip = ifd.ifi_ipackets;
		ip->ift_ib = ifd.ifi_ibytes;
		ip->ift_ie = ifd.ifi_ierrors;
		ip->ift_op = ifd.ifi_opackets;
		ip->ift_ob = ifd.ifi_obytes;
		ip->ift_oe = ifd.ifi_oerrors;
		ip->ift_co = ifd.ifi_collisions;
		ip->ift_oq = ifnet.if_snd.ifq_drops;
		sum->ift_ip += ip->ift_ip;
		sum->ift_ib += ip->ift_ib;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_ob += ip->ift_ob;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		sum->ift_oq += ip->ift_oq;
		off = (u_long)ifnet.if_list.tqe_next;
	}
	if (lastif - iftot > 0) {
		if (bflag) {
			char humbuf[HUMBUF_SIZE];

			if (hflag && humanize_number(humbuf,
			    sizeof(humbuf), sum->ift_ib - total->ift_ib, "",
			    HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
				printf("  %10s %8.8s ", humbuf, " ");
			else
				printf("  %10llu %8.8s ",
				    (unsigned long long)
				    (sum->ift_ib - total->ift_ib), " ");

			if (hflag && humanize_number(humbuf,
			    sizeof(humbuf), sum->ift_ob -  total->ift_ob, "",
			    HN_AUTOSCALE, HN_NOSPACE | HN_B) > 0)
				printf("%10s %5.5s", humbuf, " ");
			else
				printf("%10llu %5.5s",
				    (unsigned long long)
				    (sum->ift_ob - total->ift_ob), " ");
		} else {
			printf("  %8llu %5llu %8llu %5llu %5llu",
			    (unsigned long long)
				(sum->ift_ip - total->ift_ip),
			    (unsigned long long)
				(sum->ift_ie - total->ift_ie),
			    (unsigned long long)
				(sum->ift_op - total->ift_op),
			    (unsigned long long)
				(sum->ift_oe - total->ift_oe),
			    (unsigned long long)
				(sum->ift_co - total->ift_co));
		}
		if (dflag)
			printf(" %5llu",
			    (unsigned long long)(sum->ift_oq - total->ift_oq));
	}
	*total = *sum;
	putchar('\n');
	fflush(stdout);
	line++;
	if (signalled == 0)
		sigsuspend(&emptyset);

	signalled = 0;
	if (line == redraw_lines)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
static void
sidewaysintpr(unsigned int interval, u_long off)
{

	if (use_sysctl)
		sidewaysintpr_sysctl(interval);
	else
		sidewaysintpr_kvm(interval, off);
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
static void
catchalarm(int signo)
{

	signalled = true;
}

static void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			sa = (struct sockaddr *)((char *)(sa) +
			    RT_ROUNDUP(sa->sa_len));
		} else
			rti_info[i] = NULL;
	}
}

static void
fetchifs(void)
{
	struct if_msghdr *ifm;
	int mib[6] = { CTL_NET, AF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };
	struct rt_msghdr *rtm;
	struct if_data *ifd = NULL;
	struct sockaddr *sa, *rti_info[RTAX_MAX];
	struct sockaddr_dl *sdl;
	static char *buf = NULL;
	static size_t olen;
	struct if_data_ext dext;
	char *next, *lim;
	char name[IFNAMSIZ];
	size_t len;

	if (prog_sysctl(mib, 6, NULL, &len, NULL, 0) == -1)
		err(1, "sysctl");
	if (len > olen) {
		free(buf);
		if ((buf = malloc(len)) == NULL)
			err(1, NULL);
		olen = len;
	}
	if (prog_sysctl(mib, 6, buf, &len, NULL, 0) == -1)
		err(1, "sysctl");

	memset(&dext, 0, sizeof(dext));
	lim = buf + len;
	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		if (rtm->rtm_version != RTM_VERSION)
			continue;
		switch (rtm->rtm_type) {
		case RTM_IFINFO:
			ifm = (struct if_msghdr *)next;
			ifd = &ifm->ifm_data;

			sa = (struct sockaddr *)(ifm + 1);
			get_rtaddrs(ifm->ifm_addrs, sa, rti_info);

			sdl = (struct sockaddr_dl *)rti_info[RTAX_IFP];
			if (sdl == NULL || sdl->sdl_family != AF_LINK)
				continue;
			bzero(name, sizeof(name));
			if (sdl->sdl_nlen >= IFNAMSIZ)
				memcpy(name, sdl->sdl_data, IFNAMSIZ - 1);
			else if (sdl->sdl_nlen > 0)
				memcpy(name, sdl->sdl_data, sdl->sdl_nlen);

			if_data_ext_get(name, &dext);

			if ((interface != NULL && !strcmp(name, interface)) ||
			    (interface == NULL &&
			    ((ip_cur.ift_ib + ip_cur.ift_ob) == 0 ||
			     (ip_cur.ift_ib + ip_cur.ift_ob <
			     ifd->ifi_ibytes + ifd->ifi_obytes)))) {
				strlcpy(ip_cur.ift_name, name,
				    sizeof(ip_cur.ift_name));
				ip_cur.ift_ip = ifd->ifi_ipackets;
				ip_cur.ift_ib = ifd->ifi_ibytes;
				ip_cur.ift_ie = ifd->ifi_ierrors;
				ip_cur.ift_op = ifd->ifi_opackets;
				ip_cur.ift_ob = ifd->ifi_obytes;
				ip_cur.ift_oe = ifd->ifi_oerrors;
				ip_cur.ift_co = ifd->ifi_collisions;
				ip_cur.ift_iq = ifd->ifi_iqdrops;
				ip_cur.ift_oq = dext.ifi_oqdrops;
			}

			sum_cur.ift_ip += ifd->ifi_ipackets;
			sum_cur.ift_ib += ifd->ifi_ibytes;
			sum_cur.ift_ie += ifd->ifi_ierrors;
			sum_cur.ift_op += ifd->ifi_opackets;
			sum_cur.ift_ob += ifd->ifi_obytes;
			sum_cur.ift_oe += ifd->ifi_oerrors;
			sum_cur.ift_co += ifd->ifi_collisions;
			sum_cur.ift_iq += ifd->ifi_iqdrops;
			sum_cur.ift_oq += dext.ifi_oqdrops;
			break;
		}
	}

	/*
	 * If we picked an interface, be sure to keep using it for the rest
	 * of this instance.
	 */
	if (interface == NULL)
		interface = ip_cur.ift_name;
}
