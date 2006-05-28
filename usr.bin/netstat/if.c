/*	$NetBSD: if.c,v 1.60 2006/05/28 16:51:40 elad Exp $	*/

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
__RCSID("$NetBSD: if.c,v 1.60 2006/05/28 16:51:40 elad Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <arpa/inet.h>

#include <kvm.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "netstat.h"

#define	YES	1
#define	NO	0

static void sidewaysintpr __P((u_int, u_long));
static void catchalarm __P((int));

/*
 * Print a description of the network interfaces.
 * NOTE: ifnetaddr is the location of the kernel global "ifnet",
 * which is a TAILQ_HEAD.
 */
void
intpr(interval, ifnetaddr, pfunc)
	int interval;
	u_long ifnetaddr;
	void (*pfunc)(char *);
{
	struct ifnet ifnet;
	union {
		struct ifaddr ifa;
		struct in_ifaddr in;
#ifdef INET6
		struct in6_ifaddr in6;
#endif /* INET6 */
		struct ns_ifaddr ns;
		struct iso_ifaddr iso;
	} ifaddr;
	u_long ifaddraddr;
	struct sockaddr *sa;
	struct ifnet_head ifhead;	/* TAILQ_HEAD */
	char name[IFNAMSIZ + 1];	/* + 1 for `*' */
	char hbuf[NI_MAXHOST];		/* for getnameinfo() */
#ifdef INET6
	const int niflag = NI_NUMERICHOST;
#endif

	if (ifnetaddr == 0) {
		printf("ifnet: symbol not defined\n");
		return;
	}
	if (interval) {
		sidewaysintpr((unsigned)interval, ifnetaddr);
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

	if (!sflag & !pflag) {
		if (bflag) {
			printf("%-5.5s %-5.5s %-13.13s %-17.17s "
			       "%10.10s %10.10s",
			       "Name", "Mtu", "Network", "Address", 
			       "Ibytes", "Obytes");
		} else {
			printf("%-5.5s %-5.5s %-13.13s %-17.17s "
			       "%8.8s %5.5s %8.8s %5.5s %5.5s",
			       "Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs",
			       "Opkts", "Oerrs", "Colls");
		}
		if (tflag)
			printf(" %4.4s", "Time");
		if (dflag)
			printf(" %5.5s", "Drops");
		putchar('\n');
	}
	ifaddraddr = 0;
	while (ifnetaddr || ifaddraddr) {
		struct sockaddr_in *sin;
#ifdef INET6
		struct sockaddr_in6 *sin6;
#endif /* INET6 */
		char *cp;
		int n, m;

		if (ifaddraddr == 0) {
			if (kread(ifnetaddr, (char *)&ifnet, sizeof ifnet))
				return;
			memmove(name, ifnet.if_xname, IFNAMSIZ);
			name[IFNAMSIZ - 1] = '\0';	/* sanity */
			ifnetaddr = (u_long)ifnet.if_list.tqe_next;
			if (interface != 0 && strcmp(name, interface) != 0)
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
			char hexsep = '.';		/* for hexprint */
			static const char hexfmt[] = "%02x%c";	/* for hexprint */
			if (kread(ifaddraddr, (char *)&ifaddr, sizeof ifaddr)) {
				ifaddraddr = 0;
				continue;
			}
#define CP(x) ((char *)(x))
			cp = (CP(ifaddr.ifa.ifa_addr) - CP(ifaddraddr)) +
			    CP(&ifaddr);
			sa = (struct sockaddr *)cp;
			switch (sa->sa_family) {
			case AF_UNSPEC:
				printf("%-13.13s ", "none");
				printf("%-17.17s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *)sa;
#ifdef notdef
				/*
				 * can't use inet_makeaddr because kernel
				 * keeps nets unshifted.
				 */
				in = inet_makeaddr(ifaddr.in.ia_subnet,
					INADDR_ANY);
				cp = netname4(in.s_addr,
					ifaddr.in.ia_subnetmask);
#else
				cp = netname4(ifaddr.in.ia_subnet,
					ifaddr.in.ia_subnetmask);
#endif
				if (vflag)
					n = strlen(cp) < 13 ? 13 : strlen(cp);
				else
					n = 13;
				printf("%-*.*s ", n, n, cp);
				cp = routename4(sin->sin_addr.s_addr);
				if (vflag)
					n = strlen(cp) < 17 ? 17 : strlen(cp);
				else
					n = 17;
				printf("%-*.*s ", n, n, cp);
				if (aflag) {
					u_long multiaddr;
					struct in_multi inm;
		
					multiaddr = (u_long)
					    ifaddr.in.ia_multiaddrs.lh_first;
					while (multiaddr != 0) {
						kread(multiaddr, (char *)&inm,
						   sizeof inm);
						printf("\n%25s %-17.17s ", "",
						   routename4(
						      inm.inm_addr.s_addr));
						multiaddr =
						   (u_long)inm.inm_list.le_next;
					}
				}
				break;
#ifdef INET6
			case AF_INET6:
				sin6 = (struct sockaddr_in6 *)sa;
#ifdef __KAME__
				if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
					sin6->sin6_scope_id =
						ntohs(*(u_int16_t *)
						  &sin6->sin6_addr.s6_addr[2]);
					/* too little width */
					if (!vflag)
						sin6->sin6_scope_id = 0;
					sin6->sin6_addr.s6_addr[2] = 0;
					sin6->sin6_addr.s6_addr[3] = 0;
				}
#endif
				cp = netname6(&ifaddr.in6.ia_addr,
					&ifaddr.in6.ia_prefixmask);
				if (vflag)
					n = strlen(cp) < 13 ? 13 : strlen(cp);
				else
					n = 13;
				printf("%-*.*s ", n, n, cp);
				if (getnameinfo((struct sockaddr *)sin6,
						sin6->sin6_len,
						hbuf, sizeof(hbuf), NULL, 0,
						niflag) != 0) {
					cp = "?";
				} else
					cp = hbuf;
				if (vflag)
					n = strlen(cp) < 17 ? 17 : strlen(cp);
				else
					n = 17;
				printf("%-*.*s ", n, n, cp);
				if (aflag) {
					u_long multiaddr;
					struct in6_multi inm;
					struct sockaddr_in6 sin6;
		
					multiaddr = (u_long)
					    ifaddr.in6.ia6_multiaddrs.lh_first;
					while (multiaddr != 0) {
						kread(multiaddr, (char *)&inm,
						   sizeof inm);
						memset(&sin6, 0, sizeof(sin6));
						sin6.sin6_len = sizeof(struct sockaddr_in6);
						sin6.sin6_family = AF_INET6;
						sin6.sin6_addr = inm.in6m_addr;
#ifdef __KAME__
						if (IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr)) {
							sin6.sin6_scope_id =
							    ntohs(*(u_int16_t *)
								&sin6.sin6_addr.s6_addr[2]);
							sin6.sin6_addr.s6_addr[2] = 0;
							sin6.sin6_addr.s6_addr[3] = 0;
						}
#endif
						if (getnameinfo((struct sockaddr *)&sin6,
						    sin6.sin6_len, hbuf,
						    sizeof(hbuf), NULL, 0,
						    niflag) != 0) {
							strlcpy(hbuf, "??",
							    sizeof(hbuf));
						}
						cp = hbuf;
						if (vflag)
						    n = strlen(cp) < 17
							? 17 : strlen(cp);
						else
						    n = 17;
						printf("\n%25s %-*.*s ", "",
						    n, n, cp);
						multiaddr =
						   (u_long)inm.in6m_entry.le_next;
					}
				}
				break;
#endif /*INET6*/
#ifndef SMALL
			case AF_APPLETALK:
				printf("atalk:%-7.7s ",
				       atalk_print(sa,0x10));
				printf("%-17.17s ", atalk_print(sa,0x0b));
				break;
			case AF_NS:
				{
				struct sockaddr_ns *sns =
					(struct sockaddr_ns *)sa;
				u_long net;
				char netnum[10];

				*(union ns_net *)&net = sns->sns_addr.x_net;
				(void)snprintf(netnum, sizeof(netnum), "%xH",
				    (u_int32_t)ntohl(net));
				upHex(netnum);
				printf("ns:%-10s ", netnum);
				printf("%-17.17s ",
				    ns_phost((struct sockaddr *)sns));
				}
				break;
#endif
			case AF_LINK:
				printf("%-13.13s ", "<Link>");
				if (getnameinfo(sa, sa->sa_len,
				    hbuf, sizeof(hbuf), NULL, 0,
				    NI_NUMERICHOST) != 0) {
					cp = "?";
				} else
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
			ifaddraddr = (u_long)ifaddr.ifa.ifa_list.tqe_next;
		}
		if (bflag) {
			printf("%10llu %10llu", 
				(unsigned long long)ifnet.if_ibytes,
				(unsigned long long)ifnet.if_obytes);
		} else {
			printf("%8llu %5llu %8llu %5llu %5llu",
				(unsigned long long)ifnet.if_ipackets,
				(unsigned long long)ifnet.if_ierrors,
				(unsigned long long)ifnet.if_opackets,
				(unsigned long long)ifnet.if_oerrors,
				(unsigned long long)ifnet.if_collisions);
		}
		if (tflag)
			printf(" %4d", ifnet.if_timer);
		if (dflag)
			printf(" %5d", ifnet.if_snd.ifq_drops);
		putchar('\n');
	}
}

#define	MAXIF	100
struct	iftot {
	char ift_name[IFNAMSIZ];	/* interface name */
	u_quad_t ift_ip;		/* input packets */
	u_quad_t ift_ib;		/* input bytes */
	u_quad_t ift_ie;		/* input errors */
	u_quad_t ift_op;		/* output packets */
	u_quad_t ift_ob;		/* output bytes */
	u_quad_t ift_oe;		/* output errors */
	u_quad_t ift_co;		/* collisions */
	int ift_dr;			/* drops */
} iftot[MAXIF];

u_char	signalled;			/* set if alarm goes off "early" */

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
static void
sidewaysintpr(interval, off)
	unsigned interval;
	u_long off;
{
	struct itimerval it;
	struct ifnet ifnet;
	u_long firstifnet;
	struct iftot *ip, *total;
	int line;
	struct iftot *lastif, *sum, *interesting;
	struct ifnet_head ifhead;	/* TAILQ_HEAD */
	int oldmask;

	/*
	 * Find the pointer to the first ifnet structure.  Replace
	 * the pointer to the TAILQ_HEAD with the actual pointer
	 * to the first list element.
	 */
	if (kread(off, (char *)&ifhead, sizeof ifhead))
		return;
	firstifnet = (u_long)ifhead.tqh_first;

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

	(void)signal(SIGALRM, catchalarm);
	signalled = NO;

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
		ip->ift_op = 0;
		ip->ift_ob = 0;
		ip->ift_oe = 0;
		ip->ift_co = 0;
		ip->ift_dr = 0;
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
	sum->ift_op = 0;
	sum->ift_ob = 0;
	sum->ift_oe = 0;
	sum->ift_co = 0;
	sum->ift_dr = 0;
	for (off = firstifnet, ip = iftot; off && ip < lastif; ip++) {
		if (kread(off, (char *)&ifnet, sizeof ifnet)) {
			off = 0;
			continue;
		}
		if (ip == interesting) {
			if (bflag) {
				printf("%10llu %8.8s %10llu %5.5s",
				    (unsigned long long)(ifnet.if_ibytes -
					ip->ift_ib), " ",
				    (unsigned long long)(ifnet.if_obytes -
					ip->ift_ob), " ");
			} else {
				printf("%8llu %5llu %8llu %5llu %5llu",
				    (unsigned long long)
					(ifnet.if_ipackets - ip->ift_ip),
				    (unsigned long long)
					(ifnet.if_ierrors - ip->ift_ie),
				    (unsigned long long)
					(ifnet.if_opackets - ip->ift_op),
				    (unsigned long long)
					(ifnet.if_oerrors - ip->ift_oe),
				    (unsigned long long)
					(ifnet.if_collisions - ip->ift_co));
			}
			if (dflag)
				printf(" %5llu",
				    (unsigned long long)
					(ifnet.if_snd.ifq_drops - ip->ift_dr));
		}
		ip->ift_ip = ifnet.if_ipackets;
		ip->ift_ib = ifnet.if_ibytes;
		ip->ift_ie = ifnet.if_ierrors;
		ip->ift_op = ifnet.if_opackets;
		ip->ift_ob = ifnet.if_obytes;
		ip->ift_oe = ifnet.if_oerrors;
		ip->ift_co = ifnet.if_collisions;
		ip->ift_dr = ifnet.if_snd.ifq_drops;
		sum->ift_ip += ip->ift_ip;
		sum->ift_ib += ip->ift_ib;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_ob += ip->ift_ob;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		sum->ift_dr += ip->ift_dr;
		off = (u_long)ifnet.if_list.tqe_next;
	}
	if (lastif - iftot > 0) {
		if (bflag) {
			printf("  %10llu %8.8s %10llu %5.5s",
			    (unsigned long long)
				(sum->ift_ib - total->ift_ib), " ",
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
			    (unsigned long long)(sum->ift_dr - total->ift_dr));
	}
	*total = *sum;
	putchar('\n');
	fflush(stdout);
	line++;
	oldmask = sigblock(sigmask(SIGALRM));
	if (! signalled) {
		sigpause(0);
	}
	sigsetmask(oldmask);
	signalled = NO;
	if (line == 21)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
static void
catchalarm(signo)
	int signo;
{

	signalled = YES;
}
